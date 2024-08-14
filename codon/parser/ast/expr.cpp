// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "expr.h"

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/visitors/visitor.h"

#define FASTFLOAT_ALLOWS_LEADING_PLUS
#define FASTFLOAT_SKIP_WHITE_SPACE
#include "fast_float/fast_float.h"

#define ACCEPT_IMPL(T, X)                                                              \
  ExprPtr T::clone() const { return std::make_shared<T>(*this); }                      \
  void T::accept(X &visitor) { visitor.visit(this); }

using fmt::format;
using namespace codon::error;

namespace codon::ast {

Expr::Expr()
    : type(nullptr), isTypeExpr(false), staticValue(StaticValue::NOT_STATIC),
      done(false), attributes(0), origExpr(nullptr) {}
void Expr::validate() const {}
types::TypePtr Expr::getType() const { return type; }
void Expr::setType(types::TypePtr t) { this->type = std::move(t); }
bool Expr::isType() const { return isTypeExpr; }
void Expr::markType() { isTypeExpr = true; }
std::string Expr::wrapType(const std::string &sexpr) const {
  auto is = sexpr;
  if (done)
    is.insert(findStar(is), "*");
  auto s = format("({}{})", is, type ? format(" #:type \"{}\"", type->toString()) : "");
  // if (hasAttr(ExprAttr::SequenceItem)) s += "%";
  return s;
}
bool Expr::isStatic() const { return staticValue.type != StaticValue::NOT_STATIC; }
bool Expr::hasAttr(int attr) const { return (attributes & (1 << attr)); }
void Expr::setAttr(int attr) { attributes |= (1 << attr); }
std::string Expr::getTypeName() {
  if (getId()) {
    return getId()->value;
  } else {
    auto i = dynamic_cast<InstantiateExpr *>(this);
    seqassertn(i && i->typeExpr->getId(), "bad MRO");
    return i->typeExpr->getId()->value;
  }
}

StaticValue::StaticValue(StaticValue::Type t) : value(), type(t), evaluated(false) {}
StaticValue::StaticValue(int64_t i) : value(i), type(INT), evaluated(true) {}
StaticValue::StaticValue(std::string s)
    : value(std::move(s)), type(STRING), evaluated(true) {}
bool StaticValue::operator==(const StaticValue &s) const {
  if (type != s.type || s.evaluated != evaluated)
    return false;
  return !s.evaluated || value == s.value;
}
std::string StaticValue::toString() const {
  if (type == StaticValue::NOT_STATIC)
    return "";
  if (!evaluated)
    return type == StaticValue::STRING ? "str" : "int";
  return type == StaticValue::STRING ? "'" + escape(std::get<std::string>(value)) + "'"
                                     : std::to_string(std::get<int64_t>(value));
}
int64_t StaticValue::getInt() const {
  seqassertn(type == StaticValue::INT, "not an int");
  return std::get<int64_t>(value);
}
std::string StaticValue::getString() const {
  seqassertn(type == StaticValue::STRING, "not a string");
  return std::get<std::string>(value);
}

Param::Param(std::string name, ExprPtr type, ExprPtr defaultValue, int status)
    : name(std::move(name)), type(std::move(type)),
      defaultValue(std::move(defaultValue)) {
  if (status == 0 && this->type &&
      (this->type->isId("type") || this->type->isId(TYPE_TYPEVAR) ||
       (this->type->getIndex() && this->type->getIndex()->expr->isId(TYPE_TYPEVAR)) ||
       getStaticGeneric(this->type.get())))
    this->status = Generic;
  else
    this->status = (status == 0 ? Normal : (status == 1 ? Generic : HiddenGeneric));
}
Param::Param(const SrcInfo &info, std::string name, ExprPtr type, ExprPtr defaultValue,
             int status)
    : Param(name, type, defaultValue, status) {
  setSrcInfo(info);
}
std::string Param::toString() const {
  return format("({}{}{}{})", name, type ? " #:type " + type->toString() : "",
                defaultValue ? " #:default " + defaultValue->toString() : "",
                status != Param::Normal ? " #:generic" : "");
}
Param Param::clone() const {
  return Param(name, ast::clone(type), ast::clone(defaultValue), status);
}

NoneExpr::NoneExpr() : Expr() {}
std::string NoneExpr::toString() const { return wrapType("none"); }
ACCEPT_IMPL(NoneExpr, ASTVisitor);

BoolExpr::BoolExpr(bool value) : Expr(), value(value) {
  staticValue = StaticValue(value);
}
std::string BoolExpr::toString() const {
  return wrapType(format("bool {}", int(value)));
}
ACCEPT_IMPL(BoolExpr, ASTVisitor);

IntExpr::IntExpr(int64_t intValue) : Expr(), value(std::to_string(intValue)) {
  this->intValue = std::make_unique<int64_t>(intValue);
  staticValue = StaticValue(intValue);
}
IntExpr::IntExpr(const std::string &value, std::string suffix)
    : Expr(), value(), suffix(std::move(suffix)) {
  for (auto c : value)
    if (c != '_')
      this->value += c;
  try {
    if (startswith(this->value, "0b") || startswith(this->value, "0B"))
      intValue =
          std::make_unique<int64_t>(std::stoull(this->value.substr(2), nullptr, 2));
    else
      intValue = std::make_unique<int64_t>(std::stoull(this->value, nullptr, 0));
  } catch (std::out_of_range &) {
    intValue = nullptr;
  }
}
IntExpr::IntExpr(const IntExpr &expr)
    : Expr(expr), value(expr.value), suffix(expr.suffix) {
  intValue = expr.intValue ? std::make_unique<int64_t>(*(expr.intValue)) : nullptr;
}
std::string IntExpr::toString() const {
  return wrapType(format("int {}{}", value,
                         suffix.empty() ? "" : format(" #:suffix \"{}\"", suffix)));
}
ACCEPT_IMPL(IntExpr, ASTVisitor);

FloatExpr::FloatExpr(double floatValue)
    : Expr(), value(fmt::format("{:g}", floatValue)) {
  this->floatValue = std::make_unique<double>(floatValue);
}
FloatExpr::FloatExpr(const std::string &value, std::string suffix)
    : Expr(), value(value), suffix(std::move(suffix)) {
  double result;
  auto r = fast_float::from_chars(value.data(), value.data() + value.size(), result);
  if (r.ec == std::errc() || r.ec == std::errc::result_out_of_range)
    floatValue = std::make_unique<double>(result);
  else
    floatValue = nullptr;
}
FloatExpr::FloatExpr(const FloatExpr &expr)
    : Expr(expr), value(expr.value), suffix(expr.suffix) {
  floatValue = expr.floatValue ? std::make_unique<double>(*(expr.floatValue)) : nullptr;
}
std::string FloatExpr::toString() const {
  return wrapType(format("float {}{}", value,
                         suffix.empty() ? "" : format(" #:suffix \"{}\"", suffix)));
}
ACCEPT_IMPL(FloatExpr, ASTVisitor);

StringExpr::StringExpr(std::vector<std::pair<std::string, std::string>> s)
    : Expr(), strings(std::move(s)) {
  if (strings.size() == 1 && strings.back().second.empty())
    staticValue = StaticValue(strings.back().first);
}
StringExpr::StringExpr(std::string value, std::string prefix)
    : StringExpr(std::vector<std::pair<std::string, std::string>>{{value, prefix}}) {}
std::string StringExpr::toString() const {
  std::vector<std::string> s;
  for (auto &vp : strings)
    s.push_back(format("\"{}\"{}", escape(vp.first),
                       vp.second.empty() ? "" : format(" #:prefix \"{}\"", vp.second)));
  return wrapType(format("string ({})", join(s)));
}
std::string StringExpr::getValue() const {
  seqassert(!strings.empty(), "invalid StringExpr");
  return strings[0].first;
}
ACCEPT_IMPL(StringExpr, ASTVisitor);

IdExpr::IdExpr(std::string value) : Expr(), value(std::move(value)) {}
std::string IdExpr::toString() const {
  return !type ? format("'{}", value) : wrapType(format("'{}", value));
}
ACCEPT_IMPL(IdExpr, ASTVisitor);

StarExpr::StarExpr(ExprPtr what) : Expr(), what(std::move(what)) {}
StarExpr::StarExpr(const StarExpr &expr) : Expr(expr), what(ast::clone(expr.what)) {}
std::string StarExpr::toString() const {
  return wrapType(format("star {}", what->toString()));
}
ACCEPT_IMPL(StarExpr, ASTVisitor);

KeywordStarExpr::KeywordStarExpr(ExprPtr what) : Expr(), what(std::move(what)) {}
KeywordStarExpr::KeywordStarExpr(const KeywordStarExpr &expr)
    : Expr(expr), what(ast::clone(expr.what)) {}
std::string KeywordStarExpr::toString() const {
  return wrapType(format("kwstar {}", what->toString()));
}
ACCEPT_IMPL(KeywordStarExpr, ASTVisitor);

TupleExpr::TupleExpr(std::vector<ExprPtr> items) : Expr(), items(std::move(items)) {}
TupleExpr::TupleExpr(const TupleExpr &expr)
    : Expr(expr), items(ast::clone(expr.items)) {}
std::string TupleExpr::toString() const {
  return wrapType(format("tuple {}", combine(items)));
}
ACCEPT_IMPL(TupleExpr, ASTVisitor);

ListExpr::ListExpr(std::vector<ExprPtr> items) : Expr(), items(std::move(items)) {}
ListExpr::ListExpr(const ListExpr &expr) : Expr(expr), items(ast::clone(expr.items)) {}
std::string ListExpr::toString() const {
  return wrapType(!items.empty() ? format("list {}", combine(items)) : "list");
}
ACCEPT_IMPL(ListExpr, ASTVisitor);

SetExpr::SetExpr(std::vector<ExprPtr> items) : Expr(), items(std::move(items)) {}
SetExpr::SetExpr(const SetExpr &expr) : Expr(expr), items(ast::clone(expr.items)) {}
std::string SetExpr::toString() const {
  return wrapType(!items.empty() ? format("set {}", combine(items)) : "set");
}
ACCEPT_IMPL(SetExpr, ASTVisitor);

DictExpr::DictExpr(std::vector<ExprPtr> items) : Expr(), items(std::move(items)) {
  for (auto &i : items) {
    auto t = i->getTuple();
    seqassertn(t && t->items.size() == 2, "dictionary items are invalid");
  }
}
DictExpr::DictExpr(const DictExpr &expr) : Expr(expr), items(ast::clone(expr.items)) {}
std::string DictExpr::toString() const {
  return wrapType(!items.empty() ? format("dict {}", combine(items)) : "set");
}
ACCEPT_IMPL(DictExpr, ASTVisitor);

GeneratorBody GeneratorBody::clone() const {
  return {ast::clone(vars), ast::clone(gen), ast::clone(conds)};
}

GeneratorExpr::GeneratorExpr(GeneratorExpr::GeneratorKind kind, ExprPtr expr,
                             std::vector<GeneratorBody> loops)
    : Expr(), kind(kind), expr(std::move(expr)), loops(std::move(loops)) {}
GeneratorExpr::GeneratorExpr(const GeneratorExpr &expr)
    : Expr(expr), kind(expr.kind), expr(ast::clone(expr.expr)),
      loops(ast::clone_nop(expr.loops)) {}
std::string GeneratorExpr::toString() const {
  std::string prefix;
  if (kind == GeneratorKind::ListGenerator)
    prefix = "list-";
  if (kind == GeneratorKind::SetGenerator)
    prefix = "set-";
  std::string s;
  for (auto &i : loops) {
    std::string q;
    for (auto &k : i.conds)
      q += format(" (if {})", k->toString());
    s += format(" (for {} {}{})", i.vars->toString(), i.gen->toString(), q);
  }
  return wrapType(format("{}gen {}{}", prefix, expr->toString(), s));
}
ACCEPT_IMPL(GeneratorExpr, ASTVisitor);

DictGeneratorExpr::DictGeneratorExpr(ExprPtr key, ExprPtr expr,
                                     std::vector<GeneratorBody> loops)
    : Expr(), key(std::move(key)), expr(std::move(expr)), loops(std::move(loops)) {}
DictGeneratorExpr::DictGeneratorExpr(const DictGeneratorExpr &expr)
    : Expr(expr), key(ast::clone(expr.key)), expr(ast::clone(expr.expr)),
      loops(ast::clone_nop(expr.loops)) {}
std::string DictGeneratorExpr::toString() const {
  std::string s;
  for (auto &i : loops) {
    std::string q;
    for (auto &k : i.conds)
      q += format("( if {})", k->toString());
    s += format(" (for {} {}{})", i.vars->toString(), i.gen->toString(), q);
  }
  return wrapType(format("dict-gen {} {}{}", key->toString(), expr->toString(), s));
}
ACCEPT_IMPL(DictGeneratorExpr, ASTVisitor);

IfExpr::IfExpr(ExprPtr cond, ExprPtr ifexpr, ExprPtr elsexpr)
    : Expr(), cond(std::move(cond)), ifexpr(std::move(ifexpr)),
      elsexpr(std::move(elsexpr)) {}
IfExpr::IfExpr(const IfExpr &expr)
    : Expr(expr), cond(ast::clone(expr.cond)), ifexpr(ast::clone(expr.ifexpr)),
      elsexpr(ast::clone(expr.elsexpr)) {}
std::string IfExpr::toString() const {
  return wrapType(format("if-expr {} {} {}", cond->toString(), ifexpr->toString(),
                         elsexpr->toString()));
}
ACCEPT_IMPL(IfExpr, ASTVisitor);

UnaryExpr::UnaryExpr(std::string op, ExprPtr expr)
    : Expr(), op(std::move(op)), expr(std::move(expr)) {}
UnaryExpr::UnaryExpr(const UnaryExpr &expr)
    : Expr(expr), op(expr.op), expr(ast::clone(expr.expr)) {}
std::string UnaryExpr::toString() const {
  return wrapType(format("unary \"{}\" {}", op, expr->toString()));
}
ACCEPT_IMPL(UnaryExpr, ASTVisitor);

BinaryExpr::BinaryExpr(ExprPtr lexpr, std::string op, ExprPtr rexpr, bool inPlace)
    : Expr(), op(std::move(op)), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)),
      inPlace(inPlace) {}
