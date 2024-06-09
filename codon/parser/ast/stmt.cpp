// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "stmt.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "codon/parser/cache.h"
#include "codon/parser/visitors/visitor.h"

#define ACCEPT_IMPL(T, X)                                                              \
  Node *T::clone(bool clean) const { return cache->N<T>(*this, clean); }               \
  void T::accept(X &visitor) { visitor.visit(this); }

using fmt::format;
using namespace codon::error;

namespace codon::ast {

Stmt::Stmt() : Node(), done(false) {}
Stmt::Stmt(const Stmt &stmt) : Node(stmt), done(stmt.done) {}
Stmt::Stmt(const codon::SrcInfo &s) : Stmt() { setSrcInfo(s); }
Stmt::Stmt(const Stmt &expr, bool clean) : Stmt(expr) {
  if (clean)
    done = false;
}

SuiteStmt::SuiteStmt(std::vector<Stmt *> stmts) : Stmt(), stmts(std::move(stmts)) {}
SuiteStmt::SuiteStmt(const SuiteStmt &stmt, bool clean)
    : Stmt(stmt, clean), stmts(ast::clone(stmt.stmts, clean)) {}
std::string SuiteStmt::toString(int indent) const {
  if (indent == -1)
    return "";
  std::string pad = indent >= 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string s;
  for (int i = 0; i < stmts.size(); i++)
    if (stmts[i]) {
      auto is = stmts[i]->toString(indent >= 0 ? indent + INDENT_SIZE : -1);
      if (stmts[i]->done)
        is.insert(findStar(is), "*");
      s += (i ? pad : "") + is;
    }
  return format("({}suite{})", (done ? "*" : ""), (s.empty() ? s : " " + pad + s));
}
ACCEPT_IMPL(SuiteStmt, ASTVisitor);
void SuiteStmt::shallow_flatten() {
  std::vector<Stmt *> ns;
  for (auto &s : stmts) {
    if (!s)
      continue;
    if (!s->getSuite()) {
      ns.push_back(s);
    } else {
      for (auto &ss : s->getSuite()->stmts)
        ns.push_back(ss);
    }
  }
  stmts = ns;
}
Stmt **SuiteStmt::lastInBlock() {
  if (stmts.empty())
    return nullptr;
  if (auto s = stmts.back()->getSuite()) {
    auto l = s->lastInBlock();
    if (l)
      return l;
  }
  return &(stmts.back());
}
SuiteStmt *SuiteStmt::wrap(Stmt *s) {
  if (s && !s->getSuite())
    return s->cache->NS<SuiteStmt>(s, s);
  return (SuiteStmt *)s;
}

BreakStmt::BreakStmt(const BreakStmt &stmt, bool clean) : Stmt(stmt, clean) {}
std::string BreakStmt::toString(int indent) const { return "(break)"; }
ACCEPT_IMPL(BreakStmt, ASTVisitor);

ContinueStmt::ContinueStmt(const ContinueStmt &stmt, bool clean) : Stmt(stmt, clean) {}
std::string ContinueStmt::toString(int indent) const { return "(continue)"; }
ACCEPT_IMPL(ContinueStmt, ASTVisitor);

ExprStmt::ExprStmt(Expr *expr) : Stmt(), expr(expr) {}
ExprStmt::ExprStmt(const ExprStmt &stmt, bool clean)
    : Stmt(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string ExprStmt::toString(int indent) const {
  return format("(expr {})", expr->toString(indent));
}
ACCEPT_IMPL(ExprStmt, ASTVisitor);

AssignStmt::AssignStmt(Expr *lhs, Expr *rhs, Expr *type, UpdateMode update)
    : Stmt(), lhs(lhs), rhs(rhs), type(type), update(update) {}
AssignStmt::AssignStmt(const AssignStmt &stmt, bool clean)
    : Stmt(stmt, clean), lhs(ast::clone(stmt.lhs, clean)),
      rhs(ast::clone(stmt.rhs, clean)), type(ast::clone(stmt.type, clean)),
      preamble(ast::clone(stmt.preamble, clean)), update(stmt.update) {}
std::string AssignStmt::toString(int indent) const {
  return format("({} {}{}{})", update != Assign ? "update" : "assign",
                lhs->toString(indent), rhs ? " " + rhs->toString(indent) : "",
                type ? format(" #:type {}", type->toString(indent)) : "");
}
ACCEPT_IMPL(AssignStmt, ASTVisitor);

/// Unpack an assignment expression `lhs = rhs` into a list of simple assignment
/// expressions (e.g., `a = b`, `a.x = b`, or `a[x] = b`).
/// Handle Python unpacking rules.
/// @example
///   `(a, b) = c`     -> `a = c[0]; b = c[1]`
///   `a, b = c`       -> `a = c[0]; b = c[1]`
///   `[a, *x, b] = c` -> `a = c[0]; x = c[1:-1]; b = c[-1]`.
/// Non-trivial right-hand expressions are first stored in a temporary variable.
/// @example
///   `a, b = c, d + foo()` -> `assign = (c, d + foo); a = assign[0]; b = assign[1]`.
/// Each assignment is unpacked recursively to allow cases like `a, (b, c) = d`.
Stmt *AssignStmt::unpack() const {
  std::vector<Expr *> leftSide;
  if (auto et = lhs->getTuple()) {
    // Case: (a, b) = ...
    for (auto &i : et->items)
      leftSide.push_back(i);
  } else if (auto el = lhs->getList()) {
    // Case: [a, b] = ...
    for (auto &i : el->items)
      leftSide.push_back(i);
  } else {
    // Case: simple assignment (a = b, a.x = b, or a[x] = b)
    return cache->NS<AssignStmt>(this, lhs, rhs, type);
  }

  // Prepare the right-side expression
  auto srcPos = rhs;
  SuiteStmt *block = cache->NS<SuiteStmt>(this);
  auto rhs = this->rhs;
  if (!rhs->getId()) {
    // Store any non-trivial right-side expression into a variable
    auto var = cache->getTemporaryVar("assign");
    rhs = cache->NS<IdExpr>(this->rhs, var);
    block->stmts.push_back(cache->NS<AssignStmt>(this, rhs, ast::clone(this->rhs)));
  }

  // Process assignments until the fist StarExpr (if any)
  size_t st = 0;
  for (; st < leftSide.size(); st++) {
    if (leftSide[st]->getStar())
      break;
    // Transformation: `leftSide_st = rhs[st]` where `st` is static integer
    auto rightSide =
        cache->NS<IndexExpr>(rhs, ast::clone(rhs), cache->NS<IntExpr>(rhs, st));
    // Recursively process the assignment because of cases like `(a, (b, c)) = d)`
    auto aa = AssignStmt(leftSide[st], rightSide);
    aa.cache = cache;
    auto ns = aa.unpack();
    block->stmts.push_back(ns);
  }
  // Process StarExpr (if any) and the assignments that follow it
  if (st < leftSide.size() && leftSide[st]->getStar()) {
    // StarExpr becomes SliceExpr (e.g., `b` in `(a, *b, c) = d` becomes `d[1:-2]`)
    auto rightSide = cache->NS<IndexExpr>(
        rhs, ast::clone(rhs),
        cache->NS<SliceExpr>(rhs, cache->NS<IntExpr>(rhs, st),
                             // this slice is either [st:] or [st:-lhs_len + st + 1]
                             leftSide.size() == st + 1
                                 ? nullptr
                                 : cache->NS<IntExpr>(rhs, -leftSide.size() + st + 1),
                             nullptr));
    auto aa = AssignStmt(leftSide[st]->getStar()->what, rightSide);
    aa.cache = cache;
    auto ns = aa.unpack();
    block->stmts.push_back(ns);
    st += 1;
    // Process remaining assignments. They will use negative indices (-1, -2 etc.)
    // because we do not know how big is StarExpr
    for (; st < leftSide.size(); st++) {
      if (leftSide[st]->getStar())
        E(Error::ASSIGN_MULTI_STAR, leftSide[st]);
      rightSide = cache->NS<IndexExpr>(
          rhs, ast::clone(rhs), cache->NS<IntExpr>(rhs, -int(leftSide.size() - st)));
      auto aa = AssignStmt(leftSide[st], rightSide);
      aa.cache = cache;
      auto ns = aa.unpack();
      block->stmts.push_back(ns);
    }
  }
  return block;
}

DelStmt::DelStmt(Expr *expr) : Stmt(), expr(expr) {}
DelStmt::DelStmt(const DelStmt &stmt, bool clean)
    : Stmt(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string DelStmt::toString(int indent) const {
  return format("(del {})", expr->toString(indent));
}
ACCEPT_IMPL(DelStmt, ASTVisitor);

PrintStmt::PrintStmt(std::vector<Expr *> items, bool isInline)
    : Stmt(), items(std::move(items)), isInline(isInline) {}
PrintStmt::PrintStmt(const PrintStmt &stmt, bool clean)
    : Stmt(stmt, clean), items(ast::clone(stmt.items, clean)), isInline(stmt.isInline) {
}
std::string PrintStmt::toString(int indent) const {
  return format("(print {}{})", isInline ? "#:inline " : "", combine(items));
}
ACCEPT_IMPL(PrintStmt, ASTVisitor);

ReturnStmt::ReturnStmt(Expr *expr) : Stmt(), expr(expr) {}
ReturnStmt::ReturnStmt(const ReturnStmt &stmt, bool clean)
    : Stmt(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string ReturnStmt::toString(int indent) const {
  return expr ? format("(return {})", expr->toString(indent)) : "(return)";
}
ACCEPT_IMPL(ReturnStmt, ASTVisitor);

YieldStmt::YieldStmt(Expr *expr) : Stmt(), expr(expr) {}
YieldStmt::YieldStmt(const YieldStmt &stmt, bool clean)
    : Stmt(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string YieldStmt::toString(int indent) const {
  return expr ? format("(yield {})", expr->toString(indent)) : "(yield)";
}
ACCEPT_IMPL(YieldStmt, ASTVisitor);

AssertStmt::AssertStmt(Expr *expr, Expr *message)
    : Stmt(), expr(expr), message(message) {}
AssertStmt::AssertStmt(const AssertStmt &stmt, bool clean)
    : Stmt(stmt, clean), expr(ast::clone(stmt.expr, clean)),
      message(ast::clone(stmt.message, clean)) {}
std::string AssertStmt::toString(int indent) const {
  return format("(assert {}{})", expr->toString(indent),
                message ? message->toString(indent) : "");
}
ACCEPT_IMPL(AssertStmt, ASTVisitor);

WhileStmt::WhileStmt(Expr *cond, Stmt *suite, Stmt *elseSuite)
    : Stmt(), cond(cond), suite(SuiteStmt::wrap(suite)),
      elseSuite(SuiteStmt::wrap(elseSuite)) {}
WhileStmt::WhileStmt(const WhileStmt &stmt, bool clean)
    : Stmt(stmt, clean), cond(ast::clone(stmt.cond, clean)),
      suite(ast::clone(stmt.suite, clean)),
      elseSuite(ast::clone(stmt.elseSuite, clean)) {}
std::string WhileStmt::toString(int indent) const {
  if (indent == -1)
    return format("(while {})", cond->toString(indent));
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  if (elseSuite && elseSuite->firstInBlock())
    return format("(while-else {}{}{}{}{})", cond->toString(indent), pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1), pad,
                  elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
  else
    return format("(while {}{}{})", cond->toString(indent), pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
}
ACCEPT_IMPL(WhileStmt, ASTVisitor);

ForStmt::ForStmt(Expr *var, Expr *iter, Stmt *suite, Stmt *elseSuite, Expr *decorator,
                 std::vector<CallExpr::Arg> ompArgs)
    : Stmt(), var(var), iter(iter), suite(SuiteStmt::wrap(suite)),
      elseSuite(SuiteStmt::wrap(elseSuite)), decorator(decorator),
      ompArgs(std::move(ompArgs)), wrapped(false), flat(false) {}
ForStmt::ForStmt(const ForStmt &stmt, bool clean)
    : Stmt(stmt, clean), var(ast::clone(stmt.var, clean)),
      iter(ast::clone(stmt.iter, clean)), suite(ast::clone(stmt.suite, clean)),
      elseSuite(ast::clone(stmt.elseSuite, clean)),
      decorator(ast::clone(stmt.decorator, clean)),
      ompArgs(ast::clone(stmt.ompArgs, clean)), wrapped(stmt.wrapped), flat(stmt.flat) {
}
std::string ForStmt::toString(int indent) const {
  auto vs = var->toString(indent);
  if (indent == -1)
    return format("(for {} {})", vs, iter->toString(indent));

  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string attr;
  if (decorator)
    attr += " " + decorator->toString(indent);
  if (!attr.empty())
    attr = " #:attr" + attr;
  if (elseSuite && elseSuite->firstInBlock())
    return format("(for-else {} {}{}{}{}{}{})", vs, iter->toString(indent), attr, pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1), pad,
                  elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
  else
    return format("(for {} {}{}{}{})", vs, iter->toString(indent), attr, pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
}
ACCEPT_IMPL(ForStmt, ASTVisitor);

IfStmt::IfStmt(Expr *cond, Stmt *ifSuite, Stmt *elseSuite)
    : Stmt(), cond(cond), ifSuite(SuiteStmt::wrap(ifSuite)),
      elseSuite(SuiteStmt::wrap(elseSuite)) {}
IfStmt::IfStmt(const IfStmt &stmt, bool clean)
    : Stmt(stmt, clean), cond(ast::clone(stmt.cond, clean)),
      ifSuite(ast::clone(stmt.ifSuite, clean)),
      elseSuite(ast::clone(stmt.elseSuite, clean)) {}
std::string IfStmt::toString(int indent) const {
  if (indent == -1)
    return format("(if {})", cond->toString(indent));
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  return format("(if {}{}{}{})", cond->toString(indent), pad,
                ifSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1),
                elseSuite
                    ? pad + elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                    : "");
}
ACCEPT_IMPL(IfStmt, ASTVisitor);

MatchStmt::MatchCase MatchStmt::MatchCase::clone(bool clean) const {
  return {ast::clone(pattern, clean), ast::clone(guard, clean),
          ast::clone(suite, clean)};
}

MatchStmt::MatchCase::MatchCase(Expr *pattern, Expr *guard, Stmt *suite)
    : pattern(pattern), guard(guard), suite(SuiteStmt::wrap(suite)) {}

MatchStmt::MatchStmt(Expr *what, std::vector<MatchStmt::MatchCase> cases)
    : Stmt(), what(what), cases(std::move(cases)) {}
MatchStmt::MatchStmt(const MatchStmt &stmt, bool clean)
    : Stmt(stmt, clean), what(ast::clone(stmt.what, clean)),
      cases(ast::clone(stmt.cases, clean)) {}
std::string MatchStmt::toString(int indent) const {
  if (indent == -1)
    return format("(match {})", what->toString(indent));
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string padExtra = indent > 0 ? std::string(INDENT_SIZE, ' ') : "";
  std::vector<std::string> s;
  for (auto &c : cases)
    s.push_back(format("(case {}{}{}{})", c.pattern->toString(indent),
                       c.guard ? " #:guard " + c.guard->toString(indent) : "",
                       pad + padExtra,
                       c.suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1 * 2)));
  return format("(match {}{}{})", what->toString(indent), pad, join(s, pad));
}
ACCEPT_IMPL(MatchStmt, ASTVisitor);

ImportStmt::ImportStmt(Expr *from, Expr *what, std::vector<Param> args, Expr *ret,
                       std::string as, size_t dots, bool isFunction)
    : Stmt(), from(from), what(what), as(std::move(as)), dots(dots),
      args(std::move(args)), ret(ret), isFunction(isFunction) {
  validate();
}
ImportStmt::ImportStmt(const ImportStmt &stmt, bool clean)
    : Stmt(stmt, clean), from(ast::clone(stmt.from, clean)),
      what(ast::clone(stmt.what, clean)), as(stmt.as), dots(stmt.dots),
      args(ast::clone(stmt.args, clean)), ret(ast::clone(stmt.ret, clean)),
      isFunction(stmt.isFunction) {}
std::string ImportStmt::toString(int indent) const {
  std::vector<std::string> va;
  for (auto &a : args)
    va.push_back(a.toString(indent));
  return format("(import {}{}{}{}{}{})", from ? from->toString(indent) : "",
                as.empty() ? "" : format(" #:as '{}", as),
                what ? format(" #:what {}", what->toString(indent)) : "",
                dots ? format(" #:dots {}", dots) : "",
                va.empty() ? "" : format(" #:args ({})", join(va)),
                ret ? format(" #:ret {}", ret->toString(indent)) : "");
}
void ImportStmt::validate() const {
  if (from) {
    Expr *e = from;
    while (auto d = e->getDot())
      e = d->expr;
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

TryStmt::Catch::Catch(const std::string &var, Expr *exc, Stmt *suite)
    : var(var), exc(exc), suite(SuiteStmt::wrap(suite)) {}
TryStmt::Catch::Catch(const TryStmt::Catch &stmt, bool clean)
    : Stmt(stmt), var(stmt.var), exc(ast::clone(stmt.exc, clean)),
      suite(ast::clone(stmt.suite, clean)) {}
std::string TryStmt::Catch::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string padExtra = indent > 0 ? std::string(INDENT_SIZE, ' ') : "";
  return format("(catch {}{}{}{})", !var.empty() ? format("#:var '{}", var) : "",
                exc ? format(" #:exc {}", exc->toString(indent)) : "", pad + padExtra,
                suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1 * 2));
}
ACCEPT_IMPL(TryStmt::Catch, ASTVisitor);

TryStmt::TryStmt(Stmt *suite, std::vector<TryStmt::Catch *> catches, Stmt *finally)
    : Stmt(), suite(SuiteStmt::wrap(suite)), catches(std::move(catches)),
      finally(SuiteStmt::wrap(finally)) {}
TryStmt::TryStmt(const TryStmt &stmt, bool clean)
    : Stmt(stmt, clean), suite(ast::clone(stmt.suite, clean)),
      catches(ast::clone(stmt.catches, clean)),
      finally(ast::clone(stmt.finally, clean)) {}
std::string TryStmt::toString(int indent) const {
  if (indent == -1)
    return format("(try)");
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> s;
  for (auto &i : catches)
    s.push_back(i->toString(indent));
  return format(
      "(try{}{}{}{}{})", pad, suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1),
      pad, join(s, pad),
      finally ? format("{}{}", pad,
                       finally->toString(indent >= 0 ? indent + INDENT_SIZE : -1))
              : "");
}
ACCEPT_IMPL(TryStmt, ASTVisitor);

ThrowStmt::ThrowStmt(Expr *expr, bool transformed)
    : Stmt(), expr(expr), transformed(transformed) {}
ThrowStmt::ThrowStmt(const ThrowStmt &stmt, bool clean)
    : Stmt(stmt, clean), expr(ast::clone(stmt.expr, clean)),
      transformed(stmt.transformed) {}
std::string ThrowStmt::toString(int indent) const {
  return format("(throw{})", expr ? " " + expr->toString(indent) : "");
}
ACCEPT_IMPL(ThrowStmt, ASTVisitor);

GlobalStmt::GlobalStmt(std::string var, bool nonLocal)
    : Stmt(), var(std::move(var)), nonLocal(nonLocal) {}
GlobalStmt::GlobalStmt(const GlobalStmt &stmt, bool clean)
    : Stmt(stmt, clean), var(stmt.var), nonLocal(stmt.nonLocal) {}
std::string GlobalStmt::toString(int indent) const {
  return format("({} '{})", nonLocal ? "nonlocal" : "global", var);
}
ACCEPT_IMPL(GlobalStmt, ASTVisitor);

FunctionStmt::FunctionStmt(std::string name, Expr *ret, std::vector<Param> args,
                           Stmt *suite, std::vector<Expr *> decorators)
    : Stmt(), name(std::move(name)), ret(ret), args(std::move(args)),
      suite(SuiteStmt::wrap(suite)), decorators(std::move(decorators)) {
  parseDecorators();
}
FunctionStmt::FunctionStmt(const FunctionStmt &stmt, bool clean)
    : Stmt(stmt, clean), name(stmt.name), ret(ast::clone(stmt.ret, clean)),
      args(ast::clone(stmt.args, clean)), suite(ast::clone(stmt.suite, clean)),
      decorators(ast::clone(stmt.decorators, clean)) {}
std::string FunctionStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> as;
  for (auto &a : args)
    as.push_back(a.toString(indent));
  std::vector<std::string> dec;
  for (auto &a : decorators)
    if (a)
      dec.push_back(format("(dec {})", a->toString(indent)));
  if (indent == -1)
    return format("(fn '{} ({}){})", name, join(as, " "),
                  ret ? " #:ret " + ret->toString(indent) : "");
  return format("(fn '{} ({}){}{}{}{})", name, join(as, " "),
                ret ? " #:ret " + ret->toString(indent) : "",
                dec.empty() ? "" : format(" (dec {})", join(dec, " ")), pad,
                suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                      : "(suite)");
}
void FunctionStmt::validate() const {
  if (!ret && (hasAttribute(Attr::LLVM) || hasAttribute(Attr::C)))
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
    if (hasAttribute(Attr::C)) {
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
void FunctionStmt::parseDecorators() {
  std::vector<Expr *> newDecorators;
  for (auto &d : decorators) {
    if (d->isId(Attr::Attribute)) {
      if (decorators.size() != 1)
        E(Error::FN_SINGLE_DECORATOR, decorators[1], Attr::Attribute);
      setAttribute(Attr::Attribute);
    } else if (d->isId(Attr::LLVM)) {
      setAttribute(Attr::LLVM);
    } else if (d->isId(Attr::Python)) {
      if (decorators.size() != 1)
        E(Error::FN_SINGLE_DECORATOR, decorators[1], Attr::Python);
      setAttribute(Attr::Python);
    } else if (d->isId(Attr::Internal)) {
      setAttribute(Attr::Internal);
    } else if (d->isId(Attr::HiddenFromUser)) {
      setAttribute(Attr::HiddenFromUser);
    } else if (d->isId(Attr::Atomic)) {
      setAttribute(Attr::Atomic);
    } else if (d->isId(Attr::Property)) {
      setAttribute(Attr::Property);
    } else if (d->isId(Attr::StaticMethod)) {
      setAttribute(Attr::StaticMethod);
    } else if (d->isId(Attr::ForceRealize)) {
      setAttribute(Attr::ForceRealize);
    } else if (d->isId(Attr::C)) {
      setAttribute(Attr::C);
    } else {
      newDecorators.emplace_back(d);
    }
  }
  if (hasAttribute(Attr::C)) {
    for (auto &a : args) {
      if (a.name.size() > 1 && a.name[0] == '*' && a.name[1] != '*')
        setAttribute(Attr::CVarArg);
    }
  }
  if (!args.empty() && !args[0].type && args[0].name == "self") {
    setAttribute(Attr::HasSelf);
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
  bool transform(Expr *expr) override {
    if (result)
      return result;
    IdSearchVisitor v(what);
    if (expr)
      expr->accept(v);
    return result = v.result;
  }
  bool transform(Stmt *stmt) override {
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

ClassStmt::ClassStmt(std::string name, std::vector<Param> args, Stmt *suite,
                     std::vector<Expr *> decorators, std::vector<Expr *> baseClasses,
                     std::vector<Expr *> staticBaseClasses)
    : Stmt(), name(std::move(name)), args(std::move(args)),
      suite(SuiteStmt::wrap(suite)), decorators(std::move(decorators)),
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
ClassStmt::ClassStmt(const ClassStmt &stmt, bool clean)
    : Stmt(stmt, clean), name(stmt.name), args(ast::clone(stmt.args, clean)),
      suite(ast::clone(stmt.suite, clean)),
      decorators(ast::clone(stmt.decorators, clean)),
      baseClasses(ast::clone(stmt.baseClasses, clean)),
      staticBaseClasses(ast::clone(stmt.staticBaseClasses, clean)) {}
std::string ClassStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> bases;
  for (auto &b : baseClasses)
    bases.push_back(b->toString(indent));
  for (auto &b : staticBaseClasses)
    bases.push_back(fmt::format("(static {})", b->toString(indent)));
  std::string as;
  for (int i = 0; i < args.size(); i++)
    as += (i ? pad : "") + args[i].toString(indent);
  std::vector<std::string> attr;
  for (auto &a : decorators)
    attr.push_back(format("(dec {})", a->toString(indent)));
  if (indent == -1)
    return format("(class '{} ({}))", name, as);
  return format("(class '{}{}{}{}{}{})", name,
                bases.empty() ? "" : format(" (bases {})", join(bases, " ")),
                attr.empty() ? "" : format(" (attr {})", join(attr, " ")),
                as.empty() ? as : pad + as, pad,
                suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                      : "(suite)");
}
void ClassStmt::validate() const {
  std::unordered_set<std::string> seen;
  if (hasAttribute(Attr::Extend) && !args.empty())
    E(Error::CLASS_EXTENSION, args[0]);
  if (hasAttribute(Attr::Extend) && !(baseClasses.empty() && staticBaseClasses.empty()))
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
bool ClassStmt::isRecord() const { return hasAttribute(Attr::Tuple); }
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
      setAttribute(Attr::ClassDeduce);
    } else if (d->isId("__notuple__")) {
      setAttribute(Attr::ClassNoTuple);
    } else if (d->isId("dataclass")) {
    } else if (auto c = d->getCall()) {
      if (c->expr->isId(Attr::Tuple)) {
        setAttribute(Attr::Tuple);
        for (auto &m : tupleMagics)
          m.second = true;
      } else if (!c->expr->isId("dataclass")) {
        E(Error::CLASS_BAD_DECORATOR, c->expr);
      } else if (hasAttribute(Attr::Tuple)) {
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
      if (hasAttribute(Attr::Tuple))
        E(Error::CLASS_MULTIPLE_DECORATORS, d, Attr::Tuple);
      setAttribute(Attr::Tuple);
      for (auto &m : tupleMagics) {
        m.second = true;
      }
    } else if (d->isId(Attr::Extend)) {
      setAttribute(Attr::Extend);
      if (decorators.size() != 1)
        E(Error::CLASS_SINGLE_DECORATOR, decorators[decorators[0] == d], Attr::Extend);
    } else if (d->isId(Attr::Internal)) {
      setAttribute(Attr::Internal);
    } else {
      E(Error::CLASS_BAD_DECORATOR, d);
    }
  }
  if (hasAttribute(Attr::ClassDeduce))
    tupleMagics["new"] = false;
  if (!hasAttribute(Attr::Tuple)) {
    tupleMagics["init"] = tupleMagics["new"];
    tupleMagics["new"] = tupleMagics["raw"] = true;
    tupleMagics["len"] = false;
  }
  tupleMagics["dict"] = true;
  // Internal classes do not get any auto-generated members.
  std::vector<std::string> magics;
  if (!hasAttribute(Attr::Internal)) {
    for (auto &m : tupleMagics)
      if (m.second) {
        if (m.first == "new")
          magics.insert(magics.begin(), m.first);
        else
          magics.push_back(m.first);
      }
  }
  setAttribute(Attr::ClassMagic, std::make_unique<ir::StringListAttribute>(magics));

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

YieldFromStmt::YieldFromStmt(Expr *expr) : Stmt(), expr(std::move(expr)) {}
YieldFromStmt::YieldFromStmt(const YieldFromStmt &stmt, bool clean)
    : Stmt(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string YieldFromStmt::toString(int indent) const {
  return format("(yield-from {})", expr->toString(indent));
}
ACCEPT_IMPL(YieldFromStmt, ASTVisitor);

WithStmt::WithStmt(std::vector<Expr *> items, std::vector<std::string> vars,
                   Stmt *suite)
    : Stmt(), items(std::move(items)), vars(std::move(vars)),
      suite(SuiteStmt::wrap(suite)) {
  seqassert(this->items.size() == this->vars.size(), "vector size mismatch");
}
WithStmt::WithStmt(std::vector<std::pair<Expr *, Expr *>> itemVarPairs, Stmt *suite)
    : Stmt(), suite(SuiteStmt::wrap(suite)) {
  for (auto [i, j] : itemVarPairs) {
    items.push_back(i);
    if (j) {
      if (!j->getId())
        throw;
      vars.push_back(j->getId()->value);
    } else {
      vars.emplace_back();
    }
  }
}
WithStmt::WithStmt(const WithStmt &stmt, bool clean)
    : Stmt(stmt, clean), items(ast::clone(stmt.items, clean)), vars(stmt.vars),
      suite(ast::clone(stmt.suite, clean)) {}
std::string WithStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> as;
  as.reserve(items.size());
  for (int i = 0; i < items.size(); i++) {
    as.push_back(!vars[i].empty()
                     ? format("({} #:var '{})", items[i]->toString(indent), vars[i])
                     : items[i]->toString(indent));
  }
  if (indent == -1)
    return format("(with ({}))", join(as, " "));
  return format("(with ({}){}{})", join(as, " "), pad,
                suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
}
ACCEPT_IMPL(WithStmt, ASTVisitor);

CustomStmt::CustomStmt(std::string keyword, Expr *expr, Stmt *suite)
    : Stmt(), keyword(std::move(keyword)), expr(expr), suite(SuiteStmt::wrap(suite)) {}
CustomStmt::CustomStmt(const CustomStmt &stmt, bool clean)
    : Stmt(stmt, clean), keyword(stmt.keyword), expr(ast::clone(stmt.expr, clean)),
      suite(ast::clone(stmt.suite, clean)) {}
std::string CustomStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  return format("(custom-{} {}{}{})", keyword,
                expr ? format(" #:expr {}", expr->toString(indent)) : "", pad,
                suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1) : "");
}
ACCEPT_IMPL(CustomStmt, ASTVisitor);

AssignMemberStmt::AssignMemberStmt(Expr *lhs, std::string member, Expr *rhs)
    : Stmt(), lhs(lhs), member(std::move(member)), rhs(rhs) {}
AssignMemberStmt::AssignMemberStmt(const AssignMemberStmt &stmt, bool clean)
    : Stmt(stmt, clean), lhs(ast::clone(stmt.lhs, clean)), member(stmt.member),
      rhs(ast::clone(stmt.rhs, clean)) {}
std::string AssignMemberStmt::toString(int indent) const {
  return format("(assign-member {} {} {})", lhs->toString(indent), member,
                rhs->toString(indent));
}
ACCEPT_IMPL(AssignMemberStmt, ASTVisitor);

CommentStmt::CommentStmt(std::string comment) : Stmt(), comment(std::move(comment)) {}
CommentStmt::CommentStmt(const CommentStmt &stmt, bool clean)
    : Stmt(stmt, clean), comment(stmt.comment) {}
std::string CommentStmt::toString(int indent) const {
  return format("(comment \"{}\")", comment);
}
ACCEPT_IMPL(CommentStmt, ASTVisitor);

} // namespace codon::ast
