// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "stmt.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "codon/parser/cache.h"
#include "codon/parser/visitors/visitor.h"

#define ACCEPT_IMPL(T, X)                                                              \
  StmtPtr T::clone() const { return std::make_shared<T>(*this); }                      \
  void T::accept(X &visitor) { visitor.visit(this); }

using fmt::format;
using namespace codon::error;

const int INDENT_SIZE = 2;

namespace codon::ast {

Stmt::Stmt() : done(false), age(-1) {}
Stmt::Stmt(const codon::SrcInfo &s) : done(false), age(-1) { setSrcInfo(s); }
std::string Stmt::toString() const { return toString(-1); }
void Stmt::validate() const {}

SuiteStmt::SuiteStmt(std::vector<StmtPtr> stmts) : Stmt() {
  for (auto &s : stmts)
    flatten(std::move(s), this->stmts);
}
SuiteStmt::SuiteStmt(const SuiteStmt &stmt)
    : Stmt(stmt), stmts(ast::clone(stmt.stmts)) {}
std::string SuiteStmt::toString(int indent) const {
  std::string pad = indent >= 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string s;
  for (int i = 0; i < stmts.size(); i++)
    if (stmts[i]) {
      auto is = stmts[i]->toString(indent >= 0 ? indent + INDENT_SIZE : -1);
      if (stmts[i]->done)
        is.insert(findStar(is), "*");
      s += (i ? pad : "") + is;
    }
  return format("(suite{})", s.empty() ? s : " " + pad + s);
}
ACCEPT_IMPL(SuiteStmt, ASTVisitor);
void SuiteStmt::flatten(const StmtPtr &s, std::vector<StmtPtr> &stmts) {
  if (!s)
    return;
  if (!s->getSuite()) {
    stmts.push_back(s);
  } else {
    for (auto &ss : s->getSuite()->stmts)
      stmts.push_back(ss);
  }
}
StmtPtr *SuiteStmt::lastInBlock() {
  if (stmts.empty())
    return nullptr;
  if (auto s = stmts.back()->getSuite()) {
    auto l = s->lastInBlock();
    if (l)
      return l;
  }
  return &(stmts.back());
}

std::string BreakStmt::toString(int) const { return "(break)"; }
ACCEPT_IMPL(BreakStmt, ASTVisitor);

std::string ContinueStmt::toString(int) const { return "(continue)"; }
ACCEPT_IMPL(ContinueStmt, ASTVisitor);

ExprStmt::ExprStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
ExprStmt::ExprStmt(const ExprStmt &stmt) : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string ExprStmt::toString(int) const {
  return format("(expr {})", expr->toString());
}
ACCEPT_IMPL(ExprStmt, ASTVisitor);

AssignStmt::AssignStmt(ExprPtr lhs, ExprPtr rhs, ExprPtr type)
    : Stmt(), lhs(std::move(lhs)), rhs(std::move(rhs)), type(std::move(type)),
      update(Assign) {}
AssignStmt::AssignStmt(const AssignStmt &stmt)
    : Stmt(stmt), lhs(ast::clone(stmt.lhs)), rhs(ast::clone(stmt.rhs)),
      type(ast::clone(stmt.type)), update(stmt.update) {}
std::string AssignStmt::toString(int) const {
  return format("({} {}{}{})", update != Assign ? "update" : "assign", lhs->toString(),
                rhs ? " " + rhs->toString() : "",
                type ? format(" #:type {}", type->toString()) : "");
}
ACCEPT_IMPL(AssignStmt, ASTVisitor);

DelStmt::DelStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
DelStmt::DelStmt(const DelStmt &stmt) : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string DelStmt::toString(int) const {
  return format("(del {})", expr->toString());
}
ACCEPT_IMPL(DelStmt, ASTVisitor);

PrintStmt::PrintStmt(std::vector<ExprPtr> items, bool isInline)
    : Stmt(), items(std::move(items)), isInline(isInline) {}
PrintStmt::PrintStmt(const PrintStmt &stmt)
    : Stmt(stmt), items(ast::clone(stmt.items)), isInline(stmt.isInline) {}
std::string PrintStmt::toString(int) const {
  return format("(print {}{})", isInline ? "#:inline " : "", combine(items));
}
ACCEPT_IMPL(PrintStmt, ASTVisitor);

ReturnStmt::ReturnStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
ReturnStmt::ReturnStmt(const ReturnStmt &stmt)
    : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string ReturnStmt::toString(int) const {
  return expr ? format("(return {})", expr->toString()) : "(return)";
}
ACCEPT_IMPL(ReturnStmt, ASTVisitor);

YieldStmt::YieldStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
YieldStmt::YieldStmt(const YieldStmt &stmt) : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string YieldStmt::toString(int) const {
  return expr ? format("(yield {})", expr->toString()) : "(yield)";
}
ACCEPT_IMPL(YieldStmt, ASTVisitor);

AssertStmt::AssertStmt(ExprPtr expr, ExprPtr message)
    : Stmt(), expr(std::move(expr)), message(std::move(message)) {}
AssertStmt::AssertStmt(const AssertStmt &stmt)
    : Stmt(stmt), expr(ast::clone(stmt.expr)), message(ast::clone(stmt.message)) {}
std::string AssertStmt::toString(int) const {
  return format("(assert {}{})", expr->toString(), message ? message->toString() : "");
}
ACCEPT_IMPL(AssertStmt, ASTVisitor);

WhileStmt::WhileStmt(ExprPtr cond, StmtPtr suite, StmtPtr elseSuite)
    : Stmt(), cond(std::move(cond)), suite(std::move(suite)),
      elseSuite(std::move(elseSuite)) {}
WhileStmt::WhileStmt(const WhileStmt &stmt)
    : Stmt(stmt), cond(ast::clone(stmt.cond)), suite(ast::clone(stmt.suite)),
      elseSuite(ast::clone(stmt.elseSuite)) {}
std::string WhileStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  if (elseSuite && elseSuite->firstInBlock())
    return format("(while-else {}{}{}{}{})", cond->toString(), pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1), pad,
                  elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
  else
    return format("(while {}{}{})", cond->toString(), pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
}
ACCEPT_IMPL(WhileStmt, ASTVisitor);

ForStmt::ForStmt(ExprPtr var, ExprPtr iter, StmtPtr suite, StmtPtr elseSuite,
                 ExprPtr decorator, std::vector<CallExpr::Arg> ompArgs)
    : Stmt(), var(std::move(var)), iter(std::move(iter)), suite(std::move(suite)),
      elseSuite(std::move(elseSuite)), decorator(std::move(decorator)),
      ompArgs(std::move(ompArgs)), wrapped(false), flat(false) {}
ForStmt::ForStmt(const ForStmt &stmt)
    : Stmt(stmt), var(ast::clone(stmt.var)), iter(ast::clone(stmt.iter)),
      suite(ast::clone(stmt.suite)), elseSuite(ast::clone(stmt.elseSuite)),
      decorator(ast::clone(stmt.decorator)), ompArgs(ast::clone_nop(stmt.ompArgs)),
      wrapped(stmt.wrapped), flat(stmt.flat) {}
std::string ForStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string attr;
  if (decorator)
    attr += " " + decorator->toString();
  if (!attr.empty())
    attr = " #:attr" + attr;
  if (elseSuite && elseSuite->firstInBlock())
    return format("(for-else {} {}{}{}{}{}{})", var->toString(), iter->toString(), attr,
                  pad, suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1), pad,
                  elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
  else
    return format("(for {} {}{}{}{})", var->toString(), iter->toString(), attr, pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
}
ACCEPT_IMPL(ForStmt, ASTVisitor);

IfStmt::IfStmt(ExprPtr cond, StmtPtr ifSuite, StmtPtr elseSuite)
    : Stmt(), cond(std::move(cond)), ifSuite(std::move(ifSuite)),
      elseSuite(std::move(elseSuite)) {}
IfStmt::IfStmt(const IfStmt &stmt)
    : Stmt(stmt), cond(ast::clone(stmt.cond)), ifSuite(ast::clone(stmt.ifSuite)),
      elseSuite(ast::clone(stmt.elseSuite)) {}
std::string IfStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  return format("(if {}{}{}{})", cond->toString(), pad,
                ifSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1),
                elseSuite
                    ? pad + elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                    : "");
}
ACCEPT_IMPL(IfStmt, ASTVisitor);