BinaryExpr::BinaryExpr(const BinaryExpr &expr)
    : Expr(expr), op(expr.op), lexpr(ast::clone(expr.lexpr)),
      rexpr(ast::clone(expr.rexpr)), inPlace(expr.inPlace) {}
std::string BinaryExpr::toString() const {
  return wrapType(format("binary \"{}\" {} {}{}", op, lexpr->toString(),
                         rexpr->toString(), inPlace ? " #:in-place" : ""));
}
ACCEPT_IMPL(BinaryExpr, ASTVisitor);

ChainBinaryExpr::ChainBinaryExpr(std::vector<std::pair<std::string, ExprPtr>> exprs)
    : Expr(), exprs(std::move(exprs)) {}
ChainBinaryExpr::ChainBinaryExpr(const ChainBinaryExpr &expr) : Expr(expr) {
  for (auto &e : expr.exprs)
    exprs.emplace_back(make_pair(e.first, ast::clone(e.second)));
}
std::string ChainBinaryExpr::toString() const {
  std::vector<std::string> s;
  for (auto &i : exprs)
    s.push_back(format("({} \"{}\")", i.first, i.second->toString()));
  return wrapType(format("chain {}", join(s, " ")));
}
ACCEPT_IMPL(ChainBinaryExpr, ASTVisitor);

