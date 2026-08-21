// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "stmt.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "codon/parser/cache.h"
#include "codon/parser/match.h"
#include "codon/parser/visitors/visitor.h"

#define ACCEPT_IMPL(T, X)                                                              \
  ASTNode *T::clone(bool clean) const { return cache->N<T>(*this, clean); }            \
  void T::accept(X &visitor) { visitor.visit(this); }                                  \
  const char T::NodeId = 0;

using namespace codon::error;
using namespace codon::matcher;

namespace codon::ast {

Stmt::Stmt() : AcceptorExtend(), done(false) {}
Stmt::Stmt(const Stmt &stmt) : AcceptorExtend(stmt), done(stmt.done) {}
Stmt::Stmt(const codon::SrcInfo &s) : AcceptorExtend(), done(false) { setSrcInfo(s); }
Stmt::Stmt(const Stmt &stmt, bool clean) : AcceptorExtend(stmt), done(stmt.done) {
  if (clean)
    done = false;
}
std::string Stmt::wrapStmt(const std::string &s) const { return s; }

SuiteStmt::SuiteStmt(std::vector<Stmt *> stmts)
    : AcceptorExtend(), Items(std::move(stmts)) {}
SuiteStmt::SuiteStmt(const SuiteStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), Items(ast::clone(stmt.items, clean)) {}
std::string SuiteStmt::toString(int indent) const {
  if (indent == -1)
    return "";
  std::string pad = indent >= 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string s;
  for (int i = 0; i < size(); i++)
    if (items[i]) {
      auto is = items[i]->toString(indent >= 0 ? indent + INDENT_SIZE : -1);
      if (items[i]->isDone())
        is.insert(findStar(is), "*");
      s += (i ? pad : "") + is;
    }
  return wrapStmt(fmt::format("({}suite{})", (isDone() ? "*" : ""),
                              (s.empty() ? s : " " + pad + s)));
}
std::string SuiteStmt::toPythonString(bool a, int indent, int level) const {
  std::vector<std::string> v;
  std::function<void(const SuiteStmt *)> f = [&](const SuiteStmt *ss) {
    for (auto &s : *ss) {
      if (auto sp = cast<SuiteStmt>(s))
        f(sp);
      else if (s)
        v.push_back(s->toPythonString(a, indent, level + indent));
    }
  };
  f(this);
  return pyList(v, indent, level + indent);
}
void SuiteStmt::flatten() {
  std::vector<Stmt *> ns;
  for (auto &s : items) {
    if (!s)
      continue;
    if (!cast<SuiteStmt>(s)) {
      ns.push_back(s);
    } else {
      for (auto *ss : *cast<SuiteStmt>(s))
        ns.push_back(ss);
    }
  }
  items = ns;
}
void SuiteStmt::addStmt(Stmt *s) {
  if (s) {
    items.push_back(s);
    done = false;
  }
}
SuiteStmt *SuiteStmt::wrap(Stmt *s) {
  if (s && !cast<SuiteStmt>(s))
    return s->cache->NS<SuiteStmt>(s, s);
  return static_cast<SuiteStmt *>(s);
}

BreakStmt::BreakStmt(const BreakStmt &stmt, bool clean) : AcceptorExtend(stmt, clean) {}
std::string BreakStmt::toString(int indent) const { return wrapStmt("(break)"); }
std::string BreakStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode("Break", {}, this, a, indent, level);
}

ContinueStmt::ContinueStmt(const ContinueStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean) {}
std::string ContinueStmt::toString(int indent) const { return wrapStmt("(continue)"); }
std::string ContinueStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode("Continue", {}, this, a, indent, level);
}

ExprStmt::ExprStmt(Expr *expr) : AcceptorExtend(), expr(expr) {}
ExprStmt::ExprStmt(const ExprStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string ExprStmt::toString(int indent) const {
  return wrapStmt(fmt::format("(expr {})", expr->toString(indent)));
}
std::string ExprStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "Expr",
      {"value=" + (expr ? expr->toPythonString(a, indent, level + indent) : "None")},
      this, a, indent, level);
}

AssignStmt::AssignStmt(Expr *lhs, Expr *rhs, Expr *type, UpdateMode update)
    : AcceptorExtend(), lhs(lhs), rhs(rhs), type(type), update(update) {}
AssignStmt::AssignStmt(const AssignStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), lhs(ast::clone(stmt.lhs, clean)),
      rhs(ast::clone(stmt.rhs, clean)), type(ast::clone(stmt.type, clean)),
      update(stmt.update) {}
