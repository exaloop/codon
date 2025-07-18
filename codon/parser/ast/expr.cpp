// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "expr.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/match.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/visitor.h"

#define FASTFLOAT_ALLOWS_LEADING_PLUS
#define FASTFLOAT_SKIP_WHITE_SPACE
#include "fast_float/fast_float.h"

#define ACCEPT_IMPL(T, X)                                                              \
  ASTNode *T::clone(bool c) const { return cache->N<T>(*this, c); }                    \
  void T::accept(X &visitor) { visitor.visit(this); }                                  \
  const char T::NodeId = 0;

using namespace codon::error;
using namespace codon::matcher;

namespace codon::ast {

Expr::Expr() : AcceptorExtend(), type(nullptr), done(false), origExpr(nullptr) {}
Expr::Expr(const Expr &expr, bool clean) : Expr(expr) {
  if (clean) {
    type = nullptr;
    done = false;
  }
}
types::ClassType *Expr::getClassType() const {
  return type ? type->getClass() : nullptr;
}
std::string Expr::wrapType(const std::string &sexpr) const {
  auto is = sexpr;
  if (done)
    is.insert(findStar(is), "*");
  return "(" + is +
         (type && !done ? fmt::format(" #:type \"{}\"", type->debugString(2)) : "") +
         ")";
}

Param::Param(std::string name, Expr *type, Expr *defaultValue, int status)
    : name(std::move(name)), type(type), defaultValue(defaultValue) {
  if (status == 0 && (match(getType(), MOr(M<IdExpr>(TYPE_TYPE), M<IdExpr>(TRAIT_TYPE),
                                           M<IndexExpr>(M<IdExpr>(TRAIT_TYPE), M_))) ||
                      getStaticGeneric(getType()))) {
    this->status = Generic;
  } else {
    this->status = (status == 0 ? Value : (status == 1 ? Generic : HiddenGeneric));
  }
}
Param::Param(const SrcInfo &info, std::string name, Expr *type, Expr *defaultValue,
             int status)
    : Param(std::move(name), type, defaultValue, status) {
  setSrcInfo(info);
}
std::string Param::toString(int indent) const {
  return fmt::format("({}{}{}{})", name,
                     type ? " #:type " + type->toString(indent) : "",
                     defaultValue ? " #:default " + defaultValue->toString(indent) : "",
                     !isValue() ? " #:generic" : "");
}
Param Param::clone(bool clean) const {
  return Param(name, ast::clone(type, clean), ast::clone(defaultValue, clean), status);
}
std::pair<int, std::string> Param::getNameWithStars() const {
  int stars = 0;
  for (; stars < name.size() && name[stars] == '*'; stars++)
    ;
  auto n = name.substr(stars);
  return {stars, n};
}

NoneExpr::NoneExpr() : AcceptorExtend() {}
NoneExpr::NoneExpr(const NoneExpr &expr, bool clean) : AcceptorExtend(expr, clean) {}
std::string NoneExpr::toString(int) const { return wrapType("none"); }

BoolExpr::BoolExpr(bool value) : AcceptorExtend(), value(value) {}
BoolExpr::BoolExpr(const BoolExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), value(expr.value) {}
bool BoolExpr::getValue() const { return value; }
std::string BoolExpr::toString(int) const {
  return wrapType(fmt::format("bool {}", static_cast<int>(value)));
}

IntExpr::IntExpr(int64_t intValue)
    : AcceptorExtend(), value(std::to_string(intValue)), intValue(intValue) {}
IntExpr::IntExpr(const std::string &value, std::string suffix)
    : AcceptorExtend(), value(), suffix(std::move(suffix)) {
  for (auto c : value)
    if (c != '_')
      this->value += c;
  try {
    if (startswith(this->value, "0b") || startswith(this->value, "0B"))
      intValue = std::stoull(this->value.substr(2), nullptr, 2);
    else
      intValue = std::stoull(this->value, nullptr, 0);
  } catch (std::out_of_range &) {
  }
}
IntExpr::IntExpr(const IntExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), value(expr.value), suffix(expr.suffix),
      intValue(expr.intValue) {}
std::pair<std::string, std::string> IntExpr::getRawData() const {
  return {value, suffix};
}
bool IntExpr::hasStoredValue() const { return intValue.has_value(); }
int64_t IntExpr::getValue() const {
  seqassertn(hasStoredValue(), "value not set");
  return intValue.value();
}
std::string IntExpr::toString(int) const {
  return wrapType(
      fmt::format("int {}{}", value,
                  suffix.empty() ? "" : fmt::format(" #:suffix \"{}\"", suffix)));
}

FloatExpr::FloatExpr(double floatValue)
    : AcceptorExtend(), value(fmt::format("{:g}", floatValue)), floatValue(floatValue) {
}
FloatExpr::FloatExpr(const std::string &value, std::string suffix)
    : AcceptorExtend(), value(), suffix(std::move(suffix)) {
  this->value.reserve(value.size());
  std::ranges::copy_if(value.begin(), value.end(), std::back_inserter(this->value),
                       [](char c) { return c != '_'; });

  double result;
  auto r = fast_float::from_chars(this->value.data(),
                                  this->value.data() + this->value.size(), result);
  if (r.ec == std::errc() || r.ec == std::errc::result_out_of_range)
    floatValue = result;
}
FloatExpr::FloatExpr(const FloatExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), value(expr.value), suffix(expr.suffix),
      floatValue(expr.floatValue) {}
std::pair<std::string, std::string> FloatExpr::getRawData() const {
  return {value, suffix};
}
bool FloatExpr::hasStoredValue() const { return floatValue.has_value(); }
double FloatExpr::getValue() const {
  seqassertn(hasStoredValue(), "value not set");
  return floatValue.value();
}
std::string FloatExpr::toString(int) const {
  return wrapType(
      fmt::format("float {}{}", value,
                  suffix.empty() ? "" : fmt::format(" #:suffix \"{}\"", suffix)));
}

StringExpr::StringExpr(std::vector<StringExpr::String> strings)
    : AcceptorExtend(), strings(std::move(strings)) {}
StringExpr::StringExpr(std::string value, std::string prefix)
    : StringExpr(std::vector<StringExpr::String>{
          StringExpr::String{std::move(value), std::move(prefix)}}) {}
StringExpr::StringExpr(const StringExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), strings(expr.strings) {
  for (auto &s : strings)
    s.expr = ast::clone(s.expr);
}
std::string StringExpr::toString(int) const {
  std::vector<std::string> s;
  for (auto &vp : strings)
    s.push_back(fmt::format(
        "\"{}\"{}", escape(vp.value),
        vp.prefix.empty() ? "" : fmt::format(" #:prefix \"{}\"", vp.prefix)));
  return wrapType(fmt::format("string ({})", join(s)));
}
std::string StringExpr::getValue() const {
  seqassert(isSimple(), "invalid StringExpr");
  return strings[0].value;
}
bool StringExpr::isSimple() const {
  return strings.size() == 1 && strings[0].prefix.empty();
}

IdExpr::IdExpr(std::string value) : AcceptorExtend(), value(std::move(value)) {}
IdExpr::IdExpr(const IdExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), value(expr.value) {}
std::string IdExpr::toString(int) const {
  return !getType() ? fmt::format("'{}", value) : wrapType(fmt::format("'{}", value));
}

StarExpr::StarExpr(Expr *expr) : AcceptorExtend(), expr(expr) {}
StarExpr::StarExpr(const StarExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), expr(ast::clone(expr.expr, clean)) {}
std::string StarExpr::toString(int indent) const {
  return wrapType(fmt::format("star {}", expr->toString(indent)));
}

KeywordStarExpr::KeywordStarExpr(Expr *expr) : AcceptorExtend(), expr(expr) {}
KeywordStarExpr::KeywordStarExpr(const KeywordStarExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), expr(ast::clone(expr.expr, clean)) {}
std::string KeywordStarExpr::toString(int indent) const {
  return wrapType(fmt::format("kwstar {}", expr->toString(indent)));
}

TupleExpr::TupleExpr(std::vector<Expr *> items)
    : AcceptorExtend(), Items(std::move(items)) {}
TupleExpr::TupleExpr(const TupleExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)) {}
std::string TupleExpr::toString(int) const {
  return wrapType(fmt::format("tuple {}", combine(items)));
}

