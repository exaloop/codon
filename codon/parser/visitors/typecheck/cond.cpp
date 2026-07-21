// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/typecheck.h"
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Call `ready` and `notReady` depending whether the provided static expression can be
/// evaluated or not.
template <typename TT, typename TF>
auto evaluateStaticCondition(Expr *cond, TT ready, TF notReady) {
  seqassertn(cond->getType()->getStaticKind(), "not a static condition");
  if (cond->getType()->canRealize()) {
    bool isTrue = false;
    if (auto as = cond->getType()->getStrStatic())
      isTrue = !as->value.empty();
    else if (auto ai = cond->getType()->getIntStatic())
      isTrue = ai->value;
    else if (auto ab = cond->getType()->getBoolStatic())
      isTrue = ab->value;
    return ready(isTrue);
  } else {
    return notReady();
  }
}

/// Only allowed in @c MatchStmt
void TypecheckVisitor::visit(RangeExpr *expr) {
  E(Error::UNEXPECTED_TYPE, expr, "range");
}

/// Typecheck if expressions. Evaluate static if blocks if possible.
/// Also wrap conditional expressions to match each other. See @c wrapExpr for more
/// details.
void TypecheckVisitor::visit(IfExpr *expr) {
  expr->getCond()->setExpectedType(getStdLibType(StdlibTypes::Bool));
  expr->cond = transform(expr->getCond());

  // Static if evaluation
  if (expr->getCond()->getType()->getStaticKind()) {
    resultExpr = evaluateStaticCondition(
        expr->getCond(),
        [&](bool isTrue) {
          LOG_TYPECHECK("[static::cond] {}: {}", getSrcInfo(), isTrue);
          Expr *s = isTrue ? expr->getIf() : expr->getElse();
          if (hasSideEffect(expr->getCond()))
            s = N<StmtExpr>(N<ExprStmt>(expr->getCond()), s);
          s = transform(s);
          return s;
        },
        [&]() -> Expr * { return nullptr; });
    if (resultExpr)
      unify(expr->getType(), resultExpr->getType());
    else if (expr->getType()->getUnbound())
      expr->getType()->getUnbound()->staticKind = LiteralKind::Int; // determine later!
    return;
  }

  expr->ifexpr = transform(expr->getIf());
  expr->elsexpr = transform(expr->getElse());

  wrapExpr(&expr->cond, getStdLibType(StdlibTypes::Bool));
  // Add wrappers and unify both sides
  if (expr->getIf()->getType()->getStatic())
    expr->getIf()->setType(
        expr->getIf()->getType()->getStatic()->getNonStaticType()->shared_from_this());
  if (expr->getElse()->getType()->getStatic())
    expr->getElse()->setType(expr->getElse()
                                 ->getType()
                                 ->getStatic()
                                 ->getNonStaticType()
                                 ->shared_from_this());
  wrapExpr(&expr->elsexpr, expr->getIf()->getType(), nullptr, /*allowUnwrap*/ false);
  wrapExpr(&expr->ifexpr, expr->getElse()->getType(), nullptr, /*allowUnwrap*/ false);

  // Types not compatible! Check if an union can be made
  if (expr->getIf()->getType()->unify(expr->getElse()->getType(), nullptr) < 0 &&
      expr->getExpectedType() && expr->getExpectedType()->is(StdlibTypes::Union)) {
    if (!expr->getIf()->getType()->canRealize() ||
        !expr->getElse()->getType()->canRealize())
      return;
    auto T = N<InstantiateExpr>(
        N<IdExpr>(StdlibTypes::Union),
        std::vector<Expr *>{N<IdExpr>(expr->getIf()->getType()->realizedName()),
                            N<IdExpr>(expr->getElse()->getType()->realizedName())});
    expr->ifexpr = transform(N<CallExpr>(T, expr->getIf()));
    expr->elsexpr = transform(N<CallExpr>(clone(T), expr->getElse()));
  }

  unify(expr->getType(), expr->getIf()->getType());
  unify(expr->getType(), expr->getElse()->getType());
  if (expr->getCond()->isDone() && expr->getIf()->isDone() && expr->getElse()->isDone())
    expr->setDone();
}