std::string AssignStmt::toString(int indent) const {
  return wrapStmt(
      fmt::format("({} {}{}{})", update != Assign ? "update" : "assign",
                  lhs->toString(indent), rhs ? " " + rhs->toString(indent) : "",
                  type ? fmt::format(" #:type {}", type->toString(indent)) : ""));
}
std::string AssignStmt::toPythonString(bool a, int indent, int level) const {
  // AugAssign
  // Simple assign
  if (auto b = cast<BinaryExpr>(rhs); b && b->isInPlace()) {
    return pyNode(
        "AugAssign",
        {"target=" + (lhs ? lhs->toPythonString(a, indent, level + indent) : "None"),
         "op=" + pyQuote(b->getOp()),
         "value=" + b->getRhs()->toPythonString(a, indent, level + indent)},
        this, a, indent, level);
  } else if (!type) {
    return pyNode(
        "Assign",
        {"targets=" +
             pyList({lhs ? lhs->toPythonString(a, indent, level + indent) : "None"},
                    indent, level + indent),
         "value=" + (rhs ? rhs->toPythonString(a, indent, level + indent) : "None")},
        this, a, indent, level);
  } else {
    return pyNode(
        "AnnAssign",
        {"target=" + (lhs ? lhs->toPythonString(a, indent, level + indent) : "None"),
         "annotation=" +
             (type ? type->toPythonString(a, indent, level + indent) : "None"),
         "value=" + (rhs ? rhs->toPythonString(a, indent, level + indent) : "None")},
        this, a, indent, level);
  }
}

DelStmt::DelStmt(Expr *expr) : AcceptorExtend(), expr(expr) {}
DelStmt::DelStmt(const DelStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string DelStmt::toString(int indent) const {
  return wrapStmt(fmt::format("(del {})", expr->toString(indent)));
}
std::string DelStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "Delete",
      {"targets=" +
       pyList({expr ? expr->toPythonString(a, indent, level + indent) : "None"}, indent,
              level + indent)},
      this, a, indent, level);
}

PrintStmt::PrintStmt(std::vector<Expr *> items, bool noNewline)
    : AcceptorExtend(), Items(std::move(items)), noNewline(noNewline) {}
PrintStmt::PrintStmt(const PrintStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), Items(ast::clone(stmt.items, clean)),
      noNewline(stmt.noNewline) {}
std::string PrintStmt::toString(int indent) const {
  return wrapStmt(
      fmt::format("(print {}{})", noNewline ? "#:inline " : "", combine(items)));
}
std::string PrintStmt::toPythonString(bool a, int indent, int level) const {
  std::vector<std::string> v;
  for (auto x : *this)
    v.push_back(x ? x->toPythonString(a, indent, level + indent) : "None");
  if (noNewline)
    v.push_back("end=' '");
  return pyNode("Expr",
                {"value=" + pyNode("Call",
                                   {"func=Name(id='print')",
                                    "args=" + pyList(v, indent, level + indent)},
                                   this, a, indent, level)},
                this, a, indent, level);
}

ReturnStmt::ReturnStmt(Expr *expr) : AcceptorExtend(), expr(expr) {}
ReturnStmt::ReturnStmt(const ReturnStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string ReturnStmt::toString(int indent) const {
  return wrapStmt(expr ? fmt::format("(return {})", expr->toString(indent))
                       : "(return)");
}
std::string ReturnStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "Return",
      {"value=" + (expr ? expr->toPythonString(a, indent, level + indent) : "None")},
      this, a, indent, level);
}

YieldStmt::YieldStmt(Expr *expr) : AcceptorExtend(), expr(expr) {}
YieldStmt::YieldStmt(const YieldStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string YieldStmt::toString(int indent) const {
  return wrapStmt(expr ? fmt::format("(yield {})", expr->toString(indent)) : "(yield)");
}
std::string YieldStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "Expr",
      {"value=" +
       pyNode("Yield",
              {"value=" +
               (expr ? expr->toPythonString(a, indent, level + indent) : "None")},
              this, a, indent, level)},
      this, a, indent, level);
}

AssertStmt::AssertStmt(Expr *expr, Expr *message)
    : AcceptorExtend(), expr(expr), message(message) {}
AssertStmt::AssertStmt(const AssertStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), expr(ast::clone(stmt.expr, clean)),
      message(ast::clone(stmt.message, clean)) {}
std::string AssertStmt::toString(int indent) const {
  return wrapStmt(fmt::format("(assert {}{})", expr->toString(indent),
                              message ? message->toString(indent) : ""));
}
std::string AssertStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "Assert",
      {"test=" + (expr ? expr->toPythonString(a, indent, level + indent) : "None"),
       "msg=" +
           (message ? message->toPythonString(a, indent, level + indent) : "None")},
      this, a, indent, level);
}

WhileStmt::WhileStmt(Expr *cond, Stmt *suite, Stmt *elseSuite)
    : AcceptorExtend(), cond(cond), suite(SuiteStmt::wrap(suite)),
      elseSuite(SuiteStmt::wrap(elseSuite)) {}
WhileStmt::WhileStmt(const WhileStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), cond(ast::clone(stmt.cond, clean)),
      suite(ast::clone(stmt.suite, clean)),
      elseSuite(ast::clone(stmt.elseSuite, clean)) {}
