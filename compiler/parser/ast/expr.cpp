/*
 * expr.cpp --- Seq AST expressions.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include <memory>
#include <string>
#include <vector>

#include "expr.h"
#include "parser/ast.h"
#include "parser/visitors/visitor.h"

#define ACCEPT_IMPL(T, X)                                                              \
  ExprPtr T::clone() const { return make_shared<T>(*this); }                           \
  void T::accept(X &visitor) { visitor.visit(this); }

using fmt::format;
using std::move;

namespace seq {
namespace ast {

Expr::Expr()
    : type(nullptr), isTypeExpr(false), staticValue(StaticValue::NOT_STATIC),
      done(false) {}
types::TypePtr Expr::getType() const { return type; }
void Expr::setType(types::TypePtr t) { this->type = move(t); }
bool Expr::isType() const { return isTypeExpr; }
void Expr::markType() { isTypeExpr = true; }
string Expr::wrapType(const string &sexpr) const {
  return format(
      "({}{}){}", sexpr, type ? format(" #:type \"{}\"", type->toString()) : "",
      // Uncommenting this breaks static unification...
      // staticValue.type ? format(" #:static {}", staticValue.toString()) : "",
      done ? "*" : "");
}
bool Expr::isStatic() const { return staticValue.type != StaticValue::NOT_STATIC; }

StaticValue::StaticValue(StaticValue::Type t) : value(), type(t), evaluated(false) {}
StaticValue::StaticValue(int64_t i) : value(i), type(INT), evaluated(true) {}
StaticValue::StaticValue(string s) : value(move(s)), type(STRING), evaluated(true) {}
bool StaticValue::operator==(const StaticValue &s) const {
  if (type != s.type || s.evaluated != evaluated)
    return false;
  return s.evaluated ? value == s.value : true;
}
string StaticValue::toString() const {
  if (type == StaticValue::NOT_STATIC)
    return "";
  if (!evaluated)
    return type == StaticValue::STRING ? "str" : "int";
  return type == StaticValue::STRING ? "\"" + escape(std::get<string>(value)) + "\""
                                     : std::to_string(std::get<int64_t>(value));
}
int64_t StaticValue::getInt() const {
  seqassert(type == StaticValue::INT, "not an int");
  return std::get<int64_t>(value);
}
string StaticValue::getString() const {
  seqassert(type == StaticValue::STRING, "not a string");
  return std::get<string>(value);
}

Param::Param(string name, ExprPtr type, ExprPtr deflt, bool generic)
    : name(move(name)), type(move(type)), deflt(move(deflt)), generic(generic) {}
string Param::toString() const {
  return format("({}{}{}{})", name, type ? " #:type " + type->toString() : "",
                deflt ? " #:default " + deflt->toString() : "",
                generic ? " #:generic" : "");
}
Param Param::clone() const {
  return Param(name, ast::clone(type), ast::clone(deflt), generic);
}

NoneExpr::NoneExpr() : Expr() {}
string NoneExpr::toString() const { return wrapType("none"); }
ACCEPT_IMPL(NoneExpr, ASTVisitor);

BoolExpr::BoolExpr(bool value) : Expr(), value(value) {
  staticValue = StaticValue(value);
}
string BoolExpr::toString() const { return wrapType(format("bool {}", int(value))); }
ACCEPT_IMPL(BoolExpr, ASTVisitor);

IntExpr::IntExpr(int64_t intValue)
    : Expr(), value(std::to_string(intValue)), intValue(intValue) {
  staticValue = StaticValue(intValue);
}
IntExpr::IntExpr(const string &value, string suffix)
    : Expr(), value(), suffix(move(suffix)), intValue(0) {
  for (auto c : value)
    if (c != '_')
      this->value += c;
}
string IntExpr::toString() const {
  return wrapType(format("int {}{}", value,
                         suffix.empty() ? "" : format(" #:suffix \"{}\"", suffix)));
}
ACCEPT_IMPL(IntExpr, ASTVisitor);

FloatExpr::FloatExpr(double floatValue)
    : Expr(), value(std::to_string(floatValue)), floatValue(floatValue) {}
FloatExpr::FloatExpr(const string &value, string suffix)
    : Expr(), value(value), suffix(move(suffix)), floatValue(0.0) {}
string FloatExpr::toString() const {
  return wrapType(format("float {}{}", value,
                         suffix.empty() ? "" : format(" #:suffix \"{}\"", suffix)));
}
ACCEPT_IMPL(FloatExpr, ASTVisitor);

StringExpr::StringExpr(vector<pair<string, string>> s) : Expr(), strings(move(s)) {
  if (strings.size() == 1 && strings.back().second.empty())
    staticValue = StaticValue(strings.back().first);
}
StringExpr::StringExpr(string value, string prefix)
    : StringExpr(vector<pair<string, string>>{{value, prefix}}) {}
string StringExpr::toString() const {
  vector<string> s;
  for (auto &vp : strings)
    s.push_back(format("\"{}\"{}", escape(vp.first),
                       vp.second.empty() ? "" : format(" #:prefix \"{}\"", vp.second)));
  return wrapType(format("string ({})", join(s)));
}
string StringExpr::getValue() const {
  seqassert(!strings.empty(), "invalid StringExpr");
  return strings[0].first;
}
ACCEPT_IMPL(StringExpr, ASTVisitor);

IdExpr::IdExpr(string value) : Expr(), value(move(value)) {}
string IdExpr::toString() const { return wrapType(format("id '{}", value)); }
ACCEPT_IMPL(IdExpr, ASTVisitor);

StarExpr::StarExpr(ExprPtr what) : Expr(), what(move(what)) {}
StarExpr::StarExpr(const StarExpr &expr) : Expr(expr), what(ast::clone(expr.what)) {}
string StarExpr::toString() const {
  return wrapType(format("star {}", what->toString()));
}
ACCEPT_IMPL(StarExpr, ASTVisitor);

KeywordStarExpr::KeywordStarExpr(ExprPtr what) : Expr(), what(move(what)) {}
KeywordStarExpr::KeywordStarExpr(const KeywordStarExpr &expr)
    : Expr(expr), what(ast::clone(expr.what)) {}
string KeywordStarExpr::toString() const {
  return wrapType(format("kwstar {}", what->toString()));
}
ACCEPT_IMPL(KeywordStarExpr, ASTVisitor);

TupleExpr::TupleExpr(vector<ExprPtr> items) : Expr(), items(move(items)) {}
TupleExpr::TupleExpr(const TupleExpr &expr)
    : Expr(expr), items(ast::clone(expr.items)) {}
string TupleExpr::toString() const {
  return wrapType(format("tuple {}", combine(items)));
}
ACCEPT_IMPL(TupleExpr, ASTVisitor);

ListExpr::ListExpr(vector<ExprPtr> items) : Expr(), items(move(items)) {}
ListExpr::ListExpr(const ListExpr &expr) : Expr(expr), items(ast::clone(expr.items)) {}
string ListExpr::toString() const {
  return wrapType(!items.empty() ? format("list {}", combine(items)) : "list");
}
ACCEPT_IMPL(ListExpr, ASTVisitor);

SetExpr::SetExpr(vector<ExprPtr> items) : Expr(), items(move(items)) {}
SetExpr::SetExpr(const SetExpr &expr) : Expr(expr), items(ast::clone(expr.items)) {}
string SetExpr::toString() const {
  return wrapType(!items.empty() ? format("set {}", combine(items)) : "set");
}
ACCEPT_IMPL(SetExpr, ASTVisitor);

DictExpr::DictItem DictExpr::DictItem::clone() const {
  return {ast::clone(key), ast::clone(value)};
}

DictExpr::DictExpr(vector<DictExpr::DictItem> items) : Expr(), items(move(items)) {}
DictExpr::DictExpr(const DictExpr &expr)
    : Expr(expr), items(ast::clone_nop(expr.items)) {}
string DictExpr::toString() const {
  vector<string> s;
  for (auto &i : items)
    s.push_back(format("({} {})", i.key->toString(), i.value->toString()));
  return wrapType(!s.empty() ? format("dict {}", join(s, " ")) : "dict");
}
ACCEPT_IMPL(DictExpr, ASTVisitor);

GeneratorBody GeneratorBody::clone() const {
  return {ast::clone(vars), ast::clone(gen), ast::clone(conds)};
}

GeneratorExpr::GeneratorExpr(GeneratorExpr::GeneratorKind kind, ExprPtr expr,
                             vector<GeneratorBody> loops)
    : Expr(), kind(kind), expr(move(expr)), loops(move(loops)) {}
GeneratorExpr::GeneratorExpr(const GeneratorExpr &expr)
    : Expr(expr), kind(expr.kind), expr(ast::clone(expr.expr)),
      loops(ast::clone_nop(expr.loops)) {}
string GeneratorExpr::toString() const {
  string prefix;
  if (kind == GeneratorKind::ListGenerator)
    prefix = "list-";
  if (kind == GeneratorKind::SetGenerator)
    prefix = "set-";
  string s;
  for (auto &i : loops) {
    string q;
    for (auto &k : i.conds)
      q += format(" (if {})", k->toString());
    s += format(" (for {} {}{})", i.vars->toString(), i.gen->toString(), q);
  }
  return wrapType(format("{}gen {}{}", prefix, expr->toString(), s));
}
ACCEPT_IMPL(GeneratorExpr, ASTVisitor);

DictGeneratorExpr::DictGeneratorExpr(ExprPtr key, ExprPtr expr,
                                     vector<GeneratorBody> loops)
    : Expr(), key(move(key)), expr(move(expr)), loops(move(loops)) {}
DictGeneratorExpr::DictGeneratorExpr(const DictGeneratorExpr &expr)
    : Expr(expr), key(ast::clone(expr.key)), expr(ast::clone(expr.expr)),
      loops(ast::clone_nop(expr.loops)) {}
string DictGeneratorExpr::toString() const {
  string s;
  for (auto &i : loops) {
    string q;
    for (auto &k : i.conds)
      q += format("( if {})", k->toString());
    s += format(" (for {} {}{})", i.vars->toString(), i.gen->toString(), q);
  }
  return wrapType(format("dict-gen {} {}{}", key->toString(), expr->toString(), s));
}
ACCEPT_IMPL(DictGeneratorExpr, ASTVisitor);

IfExpr::IfExpr(ExprPtr cond, ExprPtr ifexpr, ExprPtr elsexpr)
    : Expr(), cond(move(cond)), ifexpr(move(ifexpr)), elsexpr(move(elsexpr)) {}
IfExpr::IfExpr(const IfExpr &expr)
    : Expr(expr), cond(ast::clone(expr.cond)), ifexpr(ast::clone(expr.ifexpr)),
      elsexpr(ast::clone(expr.elsexpr)) {}
string IfExpr::toString() const {
  return wrapType(format("if-expr {} {} {}", cond->toString(), ifexpr->toString(),
                         elsexpr->toString()));
}
ACCEPT_IMPL(IfExpr, ASTVisitor);

UnaryExpr::UnaryExpr(string op, ExprPtr expr)
    : Expr(), op(move(op)), expr(move(expr)) {}
UnaryExpr::UnaryExpr(const UnaryExpr &expr)
    : Expr(expr), op(expr.op), expr(ast::clone(expr.expr)) {}
string UnaryExpr::toString() const {
  return wrapType(format("unary \"{}\" {}", op, expr->toString()));
}
ACCEPT_IMPL(UnaryExpr, ASTVisitor);

BinaryExpr::BinaryExpr(ExprPtr lexpr, string op, ExprPtr rexpr, bool inPlace)
    : Expr(), op(move(op)), lexpr(move(lexpr)), rexpr(move(rexpr)), inPlace(inPlace) {}
BinaryExpr::BinaryExpr(const BinaryExpr &expr)
    : Expr(expr), op(expr.op), lexpr(ast::clone(expr.lexpr)),
      rexpr(ast::clone(expr.rexpr)), inPlace(expr.inPlace) {}
string BinaryExpr::toString() const {
  return wrapType(format("binary \"{}\" {} {}{}", op, lexpr->toString(),
                         rexpr->toString(), inPlace ? " #:in-place" : ""));
}
ACCEPT_IMPL(BinaryExpr, ASTVisitor);

ChainBinaryExpr::ChainBinaryExpr(vector<pair<string, ExprPtr>> exprs)
    : Expr(), exprs(move(exprs)) {}
ChainBinaryExpr::ChainBinaryExpr(const ChainBinaryExpr &expr) : Expr(expr) {
  for (auto &e : expr.exprs)
    exprs.emplace_back(make_pair(e.first, ast::clone(e.second)));
}
string ChainBinaryExpr::toString() const {
  vector<string> s;
  for (auto &i : exprs)
    s.push_back(format("({} \"{}\")", i.first, i.second->toString()));
  return wrapType(format("chain {}", join(s, " ")));
}
ACCEPT_IMPL(ChainBinaryExpr, ASTVisitor);

PipeExpr::Pipe PipeExpr::Pipe::clone() const { return {op, ast::clone(expr)}; }

PipeExpr::PipeExpr(vector<PipeExpr::Pipe> items) : Expr(), items(move(items)) {}
PipeExpr::PipeExpr(const PipeExpr &expr)
    : Expr(expr), items(ast::clone_nop(expr.items)), inTypes(expr.inTypes) {}
string PipeExpr::toString() const {
  vector<string> s;
  for (auto &i : items)
    s.push_back(format("({} \"{}\")", i.expr->toString(), i.op));
  return wrapType(format("pipe {}", join(s, " ")));
}
ACCEPT_IMPL(PipeExpr, ASTVisitor);

IndexExpr::IndexExpr(ExprPtr expr, ExprPtr index)
    : Expr(), expr(move(expr)), index(move(index)) {}
IndexExpr::IndexExpr(const IndexExpr &expr)
    : Expr(expr), expr(ast::clone(expr.expr)), index(ast::clone(expr.index)) {}
string IndexExpr::toString() const {
  return wrapType(format("index {} {}", expr->toString(), index->toString()));
}
ACCEPT_IMPL(IndexExpr, ASTVisitor);

CallExpr::Arg CallExpr::Arg::clone() const { return {name, ast::clone(value)}; }

CallExpr::CallExpr(const CallExpr &expr)
    : Expr(expr), expr(ast::clone(expr.expr)), args(ast::clone_nop(expr.args)),
      ordered(expr.ordered) {}
CallExpr::CallExpr(ExprPtr expr, vector<CallExpr::Arg> args)
    : Expr(), expr(move(expr)), args(move(args)), ordered(false) {}
CallExpr::CallExpr(ExprPtr expr, vector<ExprPtr> args)
    : expr(move(expr)), ordered(false) {
  for (auto &a : args)
    if (a)
      this->args.push_back({"", move(a)});
}
string CallExpr::toString() const {
  string s;
  for (auto &i : args)
    if (i.name.empty())
      s += " " + i.value->toString();
    else
      s += format("({}{})", i.value->toString(),
                  i.name.empty() ? "" : format(" #:name '{}", i.name));
  return wrapType(format("call {}{}", expr->toString(), s));
}
ACCEPT_IMPL(CallExpr, ASTVisitor);

DotExpr::DotExpr(ExprPtr expr, string member)
    : Expr(), expr(move(expr)), member(move(member)) {}
DotExpr::DotExpr(string left, string member)
    : Expr(), expr(make_shared<IdExpr>(left)), member(move(member)) {}
DotExpr::DotExpr(const DotExpr &expr)
    : Expr(expr), expr(ast::clone(expr.expr)), member(expr.member) {}
string DotExpr::toString() const {
  return wrapType(format("dot {} '{}", expr->toString(), member));
}
ACCEPT_IMPL(DotExpr, ASTVisitor);

SliceExpr::SliceExpr(ExprPtr start, ExprPtr stop, ExprPtr step)
    : Expr(), start(move(start)), stop(move(stop)), step(move(step)) {}
SliceExpr::SliceExpr(const SliceExpr &expr)
    : Expr(expr), start(ast::clone(expr.start)), stop(ast::clone(expr.stop)),
      step(ast::clone(expr.step)) {}
string SliceExpr::toString() const {
  return wrapType(format("slice{}{}{}",
                         start ? format(" #:start {}", start->toString()) : "",
                         stop ? format(" #:end {}", stop->toString()) : "",
                         step ? format(" #:step {}", step->toString()) : ""));
}
ACCEPT_IMPL(SliceExpr, ASTVisitor);

EllipsisExpr::EllipsisExpr(bool isPipeArg) : Expr(), isPipeArg(isPipeArg) {}
string EllipsisExpr::toString() const { return wrapType("ellipsis"); }
ACCEPT_IMPL(EllipsisExpr, ASTVisitor);

LambdaExpr::LambdaExpr(vector<string> vars, ExprPtr expr)
    : Expr(), vars(move(vars)), expr(move(expr)) {}
LambdaExpr::LambdaExpr(const LambdaExpr &expr)
    : Expr(expr), vars(expr.vars), expr(ast::clone(expr.expr)) {}
string LambdaExpr::toString() const {
  return wrapType(format("lambda ({}) {}", join(vars, " "), expr->toString()));
}
ACCEPT_IMPL(LambdaExpr, ASTVisitor);

YieldExpr::YieldExpr() : Expr() {}
string YieldExpr::toString() const { return "yield-expr"; }
ACCEPT_IMPL(YieldExpr, ASTVisitor);

AssignExpr::AssignExpr(ExprPtr var, ExprPtr expr)
    : Expr(), var(move(var)), expr(move(expr)) {}
AssignExpr::AssignExpr(const AssignExpr &expr)
    : Expr(expr), var(ast::clone(expr.var)), expr(ast::clone(expr.expr)) {}
string AssignExpr::toString() const {
  return wrapType(format("assign-expr '{} {}", var->toString(), expr->toString()));
}
ACCEPT_IMPL(AssignExpr, ASTVisitor);

RangeExpr::RangeExpr(ExprPtr start, ExprPtr stop)
    : Expr(), start(move(start)), stop(move(stop)) {}
RangeExpr::RangeExpr(const RangeExpr &expr)
    : Expr(expr), start(ast::clone(expr.start)), stop(ast::clone(expr.stop)) {}
string RangeExpr::toString() const {
  return wrapType(format("range {} {}", start->toString(), stop->toString()));
}
ACCEPT_IMPL(RangeExpr, ASTVisitor);

StmtExpr::StmtExpr(vector<shared_ptr<Stmt>> stmts, ExprPtr expr)
    : Expr(), stmts(move(stmts)), expr(move(expr)) {}
StmtExpr::StmtExpr(shared_ptr<Stmt> stmt, ExprPtr expr) : Expr(), expr(move(expr)) {
  stmts.push_back(move(stmt));
}
StmtExpr::StmtExpr(shared_ptr<Stmt> stmt, shared_ptr<Stmt> stmt2, ExprPtr expr)
    : Expr(), expr(move(expr)) {
  stmts.push_back(move(stmt));
  stmts.push_back(move(stmt2));
}
StmtExpr::StmtExpr(const StmtExpr &expr)
    : Expr(expr), stmts(ast::clone(expr.stmts)), expr(ast::clone(expr.expr)) {}
string StmtExpr::toString() const {
  return wrapType(format("stmt-expr ({}) {}", combine(stmts, " "), expr->toString()));
}
ACCEPT_IMPL(StmtExpr, ASTVisitor);

PtrExpr::PtrExpr(ExprPtr expr) : Expr(), expr(move(expr)) {}
PtrExpr::PtrExpr(const PtrExpr &expr) : Expr(expr), expr(ast::clone(expr.expr)) {}
string PtrExpr::toString() const {
  return wrapType(format("ptr {}", expr->toString()));
}
ACCEPT_IMPL(PtrExpr, ASTVisitor);

TupleIndexExpr::TupleIndexExpr(ExprPtr expr, int index)
    : Expr(), expr(move(expr)), index(index) {}
TupleIndexExpr::TupleIndexExpr(const TupleIndexExpr &expr)
    : Expr(expr), expr(ast::clone(expr.expr)), index(expr.index) {}
string TupleIndexExpr::toString() const {
  return wrapType(format("tuple-idx {} {}", expr->toString(), index));
}
ACCEPT_IMPL(TupleIndexExpr, ASTVisitor);

InstantiateExpr::InstantiateExpr(ExprPtr typeExpr, vector<ExprPtr> typeParams)
    : Expr(), typeExpr(move(typeExpr)), typeParams(move(typeParams)) {}
InstantiateExpr::InstantiateExpr(ExprPtr typeExpr, ExprPtr typeParam)
    : Expr(), typeExpr(move(typeExpr)) {
  typeParams.push_back(move(typeParam));
}
InstantiateExpr::InstantiateExpr(const InstantiateExpr &expr)
    : Expr(expr), typeExpr(ast::clone(expr.typeExpr)),
      typeParams(ast::clone(expr.typeParams)) {}
string InstantiateExpr::toString() const {
  return wrapType(
      format("instantiate {} {}", typeExpr->toString(), combine(typeParams)));
}
ACCEPT_IMPL(InstantiateExpr, ASTVisitor);

StackAllocExpr::StackAllocExpr(ExprPtr typeExpr, ExprPtr expr)
    : Expr(), typeExpr(move(typeExpr)), expr(move(expr)) {}
StackAllocExpr::StackAllocExpr(const StackAllocExpr &expr)
    : Expr(expr), typeExpr(ast::clone(expr.typeExpr)), expr(ast::clone(expr.expr)) {}
string StackAllocExpr::toString() const {
  return wrapType(format("stack-alloc {} {}", typeExpr->toString(), expr->toString()));
}
ACCEPT_IMPL(StackAllocExpr, ASTVisitor);

} // namespace ast
} // namespace seq
