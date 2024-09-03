// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "expr.h"

#include <memory>
#include <string>
#include <vector>

#include "codon/cir/attribute.h"
#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/match.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/visitor.h"

#define ACCEPT_IMPL(T, X)                                                              \
  ASTNode *T::clone(bool c) const { return cache->N<T>(*this, c); }                    \
  void T::accept(X &visitor) { visitor.visit(this); }                                  \
  const char T::NodeId = 0;

using fmt::format;
using namespace codon::error;
using namespace codon::matcher;

namespace codon::ast {

ASTNode::ASTNode(const ASTNode &node) : Node(node), cache(node.cache) {}

Expr::Expr() : AcceptorExtend(), type(nullptr), done(false), origExpr(nullptr) {}
Expr::Expr(const Expr &expr)
    : AcceptorExtend(expr), type(expr.type), done(expr.done), origExpr(expr.origExpr) {}
Expr::Expr(const Expr &expr, bool clean) : Expr(expr) {
  if (clean) {
    type = nullptr;
    done = false;
  }
}
void Expr::validate() const {}
types::ClassType *Expr::getClassType() const {
  return type ? type->getClass() : nullptr;
}
std::string Expr::wrapType(const std::string &sexpr) const {
  auto is = sexpr;
  if (done)
    is.insert(findStar(is), "*");
  auto s = format("({}{})", is,
                  type && !done ? format(" #:type \"{}\"", type->debugString(2)) : "");
  return s;
}
Expr *Expr::operator<<(types::Type *t) {
  seqassert(type, "lhs is nullptr");
  if ((*type) << t) {
    E(Error::TYPE_UNIFY, getSrcInfo(), type->prettyString(), t->prettyString());
  }
  return this;
}

Param::Param(std::string name, Expr *type, Expr *defaultValue, int status)
    : name(std::move(name)), type(type), defaultValue(defaultValue) {
  if (status == 0 &&
      (match(getType(), MOr(M<IdExpr>(TYPE_TYPE), M<IdExpr>(TYPE_TYPEVAR),
                            M<IndexExpr>(M<IdExpr>(TYPE_TYPEVAR), M_))) ||
       getStaticGeneric(getType()))) {
    this->status = Generic;
  } else {
    this->status = (status == 0 ? Value : (status == 1 ? Generic : HiddenGeneric));
  }
}
Param::Param(const SrcInfo &info, std::string name, Expr *type, Expr *defaultValue,
             int status)
    : Param(name, type, defaultValue, status) {
  setSrcInfo(info);
}
std::string Param::toString(int indent) const {
  return format("({}{}{}{})", name, type ? " #:type " + type->toString(indent) : "",
                defaultValue ? " #:default " + defaultValue->toString(indent) : "",
                !isValue() ? " #:generic" : "");
}
Param Param::clone(bool clean) const {
  return Param(name, ast::clone(type, clean), ast::clone(defaultValue, clean), status);
}

NoneExpr::NoneExpr() : AcceptorExtend() {}
NoneExpr::NoneExpr(const NoneExpr &expr, bool clean) : AcceptorExtend(expr, clean) {}
std::string NoneExpr::toString(int) const { return wrapType("none"); }

BoolExpr::BoolExpr(bool value) : AcceptorExtend(), value(value) {}
BoolExpr::BoolExpr(const BoolExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), value(expr.value) {}
bool BoolExpr::getValue() const { return value; }
std::string BoolExpr::toString(int) const {
  return wrapType(format("bool {}", int(value)));
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
  return wrapType(format("int {}{}", value,
                         suffix.empty() ? "" : format(" #:suffix \"{}\"", suffix)));
}

FloatExpr::FloatExpr(double floatValue)
    : AcceptorExtend(), value(fmt::format("{:g}", floatValue)), floatValue(floatValue) {
}
FloatExpr::FloatExpr(const std::string &value, std::string suffix)
    : AcceptorExtend(), value(value), suffix(std::move(suffix)) {
  try {
    floatValue = std::stod(value);
  } catch (std::out_of_range &) {
  }
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
  return wrapType(format("float {}{}", value,
                         suffix.empty() ? "" : format(" #:suffix \"{}\"", suffix)));
}

StringExpr::StringExpr(std::vector<StringExpr::String> s)
    : AcceptorExtend(), strings(std::move(s)) {
  unpack();
}
StringExpr::StringExpr(std::string value, std::string prefix)
    : StringExpr(std::vector<StringExpr::String>{{value, prefix}}) {}
StringExpr::StringExpr(const StringExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), strings(expr.strings) {
  for (auto &s : strings)
    s.expr = ast::clone(s.expr);
}
std::string StringExpr::toString(int) const {
  std::vector<std::string> s;
  for (auto &vp : strings)
    s.push_back(format("\"{}\"{}", escape(vp.value),
                       vp.prefix.empty() ? "" : format(" #:prefix \"{}\"", vp.prefix)));
  return wrapType(format("string ({})", join(s)));
}
std::string StringExpr::getValue() const {
  seqassert(isSimple(), "invalid StringExpr");
  return strings[0].value;
}
bool StringExpr::isSimple() const {
  return strings.size() == 1 && strings[0].prefix.empty();
}
void StringExpr::unpack() {
  std::vector<String> exprs;
  for (auto &p : strings) {
    if (p.prefix == "f" || p.prefix == "F") {
      /// Transform an F-string
      for (auto pf : unpackFString(p.value)) {
        if (pf.prefix.empty() && !exprs.empty() && exprs.back().prefix.empty()) {
          exprs.back().value += pf.value;
        } else {
          exprs.emplace_back(pf);
        }
      }
    } else if (!p.prefix.empty()) {
      exprs.emplace_back(p);
    } else if (!exprs.empty() && exprs.back().prefix.empty()) {
      exprs.back().value += p.value;
    } else {
      exprs.emplace_back(p);
    }
  }
  strings = exprs;
}
std::vector<StringExpr::String>
StringExpr::unpackFString(const std::string &value) const {
  // Strings to be concatenated
  std::vector<StringExpr::String> items;
  int braceCount = 0, braceStart = 0;
  for (int i = 0; i < value.size(); i++) {
    if (value[i] == '{') {
      if (braceStart < i)
        items.emplace_back(value.substr(braceStart, i - braceStart));
      if (!braceCount)
        braceStart = i + 1;
      braceCount++;
    } else if (value[i] == '}') {
      braceCount--;
      if (!braceCount) {
        std::string code = value.substr(braceStart, i - braceStart);
        auto offset = getSrcInfo();
        offset.col += i;
        if (!code.empty() && code.back() == '=') {
          // Special case: f"{x=}"
          code = code.substr(0, code.size() - 1);
          items.emplace_back(fmt::format("{}=", code));
        }
        items.emplace_back(code, "#f");
        items.back().setSrcInfo(offset);
      }
      braceStart = i + 1;
    }
  }
  if (braceCount > 0)
    E(Error::STR_FSTRING_BALANCE_EXTRA, getSrcInfo());
  if (braceCount < 0)
    E(Error::STR_FSTRING_BALANCE_MISSING, getSrcInfo());
  if (braceStart != value.size())
    items.emplace_back(value.substr(braceStart, value.size() - braceStart));
  return items;
}

IdExpr::IdExpr(std::string value) : AcceptorExtend(), value(std::move(value)) {}
IdExpr::IdExpr(const IdExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), value(expr.value) {}
std::string IdExpr::toString(int) const {
  return !getType() ? format("'{}", value) : wrapType(format("'{}", value));
}

StarExpr::StarExpr(Expr *expr) : AcceptorExtend(), expr(expr) {}
StarExpr::StarExpr(const StarExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), expr(ast::clone(expr.expr, clean)) {}
std::string StarExpr::toString(int indent) const {
  return wrapType(format("star {}", expr->toString(indent)));
}

KeywordStarExpr::KeywordStarExpr(Expr *expr) : AcceptorExtend(), expr(expr) {}
KeywordStarExpr::KeywordStarExpr(const KeywordStarExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), expr(ast::clone(expr.expr, clean)) {}
std::string KeywordStarExpr::toString(int indent) const {
  return wrapType(format("kwstar {}", expr->toString(indent)));
}

TupleExpr::TupleExpr(std::vector<Expr *> items)
    : AcceptorExtend(), Items(std::move(items)) {}
TupleExpr::TupleExpr(const TupleExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)) {}
std::string TupleExpr::toString(int) const {
  return wrapType(format("tuple {}", combine(items)));
}

