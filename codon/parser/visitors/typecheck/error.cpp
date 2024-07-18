// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

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
  // TODO: static can-compile check
  // if (stmt->catches.size() == 1 && stmt->catches[0].var.empty() &&
  //     stmt->catches[0].exc->isId("std.internal.types.error.StaticCompileError")) {
  //   /// TODO: this is right now _very_ dangerous; inferred types here will remain!
  //   bool compiled = true;
  //   try {
  //     auto nctx = std::make_shared<TypeContext>(*ctx);
  //     TypecheckVisitor(nctx).transform(clone(stmt->suite));
  //   } catch (const exc::ParserException &exc) {
  //     compiled = false;
  //   }
  //   resultStmt = compiled ? transform(stmt->suite) :
  //   transform(stmt->catches[0].suite); LOG("testing!! {} {}", getSrcInfo(),
  //   compiled); return;
  // }

  ctx->blockLevel++;
  transform(stmt->suite);
  ctx->blockLevel--;

  std::vector<TryStmt::Catch> catches;
  auto pyVar = ctx->cache->getTemporaryVar("pyexc");
  auto pyCatchStmt = N<WhileStmt>(N<BoolExpr>(true), N<SuiteStmt>());

  auto done = stmt->suite->isDone();
  for (auto &c : stmt->catches) {
    transform(c.exc);
    if (c.exc && c.exc->type->is("pyobj")) {
      // Transform python.Error exceptions
      if (!c.var.empty()) {
        c.suite = N<SuiteStmt>(
            N<AssignStmt>(N<IdExpr>(c.var), N<DotExpr>(N<IdExpr>(pyVar), "pytype")),
            c.suite);
      }
      c.suite =
          N<IfStmt>(N<CallExpr>(N<IdExpr>("isinstance"),
                                N<DotExpr>(N<IdExpr>(pyVar), "pytype"), clone(c.exc)),
                    N<SuiteStmt>(c.suite, N<BreakStmt>()), nullptr);
      pyCatchStmt->suite->getSuite()->stmts.push_back(c.suite);
    } else if (c.exc && c.exc->type->is("std.internal.types.error.PyError")) {
      // Transform PyExc exceptions
      if (!c.var.empty()) {
        c.suite =
            N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(c.var), N<IdExpr>(pyVar)), c.suite);
      }
      c.suite = N<SuiteStmt>(c.suite, N<BreakStmt>());
      pyCatchStmt->suite->getSuite()->stmts.push_back(c.suite);
    } else {
      // Handle all other exceptions
      transformType(c.exc);
      if (!c.var.empty()) {
        // Handle dominated except bindings
        auto changed = in(ctx->cache->replacements, c.var);
        while (auto s = in(ctx->cache->replacements, c.var))
          c.var = s->first, changed = s;
        if (changed && changed->second) {
          auto update =
              N<AssignStmt>(N<IdExpr>(format("{}.__used__", c.var)), N<BoolExpr>(true));
          update->setUpdate();
          c.suite = N<SuiteStmt>(update, c.suite);
        }
        if (changed)
          c.exc->setAttr(ExprAttr::Dominated);
        auto val = ctx->find(c.var);
        if (!changed)
          val = ctx->add(TypecheckItem::Var, c.var, c.exc->getType());
        unify(val->type, c.exc->getType());
      }
      ctx->blockLevel++;
      transform(c.suite);
      ctx->blockLevel--;
      done &= (!c.exc || c.exc->isDone()) && c.suite->isDone();
      catches.push_back(c);
    }
  }
  if (!pyCatchStmt->suite->getSuite()->stmts.empty()) {
    // Process PyError catches
    auto exc = NT<IdExpr>("std.internal.types.error.PyError");
    pyCatchStmt->suite->getSuite()->stmts.push_back(N<ThrowStmt>(nullptr));
    TryStmt::Catch c{pyVar, transformType(exc), pyCatchStmt};

    auto val = ctx->add(TypecheckItem::Var, pyVar, c.exc->getType());
    unify(val->type, c.exc->getType());
    ctx->blockLevel++;
    transform(c.suite);
    ctx->blockLevel--;
    done &= (!c.exc || c.exc->isDone()) && c.suite->isDone();
    catches.push_back(c);
  }
  stmt->catches = catches;
  if (stmt->finally) {
    ctx->blockLevel++;
    transform(stmt->finally);
    ctx->blockLevel--;
    done &= stmt->finally->isDone();
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

  transform(stmt->expr);

  if (!(stmt->expr->getCall() && stmt->expr->getCall()->expr->getId() &&
        startswith(stmt->expr->getCall()->expr->getId()->value,
                   "__internal__.set_header:0"))) {
    stmt->expr = transform(N<CallExpr>(
        N<IdExpr>("__internal__.set_header:0"), stmt->expr,
        N<StringExpr>(ctx->getRealizationBase()->name),
        N<StringExpr>(stmt->getSrcInfo().file), N<IntExpr>(stmt->getSrcInfo().line),
        N<IntExpr>(stmt->getSrcInfo().col)));
  }
  if (stmt->expr->isDone())
    stmt->setDone();
}

} // namespace codon::ast