ListExpr::ListExpr(std::vector<Expr *> items)
    : AcceptorExtend(), Items(std::move(items)) {}
ListExpr::ListExpr(const ListExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)) {}
std::string ListExpr::toString(int) const {
  return wrapType(!items.empty() ? fmt::format("list {}", combine(items)) : "list");
}

SetExpr::SetExpr(std::vector<Expr *> items)
    : AcceptorExtend(), Items(std::move(items)) {}
SetExpr::SetExpr(const SetExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)) {}
std::string SetExpr::toString(int) const {
  return wrapType(!items.empty() ? fmt::format("set {}", combine(items)) : "set");
}

DictExpr::DictExpr(std::vector<Expr *> items)
    : AcceptorExtend(), Items(std::move(items)) {
  for (auto *i : *this) {
    auto t = cast<TupleExpr>(i);
    seqassertn(t && t->size() == 2, "dictionary items are invalid");
  }
}
DictExpr::DictExpr(const DictExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)) {}
std::string DictExpr::toString(int) const {
  return wrapType(!items.empty() ? fmt::format("dict {}", combine(items)) : "set");
}

GeneratorExpr::GeneratorExpr(Cache *cache, GeneratorExpr::GeneratorKind kind,
                             Expr *expr, std::vector<Stmt *> loops)
    : AcceptorExtend(), kind(kind), loops() {
  this->cache = cache;
  seqassert(!loops.empty() && cast<ForStmt>(loops[0]), "bad generator constructor");
  loops.push_back(cache->N<SuiteStmt>(cache->N<ExprStmt>(expr)));
  formCompleteStmt(loops);
}
GeneratorExpr::GeneratorExpr(Cache *cache, Expr *key, Expr *expr,
                             std::vector<Stmt *> loops)
    : AcceptorExtend(), kind(GeneratorExpr::DictGenerator), loops() {
  this->cache = cache;
  seqassert(!loops.empty() && cast<ForStmt>(loops[0]), "bad generator constructor");
  Expr *t = cache->N<TupleExpr>(std::vector<Expr *>{key, expr});
  loops.push_back(cache->N<SuiteStmt>(cache->N<ExprStmt>(t)));
  formCompleteStmt(loops);
}
GeneratorExpr::GeneratorExpr(const GeneratorExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), kind(expr.kind),
      loops(ast::clone(expr.loops, clean)) {}