MatchStmt::MatchCase MatchStmt::MatchCase::clone() const {
  return {ast::clone(pattern), ast::clone(guard), ast::clone(suite)};
}

MatchStmt::MatchStmt(ExprPtr what, std::vector<MatchStmt::MatchCase> cases)
    : Stmt(), what(std::move(what)), cases(std::move(cases)) {}
MatchStmt::MatchStmt(const MatchStmt &stmt)
    : Stmt(stmt), what(ast::clone(stmt.what)), cases(ast::clone_nop(stmt.cases)) {}
std::string MatchStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string padExtra = indent > 0 ? std::string(INDENT_SIZE, ' ') : "";
  std::vector<std::string> s;
  for (auto &c : cases)
    s.push_back(format("(case {}{}{}{})", c.pattern->toString(),
                       c.guard ? " #:guard " + c.guard->toString() : "", pad + padExtra,
                       c.suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1 * 2)));
  return format("(match {}{}{})", what->toString(), pad, join(s, pad));
}
ACCEPT_IMPL(MatchStmt, ASTVisitor);

ImportStmt::ImportStmt(ExprPtr from, ExprPtr what, std::vector<Param> args, ExprPtr ret,
                       std::string as, size_t dots, bool isFunction)
    : Stmt(), from(std::move(from)), what(std::move(what)), as(std::move(as)),
      dots(dots), args(std::move(args)), ret(std::move(ret)), isFunction(isFunction) {
  validate();
}
ImportStmt::ImportStmt(const ImportStmt &stmt)
    : Stmt(stmt), from(ast::clone(stmt.from)), what(ast::clone(stmt.what)), as(stmt.as),
      dots(stmt.dots), args(ast::clone_nop(stmt.args)), ret(ast::clone(stmt.ret)),
      isFunction(stmt.isFunction) {}