std::string WhileStmt::toString(int indent) const {
  if (indent == -1)
    return wrapStmt(fmt::format("(while {})", cond->toString(indent)));
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  if (elseSuite && elseSuite->firstInBlock()) {
    return wrapStmt(
        fmt::format("(while-else {}{}{}{}{})", cond->toString(indent), pad,
                    suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1), pad,
                    elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)));
  } else {
    return wrapStmt(
        fmt::format("(while {}{}{})", cond->toString(indent), pad,
                    suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)));
  }
}
std::string WhileStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "While",
      {"test=" + (cond ? cond->toPythonString(a, indent, level + indent) : "None"),
       "body=" + (suite ? suite->toPythonString(a, indent, level + indent) : "[]"),
       "orelse=" +
           (elseSuite ? elseSuite->toPythonString(a, indent, level + indent) : "[]")},
      this, a, indent, level);
}

ForStmt::ForStmt(Expr *var, Expr *iter, Stmt *suite, Stmt *elseSuite, Expr *decorator,
                 std::vector<CallArg> ompArgs, bool async)
    : AcceptorExtend(), var(var), iter(iter), suite(SuiteStmt::wrap(suite)),
      elseSuite(SuiteStmt::wrap(elseSuite)), decorator(decorator),
      ompArgs(std::move(ompArgs)), async(async), wrapped(false), flat(false) {}
ForStmt::ForStmt(const ForStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), var(ast::clone(stmt.var, clean)),
      iter(ast::clone(stmt.iter, clean)), suite(ast::clone(stmt.suite, clean)),
      elseSuite(ast::clone(stmt.elseSuite, clean)),
      decorator(ast::clone(stmt.decorator, clean)),
      ompArgs(ast::clone(stmt.ompArgs, clean)), async(stmt.async),
      wrapped(stmt.wrapped), flat(stmt.flat) {}
std::string ForStmt::toString(int indent) const {
  auto vs = var->toString(indent);
  if (indent == -1)
    return wrapStmt(fmt::format("(for {} {})", vs, iter->toString(indent)));

  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string attr;
  if (decorator)
    attr += " " + decorator->toString(indent);
  if (!attr.empty())
    attr = " #:attr" + attr;
  if (elseSuite && elseSuite->firstInBlock()) {
    return wrapStmt(
        fmt::format("(for-else {} {}{}{}{}{}{})", vs, iter->toString(indent), attr, pad,
                    suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1), pad,
                    elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)));
  } else {
    return wrapStmt(
        fmt::format("(for {} {}{}{}{})", vs, iter->toString(indent), attr, pad,
                    suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)));
  }
}
std::string ForStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      async ? "AsyncFor" : "For",
      {"target=" + (var ? var->toPythonString(a, indent, level + indent) : "None"),
       "iter=" + (iter ? iter->toPythonString(a, indent, level + indent) : "None"),
       "body=" + (suite ? suite->toPythonString(a, indent, level + indent) : "[]"),
       "orelse=" +
           (elseSuite ? elseSuite->toPythonString(a, indent, level + indent) : "[]")},
      this, a, indent, level);
}

IfStmt::IfStmt(Expr *cond, Stmt *ifSuite, Stmt *elseSuite)
    : AcceptorExtend(), cond(cond), ifSuite(SuiteStmt::wrap(ifSuite)),
      elseSuite(SuiteStmt::wrap(elseSuite)) {}
IfStmt::IfStmt(const IfStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), cond(ast::clone(stmt.cond, clean)),
      ifSuite(ast::clone(stmt.ifSuite, clean)),
      elseSuite(ast::clone(stmt.elseSuite, clean)) {}
std::string IfStmt::toString(int indent) const {
  if (indent == -1)
    return wrapStmt(fmt::format("(if {})", cond->toString(indent)));
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  return wrapStmt(fmt::format(
      "(if {}{}{}{})", cond->toString(indent), pad,
      ifSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1),
      elseSuite ? pad + elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                : ""));
}
std::string IfStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "If",
      {"test=" + (cond ? cond->toPythonString(a, indent, level + indent) : "None"),
       "body=" + (ifSuite ? ifSuite->toPythonString(a, indent, level + indent) : "[]"),
       "orelse=" +
           (elseSuite ? elseSuite->toPythonString(a, indent, level + indent) : "[]")},
      this, a, indent, level);
}

MatchCase::MatchCase(Expr *pattern, Expr *guard, Stmt *suite)
    : pattern(pattern), guard(guard), suite(SuiteStmt::wrap(suite)) {}
MatchCase MatchCase::clone(bool clean) const {
  return {ast::clone(pattern, clean), ast::clone(guard, clean),
          ast::clone(suite, clean)};
}

MatchStmt::MatchStmt(Expr *expr, std::vector<MatchCase> cases)
    : AcceptorExtend(), Items(std::move(cases)), expr(expr) {}
MatchStmt::MatchStmt(const MatchStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), Items(ast::clone(stmt.items, clean)),
      expr(ast::clone(stmt.expr, clean)) {}