std::string GeneratorExpr::toString(int indent) const {
  auto pad = indent >= 0 ? ("\n" + std::string(indent + 2 * INDENT_SIZE, ' ')) : " ";
  std::string prefix;
  if (kind == GeneratorKind::ListGenerator)
    prefix = "list-";
  if (kind == GeneratorKind::SetGenerator)
    prefix = "set-";
  if (kind == GeneratorKind::DictGenerator)
    prefix = "dict-";
  auto l = loops->toString(indent >= 0 ? indent + 2 * INDENT_SIZE : -1);
  return wrapType(fmt::format("{}gen {}", prefix, l));
}
Expr *GeneratorExpr::getFinalExpr() {
  auto s = *(getFinalStmt());
  if (cast<ExprStmt>(s))
    return cast<ExprStmt>(s)->getExpr();
  return nullptr;
}
int GeneratorExpr::loopCount() const {
  int cnt = 0;
  for (Stmt *i = loops;;) {
    if (auto sf = cast<ForStmt>(i)) {
      i = sf->getSuite();
      cnt++;
    } else if (auto si = cast<IfStmt>(i)) {
      i = si->getIf();
      cnt++;
    } else if (auto ss = cast<SuiteStmt>(i)) {
      if (ss->empty())
        break;
      i = ss->back();
    } else
      break;
  }
  return cnt;
}
void GeneratorExpr::setFinalExpr(Expr *expr) {
  *(getFinalStmt()) = cache->N<ExprStmt>(expr);
}
void GeneratorExpr::setFinalStmt(Stmt *stmt) { *(getFinalStmt()) = stmt; }
Stmt *GeneratorExpr::getFinalSuite() const { return loops; }
Stmt **GeneratorExpr::getFinalStmt() {
  for (Stmt **i = &loops;;) {
    if (auto sf = cast<ForStmt>(*i))
      i = reinterpret_cast<Stmt **>(&sf->suite);
    else if (auto si = cast<IfStmt>(*i))
      i = reinterpret_cast<Stmt **>(&si->ifSuite);
    else if (auto ss = cast<SuiteStmt>(*i)) {
      if (ss->empty())
        return i;
      i = &(ss->back());
    } else
      return i;
  }
  seqassert(false, "bad generator");
  return nullptr;
}
void GeneratorExpr::formCompleteStmt(const std::vector<Stmt *> &loops) {
  Stmt *final = nullptr;
  for (size_t i = loops.size(); i-- > 0;) {
    if (auto si = cast<IfStmt>(loops[i]))
      si->ifSuite = SuiteStmt::wrap(final);
    else if (auto sf = cast<ForStmt>(loops[i]))
      sf->suite = SuiteStmt::wrap(final);
    final = loops[i];
  }
  this->loops = loops[0];
}