std::string ImportStmt::toString(int) const {
  std::vector<std::string> va;
  for (auto &a : args)
    va.push_back(a.toString());
  return format("(import {}{}{}{}{}{})", from->toString(),
                as.empty() ? "" : format(" #:as '{}", as),
                what ? format(" #:what {}", what->toString()) : "",
                dots ? format(" #:dots {}", dots) : "",
                va.empty() ? "" : format(" #:args ({})", join(va)),
                ret ? format(" #:ret {}", ret->toString()) : "");
}
void ImportStmt::validate() const {
  if (from) {
    Expr *e = from.get();
    while (auto d = e->getDot())
      e = d->expr.get();
    if (!from->isId("C") && !from->isId("python")) {
      if (!e->getId())
        E(Error::IMPORT_IDENTIFIER, e);
      if (!args.empty())
        E(Error::IMPORT_FN, args[0]);
      if (ret)
        E(Error::IMPORT_FN, ret);
      if (what && !what->getId())
        E(Error::IMPORT_IDENTIFIER, what);
    }
    if (!isFunction && !args.empty())
      E(Error::IMPORT_FN, args[0]);
  }
}
ACCEPT_IMPL(ImportStmt, ASTVisitor);

TryStmt::Catch TryStmt::Catch::clone() const {
  return {var, ast::clone(exc), ast::clone(suite)};
}

TryStmt::TryStmt(StmtPtr suite, std::vector<Catch> catches, StmtPtr finally)
    : Stmt(), suite(std::move(suite)), catches(std::move(catches)),
      finally(std::move(finally)) {}
TryStmt::TryStmt(const TryStmt &stmt)
    : Stmt(stmt), suite(ast::clone(stmt.suite)), catches(ast::clone_nop(stmt.catches)),
      finally(ast::clone(stmt.finally)) {}
std::string TryStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string padExtra = indent > 0 ? std::string(INDENT_SIZE, ' ') : "";
  std::vector<std::string> s;
  for (auto &i : catches)
    s.push_back(
        format("(catch {}{}{}{})", !i.var.empty() ? format("#:var '{}", i.var) : "",
               i.exc ? format(" #:exc {}", i.exc->toString()) : "", pad + padExtra,
               i.suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1 * 2)));
  return format(
      "(try{}{}{}{}{})", pad, suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1),
      pad, join(s, pad),
      finally ? format("{}{}", pad,
                       finally->toString(indent >= 0 ? indent + INDENT_SIZE : -1))
              : "");
}
ACCEPT_IMPL(TryStmt, ASTVisitor);