std::string MatchStmt::toString(int indent) const {
  if (indent == -1)
    return wrapStmt(fmt::format("(match {})", expr->toString(indent)));
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string padExtra = indent > 0 ? std::string(INDENT_SIZE, ' ') : "";
  std::vector<std::string> s;
  for (auto &c : items)
    s.push_back(fmt::format(
        "(case {}{}{}{})", c.pattern->toString(indent),
        c.guard ? " #:guard " + c.guard->toString(indent) : "", pad + padExtra,
        c.suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1 * 2)));
  return wrapStmt(
      fmt::format("(match {}{}{})", expr->toString(indent), pad, join(s, pad)));
}
std::string MatchStmt::toPythonString(bool a, int indent, int level) const {
  std::vector<std::string> v;
  for (auto &i : items) {
    v.push_back(pyNode(
        "MatchCase",
        {"pattern=" + (i.pattern
                           ? i.pattern->toPythonString(a, indent, 2 * level + indent)
                           : "None"),
         "guard=" + (i.guard ? i.guard->toPythonString(a, indent, 2 * level + indent)
                             : "None"),
         "body=" +
             (i.suite ? i.suite->toPythonString(a, indent, 2 * level + indent) : "[]")},
        this, a, indent, level + indent));
  }
  return pyNode(
      "Match",
      {"subject=" + (expr ? expr->toPythonString(a, indent, level + indent) : "None"),
       "cases=" + pyList(v, indent, level + indent)},
      this, a, indent, level);
}

ImportStmt::ImportStmt(Expr *from, Expr *what, std::vector<Param> args, Expr *ret,
                       std::string as, size_t dots, bool isFunction)
    : AcceptorExtend(), from(from), what(what), as(std::move(as)), dots(dots),
      args(std::move(args)), ret(ret), isFunction(isFunction) {}
ImportStmt::ImportStmt(const ImportStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), from(ast::clone(stmt.from, clean)),
      what(ast::clone(stmt.what, clean)), as(stmt.as), dots(stmt.dots),
      args(ast::clone(stmt.args, clean)), ret(ast::clone(stmt.ret, clean)),
      isFunction(stmt.isFunction) {}
std::string ImportStmt::toString(int indent) const {
  std::vector<std::string> va;
  for (auto &a : args)
    va.push_back(a.toString(indent));
  return wrapStmt(
      fmt::format("(import {}{}{}{}{}{})", from ? from->toString(indent) : "",
                  as.empty() ? "" : fmt::format(" #:as '{}", as),
                  what ? fmt::format(" #:what {}", what->toString(indent)) : "",
                  dots ? fmt::format(" #:dots {}", dots) : "",
                  va.empty() ? "" : fmt::format(" #:args ({})", join(va)),
                  ret ? fmt::format(" #:ret {}", ret->toString(indent)) : ""));
}
std::string ImportStmt::toPythonString(bool a, int indent, int level) const {
  std::string name, mod;
  std::vector<std::string> components;
  auto fr = what;
  for (; cast<DotExpr>(fr); fr = cast<DotExpr>(fr)->getExpr())
    components.push_back(cast<DotExpr>(fr)->getMember());
  components.push_back(cast<IdExpr>(fr)->getValue());
  std::ranges::reverse(components);
  name = combine2(components, ".");
  if (from) {
    components.clear();
    for (fr = from; cast<DotExpr>(fr); fr = cast<DotExpr>(fr)->getExpr())
      components.push_back(cast<DotExpr>(fr)->getMember());
    components.push_back(cast<IdExpr>(fr)->getValue());
    std::ranges::reverse(components);
    mod = combine2(components, ".");
  }

  if (!from) {
    return pyNode(
        "Import",
        {"names=" +
         pyList(
             {pyNode(
                 "Alias",
                 {
                     "name=" + name,
                     "asname=" + pyQuote(as),
                     "params=" + pyArguments(args, a, indent, 4 * indent + level),
                     "ret=" + (ret ? ret->toPythonString(a, indent, 4 * indent + level)
                                   : "None"),
                 },
                 nullptr, a, indent, 3 * indent + level)},
             indent, 2 * indent + level)},
        what, a, indent, indent + level);
  } else {
    return pyNode(
        "ImportFrom",
        {"module=" + mod,
         "names=" +
             pyList({pyNode("Alias",
                            {
                                "name=" + name,
                                "asname=" + pyQuote(as),
                                "params=" +
                                    pyArguments(args, a, indent, 4 * indent + level),
                                "ret=" + (ret ? ret->toPythonString(a, indent,
                                                                    4 * indent + level)
                                              : "None"),
                            },
                            nullptr, a, indent, 3 * indent + level)},
                    indent, 2 * indent + level),
         "level=" + std::to_string(dots)},
        what, a, indent, indent + level);
  }
}

ExceptStmt::ExceptStmt(const std::string &var, Expr *exc, Stmt *suite)
    : var(var), exc(exc), suite(SuiteStmt::wrap(suite)) {}