PipeExpr::Pipe PipeExpr::Pipe::clone() const { return {op, ast::clone(expr)}; }

PipeExpr::PipeExpr(std::vector<PipeExpr::Pipe> items)
    : Expr(), items(std::move(items)) {
  for (auto &i : this->items) {
    if (auto call = i.expr->getCall()) {
      for (auto &a : call->args)
        if (auto el = a.value->getEllipsis())
          el->mode = EllipsisExpr::PIPE;
    }
  }
}
PipeExpr::PipeExpr(const PipeExpr &expr)
    : Expr(expr), items(ast::clone_nop(expr.items)), inTypes(expr.inTypes) {}
void PipeExpr::validate() const {}
std::string PipeExpr::toString() const {
  std::vector<std::string> s;
  for (auto &i : items)
    s.push_back(format("({} \"{}\")", i.expr->toString(), i.op));
  return wrapType(format("pipe {}", join(s, " ")));
}
ACCEPT_IMPL(PipeExpr, ASTVisitor);

IndexExpr::IndexExpr(ExprPtr expr, ExprPtr index)
    : Expr(), expr(std::move(expr)), index(std::move(index)) {}
IndexExpr::IndexExpr(const IndexExpr &expr)
    : Expr(expr), expr(ast::clone(expr.expr)), index(ast::clone(expr.index)) {}