ThrowStmt::ThrowStmt(ExprPtr expr, bool transformed)
    : Stmt(), expr(std::move(expr)), transformed(transformed) {}
ThrowStmt::ThrowStmt(const ThrowStmt &stmt)
    : Stmt(stmt), expr(ast::clone(stmt.expr)), transformed(stmt.transformed) {}
std::string ThrowStmt::toString(int) const {
  return format("(throw{})", expr ? " " + expr->toString() : "");
}
ACCEPT_IMPL(ThrowStmt, ASTVisitor);

GlobalStmt::GlobalStmt(std::string var, bool nonLocal)
    : Stmt(), var(std::move(var)), nonLocal(nonLocal) {}
std::string GlobalStmt::toString(int) const {
  return format("({} '{})", nonLocal ? "nonlocal" : "global", var);
}
ACCEPT_IMPL(GlobalStmt, ASTVisitor);

Attr::Attr(const std::vector<std::string> &attrs)
    : module(), parentClass(), isAttribute(false) {
  for (auto &a : attrs)
    set(a);
}
void Attr::set(const std::string &attr) { customAttr.insert(attr); }
void Attr::unset(const std::string &attr) { customAttr.erase(attr); }
bool Attr::has(const std::string &attr) const { return in(customAttr, attr); }

const std::string Attr::LLVM = "llvm";
const std::string Attr::Python = "python";
const std::string Attr::Atomic = "atomic";
const std::string Attr::Property = "property";
const std::string Attr::StaticMethod = "staticmethod";
const std::string Attr::Attribute = "__attribute__";
const std::string Attr::Internal = "__internal__";
const std::string Attr::ForceRealize = "__force__";
const std::string Attr::RealizeWithoutSelf =
    "std.internal.attributes.realize_without_self";
const std::string Attr::HiddenFromUser = "__hidden__";
const std::string Attr::C = "C";
const std::string Attr::CVarArg = ".__vararg__";
const std::string Attr::Method = ".__method__";
const std::string Attr::Capture = ".__capture__";
const std::string Attr::HasSelf = ".__hasself__";
const std::string Attr::IsGenerator = ".__generator__";
const std::string Attr::Extend = "extend";
const std::string Attr::Tuple = "tuple";
const std::string Attr::Test = "std.internal.attributes.test";
const std::string Attr::Overload = "overload";
const std::string Attr::Export = "std.internal.attributes.export";

FunctionStmt::FunctionStmt(std::string name, ExprPtr ret, std::vector<Param> args,
                           StmtPtr suite, Attr attributes,
                           std::vector<ExprPtr> decorators)
    : Stmt(), name(std::move(name)), ret(std::move(ret)), args(std::move(args)),
      suite(std::move(suite)), attributes(std::move(attributes)),
      decorators(std::move(decorators)) {
  parseDecorators();
}
FunctionStmt::FunctionStmt(const FunctionStmt &stmt)
    : Stmt(stmt), name(stmt.name), ret(ast::clone(stmt.ret)),
      args(ast::clone_nop(stmt.args)), suite(ast::clone(stmt.suite)),
      attributes(stmt.attributes), decorators(ast::clone(stmt.decorators)) {}
