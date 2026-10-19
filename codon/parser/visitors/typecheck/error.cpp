// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast {

using namespace types;
using namespace matcher;
using namespace error;

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
  auto pyCatchStmt = N<WhileStmt>(N<BoolExpr>(true), N<SuiteStmt>());

  auto done = stmt->getSuite()->isDone();
  for (auto &c : *stmt) {
    TypeContext::Item val = nullptr;
    if (!c->getVar().empty()) {
      if (!c->hasAttribute(Attr::ExprDominated) &&
          !c->hasAttribute(Attr::ExprDominatedUsed)) {
        val = ctx->addVar(getUnmangledName(c->getVar()),
                          ctx->generateCanonicalName(c->getVar()), instantiateUnbound(),
                          getTime());
      } else if (c->hasAttribute(Attr::ExprDominatedUsed)) {
        val = ctx->forceFind(c->getVar());
        c->eraseAttribute(Attr::ExprDominatedUsed);
        c->setAttribute(Attr::ExprDominated);
        c->suite = N<SuiteStmt>(
            N<AssignStmt>(N<IdExpr>(fmt::format("{}{}", getUnmangledName(c->getVar()),
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

      if (!stmt->hasAttribute(Attr::TryPyVar))
        stmt->setAttribute(Attr::TryPyVar, getTemporaryVar("pyexc"));
      auto pyVar = stmt->getAttribute<ir::StringValueAttribute>(Attr::TryPyVar)->value;

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
    } else if (c->getException() &&
               extractClassType(c->getException())->is(StdlibTypes::PyError)) {
      // Transform PyExc exceptions
      if (!stmt->hasAttribute(Attr::TryPyVar))
        stmt->setAttribute(Attr::TryPyVar, getTemporaryVar("pyexc"));
      auto pyVar = stmt->getAttribute<ir::StringValueAttribute>(Attr::TryPyVar)->value;

      if (!c->var.empty()) {
        c->suite = N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(c->var), N<IdExpr>(pyVar)),
                                c->getSuite());
      }
      c->suite = N<SuiteStmt>(c->getSuite(), N<BreakStmt>());
      cast<SuiteStmt>(pyCatchStmt->getSuite())->addStmt(c->getSuite());
    } else {
      // Handle all other exceptions
      c->exc = transformType(c->getException());

      if (c->getException()) {
        auto t = extractClassType(c->getException());
        bool exceptionOK = false;
        for (auto &p : getMRO(t))
          if (p->is(StdlibTypes::BaseException)) {
            exceptionOK = true;
            break;
          }
        if (!exceptionOK)
          E(Error::CATCH_EXCEPTION_TYPE, c->getException(), t->prettyString());
        if (val)
          unify(val->getType(), extractType(c->getException()));
      }
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
    auto pyVar = stmt->getAttribute<ir::StringValueAttribute>(Attr::TryPyVar)->value;
    auto exc = N<IdExpr>(StdlibTypes::PyError);
    cast<SuiteStmt>(pyCatchStmt->getSuite())->addStmt(N<ThrowStmt>(nullptr));
    auto c = N<ExceptStmt>(pyVar, transformType(exc), pyCatchStmt);

    auto val = ctx->addVar(
        pyVar, pyVar, extractType(c->getException())->shared_from_this(), getTime());
    ctx->blockLevel++;
    c->suite = SuiteStmt::wrap(transform(c->getSuite()));
    ctx->blockLevel--;
    done &= (!c->exc || c->exc->isDone()) && c->getSuite()->isDone();
    catches.push_back(c);
  }
  stmt->items = catches;

  if (stmt->getElse()) {
    ctx->blockLevel++;
    stmt->elseSuite = SuiteStmt::wrap(transform(stmt->getElse()));
    ctx->blockLevel--;
    done &= stmt->getElse()->isDone();
  }

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
///   `raise exc` -> ```raise BaseException.set_header(exc, "fn", "file", line, col)```
void TypecheckVisitor::visit(ThrowStmt *stmt) {
  if (!stmt->expr) {
    stmt->setDone();
    return;
  }

  stmt->expr = transform(stmt->getExpr());
  // Cause staging can wrap the header in a StmtExpr. Recognize it on later passes
  // so neither exception construction nor header initialization is repeated.
  // Internal explicit rethrows also retain the original exception's metadata.
  if (!stmt->hasAttribute(Attr::Internal) &&
      !match(getHeadExpr(stmt->getExpr()),
             M<CallExpr>(M<IdExpr>(getMangledMethod("std.internal.types.error",
                                                    "BaseException", "_set_header")),
                         M_))) {
    auto exception = stmt->getExpr();
    auto cause = stmt->getFrom()
                     ? transform(stmt->getFrom())
                     : transform(N<CallExpr>(N<IdExpr>(StdlibTypes::NoneType)));
    std::vector<Stmt *> setup;
    // `raise Error from cause()` evaluates cause() before constructing Error().
    // Save raw operands in source order before normalizing classes to instances;
    // in particular, a failing cause expression must not run Error's constructor.
    if (isTypeExpr(exception) && stmt->getFrom()) {
      auto exceptionName = getTemporaryVar("exception");
      auto causeName = getTemporaryVar("cause");
      setup.push_back(transform(N<AssignStmt>(N<IdExpr>(exceptionName), exception)));
      setup.push_back(transform(N<AssignStmt>(N<IdExpr>(causeName), cause)));
      exception = transform(N<IdExpr>(exceptionName));
      cause = transform(N<IdExpr>(causeName));
    }
    if (isTypeExpr(exception))
      exception = transform(N<CallExpr>(exception));
    if (isTypeExpr(cause))
      cause = transform(N<CallExpr>(cause));
    Expr *header = N<CallExpr>(
        N<IdExpr>(getMangledMethod("std.internal.types.error", "BaseException",
                                   "_set_header")),
        exception, N<StringExpr>(ctx->getBase()->name),
        N<StringExpr>(stmt->getSrcInfo().file), N<IntExpr>(stmt->getSrcInfo().line),
        N<IntExpr>(stmt->getSrcInfo().col), cause);
    stmt->expr = transform(setup.empty() ? header : N<StmtExpr>(setup, header));
  }
  if (stmt->getExpr()->isDone())
    stmt->setDone();
}

/// Save the manager and exit callable independently of the entry-result binding.
/// An exceptional exit can suppress the caught exception; the finally path handles
/// normal completion and return/break/continue without calling exit twice. Entry
/// remains outside this manager's try so a failed entry does not invoke its exit.
/// Legacy None exits use only finally. The static alternatives share their body
/// because only the selected branch is transformed, avoiding recursive duplication.
void TypecheckVisitor::visit(WithStmt *stmt) {
  seqassert(!stmt->empty(), "stmt->items is empty");

  bool isAsync = stmt->isAsync();

  std::vector<Stmt *> content;
  for (auto i = stmt->items.size(); i-- > 0;) {
    auto var = getTemporaryVar("with");
    auto as = N<AssignStmt>(N<IdExpr>(var), (*stmt)[i]);
    auto exitVar = getTemporaryVar("exit");
    auto exceptional = getTemporaryVar("with_exception");
    auto exception = getTemporaryVar("exception");
    auto exitResult = getTemporaryVar("exit_result");
    auto saveExit =
        N<AssignStmt>(N<IdExpr>(exitVar),
                      N<DotExpr>(N<IdExpr>(var), isAsync ? "__aexit__" : "__exit__"));

    Expr *enter =
        N<CallExpr>(N<DotExpr>(N<IdExpr>(var), isAsync ? "__aenter__" : "__enter__"));
    if (isAsync)
      enter = N<AwaitExpr>(enter);
    auto callExit = [&](Expr *exceptionType, Expr *exceptionValue) -> Expr * {
      Expr *call = N<CallExpr>(
          N<IdExpr>(getMangledMethod("std.internal.types.error", "BaseException",
                                     isAsync ? "_context_aexit" : "_context_exit")),
          N<IdExpr>(exitVar), exceptionType, exceptionValue);
      return isAsync ? N<AwaitExpr>(call) : call;
    };
    auto body = !content.empty() ? N<SuiteStmt>(content) : clone(stmt->getSuite());
    Stmt *entry = N<ExprStmt>(enter);
    if (!stmt->vars[i].empty()) {
      auto entered = getTemporaryVar("entered");
      entry = N<AssignStmt>(N<IdExpr>(entered), enter);
      body = N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(stmt->vars[i]), N<IdExpr>(entered),
                                        nullptr,
                                        (*stmt)[i]->hasAttribute(Attr::ExprDominated)
                                            ? AssignStmt::UpdateMode::Update
                                            : AssignStmt::UpdateMode::Assign),
                          body);
    }
    // An awaited exit can suspend outside the original native catch frame. Rethrow
    // the captured object rather than relying on the thread's active exception.
    auto rethrow = N<ThrowStmt>(N<IdExpr>(exception));
    rethrow->setAttribute(Attr::Internal);
    auto handler = N<ExceptStmt>(
        exception, N<IdExpr>(StdlibTypes::BaseException),
        N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(exceptional), N<BoolExpr>(true), nullptr,
                                   AssignStmt::UpdateMode::Update),
                     N<AssignStmt>(N<IdExpr>(exitResult),
                                   callExit(N<CallExpr>(N<IdExpr>(getMangledFunc(
                                                            "std.internal.types.rtti",
                                                            "get_typeinfo")),
                                                        N<IdExpr>(exception)),
                                            N<IdExpr>(exception))),
                     N<IfStmt>(N<UnaryExpr>("!", N<IdExpr>(exitResult)), rethrow)));
    auto fullCleanup = N<SuiteStmt>(
        N<AssignStmt>(N<IdExpr>(exceptional), N<BoolExpr>(false)),
        N<TryStmt>(body, std::vector<ExceptStmt *>{handler}, nullptr,
                   N<SuiteStmt>(N<IfStmt>(
                       N<UnaryExpr>("!", N<IdExpr>(exceptional)),
                       N<ExprStmt>(
                           callExit(N<CallExpr>(N<IdExpr>(StdlibTypes::NoneType)),
                                    N<CallExpr>(N<IdExpr>(StdlibTypes::NoneType))))))));
    Expr *legacyExit = N<CallExpr>(N<IdExpr>(exitVar));
    if (isAsync)
      legacyExit = N<AwaitExpr>(legacyExit);
    auto legacyCleanup = N<TryStmt>(body, std::vector<ExceptStmt *>{}, nullptr,
                                    N<SuiteStmt>(N<ExprStmt>(legacyExit)));
    auto legacyNone = N<CallExpr>(
        N<IdExpr>(getMangledMethod("std.internal.types.error", "BaseException",
                                   "_context_exit_is_legacy_none")),
        N<IdExpr>(exitVar), N<BoolExpr>(isAsync));
    content = std::vector<Stmt *>{as, saveExit, entry,
                                  N<IfStmt>(legacyNone, legacyCleanup, fullCleanup)};
  }
  resultStmt = transform(N<SuiteStmt>(content));
}

} // namespace codon::ast