IfExpr::IfExpr(Expr *cond, Expr *ifexpr, Expr *elsexpr)
    : AcceptorExtend(), cond(cond), ifexpr(ifexpr), elsexpr(elsexpr) {}
IfExpr::IfExpr(const IfExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), cond(ast::clone(expr.cond, clean)),
      ifexpr(ast::clone(expr.ifexpr, clean)), elsexpr(ast::clone(expr.elsexpr, clean)) {
}
std::string IfExpr::toString(int indent) const {
  return wrapType(fmt::format("if-expr {} {} {}", cond->toString(indent),
                              ifexpr->toString(indent), elsexpr->toString(indent)));
}

UnaryExpr::UnaryExpr(std::string op, Expr *expr)
    : AcceptorExtend(), op(std::move(op)), expr(expr) {}
UnaryExpr::UnaryExpr(const UnaryExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), op(expr.op), expr(ast::clone(expr.expr, clean)) {}
std::string UnaryExpr::toString(int indent) const {
  return wrapType(fmt::format("unary \"{}\" {}", op, expr->toString(indent)));
}

BinaryExpr::BinaryExpr(Expr *lexpr, std::string op, Expr *rexpr, bool inPlace)
    : AcceptorExtend(), op(std::move(op)), lexpr(lexpr), rexpr(rexpr),
      inPlace(inPlace) {}
BinaryExpr::BinaryExpr(const BinaryExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), op(expr.op), lexpr(ast::clone(expr.lexpr, clean)),
      rexpr(ast::clone(expr.rexpr, clean)), inPlace(expr.inPlace) {}
std::string BinaryExpr::toString(int indent) const {
  return wrapType(fmt::format("binary \"{}\" {} {}{}", op, lexpr->toString(indent),
                              rexpr->toString(indent), inPlace ? " #:in-place" : ""));
}

ChainBinaryExpr::ChainBinaryExpr(std::vector<std::pair<std::string, Expr *>> exprs)
    : AcceptorExtend(), exprs(std::move(exprs)) {}
ChainBinaryExpr::ChainBinaryExpr(const ChainBinaryExpr &expr, bool clean)
    : AcceptorExtend(expr, clean) {
  for (auto &e : expr.exprs)
    exprs.emplace_back(e.first, ast::clone(e.second, clean));
}
std::string ChainBinaryExpr::toString(int indent) const {
  std::vector<std::string> s;
  for (auto &i : exprs)
    s.push_back(fmt::format("({} \"{}\")", i.first, i.second->toString(indent)));
  return wrapType(fmt::format("chain {}", join(s, " ")));
}

Pipe Pipe::clone(bool clean) const { return {op, ast::clone(expr, clean)}; }

PipeExpr::PipeExpr(std::vector<Pipe> items)
    : AcceptorExtend(), Items(std::move(items)) {
  for (auto &i : *this) {
    if (auto call = cast<CallExpr>(i.expr)) {
      for (auto &a : *call)
        if (auto el = cast<EllipsisExpr>(a.value))
          el->mode = EllipsisExpr::PIPE;
    }
  }
}
PipeExpr::PipeExpr(const PipeExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)),
      inTypes(expr.inTypes) {}
std::string PipeExpr::toString(int indent) const {
  std::vector<std::string> s;
  for (auto &i : items)
    s.push_back(fmt::format("({} \"{}\")", i.expr->toString(indent), i.op));
  return wrapType(fmt::format("pipe {}", join(s, " ")));
}

IndexExpr::IndexExpr(Expr *expr, Expr *index)
    : AcceptorExtend(), expr(expr), index(index) {}
IndexExpr::IndexExpr(const IndexExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), expr(ast::clone(expr.expr, clean)),
      index(ast::clone(expr.index, clean)) {}
std::string IndexExpr::toString(int indent) const {
  return wrapType(
      fmt::format("index {} {}", expr->toString(indent), index->toString(indent)));
}

CallArg CallArg::clone(bool clean) const {
  return CallArg{name, ast::clone(value, clean)};
}
CallArg::CallArg(const SrcInfo &info, std::string name, Expr *value)
    : name(std::move(name)), value(value) {
  setSrcInfo(info);
}
CallArg::CallArg(std::string name, Expr *value) : name(std::move(name)), value(value) {
  if (value)
    setSrcInfo(value->getSrcInfo());
}
CallArg::CallArg(Expr *value) : CallArg("", value) {}