std::string IndexExpr::toString() const {
  return wrapType(format("index {} {}", expr->toString(), index->toString()));
}
ACCEPT_IMPL(IndexExpr, ASTVisitor);

CallExpr::Arg CallExpr::Arg::clone() const { return {name, ast::clone(value)}; }
CallExpr::Arg::Arg(const SrcInfo &info, const std::string &name, ExprPtr value)
    : name(name), value(value) {
  setSrcInfo(info);
}
CallExpr::Arg::Arg(const std::string &name, ExprPtr value) : name(name), value(value) {
  if (value)
    setSrcInfo(value->getSrcInfo());
}
CallExpr::Arg::Arg(ExprPtr value) : CallExpr::Arg("", value) {}

CallExpr::CallExpr(const CallExpr &expr)
    : Expr(expr), expr(ast::clone(expr.expr)), args(ast::clone_nop(expr.args)),
      ordered(expr.ordered) {}
CallExpr::CallExpr(ExprPtr expr, std::vector<CallExpr::Arg> args)
    : Expr(), expr(std::move(expr)), args(std::move(args)), ordered(false) {
  validate();
}
CallExpr::CallExpr(ExprPtr expr, std::vector<ExprPtr> args)
    : expr(std::move(expr)), ordered(false) {
  for (auto &a : args)
    if (a)
      this->args.push_back({"", std::move(a)});
  validate();
}
void CallExpr::validate() const {
  bool namesStarted = false, foundEllipsis = false;
  for (auto &a : args) {
    if (a.name.empty() && namesStarted &&
        !(CAST(a.value, KeywordStarExpr) || a.value->getEllipsis()))
      E(Error::CALL_NAME_ORDER, a.value);
    if (!a.name.empty() && (a.value->getStar() || CAST(a.value, KeywordStarExpr)))
      E(Error::CALL_NAME_STAR, a.value);
    if (a.value->getEllipsis() && foundEllipsis)
      E(Error::CALL_ELLIPSIS, a.value);
    foundEllipsis |= bool(a.value->getEllipsis());
    namesStarted |= !a.name.empty();
  }
}
std::string CallExpr::toString() const {
  std::string s;
  for (auto &i : args)
    if (i.name.empty())
      s += " " + i.value->toString();
    else
      s += format("({}{})", i.value->toString(),
                  i.name.empty() ? "" : format(" #:name '{}", i.name));
  return wrapType(format("call {} {}", expr->toString(), s));
}
ACCEPT_IMPL(CallExpr, ASTVisitor);