ExceptStmt::ExceptStmt(const ExceptStmt &stmt, bool clean)
    : AcceptorExtend(stmt), var(stmt.var), exc(ast::clone(stmt.exc, clean)),
      suite(ast::clone(stmt.suite, clean)) {}
std::string ExceptStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string padExtra = indent > 0 ? std::string(INDENT_SIZE, ' ') : "";
  return wrapStmt(fmt::format(
      "(catch {}{}{}{})", !var.empty() ? fmt::format("#:var '{}", var) : "",
      exc ? fmt::format(" #:exc {}", exc->toString(indent)) : "", pad + padExtra,
      suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1 * 2)));
}
std::string ExceptStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "ExceptHandler",
      {"type=" + (exc ? exc->toPythonString(a, indent, level + indent) : "None"),
       "name=" + pyQuote(var),
       "body=" + (suite ? suite->toPythonString(a, indent, level + indent) : "[]")},
      this, a, indent, level);
}

TryStmt::TryStmt(Stmt *suite, std::vector<ExceptStmt *> excepts, Stmt *elseSuite,
                 Stmt *finally)
    : AcceptorExtend(), Items(std::move(excepts)), suite(SuiteStmt::wrap(suite)),
      elseSuite(SuiteStmt::wrap(elseSuite)), finally(SuiteStmt::wrap(finally)) {}
TryStmt::TryStmt(const TryStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), Items(ast::clone(stmt.items, clean)),
      suite(ast::clone(stmt.suite, clean)),
      elseSuite(ast::clone(stmt.elseSuite, clean)),
      finally(ast::clone(stmt.finally, clean)) {}
std::string TryStmt::toString(int indent) const {
  if (indent == -1)
    return wrapStmt(fmt::format("(try)"));
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> s;
  for (auto &i : items)
    s.push_back(i->toString(indent));
  return wrapStmt(fmt::format(
      "(try{}{}{}{}{})", pad, suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1),
      pad, join(s, pad),
      elseSuite
          ? fmt::format("{}(else {})", pad,
                        elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1))
          : "",
      finally ? fmt::format("{}(finally {})", pad,
                            finally->toString(indent >= 0 ? indent + INDENT_SIZE : -1))
              : ""));
}
std::string TryStmt::toPythonString(bool a, int indent, int level) const {
  std::vector<std::string> v;
  for (auto &i : items)
    v.push_back(i->toPythonString(a, indent, indent + level));
  return pyNode(
      "Try",
      {"body=" + (suite ? suite->toPythonString(a, indent, level + indent) : "[]"),
       "handlers=" + pyList(v, indent, level + indent),
       "orelse=" +
           (elseSuite ? elseSuite->toPythonString(a, indent, level + indent) : "[]"),
       "finalbody=" +
           (finally ? finally->toPythonString(a, indent, level + indent) : "[]")},
      this, a, indent, level);
}

ThrowStmt::ThrowStmt(Expr *expr, Expr *from, bool transformed)
    : AcceptorExtend(), expr(expr), from(from), transformed(transformed) {}
ThrowStmt::ThrowStmt(const ThrowStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), expr(ast::clone(stmt.expr, clean)),
      from(ast::clone(stmt.from, clean)), transformed(stmt.transformed) {}
std::string ThrowStmt::toString(int indent) const {
  return wrapStmt(
      fmt::format("(throw{}{})", expr ? " " + expr->toString(indent) : "",
                  from ? fmt::format(" :from {}", from->toString(indent)) : ""));
}
std::string ThrowStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "Raise",
      {"exc=" + (expr ? expr->toPythonString(a, indent, level + indent) : "None"),
       "cause=" + (from ? from->toPythonString(a, indent, level + indent) : "None")},
      this, a, indent, level);
}

GlobalStmt::GlobalStmt(std::string var, bool nonLocal)
    : AcceptorExtend(), var(std::move(var)), nonLocal(nonLocal) {}
GlobalStmt::GlobalStmt(const GlobalStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), var(stmt.var), nonLocal(stmt.nonLocal) {}
std::string GlobalStmt::toString(int indent) const {
  return wrapStmt(fmt::format("({} '{})", nonLocal ? "nonlocal" : "global", var));
}
std::string GlobalStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(nonLocal ? "Nonlocal" : "Global", {"names=[" + pyQuote(var) + "]"},
                this, a, indent, level);
}

FunctionStmt::FunctionStmt(std::string name, Expr *ret, std::vector<Param> args,
                           Stmt *suite, std::vector<Expr *> decorators, bool async)
    : AcceptorExtend(), Items(std::move(args)), name(std::move(name)), ret(ret),
      suite(SuiteStmt::wrap(suite)), decorators(std::move(decorators)), async(async) {}
FunctionStmt::FunctionStmt(const FunctionStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), Items(ast::clone(stmt.items, clean)),
      name(stmt.name), ret(ast::clone(stmt.ret, clean)),
      suite(ast::clone(stmt.suite, clean)),
      decorators(ast::clone(stmt.decorators, clean)), async(stmt.async) {}