CallExpr::CallExpr(const CallExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)),
      expr(ast::clone(expr.expr, clean)), ordered(expr.ordered), partial(expr.partial) {
}
CallExpr::CallExpr(Expr *expr, std::vector<CallArg> args)
    : AcceptorExtend(), Items(std::move(args)), expr(expr), ordered(false),
      partial(false) {}
CallExpr::CallExpr(Expr *expr, const std::vector<Expr *> &args)
    : AcceptorExtend(), Items({}), expr(expr), ordered(false), partial(false) {
  for (auto a : args)
    if (a)
      items.emplace_back("", a);
}
std::string CallExpr::toString(int indent) const {
  std::vector<std::string> s;
  auto pad = indent >= 0 ? ("\n" + std::string(indent + 2 * INDENT_SIZE, ' ')) : " ";
  for (auto &i : *this) {
    if (!i.name.empty())
      s.emplace_back(pad + fmt::format("#:name '{}", i.name));
    s.emplace_back(pad +
                   i.value->toString(indent >= 0 ? indent + 2 * INDENT_SIZE : -1));
  }
  return wrapType(fmt::format("call{} {}{}", partial ? "-partial" : "",
                              expr->toString(indent), join(s, "")));
}

DotExpr::DotExpr(Expr *expr, std::string member)
    : AcceptorExtend(), expr(expr), member(std::move(member)) {}
DotExpr::DotExpr(const DotExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), expr(ast::clone(expr.expr, clean)),
      member(expr.member) {}
std::string DotExpr::toString(int indent) const {
  return wrapType(fmt::format("dot {} '{}", expr->toString(indent), member));
}

SliceExpr::SliceExpr(Expr *start, Expr *stop, Expr *step)
    : AcceptorExtend(), start(start), stop(stop), step(step) {}
SliceExpr::SliceExpr(const SliceExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), start(ast::clone(expr.start, clean)),
      stop(ast::clone(expr.stop, clean)), step(ast::clone(expr.step, clean)) {}
std::string SliceExpr::toString(int indent) const {
  return wrapType(fmt::format(
      "slice{}{}{}", start ? fmt::format(" #:start {}", start->toString(indent)) : "",
      stop ? fmt::format(" #:end {}", stop->toString(indent)) : "",
      step ? fmt::format(" #:step {}", step->toString(indent)) : ""));
}

EllipsisExpr::EllipsisExpr(EllipsisType mode) : AcceptorExtend(), mode(mode) {}
EllipsisExpr::EllipsisExpr(const EllipsisExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), mode(expr.mode) {}
std::string EllipsisExpr::toString(int) const {
  return wrapType(fmt::format(
      "ellipsis{}", mode == PIPE ? " #:pipe" : (mode == PARTIAL ? " #:partial" : "")));
}

LambdaExpr::LambdaExpr(std::vector<Param> vars, Expr *expr)
    : AcceptorExtend(), Items(std::move(vars)), expr(expr) {}
LambdaExpr::LambdaExpr(const LambdaExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)),
      expr(ast::clone(expr.expr, clean)) {}
std::string LambdaExpr::toString(int indent) const {
  std::vector<std::string> as;
  for (auto &a : items)
    as.push_back(a.toString(indent));
  return wrapType(fmt::format("lambda ({}) {}", join(as, " "), expr->toString(indent)));
}

YieldExpr::YieldExpr() : AcceptorExtend() {}
YieldExpr::YieldExpr(const YieldExpr &expr, bool clean) : AcceptorExtend(expr, clean) {}
std::string YieldExpr::toString(int) const { return "yield-expr"; }

AssignExpr::AssignExpr(Expr *var, Expr *expr)
    : AcceptorExtend(), var(var), expr(expr) {}
AssignExpr::AssignExpr(const AssignExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), var(ast::clone(expr.var, clean)),
      expr(ast::clone(expr.expr, clean)) {}
std::string AssignExpr::toString(int indent) const {
  return wrapType(
      fmt::format("assign-expr '{} {}", var->toString(indent), expr->toString(indent)));
}

RangeExpr::RangeExpr(Expr *start, Expr *stop)
    : AcceptorExtend(), start(start), stop(stop) {}
RangeExpr::RangeExpr(const RangeExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), start(ast::clone(expr.start, clean)),
      stop(ast::clone(expr.stop, clean)) {}
std::string RangeExpr::toString(int indent) const {
  return wrapType(
      fmt::format("range {} {}", start->toString(indent), stop->toString(indent)));
}