ListExpr::ListExpr(std::vector<Expr *> items)
    : AcceptorExtend(), Items(std::move(items)) {}
ListExpr::ListExpr(const ListExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)) {}
std::string ListExpr::toString(int) const {
  return wrapType(!items.empty() ? format("list {}", combine(items)) : "list");
}

SetExpr::SetExpr(std::vector<Expr *> items)
    : AcceptorExtend(), Items(std::move(items)) {}
SetExpr::SetExpr(const SetExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)) {}
std::string SetExpr::toString(int) const {
  return wrapType(!items.empty() ? format("set {}", combine(items)) : "set");
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
  return wrapType(!items.empty() ? format("dict {}", combine(items)) : "set");
}

GeneratorExpr::GeneratorExpr(Cache *cache, GeneratorExpr::GeneratorKind kind,
                             Expr *expr, std::vector<Stmt *> loops)
    : AcceptorExtend(), kind(kind) {
  this->cache = cache;
  seqassert(!loops.empty() && cast<ForStmt>(loops[0]), "bad generator constructor");
  loops.push_back(cache->N<SuiteStmt>(cache->N<ExprStmt>(expr)));
  formCompleteStmt(loops);
}
GeneratorExpr::GeneratorExpr(Cache *cache, Expr *key, Expr *expr,
                             std::vector<Stmt *> loops)
    : AcceptorExtend(), kind(GeneratorExpr::DictGenerator) {
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
  return wrapType(format("{}gen {}", prefix, l));
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
      i = (Stmt **)&sf->suite;
    else if (auto si = cast<IfStmt>(*i))
      i = (Stmt **)&si->ifSuite;
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
  return wrapType(format("if-expr {} {} {}", cond->toString(indent),
                         ifexpr->toString(indent), elsexpr->toString(indent)));
}