std::string FunctionStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> as;
  for (auto &a : args)
    as.push_back(a.toString());
  std::vector<std::string> dec, attr;
  for (auto &a : decorators)
    if (a)
      dec.push_back(format("(dec {})", a->toString()));
  for (auto &a : attributes.customAttr)
    attr.push_back(format("'{}'", a));
  return format("(fn '{} ({}){}{}{}{}{})", name, join(as, " "),
                ret ? " #:ret " + ret->toString() : "",
                dec.empty() ? "" : format(" (dec {})", join(dec, " ")),
                attr.empty() ? "" : format(" (attr {})", join(attr, " ")), pad,
                suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                      : "(suite)");
}
void FunctionStmt::validate() const {
  if (!ret && (attributes.has(Attr::LLVM) || attributes.has(Attr::C)))
    E(Error::FN_LLVM, getSrcInfo());

  std::unordered_set<std::string> seenArgs;
  bool defaultsStarted = false, hasStarArg = false, hasKwArg = false;
  for (size_t ia = 0; ia < args.size(); ia++) {
    auto &a = args[ia];
    auto n = a.name;
    int stars = trimStars(n);
    if (stars == 2) {
      if (hasKwArg)
        E(Error::FN_MULTIPLE_ARGS, a);
      if (a.defaultValue)
        E(Error::FN_DEFAULT_STARARG, a.defaultValue);
      if (ia != args.size() - 1)
        E(Error::FN_LAST_KWARG, a);
      hasKwArg = true;
    } else if (stars == 1) {
      if (hasStarArg)
        E(Error::FN_MULTIPLE_ARGS, a);
      if (a.defaultValue)
        E(Error::FN_DEFAULT_STARARG, a.defaultValue);
      hasStarArg = true;
    }
    if (in(seenArgs, n))
      E(Error::FN_ARG_TWICE, a, n);
    seenArgs.insert(n);
    if (!a.defaultValue && defaultsStarted && !stars && a.status == Param::Normal)
      E(Error::FN_DEFAULT, a, n);
    defaultsStarted |= bool(a.defaultValue);
    if (attributes.has(Attr::C)) {
      if (a.defaultValue)
        E(Error::FN_C_DEFAULT, a.defaultValue, n);
      if (stars != 1 && !a.type)
        E(Error::FN_C_TYPE, a, n);
    }
  }
}
ACCEPT_IMPL(FunctionStmt, ASTVisitor);
std::string FunctionStmt::signature() const {
  std::vector<std::string> s;
  for (auto &a : args)
    s.push_back(a.type ? a.type->toString() : "-");
  return format("{}", join(s, ":"));
}
bool FunctionStmt::hasAttr(const std::string &attr) const {
  return attributes.has(attr);
}
void FunctionStmt::parseDecorators() {
  std::vector<ExprPtr> newDecorators;
  for (auto &d : decorators) {
    if (d->isId(Attr::Attribute)) {
      if (decorators.size() != 1)
        E(Error::FN_SINGLE_DECORATOR, decorators[1], Attr::Attribute);
      attributes.isAttribute = true;
    } else if (d->isId(Attr::LLVM)) {
      attributes.set(Attr::LLVM);
    } else if (d->isId(Attr::Python)) {
      if (decorators.size() != 1)
        E(Error::FN_SINGLE_DECORATOR, decorators[1], Attr::Python);
      attributes.set(Attr::Python);
    } else if (d->isId(Attr::Internal)) {
      attributes.set(Attr::Internal);
    } else if (d->isId(Attr::HiddenFromUser)) {
      attributes.set(Attr::HiddenFromUser);
    } else if (d->isId(Attr::Atomic)) {
      attributes.set(Attr::Atomic);
    } else if (d->isId(Attr::Property)) {
      attributes.set(Attr::Property);
    } else if (d->isId(Attr::StaticMethod)) {
      attributes.set(Attr::StaticMethod);
    } else if (d->isId(Attr::ForceRealize)) {
      attributes.set(Attr::ForceRealize);
    } else if (d->isId(Attr::C)) {
      attributes.set(Attr::C);
    } else {
      newDecorators.emplace_back(d);
    }
  }
  if (attributes.has(Attr::C)) {
    for (auto &a : args) {
      if (a.name.size() > 1 && a.name[0] == '*' && a.name[1] != '*')
        attributes.set(Attr::CVarArg);
    }
  }
  if (!args.empty() && !args[0].type && args[0].name == "self") {
    attributes.set(Attr::HasSelf);
  }
  decorators = newDecorators;
  validate();
}
size_t FunctionStmt::getStarArgs() const {
  size_t i = 0;
  while (i < args.size()) {
    if (startswith(args[i].name, "*") && !startswith(args[i].name, "**"))
      break;
    i++;
  }
  return i;
}
size_t FunctionStmt::getKwStarArgs() const {
  size_t i = 0;
  while (i < args.size()) {
    if (startswith(args[i].name, "**"))
      break;
    i++;
  }
  return i;
}
std::string FunctionStmt::getDocstr() {
  if (auto s = suite->firstInBlock()) {
    if (auto e = s->getExpr()) {
      if (auto ss = e->expr->getString())
        return ss->getValue();
    }
  }
  return "";
}