StmtExpr::StmtExpr(std::vector<Stmt *> stmts, Expr *expr)
    : AcceptorExtend(), Items(std::move(stmts)), expr(expr) {}
StmtExpr::StmtExpr(Stmt *stmt, Expr *expr) : AcceptorExtend(), Items({}), expr(expr) {
  items.push_back(stmt);
}
StmtExpr::StmtExpr(Stmt *stmt, Stmt *stmt2, Expr *expr)
    : AcceptorExtend(), Items({}), expr(expr) {
  items.push_back(stmt);
  items.push_back(stmt2);
}
StmtExpr::StmtExpr(const StmtExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)),
      expr(ast::clone(expr.expr, clean)) {}
std::string StmtExpr::toString(int indent) const {
  auto pad = indent >= 0 ? ("\n" + std::string(indent + 2 * INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> s;
  s.reserve(items.size());
  for (auto &i : items)
    s.emplace_back(pad + i->toString(indent >= 0 ? indent + 2 * INDENT_SIZE : -1));
  return wrapType(
      fmt::format("stmt-expr {} ({})", expr->toString(indent), join(s, "")));
}

InstantiateExpr::InstantiateExpr(Expr *expr, std::vector<Expr *> typeParams)
    : AcceptorExtend(), Items(std::move(typeParams)), expr(expr) {}
InstantiateExpr::InstantiateExpr(Expr *expr, Expr *typeParam)
    : AcceptorExtend(), Items({typeParam}), expr(expr) {}
InstantiateExpr::InstantiateExpr(const InstantiateExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)),
      expr(ast::clone(expr.expr, clean)) {}
std::string InstantiateExpr::toString(int indent) const {
  return wrapType(
      fmt::format("instantiate {} {}", expr->toString(indent), combine(items)));
}

bool isId(Expr *e, const std::string &s) {
  auto ie = cast<IdExpr>(e);
  return ie && ie->getValue() == s;
}

types::LiteralKind getStaticGeneric(Expr *e) {
  IdExpr *ie = nullptr;
  if (match(e, M<IndexExpr>(M<IdExpr>(MOr("Static", "Literal")), MVar<IdExpr>(ie)))) {
    return types::Type::literalFromString(ie->getValue());
  }
  return types::LiteralKind::Runtime;
}

const char ASTNode::NodeId = 0;
const char Expr::NodeId = 0;
ACCEPT_IMPL(NoneExpr, ASTVisitor);
ACCEPT_IMPL(BoolExpr, ASTVisitor);
ACCEPT_IMPL(IntExpr, ASTVisitor);
ACCEPT_IMPL(FloatExpr, ASTVisitor);
ACCEPT_IMPL(StringExpr, ASTVisitor);
ACCEPT_IMPL(IdExpr, ASTVisitor);
ACCEPT_IMPL(StarExpr, ASTVisitor);
ACCEPT_IMPL(KeywordStarExpr, ASTVisitor);
ACCEPT_IMPL(TupleExpr, ASTVisitor);
ACCEPT_IMPL(ListExpr, ASTVisitor);
ACCEPT_IMPL(SetExpr, ASTVisitor);
ACCEPT_IMPL(DictExpr, ASTVisitor);
ACCEPT_IMPL(GeneratorExpr, ASTVisitor);
ACCEPT_IMPL(IfExpr, ASTVisitor);
ACCEPT_IMPL(UnaryExpr, ASTVisitor);
ACCEPT_IMPL(BinaryExpr, ASTVisitor);
ACCEPT_IMPL(ChainBinaryExpr, ASTVisitor);
ACCEPT_IMPL(PipeExpr, ASTVisitor);
ACCEPT_IMPL(IndexExpr, ASTVisitor);
ACCEPT_IMPL(CallExpr, ASTVisitor);
ACCEPT_IMPL(DotExpr, ASTVisitor);
ACCEPT_IMPL(SliceExpr, ASTVisitor);
ACCEPT_IMPL(EllipsisExpr, ASTVisitor);
ACCEPT_IMPL(LambdaExpr, ASTVisitor);
ACCEPT_IMPL(YieldExpr, ASTVisitor);
ACCEPT_IMPL(AssignExpr, ASTVisitor);
ACCEPT_IMPL(RangeExpr, ASTVisitor);
ACCEPT_IMPL(StmtExpr, ASTVisitor);
ACCEPT_IMPL(InstantiateExpr, ASTVisitor);

} // namespace codon::ast
