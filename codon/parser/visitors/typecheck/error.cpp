// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;
using namespace matcher;

/// Transform asserts.
/// @example
///   `assert foo()` ->
///   `if not foo(): raise __internal__.seq_assert([file], [line], "")`
///   `assert foo(), msg` ->
///   `if not foo(): raise __internal__.seq_assert([file], [line], str(msg))`
/// Use `seq_assert_test` instead of `seq_assert` and do not raise anything during unit
/// testing (i.e., when the enclosing function is marked with `@test`).
void TypecheckVisitor::visit(AssertStmt *stmt) {
  Expr *msg = N<StringExpr>("");
  if (stmt->getMessage())
    msg = N<CallExpr>(N<IdExpr>("str"), stmt->getMessage());
  auto test = ctx->inFunction() && ctx->getBase()->func->hasAttribute(Attr::Test);
  auto ex = N<CallExpr>(
      N<DotExpr>(N<IdExpr>("__internal__"), test ? "seq_assert_test" : "seq_assert"),
      N<StringExpr>(stmt->getSrcInfo().file), N<IntExpr>(stmt->getSrcInfo().line), msg);
  auto cond = N<UnaryExpr>("!", stmt->getExpr());
  if (test) {
    resultStmt = transform(N<IfStmt>(cond, N<ExprStmt>(ex)));
  } else {
    resultStmt = transform(N<IfStmt>(cond, N<ThrowStmt>(ex)));
  }
}

/// Typecheck try-except statements. Handle Python exceptions separately.
/// @example
///   ```try: ...
///      except python.Error as e: ...
///      except PyExc as f: ...
///      except ValueError as g: ...
///   ``` -> ```
///      try: ...
///      except ValueError as g: ...                   # ValueError
///      except PyExc as exc:
///        while True:
///          if isinstance(exc.pytype, python.Error):  # python.Error
///            e = exc.pytype; ...; break
///          f = exc; ...; break                       # PyExc
///          raise```
void TypecheckVisitor::visit(TryStmt *stmt) {
  ctx->blockLevel++;
  stmt->suite = SuiteStmt::wrap(transform(stmt->getSuite()));
  ctx->blockLevel--;

  std::vector<ExceptStmt *> catches;
  auto pyVar = getTemporaryVar("pyexc");
  auto pyCatchStmt = N<WhileStmt>(N<BoolExpr>(true), N<SuiteStmt>());

  auto done = stmt->getSuite()->isDone();
  for (auto &c : *stmt) {
    TypeContext::Item val = nullptr;
    if (!c->getVar().empty()) {
      if (!c->hasAttribute(Attr::ExprDominated) &&
          !c->hasAttribute(Attr::ExprDominatedUsed)) {
        val = ctx->addVar(c->getVar(), ctx->generateCanonicalName(c->getVar()),
                          instantiateUnbound());
        val->time = getTime();
      } else if (c->hasAttribute(Attr::ExprDominatedUsed)) {
        val = ctx->forceFind(c->getVar());
        c->eraseAttribute(Attr::ExprDominatedUsed);
        c->setAttribute(Attr::ExprDominated);
        c->suite = N<SuiteStmt>(
            N<AssignStmt>(N<IdExpr>(format("{}{}", getUnmangledName(c->getVar()),
                                           VAR_USED_SUFFIX)),
                          N<BoolExpr>(true), nullptr, AssignStmt::UpdateMode::Update),
            c->getSuite());
      } else {
        val = ctx->forceFind(c->getVar());
      }
      c->var = val->canonicalName;
    }
    c->exc = transform(c->getException());
    if (c->getException() && extractClassType(c->getException())->is("pyobj")) {
      // Transform python.Error exceptions
      if (!c->var.empty()) {
        c->suite = N<SuiteStmt>(
            N<AssignStmt>(N<IdExpr>(c->var), N<DotExpr>(N<IdExpr>(pyVar), "pytype")),
            c->getSuite());
      }
      c->suite = SuiteStmt::wrap(N<IfStmt>(
          N<CallExpr>(N<IdExpr>("isinstance"), N<DotExpr>(N<IdExpr>(pyVar), "pytype"),
                      c->getException()),
          N<SuiteStmt>(c->getSuite(), N<BreakStmt>()), nullptr));
      cast<SuiteStmt>(pyCatchStmt->getSuite())->addStmt(c->getSuite());
    } else if (c->getException() && extractClassType(c->getException())
                                        ->is("std.internal.python.PyError.0")) {
      // Transform PyExc exceptions
      if (!c->var.empty()) {
        c->suite = N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(c->var), N<IdExpr>(pyVar)),
                                c->getSuite());
      }
      c->suite = N<SuiteStmt>(c->getSuite(), N<BreakStmt>());
      cast<SuiteStmt>(pyCatchStmt->getSuite())->addStmt(c->getSuite());
    } else {
      // Handle all other exceptions
      c->exc = transformType(c->getException());
      if (val)
        unify(val->getType(), extractType(c->getException()));
      ctx->blockLevel++;
      c->suite = SuiteStmt::wrap(transform(c->getSuite()));
      ctx->blockLevel--;
      done &= (!c->getException() || c->getException()->isDone()) &&
              c->getSuite()->isDone();
      catches.push_back(c);
    }
  }
  if (!cast<SuiteStmt>(pyCatchStmt->getSuite())->empty()) {
    // Process PyError catches
    auto exc = N<IdExpr>("std.internal.python.PyError.0");
    cast<SuiteStmt>(pyCatchStmt->getSuite())->addStmt(N<ThrowStmt>(nullptr));
    auto c = N<ExceptStmt>(pyVar, transformType(exc), pyCatchStmt);

    auto val =
        ctx->addVar(pyVar, pyVar, extractType(c->getException())->shared_from_this());
    val->time = getTime();
    ctx->blockLevel++;
    c->suite = SuiteStmt::wrap(transform(c->getSuite()));
    ctx->blockLevel--;
    done &= (!c->exc || c->exc->isDone()) && c->getSuite()->isDone();
    catches.push_back(c);
  }
  stmt->items = catches;
  if (stmt->getFinally()) {
    ctx->blockLevel++;
    stmt->finally = SuiteStmt::wrap(transform(stmt->getFinally()));
    ctx->blockLevel--;
    done &= stmt->getFinally()->isDone();
  }

  if (done)
    stmt->setDone();
}