// Search expression tree for a identifier
class IdSearchVisitor : public CallbackASTVisitor<bool, bool> {
  std::string what;
  bool result;

public:
  IdSearchVisitor(std::string what) : what(std::move(what)), result(false) {}
  bool transform(const std::shared_ptr<Expr> &expr) override {
    if (result)
      return result;
    IdSearchVisitor v(what);
    if (expr)
      expr->accept(v);
    return result = v.result;
  }
  bool transform(const std::shared_ptr<Stmt> &stmt) override {
    if (result)
      return result;
    IdSearchVisitor v(what);
    if (stmt)
      stmt->accept(v);
    return result = v.result;
  }
  void visit(IdExpr *expr) override {
    if (expr->value == what)
      result = true;
  }
};

/// Check if a function can be called with the given arguments.
/// See @c reorderNamedArgs for details.
std::unordered_set<std::string> FunctionStmt::getNonInferrableGenerics() {
  std::unordered_set<std::string> nonInferrableGenerics;
  for (auto &a : args) {
    if (a.status == Param::Generic && !a.defaultValue) {
      bool inferrable = false;
      for (auto &b : args)
        if (b.type && IdSearchVisitor(a.name).transform(b.type)) {
          inferrable = true;
          break;
        }
      if (ret && IdSearchVisitor(a.name).transform(ret))
        inferrable = true;
      if (!inferrable)
        nonInferrableGenerics.insert(a.name);
    }
  }
  return nonInferrableGenerics;
}

ClassStmt::ClassStmt(std::string name, std::vector<Param> args, StmtPtr suite,
                     std::vector<ExprPtr> decorators, std::vector<ExprPtr> baseClasses,
                     std::vector<ExprPtr> staticBaseClasses)
    : Stmt(), name(std::move(name)), args(std::move(args)), suite(std::move(suite)),
      decorators(std::move(decorators)),
      staticBaseClasses(std::move(staticBaseClasses)) {
  for (auto &b : baseClasses) {
    if (b->getIndex() && b->getIndex()->expr->isId("Static")) {
      this->staticBaseClasses.push_back(b->getIndex()->index);
    } else {
      this->baseClasses.push_back(b);
    }
  }
  parseDecorators();
}
ClassStmt::ClassStmt(std::string name, std::vector<Param> args, StmtPtr suite,
                     Attr attr)
    : Stmt(), name(std::move(name)), args(std::move(args)), suite(std::move(suite)),
      attributes(std::move(attr)) {
  validate();
}
ClassStmt::ClassStmt(const ClassStmt &stmt)
    : Stmt(stmt), name(stmt.name), args(ast::clone_nop(stmt.args)),
      suite(ast::clone(stmt.suite)), attributes(stmt.attributes),
      decorators(ast::clone(stmt.decorators)),
      baseClasses(ast::clone(stmt.baseClasses)),
      staticBaseClasses(ast::clone(stmt.staticBaseClasses)) {}
