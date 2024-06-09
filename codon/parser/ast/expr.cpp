// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "expr.h"

#include <memory>
#include <string>
#include <vector>

#include "codon/cir/attribute.h"
#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/visitor.h"

#define ACCEPT_IMPL(T, X)                                                              \
  Node *T::clone(bool c) const { return cache->N<T>(*this, c); }                       \
  void T::accept(X &visitor) { visitor.visit(this); }

using fmt::format;
using namespace codon::error;

namespace codon::ast {

const std::string Attr::Module = "module";
const std::string Attr::ParentClass = "parentClass";
const std::string Attr::Attributes = "attributes";
const std::string Attr::Bindings = "bindings";

const std::string Attr::LLVM = "llvm";
const std::string Attr::Python = "python";
const std::string Attr::Atomic = "atomic";
const std::string Attr::Property = "property";
const std::string Attr::StaticMethod = "staticmethod";
const std::string Attr::Attribute = "__attribute__";
const std::string Attr::C = "C";

const std::string Attr::Internal = "__internal__";
const std::string Attr::HiddenFromUser = "__hidden__";
const std::string Attr::ForceRealize = "__force__";
const std::string Attr::RealizeWithoutSelf =
    "std.internal.attributes.realize_without_self.0:0";

const std::string Attr::CVarArg = ".__vararg__";
const std::string Attr::Method = ".__method__";
const std::string Attr::Capture = ".__capture__";
const std::string Attr::HasSelf = ".__hasself__";
const std::string Attr::IsGenerator = ".__generator__";

const std::string Attr::Extend = "extend";
const std::string Attr::Tuple = "tuple";
const std::string Attr::ClassDeduce = "deduce";
const std::string Attr::ClassNoTuple = "__notuple__";

const std::string Attr::Test = "std.internal.attributes.test.0:0";
const std::string Attr::Overload = "overload:0";
const std::string Attr::Export = "std.internal.attributes.export.0:0";

const std::string Attr::ClassMagic = "classMagic";
const std::string Attr::ExprSequenceItem = "exprSequenceItem";
const std::string Attr::ExprStarSequenceItem = "exprStarSequenceItem";
const std::string Attr::ExprList = "exprList";
const std::string Attr::ExprSet = "exprSet";
const std::string Attr::ExprDict = "exprDict";
const std::string Attr::ExprPartial = "exprPartial";
const std::string Attr::ExprDominated = "exprDominated";
const std::string Attr::ExprStarArgument = "exprStarArgument";
const std::string Attr::ExprKwStarArgument = "exprKwStarArgument";
const std::string Attr::ExprOrderedCall = "exprOrderedCall";
const std::string Attr::ExprExternVar = "exprExternVar";
const std::string Attr::ExprDominatedUndefCheck = "exprDominatedUndefCheck";
const std::string Attr::ExprDominatedUsed = "exprDominatedUsed";

Node::Node(const Node &node)
    : cache(node.cache), attributes(codon::clone(node.attributes)) {}

Expr::Expr() : Node(), type(nullptr), done(false), origExpr(nullptr) {}
Expr::Expr(const Expr &expr)
    : Node(expr), type(expr.type), done(expr.done), origExpr(expr.origExpr) {}
Expr::Expr(const Expr &expr, bool clean) : Expr(expr) {
  if (clean) {
    type = nullptr;
    done = false;
  }
}
void Expr::validate() const {}
types::TypePtr Expr::getType() const { return type; }
void Expr::setType(types::TypePtr t) { this->type = std::move(t); }
std::string Expr::wrapType(const std::string &sexpr) const {
  auto is = sexpr;
  if (done)
    is.insert(findStar(is), "*");
  auto s = format("({}{})", is,
                  type && !done ? format(" #:type \"{}\"", type->debugString(2)) : "");
  return s;
}
std::string Expr::getTypeName() {
  if (getId()) {
    return getId()->value;
  } else {
    auto i = CAST(this, InstantiateExpr);
    seqassertn(i && i->typeExpr->getId(), "bad type expr");
    return i->typeExpr->getId()->value;
  }
}

Param::Param(std::string name, Expr *type, Expr *defaultValue, int status)
    : name(std::move(name)), type(type), defaultValue(defaultValue) {
  if (status == 0 && this->type &&
      (this->type->isId("type") || this->type->isId(TYPE_TYPEVAR) ||
       (this->type->getIndex() && this->type->getIndex()->expr->isId(TYPE_TYPEVAR)) ||
       getStaticGeneric(this->type)))
    this->status = Generic;
  else
    this->status = (status == 0 ? Normal : (status == 1 ? Generic : HiddenGeneric));
}
Param::Param(const SrcInfo &info, std::string name, Expr *type, Expr *defaultValue,
             int status)
    : Param(name, type, defaultValue, status) {
  setSrcInfo(info);
}
std::string Param::toString(int indent) const {
  return format("({}{}{}{})", name, type ? " #:type " + type->toString(indent) : "",
                defaultValue ? " #:default " + defaultValue->toString(indent) : "",
                status != Param::Normal ? " #:generic" : "");
}
Param Param::clone(bool clean) const {
  return Param(name, ast::clone(type, clean), ast::clone(defaultValue, clean), status);
}

NoneExpr::NoneExpr() : Expr() {}
NoneExpr::NoneExpr(const NoneExpr &expr, bool clean) : Expr(expr, clean) {}
std::string NoneExpr::toString(int) const { return wrapType("none"); }
ACCEPT_IMPL(NoneExpr, ASTVisitor);

BoolExpr::BoolExpr(bool value) : Expr(), value(value) {}
BoolExpr::BoolExpr(const BoolExpr &expr, bool clean)
    : Expr(expr, clean), value(expr.value) {}
std::string BoolExpr::toString(int) const {
  return wrapType(format("bool {}", int(value)));
}
ACCEPT_IMPL(BoolExpr, ASTVisitor);

IntExpr::IntExpr(int64_t intValue) : Expr(), value(std::to_string(intValue)) {
  this->intValue = std::make_unique<int64_t>(intValue);
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
IntExpr::IntExpr(const IntExpr &expr, bool clean)
    : Expr(expr, clean), value(expr.value), suffix(expr.suffix) {
  intValue = expr.intValue ? std::make_unique<int64_t>(*(expr.intValue)) : nullptr;
}
std::string IntExpr::toString(int) const {
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
  try {
    floatValue = std::make_unique<double>(std::stod(value));
  } catch (std::out_of_range &) {
    floatValue = nullptr;
  }
}
FloatExpr::FloatExpr(const FloatExpr &expr, bool clean)
    : Expr(expr, clean), value(expr.value), suffix(expr.suffix) {
  floatValue = expr.floatValue ? std::make_unique<double>(*(expr.floatValue)) : nullptr;
}
std::string FloatExpr::toString(int) const {
  return wrapType(format("float {}{}", value,
                         suffix.empty() ? "" : format(" #:suffix \"{}\"", suffix)));
}
ACCEPT_IMPL(FloatExpr, ASTVisitor);

StringExpr::StringExpr(std::vector<StringExpr::String> s)
    : Expr(), strings(std::move(s)) {}
StringExpr::StringExpr(std::string value, std::string prefix)
    : StringExpr(std::vector<StringExpr::String>{{value, prefix}}) {
  unpack();
}
StringExpr::StringExpr(const StringExpr &expr, bool clean)
    : Expr(expr, clean), strings(expr.strings) {
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
  seqassert(!strings.empty(), "invalid StringExpr");
  return strings[0].value;
}
ACCEPT_IMPL(StringExpr, ASTVisitor);
void StringExpr::unpack() {
  std::vector<String> exprs;
  for (auto &p : strings) {
    if (p.prefix == "f" || p.prefix == "F") {
      /// Transform an F-string
      for (auto pf : unpackFString(p.value)) {
        if (pf.prefix.empty() && !exprs.empty() && exprs.back().prefix.empty()) {
          exprs.back().prefix += pf.prefix;
        } else {
          exprs.emplace_back(pf);
        }
      }
    } else if (!p.prefix.empty()) {
      exprs.emplace_back(p);
    } else if (!exprs.empty() && exprs.back().prefix.empty()) {
      exprs.back().prefix += p.prefix;
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

IdExpr::IdExpr(std::string value) : Expr(), value(std::move(value)) {}
IdExpr::IdExpr(const IdExpr &expr, bool clean) : Expr(expr, clean), value(expr.value) {}
std::string IdExpr::toString(int) const {
  return !type ? format("'{}", value) : wrapType(format("'{}", value));
}
ACCEPT_IMPL(IdExpr, ASTVisitor);

StarExpr::StarExpr(Expr *what) : Expr(), what(what) {}
StarExpr::StarExpr(const StarExpr &expr, bool clean)
    : Expr(expr, clean), what(ast::clone(expr.what, clean)) {}
std::string StarExpr::toString(int indent) const {
  return wrapType(format("star {}", what->toString(indent)));
}
ACCEPT_IMPL(StarExpr, ASTVisitor);

KeywordStarExpr::KeywordStarExpr(Expr *what) : Expr(), what(what) {}
KeywordStarExpr::KeywordStarExpr(const KeywordStarExpr &expr, bool clean)
    : Expr(expr, clean), what(ast::clone(expr.what, clean)) {}
std::string KeywordStarExpr::toString(int indent) const {
  return wrapType(format("kwstar {}", what->toString(indent)));
}
ACCEPT_IMPL(KeywordStarExpr, ASTVisitor);

TupleExpr::TupleExpr(std::vector<Expr *> items) : Expr(), items(std::move(items)) {}
TupleExpr::TupleExpr(const TupleExpr &expr, bool clean)
    : Expr(expr, clean), items(ast::clone(expr.items, clean)) {}
std::string TupleExpr::toString(int) const {
  return wrapType(format("tuple {}", combine(items)));
}
ACCEPT_IMPL(TupleExpr, ASTVisitor);

ListExpr::ListExpr(std::vector<Expr *> items) : Expr(), items(std::move(items)) {}
ListExpr::ListExpr(const ListExpr &expr, bool clean)
    : Expr(expr, clean), items(ast::clone(expr.items, clean)) {}
std::string ListExpr::toString(int) const {
  return wrapType(!items.empty() ? format("list {}", combine(items)) : "list");
}
ACCEPT_IMPL(ListExpr, ASTVisitor);

SetExpr::SetExpr(std::vector<Expr *> items) : Expr(), items(std::move(items)) {}
SetExpr::SetExpr(const SetExpr &expr, bool clean)
    : Expr(expr, clean), items(ast::clone(expr.items, clean)) {}
std::string SetExpr::toString(int) const {
  return wrapType(!items.empty() ? format("set {}", combine(items)) : "set");
}
ACCEPT_IMPL(SetExpr, ASTVisitor);

DictExpr::DictExpr(std::vector<Expr *> items) : Expr(), items(std::move(items)) {
  for (const auto &i : this->items) {
    auto t = i->getTuple();
    seqassertn(t && t->items.size() == 2, "dictionary items are invalid");
  }
}
DictExpr::DictExpr(const DictExpr &expr, bool clean)
    : Expr(expr, clean), items(ast::clone(expr.items, clean)) {}
std::string DictExpr::toString(int) const {
  return wrapType(!items.empty() ? format("dict {}", combine(items)) : "set");
}
ACCEPT_IMPL(DictExpr, ASTVisitor);

GeneratorExpr::GeneratorExpr(Cache *cache, GeneratorExpr::GeneratorKind kind,
                             Expr *expr, std::vector<Stmt *> loops)
    : Expr(), kind(kind) {
  this->cache = cache;
  seqassert(!loops.empty() && loops[0]->getFor(), "bad generator constructor");
  loops.push_back(cache->N<SuiteStmt>(cache->N<ExprStmt>(expr)));
  formCompleteStmt(loops);
}
GeneratorExpr::GeneratorExpr(Cache *cache, Expr *key, Expr *expr,
                             std::vector<Stmt *> loops)
    : Expr(), kind(GeneratorExpr::DictGenerator) {
  this->cache = cache;
  seqassert(!loops.empty() && loops[0]->getFor(), "bad generator constructor");
  Expr *t = cache->N<TupleExpr>(std::vector<Expr *>{key, expr});
  loops.push_back(cache->N<SuiteStmt>(cache->N<ExprStmt>(t)));
  formCompleteStmt(loops);
}
GeneratorExpr::GeneratorExpr(const GeneratorExpr &expr, bool clean)
    : Expr(expr, clean), kind(expr.kind), loops(ast::clone(expr.loops, clean)) {}
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
ACCEPT_IMPL(GeneratorExpr, ASTVisitor);
Expr *GeneratorExpr::getFinalExpr() {
  auto s = *(getFinalStmt());
  if (s->getExpr())
    return s->getExpr()->expr;
  return nullptr;
}
int GeneratorExpr::loopCount() const {
  int cnt = 0;
  for (Stmt *i = loops;;) {
    if (auto sf = i->getFor()) {
      i = sf->suite;
      cnt++;
    } else if (auto si = i->getIf()) {
      i = si->ifSuite;
      cnt++;
    } else if (auto ss = i->getSuite()) {
      if (ss->stmts.empty())
        break;
      i = ss->stmts.back();
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
    if (auto sf = (*i)->getFor())
      i = (Stmt **)&sf->suite;
    else if (auto si = (*i)->getIf())
      i = (Stmt **)&si->ifSuite;
    else if (auto ss = (*i)->getSuite()) {
      if (ss->stmts.empty())
        return i;
      i = &(ss->stmts.back());
    } else
      return i;
  }
  seqassert(false, "bad generator");
  return nullptr;
}
void GeneratorExpr::formCompleteStmt(const std::vector<Stmt *> &loops) {
  Stmt *final = nullptr;
  for (size_t i = loops.size(); i-- > 0;) {
    if (auto si = loops[i]->getIf())
      si->ifSuite = SuiteStmt::wrap(final);
    else if (auto sf = loops[i]->getFor())
      sf->suite = SuiteStmt::wrap(final);
    final = loops[i];
  }
  this->loops = loops[0];
}
// Stmt * &GeneratorExpr::getFinalStmt(Stmt * &s) {
//   if (auto i = s->getIf())
//     return getFinalStmt(i->ifSuite);
//   if (auto f = s->getFor())
//     return getFinalStmt(f->suite);
//   return s;
// }
// Stmt * &GeneratorExpr::getFinalStmt() { return getFinalStmt(loops); }

IfExpr::IfExpr(Expr *cond, Expr *ifexpr, Expr *elsexpr)
    : Expr(), cond(cond), ifexpr(ifexpr), elsexpr(elsexpr) {}
IfExpr::IfExpr(const IfExpr &expr, bool clean)
    : Expr(expr, clean), cond(ast::clone(expr.cond, clean)),
      ifexpr(ast::clone(expr.ifexpr, clean)), elsexpr(ast::clone(expr.elsexpr, clean)) {
}
std::string IfExpr::toString(int indent) const {
  return wrapType(format("if-expr {} {} {}", cond->toString(indent),
                         ifexpr->toString(indent), elsexpr->toString(indent)));
}
ACCEPT_IMPL(IfExpr, ASTVisitor);

UnaryExpr::UnaryExpr(std::string op, Expr *expr)
    : Expr(), op(std::move(op)), expr(expr) {}
UnaryExpr::UnaryExpr(const UnaryExpr &expr, bool clean)
    : Expr(expr, clean), op(expr.op), expr(ast::clone(expr.expr, clean)) {}
std::string UnaryExpr::toString(int indent) const {
  return wrapType(format("unary \"{}\" {}", op, expr->toString(indent)));
}
ACCEPT_IMPL(UnaryExpr, ASTVisitor);

BinaryExpr::BinaryExpr(Expr *lexpr, std::string op, Expr *rexpr, bool inPlace)
    : Expr(), op(std::move(op)), lexpr(lexpr), rexpr(rexpr), inPlace(inPlace) {}
BinaryExpr::BinaryExpr(const BinaryExpr &expr, bool clean)
    : Expr(expr, clean), op(expr.op), lexpr(ast::clone(expr.lexpr, clean)),
      rexpr(ast::clone(expr.rexpr, clean)), inPlace(expr.inPlace) {}
std::string BinaryExpr::toString(int indent) const {
  return wrapType(format("binary \"{}\" {} {}{}", op, lexpr->toString(indent),
                         rexpr->toString(indent), inPlace ? " #:in-place" : ""));
}
ACCEPT_IMPL(BinaryExpr, ASTVisitor);

ChainBinaryExpr::ChainBinaryExpr(std::vector<std::pair<std::string, Expr *>> exprs)
    : Expr(), exprs(std::move(exprs)) {}
ChainBinaryExpr::ChainBinaryExpr(const ChainBinaryExpr &expr, bool clean)
    : Expr(expr, clean) {
  for (auto &e : expr.exprs)
    exprs.emplace_back(make_pair(e.first, ast::clone(e.second, clean)));
}
std::string ChainBinaryExpr::toString(int indent) const {
  std::vector<std::string> s;
  for (auto &i : exprs)
    s.push_back(format("({} \"{}\")", i.first, i.second->toString(indent)));
  return wrapType(format("chain {}", join(s, " ")));
}
ACCEPT_IMPL(ChainBinaryExpr, ASTVisitor);

PipeExpr::Pipe PipeExpr::Pipe::clone(bool clean) const {
  return {op, ast::clone(expr, clean)};
}

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
PipeExpr::PipeExpr(const PipeExpr &expr, bool clean)
    : Expr(expr, clean), items(ast::clone(expr.items, clean)), inTypes(expr.inTypes) {}
void PipeExpr::validate() const {}
std::string PipeExpr::toString(int indent) const {
  std::vector<std::string> s;
  for (auto &i : items)
    s.push_back(format("({} \"{}\")", i.expr->toString(indent), i.op));
  return wrapType(format("pipe {}", join(s, " ")));
}
ACCEPT_IMPL(PipeExpr, ASTVisitor);

IndexExpr::IndexExpr(Expr *expr, Expr *index) : Expr(), expr(expr), index(index) {}
IndexExpr::IndexExpr(const IndexExpr &expr, bool clean)
    : Expr(expr, clean), expr(ast::clone(expr.expr, clean)),
      index(ast::clone(expr.index, clean)) {}
std::string IndexExpr::toString(int indent) const {
  return wrapType(
      format("index {} {}", expr->toString(indent), index->toString(indent)));
}
ACCEPT_IMPL(IndexExpr, ASTVisitor);

CallExpr::Arg CallExpr::Arg::clone(bool clean) const {
  return {name, ast::clone(value, clean)};
}
CallExpr::Arg::Arg(const SrcInfo &info, const std::string &name, Expr *value)
    : name(name), value(value) {
  setSrcInfo(info);
}
CallExpr::Arg::Arg(const std::string &name, Expr *value) : name(name), value(value) {
  if (value)
    setSrcInfo(value->getSrcInfo());
}
CallExpr::Arg::Arg(Expr *value) : CallExpr::Arg("", value) {}

CallExpr::CallExpr(const CallExpr &expr, bool clean)
    : Expr(expr, clean), expr(ast::clone(expr.expr, clean)),
      args(ast::clone(expr.args, clean)), ordered(expr.ordered), partial(expr.partial) {
}
CallExpr::CallExpr(Expr *expr, std::vector<CallExpr::Arg> args)
    : Expr(), expr(expr), args(std::move(args)), ordered(false), partial(false) {
  validate();
}
CallExpr::CallExpr(Expr *expr, std::vector<Expr *> args)
    : expr(expr), ordered(false), partial(false) {
  for (auto a : args)
    if (a)
      this->args.emplace_back("", a);
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
std::string CallExpr::toString(int indent) const {
  std::vector<std::string> s;
  auto pad = indent >= 0 ? ("\n" + std::string(indent + 2 * INDENT_SIZE, ' ')) : " ";
  for (auto &i : args) {
    if (i.name.empty())
      s.emplace_back(pad + format("#:name '{}", i.name));
    s.emplace_back(pad +
                   i.value->toString(indent >= 0 ? indent + 2 * INDENT_SIZE : -1));
  }
  return wrapType(format("call{} {}{}", partial ? "-partial" : "",
                         expr->toString(indent), fmt::join(s, "")));
}
ACCEPT_IMPL(CallExpr, ASTVisitor);

DotExpr::DotExpr(Expr *expr, std::string member)
    : Expr(), expr(expr), member(std::move(member)) {}
DotExpr::DotExpr(const DotExpr &expr, bool clean)
    : Expr(expr, clean), expr(ast::clone(expr.expr, clean)), member(expr.member) {}
std::string DotExpr::toString(int indent) const {
  return wrapType(format("dot {} '{}", expr->toString(indent), member));
}
ACCEPT_IMPL(DotExpr, ASTVisitor);

SliceExpr::SliceExpr(Expr *start, Expr *stop, Expr *step)
    : Expr(), start(start), stop(stop), step(step) {}
SliceExpr::SliceExpr(const SliceExpr &expr, bool clean)
    : Expr(expr, clean), start(ast::clone(expr.start, clean)),
      stop(ast::clone(expr.stop, clean)), step(ast::clone(expr.step, clean)) {}
std::string SliceExpr::toString(int indent) const {
  return wrapType(format("slice{}{}{}",
                         start ? format(" #:start {}", start->toString(indent)) : "",
                         stop ? format(" #:end {}", stop->toString(indent)) : "",
                         step ? format(" #:step {}", step->toString(indent)) : ""));
}
ACCEPT_IMPL(SliceExpr, ASTVisitor);

EllipsisExpr::EllipsisExpr(EllipsisType mode) : Expr(), mode(mode) {}
EllipsisExpr::EllipsisExpr(const EllipsisExpr &expr, bool clean)
    : Expr(expr, clean), mode(expr.mode) {}
std::string EllipsisExpr::toString(int) const {
  return wrapType(format(
      "ellipsis{}", mode == PIPE ? " #:pipe" : (mode == PARTIAL ? "#:partial" : "")));
}
ACCEPT_IMPL(EllipsisExpr, ASTVisitor);

LambdaExpr::LambdaExpr(std::vector<std::string> vars, Expr *expr)
    : Expr(), vars(std::move(vars)), expr(expr) {}
LambdaExpr::LambdaExpr(const LambdaExpr &expr, bool clean)
    : Expr(expr, clean), vars(expr.vars), expr(ast::clone(expr.expr, clean)) {}
std::string LambdaExpr::toString(int indent) const {
  return wrapType(format("lambda ({}) {}", join(vars, " "), expr->toString(indent)));
}
ACCEPT_IMPL(LambdaExpr, ASTVisitor);

YieldExpr::YieldExpr() : Expr() {}
YieldExpr::YieldExpr(const YieldExpr &expr, bool clean) : Expr(expr, clean) {}
std::string YieldExpr::toString(int) const { return "yield-expr"; }
ACCEPT_IMPL(YieldExpr, ASTVisitor);

AssignExpr::AssignExpr(Expr *var, Expr *expr) : Expr(), var(var), expr(expr) {}
AssignExpr::AssignExpr(const AssignExpr &expr, bool clean)
    : Expr(expr, clean), var(ast::clone(expr.var, clean)),
      expr(ast::clone(expr.expr, clean)) {}
std::string AssignExpr::toString(int indent) const {
  return wrapType(
      format("assign-expr '{} {}", var->toString(indent), expr->toString(indent)));
}
ACCEPT_IMPL(AssignExpr, ASTVisitor);

RangeExpr::RangeExpr(Expr *start, Expr *stop) : Expr(), start(start), stop(stop) {}
RangeExpr::RangeExpr(const RangeExpr &expr, bool clean)
    : Expr(expr, clean), start(ast::clone(expr.start, clean)),
      stop(ast::clone(expr.stop, clean)) {}
std::string RangeExpr::toString(int indent) const {
  return wrapType(
      format("range {} {}", start->toString(indent), stop->toString(indent)));
}
ACCEPT_IMPL(RangeExpr, ASTVisitor);

StmtExpr::StmtExpr(std::vector<Stmt *> stmts, Expr *expr)
    : Expr(), stmts(std::move(stmts)), expr(expr) {}
StmtExpr::StmtExpr(Stmt *stmt, Expr *expr) : Expr(), expr(expr) {
  stmts.push_back(stmt);
}
StmtExpr::StmtExpr(Stmt *stmt, Stmt *stmt2, Expr *expr) : Expr(), expr(expr) {
  stmts.push_back(stmt);
  stmts.push_back(stmt2);
}
StmtExpr::StmtExpr(const StmtExpr &expr, bool clean)
    : Expr(expr, clean), stmts(ast::clone(expr.stmts, clean)),
      expr(ast::clone(expr.expr, clean)) {}
std::string StmtExpr::toString(int indent) const {
  auto pad = indent >= 0 ? ("\n" + std::string(indent + 2 * INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> s;
  s.reserve(stmts.size());
  for (auto &i : stmts)
    s.emplace_back(pad + i->toString(indent >= 0 ? indent + 2 * INDENT_SIZE : -1));
  return wrapType(
      format("stmt-expr {} ({})", expr->toString(indent), fmt::join(s, "")));
}
ACCEPT_IMPL(StmtExpr, ASTVisitor);

InstantiateExpr::InstantiateExpr(Expr *typeExpr, std::vector<Expr *> typeParams)
    : Expr(), typeExpr(typeExpr), typeParams(std::move(typeParams)) {}
InstantiateExpr::InstantiateExpr(Expr *typeExpr, Expr *typeParam)
    : Expr(), typeExpr(typeExpr) {
  typeParams.push_back(std::move(typeParam));
}
InstantiateExpr::InstantiateExpr(const InstantiateExpr &expr, bool clean)
    : Expr(expr, clean), typeExpr(ast::clone(expr.typeExpr, clean)),
      typeParams(ast::clone(expr.typeParams, clean)) {}
std::string InstantiateExpr::toString(int indent) const {
  return wrapType(
      format("instantiate {} {}", typeExpr->toString(indent), combine(typeParams)));
}
ACCEPT_IMPL(InstantiateExpr, ASTVisitor);

char getStaticGeneric(Expr *e) {
  if (e && e->getIndex() && e->getIndex()->expr->isId("Static")) {
    if (e->getIndex()->index && e->getIndex()->index->isId("bool"))
      return 3;
    if (e->getIndex()->index && e->getIndex()->index->isId("str"))
      return 2;
    if (e->getIndex()->index && e->getIndex()->index->isId("int"))
      return 1;
    return 4;
  }
  return 0;
}

} // namespace codon::ast