/// Typecheck if statements. Evaluate static if blocks if possible.
/// See @c wrapExpr for more details.
void TypecheckVisitor::visit(IfStmt *stmt) {
  stmt->getCond()->setExpectedType(getStdLibType(StdlibTypes::Bool));
  stmt->cond = transform(stmt->getCond());

  if (auto ci = cast<CallExpr>(stmt->getCond())) {
    if (auto ei = cast<IdExpr>(ci->getExpr());
        ei && ei->getType() && ei->getType()->getFunc() &&
        (startswith(ei->getType()->getFunc()->getFuncName(),
                    getMangledMethod("", "RTTIType", "_isinstance")) ||
         startswith(ei->getType()->getFunc()->getFuncName(),
                    getMangledMethod("", "Any", "_isinstance")))) {
      if (auto arg = cast<IdExpr>((*ci)[0])) {
        // isinstance(a, T) ->
        // { if (c := isinstance(a, T)): i = getinstance(a, T)) } ; c
        std::string tmpName =
            ctx->generateCanonicalName(getUnmangledName(arg->getValue()));
        std::string condName = getTemporaryVar("cond");
        stmt->getIf()->setAttribute(
            Attr::LocalRenames, std::make_unique<ir::KeyValueAttribute>(
                                    std::unordered_map<std::string, std::string>{
                                        {getUnmangledName(arg->getValue()), tmpName}}));
        auto s = N<SuiteStmt>(
            N<AssignStmt>(N<IdExpr>(condName), stmt->getCond()),
            N<IfStmt>(N<IdExpr>(condName),
                      N<SuiteStmt>(
                          N<AssignStmt>(
                              N<IdExpr>(tmpName),
                              N<CallExpr>(N<IdExpr>(replace(
                                              ei->getType()->getFunc()->getFuncName(),
                                              "_isinstance", "_getinstance")),
                                          (*ci)[0], (*ci)[1])),
                          stmt->getIf()),
                      stmt->getElse()));
        resultStmt = transform(s);
        return;
      }
    }
  }

  // Static if evaluation
  if (stmt->getCond()->getType()->getStaticKind()) {
    resultStmt = evaluateStaticCondition(
        stmt->getCond(),
        [&](bool isTrue) {
          LOG_TYPECHECK("[static::cond] {}: {}", getSrcInfo(), isTrue);
          Stmt *s = isTrue ? stmt->getIf() : stmt->getElse();
          if (hasSideEffect(stmt->getCond()))
            s = N<SuiteStmt>(N<ExprStmt>(stmt->getCond()), s);
          s = transform(s);
          return s ? s : transform(N<SuiteStmt>());
        },
        [&]() -> Stmt * { return nullptr; });
    return;
  }

  wrapExpr(&stmt->cond, getStdLibType(StdlibTypes::Bool));
  ctx->blockLevel++;
  stmt->ifSuite = SuiteStmt::wrap(transform(stmt->getIf()));
  stmt->elseSuite = SuiteStmt::wrap(transform(stmt->getElse()));
  ctx->blockLevel--;

  if (stmt->cond->isDone() && (!stmt->getIf() || stmt->getIf()->isDone()) &&
      (!stmt->getElse() || stmt->getElse()->isDone()))
    stmt->setDone();
}

/// Simplify match statement by transforming it into a series of conditional statements.
/// @example
///   ```match e:
///        case pattern1: ...
///        case pattern2 if guard: ...
///        ...``` ->
///   ```_match = e
///      while True:  # used to simulate goto statement with break
///        [pattern1 transformation]: (...; break)
///        [pattern2 transformation]: if guard: (...; break)
///        ...
///        break  # exit the loop no matter what```
/// The first pattern that matches the given expression will be used; other patterns
/// will not be used (i.e., there is no fall-through). See @c transformPattern for
/// pattern transformations
void TypecheckVisitor::visit(MatchStmt *stmt) {
  auto var = getTemporaryVar("match");
  auto result = N<SuiteStmt>();
  result->addStmt(transform(N<AssignStmt>(N<IdExpr>(var), clone(stmt->getExpr()))));
  for (auto &c : *stmt) {
    Stmt *suite = N<SuiteStmt>(c.getSuite(), N<BreakStmt>());
    if (c.getGuard())
      suite = N<IfStmt>(c.getGuard(), suite);
    result->addStmt(transformPattern(N<IdExpr>(var), c.getPattern(), suite));
  }
  // Make sure to break even if there is no case _ to prevent infinite loop
  result->addStmt(N<BreakStmt>());
  resultStmt = transform(N<WhileStmt>(N<BoolExpr>(true), result));
}