std::string ClassStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> bases;
  for (auto &b : baseClasses)
    bases.push_back(b->toString());
  for (auto &b : staticBaseClasses)
    bases.push_back(fmt::format("(static {})", b->toString()));
  std::string as;
  for (int i = 0; i < args.size(); i++)
    as += (i ? pad : "") + args[i].toString();
  std::vector<std::string> attr;
  for (auto &a : decorators)
    attr.push_back(format("(dec {})", a->toString()));
  return format("(class '{}{}{}{}{}{})", name,
                bases.empty() ? "" : format(" (bases {})", join(bases, " ")),
                attr.empty() ? "" : format(" (attr {})", join(attr, " ")),
                as.empty() ? as : pad + as, pad,
                suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                      : "(suite)");
}
void ClassStmt::validate() const {
  std::unordered_set<std::string> seen;
  if (attributes.has(Attr::Extend) && !args.empty())
    E(Error::CLASS_EXTENSION, args[0]);
  if (attributes.has(Attr::Extend) &&
      !(baseClasses.empty() && staticBaseClasses.empty()))
    E(Error::CLASS_EXTENSION,
      baseClasses.empty() ? staticBaseClasses[0] : baseClasses[0]);
  for (auto &a : args) {
    if (!a.type && !a.defaultValue)
      E(Error::CLASS_MISSING_TYPE, a, a.name);
    if (in(seen, a.name))
      E(Error::CLASS_ARG_TWICE, a, a.name);
    seen.insert(a.name);
  }
}
ACCEPT_IMPL(ClassStmt, ASTVisitor);
bool ClassStmt::isRecord() const { return hasAttr(Attr::Tuple); }
bool ClassStmt::hasAttr(const std::string &attr) const { return attributes.has(attr); }
void ClassStmt::parseDecorators() {
  // @tuple(init=, repr=, eq=, order=, hash=, pickle=, container=, python=, add=,
  // internal=...)
  // @dataclass(...)
  // @extend

  std::map<std::string, bool> tupleMagics = {
      {"new", true},           {"repr", false},    {"hash", false},
      {"eq", false},           {"ne", false},      {"lt", false},
      {"le", false},           {"gt", false},      {"ge", false},
      {"pickle", true},        {"unpickle", true}, {"to_py", false},
      {"from_py", false},      {"iter", false},    {"getitem", false},
      {"len", false},          {"to_gpu", false},  {"from_gpu", false},
      {"from_gpu_new", false}, {"tuplesize", true}};

  for (auto &d : decorators) {
    if (d->isId("deduce")) {
      attributes.customAttr.insert("deduce");
    } else if (d->isId("__notuple__")) {
      attributes.customAttr.insert("__notuple__");
    } else if (auto c = d->getCall()) {
      if (c->expr->isId(Attr::Tuple)) {
        attributes.set(Attr::Tuple);
        for (auto &m : tupleMagics)
          m.second = true;
      } else if (!c->expr->isId("dataclass")) {
        E(Error::CLASS_BAD_DECORATOR, c->expr);
      } else if (attributes.has(Attr::Tuple)) {
        E(Error::CLASS_CONFLICT_DECORATOR, c, "dataclass", Attr::Tuple);
      }
      for (auto &a : c->args) {
        auto b = CAST(a.value, BoolExpr);
        if (!b)
          E(Error::CLASS_NONSTATIC_DECORATOR, a);
        char val = char(b->value);
        if (a.name == "init") {
          tupleMagics["new"] = val;
        } else if (a.name == "repr") {
          tupleMagics["repr"] = val;
        } else if (a.name == "eq") {
          tupleMagics["eq"] = tupleMagics["ne"] = val;
        } else if (a.name == "order") {
          tupleMagics["lt"] = tupleMagics["le"] = tupleMagics["gt"] =
              tupleMagics["ge"] = val;
        } else if (a.name == "hash") {
          tupleMagics["hash"] = val;
        } else if (a.name == "pickle") {
          tupleMagics["pickle"] = tupleMagics["unpickle"] = val;
        } else if (a.name == "python") {
          tupleMagics["to_py"] = tupleMagics["from_py"] = val;
        } else if (a.name == "gpu") {
          tupleMagics["to_gpu"] = tupleMagics["from_gpu"] =
              tupleMagics["from_gpu_new"] = val;
        } else if (a.name == "container") {
          tupleMagics["iter"] = tupleMagics["getitem"] = val;
        } else {
          E(Error::CLASS_BAD_DECORATOR_ARG, a);
        }
      }
    } else if (d->isId(Attr::Tuple)) {
      if (attributes.has(Attr::Tuple))
        E(Error::CLASS_MULTIPLE_DECORATORS, d, Attr::Tuple);
      attributes.set(Attr::Tuple);
      for (auto &m : tupleMagics) {
        m.second = true;
      }
    } else if (d->isId(Attr::Extend)) {
      attributes.set(Attr::Extend);
      if (decorators.size() != 1)
        E(Error::CLASS_SINGLE_DECORATOR, decorators[decorators[0] == d], Attr::Extend);
    } else if (d->isId(Attr::Internal)) {
      attributes.set(Attr::Internal);
    } else {
      E(Error::CLASS_BAD_DECORATOR, d);
    }
  }
  if (attributes.has("deduce"))
    tupleMagics["new"] = false;
  if (!attributes.has(Attr::Tuple)) {
    tupleMagics["init"] = tupleMagics["new"];
    tupleMagics["new"] = tupleMagics["raw"] = true;
    tupleMagics["len"] = false;
  }
  tupleMagics["dict"] = true;
  // Internal classes do not get any auto-generated members.
  attributes.magics.clear();
  if (!attributes.has(Attr::Internal)) {
    for (auto &m : tupleMagics)
      if (m.second)
        attributes.magics.insert(m.first);
  }

  validate();
}
bool ClassStmt::isClassVar(const Param &p) {
  if (!p.defaultValue)
    return false;
  if (!p.type)
    return true;
  if (auto i = p.type->getIndex())
    return i->expr->isId("ClassVar");
  return false;
}
std::string ClassStmt::getDocstr() {
  if (auto s = suite->firstInBlock()) {
    if (auto e = s->getExpr()) {
      if (auto ss = e->expr->getString())
        return ss->getValue();
    }
  }
  return "";
}