UnaryExpr::UnaryExpr(std::string op, Expr *expr)
    : AcceptorExtend(), op(std::move(op)), expr(expr) {}
UnaryExpr::UnaryExpr(const UnaryExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), op(expr.op), expr(ast::clone(expr.expr, clean)) {}
std::string UnaryExpr::toString(int indent) const {
  return wrapType(format("unary \"{}\" {}", op, expr->toString(indent)));
}

BinaryExpr::BinaryExpr(Expr *lexpr, std::string op, Expr *rexpr, bool inPlace)
    : AcceptorExtend(), op(std::move(op)), lexpr(lexpr), rexpr(rexpr),
      inPlace(inPlace) {}
BinaryExpr::BinaryExpr(const BinaryExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), op(expr.op), lexpr(ast::clone(expr.lexpr, clean)),
      rexpr(ast::clone(expr.rexpr, clean)), inPlace(expr.inPlace) {}
std::string BinaryExpr::toString(int indent) const {
  return wrapType(format("binary \"{}\" {} {}{}", op, lexpr->toString(indent),
                         rexpr->toString(indent), inPlace ? " #:in-place" : ""));
}

ChainBinaryExpr::ChainBinaryExpr(std::vector<std::pair<std::string, Expr *>> exprs)
    : AcceptorExtend(), exprs(std::move(exprs)) {}