std::string FunctionStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> as;
  for (auto &a : items)
    as.push_back(a.toString(indent));
  std::vector<std::string> dec;
  for (auto &a : decorators)
    if (a)
      dec.push_back(fmt::format("(dec {})", a->toString(indent)));
  if (indent == -1)
    return wrapStmt(fmt::format("(fn '{} ({}){})", name, join(as, " "),
                                ret ? " #:ret " + ret->toString(indent) : ""));
  return wrapStmt(fmt::format(
      "(fn '{} ({}){}{}{}{})", name, join(as, " "),
      ret ? " #:ret " + ret->toString(indent) : "",
      dec.empty() ? "" : fmt::format(" (dec {})", join(dec, " ")), pad,
      suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1) : "(suite)"));
}
std::string FunctionStmt::toPythonString(bool a, int indent, int level) const {
  std::vector<std::string> dv;
  for (auto &i : decorators) {
    dv.push_back(i->toPythonString(a, indent, indent + level));
  }
  return pyNode(
      async ? "AsyncFunctionDef" : "FunctionDef",
      {"name=" + pyQuote(name), "args=" + pyArguments(items, a, indent, level + 2*indent),
       "body=" + (suite ? suite->toPythonString(a, indent, level + 2*indent) : "[]"),
       "decorator_list=" + pyList(dv, indent, level + 2*indent),
       "returns=" + (ret ? ret->toPythonString(a, indent, level + 2*indent) : "None")},
      this, a, indent, indent+level);
}
std::string FunctionStmt::getSignature() {
  if (signature.empty()) {
    std::vector<std::string> s;
    for (auto &a : items)
      s.push_back(a.type ? a.type->toString() : "-");
    signature = join(s, ":");
  }
  return signature;
}
size_t FunctionStmt::getStarArgs() const {
  size_t i = 0;
  while (i < items.size()) {
    if (startswith(items[i].name, "*") && !startswith(items[i].name, "**"))
      break;
    i++;
  }
  return i;
}
size_t FunctionStmt::getKwStarArgs() const {
  size_t i = 0;
  while (i < items.size()) {
    if (startswith(items[i].name, "**"))
      break;
    i++;
  }
  return i;
}
std::string FunctionStmt::getDocstr() const {
  if (auto s = suite->firstInBlock()) {
    if (auto e = cast<ExprStmt>(s)) {
      if (auto ss = cast<StringExpr>(e->getExpr()))
        return ss->getValue();
    }
  }
  return "";
}
bool FunctionStmt::hasFunctionAttribute(const std::string &attr) const {
  if (auto f = getAttribute<ir::KeyValueAttribute>(Attr::FunctionAttributes)) {
    return in(f->attributes, attr) != nullptr;
  }
  return false;
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
    if (expr->getValue() == what)
      result = true;
  }
};

/// Check if a function can be called with the given arguments.
/// See @c reorderNamedArgs for details.
std::unordered_set<std::string> FunctionStmt::getNonInferrableGenerics() const {
  std::unordered_set<std::string> nonInferrableGenerics;
  for (const auto &a : items) {
    if (a.status == Param::Generic && !a.defaultValue) {
      bool inferrable = false;
      for (const auto &b : items)
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
                     std::vector<Expr *> decorators,
                     const std::vector<Expr *> &baseClasses)
    : AcceptorExtend(), Items(std::move(args)), name(std::move(name)),
      suite(SuiteStmt::wrap(suite)), decorators(std::move(decorators)),
      baseClasses(std::move(baseClasses)) {}
ClassStmt::ClassStmt(const ClassStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), Items(ast::clone(stmt.items, clean)),
      name(stmt.name), suite(ast::clone(stmt.suite, clean)),
      decorators(ast::clone(stmt.decorators, clean)),
      baseClasses(ast::clone(stmt.baseClasses, clean)) {}
std::string ClassStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> bases;
  for (auto &b : baseClasses)
    bases.push_back(b->toString(indent));
  std::string as;
  for (int i = 0; i < items.size(); i++)
    as += (i ? pad : "") + items[i].toString(indent);
  std::vector<std::string> attr;
  for (auto &a : decorators)
    attr.push_back(fmt::format("(dec {})", a->toString(indent)));
  if (indent == -1)
    return wrapStmt(fmt::format("(class '{} ({}))", name, as));
  return wrapStmt(fmt::format(
      "(class '{}{}{}{}{}{})", name,
      bases.empty() ? "" : fmt::format(" (bases {})", join(bases, " ")),
      attr.empty() ? "" : fmt::format(" (attr {})", join(attr, " ")),
      as.empty() ? as : pad + as, pad,
      suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1) : "(suite)"));
}
std::string ClassStmt::toPythonString(bool a, int indent, int level) const {
  std::vector<std::string> b;
  for (auto x : baseClasses)
    b.push_back(x ? x->toPythonString(a, indent, level + indent) : "None");
  std::vector<std::string> dv;
  for (auto &i : decorators) {
    dv.push_back(i->toPythonString(a, indent, indent + level));
  }
  return pyNode(
      "ClassDef",
      {"name=" + pyQuote(name), "bases=" + pyList(b, indent, level + indent),
       "keywords=" + pyArguments(items, a, indent, level + indent),
       "body=" + (suite ? suite->toPythonString(a, indent, level + indent) : "[]"),
       "decorator_list=" + pyList(dv, indent, level + indent)},
      this, a, indent, level);
}
bool ClassStmt::isRecord() const { return hasAttribute(Attr::Tuple); }
bool ClassStmt::isClassVar(const Param &p) {
  if (!p.type)
    return true;
  if (auto i = cast<IndexExpr>(p.type))
    return isId(i->getExpr(), "ClassVar");
  return false;
}
std::string ClassStmt::getDocstr() const {
  if (auto s = suite->firstInBlock()) {
    if (auto e = cast<ExprStmt>(s)) {
      if (auto ss = cast<StringExpr>(e->getExpr()))
        return ss->getValue();
    }
  }
  return "";
}