DotExpr::DotExpr(ExprPtr expr, std::string member)
    : Expr(), expr(std::move(expr)), member(std::move(member)) {}
DotExpr::DotExpr(const std::string &left, std::string member)
    : Expr(), expr(std::make_shared<IdExpr>(left)), member(std::move(member)) {}
DotExpr::DotExpr(const DotExpr &expr)
    : Expr(expr), expr(ast::clone(expr.expr)), member(expr.member) {}
std::string DotExpr::toString() const {
  return wrapType(format("dot {} '{}", expr->toString(), member));
}
ACCEPT_IMPL(DotExpr, ASTVisitor);

SliceExpr::SliceExpr(ExprPtr start, ExprPtr stop, ExprPtr step)
    : Expr(), start(std::move(start)), stop(std::move(stop)), step(std::move(step)) {}
SliceExpr::SliceExpr(const SliceExpr &expr)
    : Expr(expr), start(ast::clone(expr.start)), stop(ast::clone(expr.stop)),
      step(ast::clone(expr.step)) {}
std::string SliceExpr::toString() const {
  return wrapType(format("slice{}{}{}",
                         start ? format(" #:start {}", start->toString()) : "",
                         stop ? format(" #:end {}", stop->toString()) : "",
                         step ? format(" #:step {}", step->toString()) : ""));
}
ACCEPT_IMPL(SliceExpr, ASTVisitor);

EllipsisExpr::EllipsisExpr(EllipsisType mode) : Expr(), mode(mode) {}
std::string EllipsisExpr::toString() const {
  return wrapType(format(
      "ellipsis{}", mode == PIPE ? " #:pipe" : (mode == PARTIAL ? "#:partial" : "")));
}
ACCEPT_IMPL(EllipsisExpr, ASTVisitor);