ChainBinaryExpr::ChainBinaryExpr(const ChainBinaryExpr &expr, bool clean)
    : AcceptorExtend(expr, clean) {
  for (auto &e : expr.exprs)
    exprs.emplace_back(make_pair(e.first, ast::clone(e.second, clean)));
}
std::string ChainBinaryExpr::toString(int indent) const {
  std::vector<std::string> s;
  for (auto &i : exprs)
    s.push_back(format("({} \"{}\")", i.first, i.second->toString(indent)));
  return wrapType(format("chain {}", join(s, " ")));
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
void PipeExpr::validate() const {}
std::string PipeExpr::toString(int indent) const {
  std::vector<std::string> s;
  for (auto &i : items)
    s.push_back(format("({} \"{}\")", i.expr->toString(indent), i.op));
  return wrapType(format("pipe {}", join(s, " ")));
}

IndexExpr::IndexExpr(Expr *expr, Expr *index)
    : AcceptorExtend(), expr(expr), index(index) {}
IndexExpr::IndexExpr(const IndexExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), expr(ast::clone(expr.expr, clean)),
      index(ast::clone(expr.index, clean)) {}
std::string IndexExpr::toString(int indent) const {
  return wrapType(
      format("index {} {}", expr->toString(indent), index->toString(indent)));
}

CallArg CallArg::clone(bool clean) const { return {name, ast::clone(value, clean)}; }
CallArg::CallArg(const SrcInfo &info, const std::string &name, Expr *value)
    : name(name), value(value) {
  setSrcInfo(info);
}
CallArg::CallArg(const std::string &name, Expr *value) : name(name), value(value) {
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
      partial(false) {
  validate();
}
CallExpr::CallExpr(Expr *expr, std::vector<Expr *> args)
    : AcceptorExtend(), Items({}), expr(expr), ordered(false), partial(false) {
  for (auto a : args)
    if (a)
      items.emplace_back("", a);
  validate();
}
void CallExpr::validate() const {
  bool namesStarted = false, foundEllipsis = false;
  for (auto &a : *this) {
    if (a.name.empty() && namesStarted &&
        !(cast<KeywordStarExpr>(a.value) || cast<EllipsisExpr>(a.value)))
      E(Error::CALL_NAME_ORDER, a.value);
    if (!a.name.empty() && (cast<StarExpr>(a.value) || cast<KeywordStarExpr>(a.value)))
      E(Error::CALL_NAME_STAR, a.value);
    if (cast<EllipsisExpr>(a.value) && foundEllipsis)
      E(Error::CALL_ELLIPSIS, a.value);
    foundEllipsis |= bool(cast<EllipsisExpr>(a.value));
    namesStarted |= !a.name.empty();
  }
}
std::string CallExpr::toString(int indent) const {
  std::vector<std::string> s;
  auto pad = indent >= 0 ? ("\n" + std::string(indent + 2 * INDENT_SIZE, ' ')) : " ";
  for (auto &i : *this) {
    if (i.name.empty())
      s.emplace_back(pad + format("#:name '{}", i.name));
    s.emplace_back(pad +
                   i.value->toString(indent >= 0 ? indent + 2 * INDENT_SIZE : -1));
  }
  return wrapType(format("call{} {}{}", partial ? "-partial" : "",
                         expr->toString(indent), fmt::join(s, "")));
}

DotExpr::DotExpr(Expr *expr, std::string member)
    : AcceptorExtend(), expr(expr), member(std::move(member)) {}
DotExpr::DotExpr(const DotExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), expr(ast::clone(expr.expr, clean)),
      member(expr.member) {}
std::string DotExpr::toString(int indent) const {
  return wrapType(format("dot {} '{}", expr->toString(indent), member));
}

SliceExpr::SliceExpr(Expr *start, Expr *stop, Expr *step)
    : AcceptorExtend(), start(start), stop(stop), step(step) {}
SliceExpr::SliceExpr(const SliceExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), start(ast::clone(expr.start, clean)),
      stop(ast::clone(expr.stop, clean)), step(ast::clone(expr.step, clean)) {}
std::string SliceExpr::toString(int indent) const {
  return wrapType(format("slice{}{}{}",
                         start ? format(" #:start {}", start->toString(indent)) : "",
                         stop ? format(" #:end {}", stop->toString(indent)) : "",
                         step ? format(" #:step {}", step->toString(indent)) : ""));
}

EllipsisExpr::EllipsisExpr(EllipsisType mode) : AcceptorExtend(), mode(mode) {}
EllipsisExpr::EllipsisExpr(const EllipsisExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), mode(expr.mode) {}
std::string EllipsisExpr::toString(int) const {
  return wrapType(format(
      "ellipsis{}", mode == PIPE ? " #:pipe" : (mode == PARTIAL ? "#:partial" : "")));
}

LambdaExpr::LambdaExpr(std::vector<std::string> vars, Expr *expr)
    : AcceptorExtend(), Items(std::move(vars)), expr(expr) {}
LambdaExpr::LambdaExpr(const LambdaExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(expr.items),
      expr(ast::clone(expr.expr, clean)) {}
std::string LambdaExpr::toString(int indent) const {
  return wrapType(format("lambda ({}) {}", join(items, " "), expr->toString(indent)));
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
      format("assign-expr '{} {}", var->toString(indent), expr->toString(indent)));
}

RangeExpr::RangeExpr(Expr *start, Expr *stop)
    : AcceptorExtend(), start(start), stop(stop) {}
RangeExpr::RangeExpr(const RangeExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), start(ast::clone(expr.start, clean)),
      stop(ast::clone(expr.stop, clean)) {}
std::string RangeExpr::toString(int indent) const {
  return wrapType(
      format("range {} {}", start->toString(indent), stop->toString(indent)));
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
      format("stmt-expr {} ({})", expr->toString(indent), fmt::join(s, "")));
}

InstantiateExpr::InstantiateExpr(Expr *expr, std::vector<Expr *> typeParams)
    : AcceptorExtend(), Items(std::move(typeParams)), expr(expr) {}
InstantiateExpr::InstantiateExpr(Expr *expr, Expr *typeParam)
    : AcceptorExtend(), Items({typeParam}), expr(expr) {}
InstantiateExpr::InstantiateExpr(const InstantiateExpr &expr, bool clean)
    : AcceptorExtend(expr, clean), Items(ast::clone(expr.items, clean)),
      expr(ast::clone(expr.expr, clean)) {}
std::string InstantiateExpr::toString(int indent) const {
  return wrapType(format("instantiate {} {}", expr->toString(indent), combine(items)));
}

bool isId(Expr *e, const std::string &s) {
  auto ie = cast<IdExpr>(e);
  return ie && ie->getValue() == s;
}

char getStaticGeneric(Expr *e) {
  auto ie = cast<IndexExpr>(e);
  if (!ie)
    return 0;
  if (cast<IdExpr>(ie->getExpr()) &&
      cast<IdExpr>(ie->getExpr())->getValue() == "Static") {
    auto ixe = cast<IdExpr>(ie->getIndex());
    if (!ixe)
      return 0;
    if (ixe->getValue() == "bool")
      return 3;
    if (ixe->getValue() == "str")
      return 2;
    if (ixe->getValue() == "int")
      return 1;
    return 4;
  }
  return 0;
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