/// Transform `raise` statements.
/// @example
///   `raise exc` -> ```raise __internal__.set_header(exc, "fn", "file", line, col)```
void TypecheckVisitor::visit(ThrowStmt *stmt) {
  if (!stmt->expr) {
    stmt->setDone();
    return;
  }

  stmt->expr = transform(stmt->getExpr());
  if (!match(stmt->getExpr(),
             M<CallExpr>(M<IdExpr>("__internal__.set_header:0"), M_))) {
    stmt->expr = transform(N<CallExpr>(
        N<IdExpr>("__internal__.set_header:0"), stmt->getExpr(),
        N<StringExpr>(ctx->getBase()->name), N<StringExpr>(stmt->getSrcInfo().file),
        N<IntExpr>(stmt->getSrcInfo().line), N<IntExpr>(stmt->getSrcInfo().col),
        stmt->getFrom()
            ? (Expr *)N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_super"),
                                  stmt->getFrom(),
                                  N<IdExpr>("std.internal.types.error.BaseException.0"))
            : N<CallExpr>(N<IdExpr>("NoneType"))));
  }
  if (stmt->getExpr()->isDone())
    stmt->setDone();
}

/// Transform with statements.
/// @example
///   `with foo(), bar() as a: ...` ->
///   ```tmp = foo()
///      tmp.__enter__()
///      try:
///        a = bar()
///        a.__enter__()
///        try:
///          ...
///        finally:
///          a.__exit__()
///      finally:
///        tmp.__exit__()```
void TypecheckVisitor::visit(WithStmt *stmt) {
  seqassert(!stmt->empty(), "stmt->items is empty");
  std::vector<Stmt *> content;
  for (auto i = stmt->items.size(); i-- > 0;) {
    std::string var = stmt->vars[i].empty() ? getTemporaryVar("with") : stmt->vars[i];
    auto as = N<AssignStmt>(N<IdExpr>(var), (*stmt)[i], nullptr,
                            (*stmt)[i]->hasAttribute(Attr::ExprDominated)
                                ? AssignStmt::UpdateMode::Update
                                : AssignStmt::UpdateMode::Assign);
    content = std::vector<Stmt *>{
        as, N<ExprStmt>(N<CallExpr>(N<DotExpr>(N<IdExpr>(var), "__enter__"))),
        N<TryStmt>(!content.empty() ? N<SuiteStmt>(content) : clone(stmt->getSuite()),
                   std::vector<ExceptStmt *>{},
                   N<SuiteStmt>(N<ExprStmt>(
                       N<CallExpr>(N<DotExpr>(N<IdExpr>(var), "__exit__")))))};
  }
  resultStmt = transform(N<SuiteStmt>(content));
}

} // namespace codon::ast