LambdaExpr::LambdaExpr(std::vector<std::string> vars, ExprPtr expr)
    : Expr(), vars(std::move(vars)), expr(std::move(expr)) {}
LambdaExpr::LambdaExpr(const LambdaExpr &expr)
    : Expr(expr), vars(expr.vars), expr(ast::clone(expr.expr)) {}
std::string LambdaExpr::toString() const {
  return wrapType(format("lambda ({}) {}", join(vars, " "), expr->toString()));
}
ACCEPT_IMPL(LambdaExpr, ASTVisitor);

YieldExpr::YieldExpr() : Expr() {}
std::string YieldExpr::toString() const { return "yield-expr"; }
ACCEPT_IMPL(YieldExpr, ASTVisitor);

AssignExpr::AssignExpr(ExprPtr var, ExprPtr expr)
    : Expr(), var(std::move(var)), expr(std::move(expr)) {}
AssignExpr::AssignExpr(const AssignExpr &expr)
    : Expr(expr), var(ast::clone(expr.var)), expr(ast::clone(expr.expr)) {}
std::string AssignExpr::toString() const {
  return wrapType(format("assign-expr '{} {}", var->toString(), expr->toString()));
}
ACCEPT_IMPL(AssignExpr, ASTVisitor);

RangeExpr::RangeExpr(ExprPtr start, ExprPtr stop)
    : Expr(), start(std::move(start)), stop(std::move(stop)) {}
RangeExpr::RangeExpr(const RangeExpr &expr)
    : Expr(expr), start(ast::clone(expr.start)), stop(ast::clone(expr.stop)) {}
std::string RangeExpr::toString() const {
  return wrapType(format("range {} {}", start->toString(), stop->toString()));
}
ACCEPT_IMPL(RangeExpr, ASTVisitor);

StmtExpr::StmtExpr(std::vector<std::shared_ptr<Stmt>> stmts, ExprPtr expr)
    : Expr(), stmts(std::move(stmts)), expr(std::move(expr)) {}
StmtExpr::StmtExpr(std::shared_ptr<Stmt> stmt, ExprPtr expr)
    : Expr(), expr(std::move(expr)) {
  stmts.push_back(std::move(stmt));
}
StmtExpr::StmtExpr(std::shared_ptr<Stmt> stmt, std::shared_ptr<Stmt> stmt2,
                   ExprPtr expr)
    : Expr(), expr(std::move(expr)) {
  stmts.push_back(std::move(stmt));
  stmts.push_back(std::move(stmt2));
}
StmtExpr::StmtExpr(const StmtExpr &expr)
    : Expr(expr), stmts(ast::clone(expr.stmts)), expr(ast::clone(expr.expr)) {}
std::string StmtExpr::toString() const {
  return wrapType(format("stmt-expr ({}) {}", combine(stmts, " "), expr->toString()));
}
ACCEPT_IMPL(StmtExpr, ASTVisitor);

InstantiateExpr::InstantiateExpr(ExprPtr typeExpr, std::vector<ExprPtr> typeParams)
    : Expr(), typeExpr(std::move(typeExpr)), typeParams(std::move(typeParams)) {}
InstantiateExpr::InstantiateExpr(ExprPtr typeExpr, ExprPtr typeParam)
    : Expr(), typeExpr(std::move(typeExpr)) {
  typeParams.push_back(std::move(typeParam));
}
InstantiateExpr::InstantiateExpr(const InstantiateExpr &expr)
    : Expr(expr), typeExpr(ast::clone(expr.typeExpr)),
      typeParams(ast::clone(expr.typeParams)) {}
std::string InstantiateExpr::toString() const {
  return wrapType(
      format("instantiate {} {}", typeExpr->toString(), combine(typeParams)));
}
ACCEPT_IMPL(InstantiateExpr, ASTVisitor);

StaticValue::Type getStaticGeneric(Expr *e) {
  if (e && e->getIndex() && e->getIndex()->expr->isId("Static")) {
    if (e->getIndex()->index && e->getIndex()->index->isId("str"))
      return StaticValue::Type::STRING;
    if (e->getIndex()->index && e->getIndex()->index->isId("int"))
      return StaticValue::Type::INT;
    return StaticValue::Type::NOT_SUPPORTED;
  }
  return StaticValue::Type::NOT_STATIC;
}

} // namespace codon::ast