YieldFromStmt::YieldFromStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
YieldFromStmt::YieldFromStmt(const YieldFromStmt &stmt)
    : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string YieldFromStmt::toString(int) const {
  return format("(yield-from {})", expr->toString());
}
ACCEPT_IMPL(YieldFromStmt, ASTVisitor);

WithStmt::WithStmt(std::vector<ExprPtr> items, std::vector<std::string> vars,
                   StmtPtr suite)
    : Stmt(), items(std::move(items)), vars(std::move(vars)), suite(std::move(suite)) {
  seqassert(this->items.size() == this->vars.size(), "vector size mismatch");
}
WithStmt::WithStmt(std::vector<std::pair<ExprPtr, ExprPtr>> itemVarPairs, StmtPtr suite)
    : Stmt(), suite(std::move(suite)) {
  for (auto &i : itemVarPairs) {
    items.push_back(std::move(i.first));
    if (i.second) {
      if (!i.second->getId())
        throw;
      vars.push_back(i.second->getId()->value);
    } else {
      vars.emplace_back();
    }
  }
}
WithStmt::WithStmt(const WithStmt &stmt)
    : Stmt(stmt), items(ast::clone(stmt.items)), vars(stmt.vars),
      suite(ast::clone(stmt.suite)) {}
std::string WithStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> as;
  as.reserve(items.size());
  for (int i = 0; i < items.size(); i++) {
    as.push_back(!vars[i].empty()
                     ? format("({} #:var '{})", items[i]->toString(), vars[i])
                     : items[i]->toString());
  }
  return format("(with ({}){}{})", join(as, " "), pad,
                suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
}
ACCEPT_IMPL(WithStmt, ASTVisitor);

CustomStmt::CustomStmt(std::string keyword, ExprPtr expr, StmtPtr suite)
    : Stmt(), keyword(std::move(keyword)), expr(std::move(expr)),
      suite(std::move(suite)) {}
CustomStmt::CustomStmt(const CustomStmt &stmt)
    : Stmt(stmt), keyword(stmt.keyword), expr(ast::clone(stmt.expr)),
      suite(ast::clone(stmt.suite)) {}
std::string CustomStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  return format("(custom-{} {}{}{})", keyword,
                expr ? format(" #:expr {}", expr->toString()) : "", pad,
                suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1) : "");
}
ACCEPT_IMPL(CustomStmt, ASTVisitor);

AssignMemberStmt::AssignMemberStmt(ExprPtr lhs, std::string member, ExprPtr rhs)
    : Stmt(), lhs(std::move(lhs)), member(std::move(member)), rhs(std::move(rhs)) {}
AssignMemberStmt::AssignMemberStmt(const AssignMemberStmt &stmt)
    : Stmt(stmt), lhs(ast::clone(stmt.lhs)), member(stmt.member),
      rhs(ast::clone(stmt.rhs)) {}
std::string AssignMemberStmt::toString(int) const {
  return format("(assign-member {} {} {})", lhs->toString(), member, rhs->toString());
}
ACCEPT_IMPL(AssignMemberStmt, ASTVisitor);

CommentStmt::CommentStmt(std::string comment) : Stmt(), comment(std::move(comment)) {}
std::string CommentStmt::toString(int) const {
  return format("(comment \"{}\")", comment);
}
ACCEPT_IMPL(CommentStmt, ASTVisitor);

} // namespace codon::ast