YieldFromStmt::YieldFromStmt(Expr *expr) : AcceptorExtend(), expr(std::move(expr)) {}
YieldFromStmt::YieldFromStmt(const YieldFromStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), expr(ast::clone(stmt.expr, clean)) {}
std::string YieldFromStmt::toString(int indent) const {
  return wrapStmt(fmt::format("(yield-from {})", expr->toString(indent)));
}
std::string YieldFromStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "Expr",
      {"value=" +
       pyNode("YieldFrom",
              {"value=" +
               (expr ? expr->toPythonString(a, indent, level + indent) : "None")},
              this, a, indent, level)},
      this, a, indent, level);
}

WithStmt::WithStmt(std::vector<Expr *> items, std::vector<std::string> vars,
                   Stmt *suite, bool isAsync)
    : AcceptorExtend(), Items(std::move(items)), vars(std::move(vars)),
      suite(SuiteStmt::wrap(suite)), async(isAsync) {
  seqassert(this->items.size() == this->vars.size(), "vector size mismatch");
}
WithStmt::WithStmt(std::vector<std::pair<Expr *, Expr *>> itemVarPairs, Stmt *suite,
                   bool isAsync)
    : AcceptorExtend(), Items({}), suite(SuiteStmt::wrap(suite)), async(isAsync) {
  for (auto [i, j] : itemVarPairs) {
    items.push_back(i);
    if (auto je = cast<IdExpr>(j)) {
      vars.push_back(je->getValue());
    } else {
      vars.emplace_back();
    }
  }
}
WithStmt::WithStmt(const WithStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), Items(ast::clone(stmt.items, clean)),
      vars(stmt.vars), suite(ast::clone(stmt.suite, clean)), async(stmt.async) {}
std::string WithStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> as;
  as.reserve(items.size());
  for (int i = 0; i < items.size(); i++) {
    as.push_back(!vars[i].empty() ? fmt::format("({} #:var '{})",
                                                items[i]->toString(indent), vars[i])
                                  : items[i]->toString(indent));
  }
  if (indent == -1)
    return wrapStmt(fmt::format("(with ({}))", join(as, " ")));
  return wrapStmt(
      fmt::format("(with ({}){}{})", join(as, " "), pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)));
}
std::string WithStmt::toPythonString(bool a, int indent, int level) const {
  std::vector<std::string> v;
  for (int i = 0; i < items.size(); i++) {
    v.push_back(pyNode(
        "WithItem",
        {"context_expr=" +
             (items[i] ? items[i]->toPythonString(a, indent, level + indent) : "None"),
         "optional_vars=" + pyQuote(vars[i])},
        this, a, indent, level));
  }
  return pyNode(
      async ? "AsyncWith" : "With",
      {"items=" + pyList(v, indent, level + indent),
       "body=" + (suite ? suite->toPythonString(a, indent, level + indent) : "[]")},
      this, a, indent, level);
}

CustomStmt::CustomStmt(std::string keyword, Expr *expr, Stmt *suite)
    : AcceptorExtend(), keyword(std::move(keyword)), expr(expr),
      suite(SuiteStmt::wrap(suite)) {}
CustomStmt::CustomStmt(const CustomStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), keyword(stmt.keyword),
      expr(ast::clone(stmt.expr, clean)), suite(ast::clone(stmt.suite, clean)) {}
std::string CustomStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  return wrapStmt(fmt::format(
      "(custom-{} {}{}{})", keyword,
      expr ? fmt::format(" #:expr {}", expr->toString(indent)) : "", pad,
      suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1) : ""));
}
std::string CustomStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "Custom",
      {"keyword=" + pyQuote(keyword),
       "body=" + (suite ? suite->toPythonString(a, indent, level + indent) : "[]")},
      this, a, indent, level);
}

DirectiveStmt::DirectiveStmt(std::string key, std::string value)
    : AcceptorExtend(), key(std::move(key)), value(std::move(value)) {}