/// Transform a match pattern into a series of if statements.
/// @example
///   `case True`          -> `if isinstance(var, bool): if var == True`
///   `case 1`             -> `if isinstance(var, "int"): if var == 1`
///   `case 1...3`         -> ```if isinstance(var, "int"):
///                                if var >= 1: if var <= 3```
///   `case (1, pat)`      -> ```if isinstance(var, "Tuple"): if static.len(var) == 2:
///                                 if match(var[0], 1): if match(var[1], pat)```
///   `case [1, ..., pat]` -> ```if isinstance(var, "List"): if len(var) >= 2:
///                                 if match(var[0], 1): if match(var[-1], pat)```
///   `case 1 or pat`      -> `if match(var, 1): if match(var, pat)`
///                           (note: pattern suite is cloned for each `or`)
///   `case (x := pat)`    -> `(x := var; if match(var, pat))`
///   `case x`             -> `(x := var)`
///                           (only when `x` is not '_')
///   `case expr`          -> `if hasattr(typeof(var), "__match__"): if
///   var.__match__(foo())`
///                           (any expression that does not fit above patterns)
Stmt *TypecheckVisitor::transformPattern(Expr *var, Expr *pattern, Stmt *suite) {
  // Convenience function to generate `isinstance(e, typ)` calls
  auto isinstance = [&](Expr *e, const std::string &typ) -> Expr * {
    return N<CallExpr>(N<IdExpr>("isinstance"), clone(e), N<IdExpr>(typ));
  };
  // Convenience function to find the index of an ellipsis within a list pattern
  auto findEllipsis = [&](const std::vector<Expr *> &items) {
    size_t i = items.size();
    for (auto it = 0; it < items.size(); it++)
      if (cast<EllipsisExpr>(items[it])) {
        if (i != items.size())
          E(Error::MATCH_MULTI_ELLIPSIS, items[it], "multiple ellipses in pattern");
        i = it;
      }
    return i;
  };

  // See the above examples for transformation details
  if (cast<IntExpr>(pattern) || cast<BoolExpr>(pattern)) {
    // Bool and int patterns
    return N<IfStmt>(
        isinstance(var, cast<BoolExpr>(pattern) ? StdlibTypes::Bool : "int"),
        N<IfStmt>(N<BinaryExpr>(var, "==", pattern), suite));
  } else if (auto er = cast<RangeExpr>(pattern)) {
    // Range pattern
    return N<IfStmt>(
        isinstance(var, "int"),
        N<IfStmt>(N<BinaryExpr>(var, ">=", er->start),
                  N<IfStmt>(N<BinaryExpr>(clone(var), "<=", er->stop), suite)));
  } else if (auto et = cast<TupleExpr>(pattern)) {
    // Tuple pattern
    for (auto it = et->items.size(); it-- > 0;) {
      suite =
          transformPattern(N<IndexExpr>(clone(var), N<IntExpr>(it)), (*et)[it], suite);
    }
    return N<IfStmt>(
        isinstance(var, StdlibTypes::Tuple),
        N<IfStmt>(N<BinaryExpr>(
                      N<CallExpr>(
                          N<IdExpr>(getMangledFunc("std.internal.static", "len")), var),
                      "==", N<IntExpr>(et->size())),
                  suite));
  } else if (auto el = cast<ListExpr>(pattern)) {
    // List pattern
    size_t ellipsis = findEllipsis(el->items), sz = el->size();
    std::string op;
    if (ellipsis == el->size()) {
      op = "==";
    } else {
      op = ">=", sz -= 1;
    }
    for (auto it = el->size(); it-- > ellipsis + 1;) {
      suite = transformPattern(N<IndexExpr>(clone(var), N<IntExpr>(it - el->size())),
                               (*el)[it], suite);
    }
    for (auto it = ellipsis; it-- > 0;) {
      suite =
          transformPattern(N<IndexExpr>(clone(var), N<IntExpr>(it)), (*el)[it], suite);
    }
    return N<IfStmt>(
        isinstance(var, "List"),
        N<IfStmt>(N<BinaryExpr>(N<CallExpr>(N<IdExpr>("len"), var), op, N<IntExpr>(sz)),
                  suite));
  } else if (auto eb = cast<BinaryExpr>(pattern)) {
    // Or pattern
    if (eb->op == "|" || eb->op == "||") {
      return N<SuiteStmt>(transformPattern(clone(var), eb->lexpr, clone(suite)),
                          transformPattern(var, eb->rexpr, suite));
    }
  } else if (auto ei = cast<IdExpr>(pattern)) {
    // Wildcard pattern
    if (ei->value != "_") {
      return N<SuiteStmt>(N<AssignStmt>(pattern, var), suite);
    } else {
      return suite;
    }
  } else if (auto ea = cast<AssignExpr>(pattern)) {
    // Bound pattern
    seqassert(cast<IdExpr>(ea->getVar()),
              "only simple assignment expressions are supported");
    return N<SuiteStmt>(N<AssignStmt>(ea->getVar(), clone(var)),
                        transformPattern(var, ea->getExpr(), suite));
  }
  pattern = transform(pattern); // transform to check for pattern errors
  if (cast<EllipsisExpr>(pattern))
    pattern = N<CallExpr>(N<IdExpr>("ellipsis"));
  // Fallback (`__match__`) pattern
  auto p = N<IfStmt>(
      N<CallExpr>(N<IdExpr>("hasattr"), clone(var), N<StringExpr>("__match__"),
                  clone(pattern)),
      N<IfStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__match__"), clone(pattern)),
                clone(suite)),
      N<IfStmt>(N<CallExpr>(N<IdExpr>("isinstance"),
                            N<CallExpr>(N<IdExpr>(StdlibTypes::Type), clone(var)),
                            N<CallExpr>(N<IdExpr>(StdlibTypes::Type), clone(pattern))),
                N<IfStmt>(N<BinaryExpr>(var, "==", pattern), suite)));
  return p;
}

} // namespace codon::ast