DirectiveStmt::DirectiveStmt(const DirectiveStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), key(stmt.key), value(stmt.value) {}
std::string DirectiveStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  return wrapStmt(fmt::format("(directive {} '{}')", key, value));
}
std::string DirectiveStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode("Directive", {"key=" + pyQuote(key), "value=" + pyQuote(value)}, this,
                a, indent, level);
}

AssignMemberStmt::AssignMemberStmt(Expr *lhs, std::string member, Expr *rhs, Expr *type)
    : AcceptorExtend(), lhs(lhs), member(std::move(member)), rhs(rhs), type(type) {}
AssignMemberStmt::AssignMemberStmt(const AssignMemberStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), lhs(ast::clone(stmt.lhs, clean)),
      member(stmt.member), rhs(ast::clone(stmt.rhs, clean)),
      type(ast::clone(stmt.type, clean)) {}
std::string AssignMemberStmt::toString(int indent) const {
  return wrapStmt(fmt::format("(assign-member {} {} {})", lhs->toString(indent), member,
                              rhs->toString(indent)));
}
std::string AssignMemberStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode(
      "Assign",
      {"targets=" +
           pyList(
               {pyNode("Attribute",
                       {"value=" + (lhs ? lhs->toPythonString(a, indent, level + indent)
                                        : "None"),
                        "attr=" + pyQuote(member)},
                       this, a, indent, level)},
               indent, level + indent),
       "value=" + (rhs ? rhs->toPythonString(a, indent, level + indent) : "None")},
      this, a, indent, level);
}

CommentStmt::CommentStmt(std::string comment)
    : AcceptorExtend(), comment(std::move(comment)) {}
CommentStmt::CommentStmt(const CommentStmt &stmt, bool clean)
    : AcceptorExtend(stmt, clean), comment(stmt.comment) {}
std::string CommentStmt::toString(int indent) const {
  return wrapStmt(fmt::format("(comment \"{}\")", comment));
}
std::string CommentStmt::toPythonString(bool a, int indent, int level) const {
  return pyNode("Comment", {"value=" + pyQuote(comment)}, this, a, indent, level);
}

const char Stmt::NodeId = 0;
ACCEPT_IMPL(SuiteStmt, ASTVisitor);
ACCEPT_IMPL(BreakStmt, ASTVisitor);
ACCEPT_IMPL(ContinueStmt, ASTVisitor);
ACCEPT_IMPL(ExprStmt, ASTVisitor);
ACCEPT_IMPL(AssignStmt, ASTVisitor);
ACCEPT_IMPL(DelStmt, ASTVisitor);
ACCEPT_IMPL(PrintStmt, ASTVisitor);
ACCEPT_IMPL(ReturnStmt, ASTVisitor);
ACCEPT_IMPL(YieldStmt, ASTVisitor);
ACCEPT_IMPL(AssertStmt, ASTVisitor);
ACCEPT_IMPL(WhileStmt, ASTVisitor);
ACCEPT_IMPL(ForStmt, ASTVisitor);
ACCEPT_IMPL(IfStmt, ASTVisitor);
ACCEPT_IMPL(MatchStmt, ASTVisitor);
ACCEPT_IMPL(ImportStmt, ASTVisitor);
ACCEPT_IMPL(ExceptStmt, ASTVisitor);
ACCEPT_IMPL(TryStmt, ASTVisitor);
ACCEPT_IMPL(ThrowStmt, ASTVisitor);
ACCEPT_IMPL(GlobalStmt, ASTVisitor);
ACCEPT_IMPL(FunctionStmt, ASTVisitor);
ACCEPT_IMPL(ClassStmt, ASTVisitor);
ACCEPT_IMPL(YieldFromStmt, ASTVisitor);
ACCEPT_IMPL(WithStmt, ASTVisitor);
ACCEPT_IMPL(CustomStmt, ASTVisitor);
ACCEPT_IMPL(DirectiveStmt, ASTVisitor);
ACCEPT_IMPL(AssignMemberStmt, ASTVisitor);
ACCEPT_IMPL(CommentStmt, ASTVisitor);

} // namespace codon::ast

namespace tser {
void operator<<(codon::ast::Stmt *t, BinaryArchive &a) {
  using S = codon::PolymorphicSerializer<BinaryArchive, codon::ast::Stmt>;
  a.save(t != nullptr);
  if (t) {
    auto typ = t->dynamicNodeId();
    auto key = S::_serializers[const_cast<void *>(typ)];
    a.save(key);
    S::save(key, t, a);
  }
}
void operator>>(codon::ast::Stmt *&t, BinaryArchive &a) {
  using S = codon::PolymorphicSerializer<BinaryArchive, codon::ast::Stmt>;
  bool empty = a.load<bool>();
  if (!empty) {
    std::string key = a.load<std::string>();
    S::load(key, t, a);
  } else {
    t = nullptr;
  }
}
} // namespace tser
