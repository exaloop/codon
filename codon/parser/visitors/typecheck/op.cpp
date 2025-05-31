// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;
using namespace matcher;

/// Replace unary operators with the appropriate magic calls.
/// Also evaluate static expressions. See @c evaluateStaticUnary for details.
void TypecheckVisitor::visit(UnaryExpr *expr) {
  expr->expr = transform(expr->getExpr());

  if (cast<IntExpr>(expr->getExpr()) && expr->getOp() == "-") {
    // Special case: make - INT(val) same as INT(-val) to simplify IR and everything
    resultExpr = transform(N<IntExpr>(-cast<IntExpr>(expr->getExpr())->getValue()));
    return;
  }

  StaticType *staticType = nullptr;
  static std::unordered_map<int, std::unordered_set<std::string>> staticOps = {
      {1, {"-", "+", "!", "~"}}, {2, {"!"}}, {3, {"!"}}};
  // Handle static expressions
  if (auto s = expr->getExpr()->getType()->isStaticType()) {
    if (in(staticOps[s], expr->getOp())) {
      if ((resultExpr = evaluateStaticUnary(expr))) {
        staticType = resultExpr->getType()->getStatic();
      } else {
        return;
      }
    }
  } else if (isUnbound(expr->getExpr())) {
    return;
  }

  if (expr->getOp() == "!") {
    // `not expr` -> `expr.__bool__().__invert__()`
    resultExpr = transform(N<CallExpr>(N<DotExpr>(
        N<CallExpr>(N<DotExpr>(expr->getExpr(), "__bool__")), "__invert__")));
  } else {
    std::string magic;
    if (expr->getOp() == "~")
      magic = "invert";
    else if (expr->getOp() == "+")
      magic = "pos";
    else if (expr->getOp() == "-")
      magic = "neg";
    else
      seqassert(false, "invalid unary operator '{}'", expr->getOp());
    resultExpr =
        transform(N<CallExpr>(N<DotExpr>(expr->getExpr(), format("__{}__", magic))));
  }

  if (staticType)
    resultExpr->setType(staticType->shared_from_this());
}

/// Replace binary operators with the appropriate magic calls.
/// See @c transformBinarySimple , @c transformBinaryIs , @c transformBinaryMagic and
/// @c transformBinaryInplaceMagic for details.
/// Also evaluate static expressions. See @c evaluateStaticBinary for details.
void TypecheckVisitor::visit(BinaryExpr *expr) {
  expr->lexpr = transform(expr->getLhs(), true);

  // Static short-circuit
  if (expr->getLhs()->getType()->isStaticType() && expr->op == "&&") {
    if (auto tb = expr->getLhs()->getType()->getBoolStatic()) {
      if (!tb->value) {
        if (ctx->expectedType && ctx->expectedType->is("bool"))
          resultExpr = transform(N<BoolExpr>(false));
        else
          resultExpr = expr->getLhs();
        return;
      }
    } else if (auto ts = expr->getLhs()->getType()->getStrStatic()) {
      if (ts->value.empty()) {
        if (ctx->expectedType && ctx->expectedType->is("bool"))
          resultExpr = transform(N<BoolExpr>(false));
        else
          resultExpr = expr->getLhs();
        return;
      }
    } else if (auto ti = expr->getLhs()->getType()->getIntStatic()) {
      if (!ti->value) {
        if (ctx->expectedType && ctx->expectedType->is("bool"))
          resultExpr = transform(N<BoolExpr>(false));
        else
          resultExpr = expr->getLhs();
        return;
      }
    } else {
      expr->getType()->getUnbound()->isStatic = 3;
      return;
    }
  } else if (expr->getLhs()->getType()->isStaticType() && expr->op == "||") {
    if (auto tb = expr->getLhs()->getType()->getBoolStatic()) {
      if (tb->value) {
        if (ctx->expectedType && ctx->expectedType->is("bool"))
          resultExpr = transform(N<BoolExpr>(true));
        else
          resultExpr = expr->getLhs();
        return;
      }
    } else if (auto ts = expr->getLhs()->getType()->getStrStatic()) {
      if (!ts->value.empty()) {
        if (ctx->expectedType && ctx->expectedType->is("bool"))
          resultExpr = transform(N<BoolExpr>(true));
        else
          resultExpr = expr->getLhs();
        return;
      }
    } else if (auto ti = expr->getLhs()->getType()->getIntStatic()) {
      if (ti->value) {
        if (ctx->expectedType && ctx->expectedType->is("bool"))
          resultExpr = transform(N<BoolExpr>(true));
        else
          resultExpr = expr->getLhs();
        return;
      }
    } else {
      expr->getType()->getUnbound()->isStatic = 3;
      return;
    }
  }

  expr->rexpr = transform(expr->getRhs(), true);

  StaticType *staticType = nullptr;
  static std::unordered_map<int, std::unordered_set<std::string>> staticOps = {
      {1,
       {"<", "<=", ">", ">=", "==", "!=", "&&", "||", "+", "-", "*", "//", "%", "&",
        "|", "^", ">>", "<<"}},
      {2, {"==", "!=", "+"}},
      {3, {"<", "<=", ">", ">=", "==", "!=", "&&", "||"}}};
  if (expr->getLhs()->getType()->isStaticType() &&
      expr->getRhs()->getType()->isStaticType()) {
    auto l = expr->getLhs()->getType()->isStaticType();
    auto r = expr->getRhs()->getType()->isStaticType();
    bool isStatic = l == r && in(staticOps[l], expr->getOp());
    if (!isStatic && ((l == 1 && r == 3) || (r == 1 && l == 3)) &&
        in(staticOps[1], expr->getOp()))
      isStatic = true;
    if (isStatic) {
      if ((resultExpr = evaluateStaticBinary(expr)))
        staticType = resultExpr->getType()->getStatic();
      else
        return;
    }
  }

  if (isTypeExpr(expr->getLhs()) && isTypeExpr(expr->getRhs()) &&
      expr->getOp() == "|") {
    // Case: unions
    resultExpr = transform(N<InstantiateExpr>(
        N<IdExpr>("Union"), std::vector<Expr *>{expr->getLhs(), expr->getRhs()}));
  } else if (auto e = transformBinarySimple(expr)) {
    // Case: simple binary expressions
    resultExpr = e;
  } else if (expr->getLhs()->getType()->getUnbound() ||
             (expr->getOp() != "is" && expr->getRhs()->getType()->getUnbound())) {
    // Case: types are unknown, so continue later
    return;
  } else if (expr->getOp() == "is") {
    // Case: is operator
    resultExpr = transformBinaryIs(expr);
  } else {
    if (auto ei = transformBinaryInplaceMagic(expr, false)) {
      // Case: in-place magic methods
      resultExpr = ei;
    } else if (auto em = transformBinaryMagic(expr)) {
      // Case: normal magic methods
      resultExpr = em;
    } else if (expr->getLhs()->getType()->is(TYPE_OPTIONAL)) {
      // Special case: handle optionals if everything else fails.
      // Assumes that optionals have no relevant magics (except for __eq__)
      resultExpr =
          transform(N<BinaryExpr>(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr->getLhs()),
                                  expr->getOp(), expr->getRhs(), expr->isInPlace()));
    } else {
      // Nothing found: report an error
      E(Error::OP_NO_MAGIC, expr, expr->getOp(),
        expr->getLhs()->getType()->prettyString(),
        expr->getRhs()->getType()->prettyString());
    }
  }

  if (staticType)
    resultExpr->setType(staticType->shared_from_this());
}

/// Transform chain binary expression.
/// @example
///   `a <= b <= c` -> `(a <= (chain := b)) and (chain <= c)`
/// The assignment above ensures that all expressions are executed only once.
void TypecheckVisitor::visit(ChainBinaryExpr *expr) {
  seqassert(expr->exprs.size() >= 2, "not enough expressions in ChainBinaryExpr");
  std::vector<Expr *> items;
  std::string prev;
  for (int i = 1; i < expr->exprs.size(); i++) {
    auto l = prev.empty() ? clone(expr->exprs[i - 1].second) : N<IdExpr>(prev);
    prev = ctx->generateCanonicalName("chain");
    auto r =
        (i + 1 == expr->exprs.size())
            ? clone(expr->exprs[i].second)
            : N<StmtExpr>(N<AssignStmt>(N<IdExpr>(prev), clone(expr->exprs[i].second)),
                          N<IdExpr>(prev));
    items.emplace_back(N<BinaryExpr>(l, expr->exprs[i].first, r));
  }

  Expr *final = items.back();
  for (auto i = items.size() - 1; i-- > 0;)
    final = N<BinaryExpr>(items[i], "&&", final);

  auto oldExpectedType = getStdLibType("bool")->shared_from_this();
  std::swap(ctx->expectedType, oldExpectedType);
  resultExpr = transform(final);
  std::swap(ctx->expectedType, oldExpectedType);
}

/// Helper function that locates the pipe ellipsis within a collection of (possibly
/// nested) CallExprs.
/// @return  List of CallExprs and their locations within the parent CallExpr
///          needed to access the ellipsis.
/// @example `foo(bar(1, baz(...)))` returns `[{0, baz}, {1, bar}, {0, foo}]`
std::vector<std::pair<size_t, Expr *>> TypecheckVisitor::findEllipsis(Expr *expr) {
  auto call = cast<CallExpr>(expr);
  if (!call)
    return {};
  size_t ai = 0;
  for (auto &a : *call) {
    if (auto el = cast<EllipsisExpr>(a)) {
      if (el->isPipe())
        return {{ai, expr}};
    } else if (cast<CallExpr>(a)) {
      auto v = findEllipsis(a);
      if (!v.empty()) {
        v.emplace_back(ai, expr);
        return v;
      }
    }
    ai++;
  }
  return {};
}

/// Typecheck pipe expressions.
/// Each stage call `foo(x)` without an ellipsis will be transformed to `foo(..., x)`.
/// Stages that are not in the form of CallExpr will be transformed to it (e.g., `foo`
/// -> `foo(...)`).
/// Special care is taken of stages that can expand to multiple stages (e.g., `a |> foo`
/// might become `a |> unwrap |> foo` to satisfy type constraints.
void TypecheckVisitor::visit(PipeExpr *expr) {
  bool hasGenerator = false;

  // Return T if t is of type `Generator[T]`; otherwise just `type(t)`
  auto getIterableType = [&](Type *t) {
    if (t->is("Generator")) {
      hasGenerator = true;
      return extractClassGeneric(t);
    }
    return t;
  };

  // List of output types
  // (e.g., for `a|>b|>c` it is `[type(a), type(a|>b), type(a|>b|>c)]`).
  // Note: the generator types are completely preserved (i.e., not extracted)
  expr->inTypes.clear();

  // Process the pipeline head
  expr->front().expr = transform(expr->front().expr);
  auto inType = expr->front().expr->getType(); // input type to the next stage
  expr->inTypes.push_back(inType->shared_from_this());
  inType = getIterableType(inType);
  auto done = expr->front().expr->isDone();
  for (size_t pi = 1; pi < expr->size(); pi++) {
    int inTypePos = -1;                   // ellipsis position
    Expr **ec = &((*expr)[pi].expr);      // a pointer so that we can replace it
    while (auto se = cast<StmtExpr>(*ec)) // handle StmtExpr (e.g., in partial calls)
      ec = &(se->expr);

    if (auto call = cast<CallExpr>(*ec)) {
      // Case: a call. Find the position of the pipe ellipsis within it
      for (size_t ia = 0; inTypePos == -1 && ia < call->size(); ia++)
        if (cast<EllipsisExpr>((*call)[ia].value))
          inTypePos = int(ia);
      // No ellipses found? Prepend it as the first argument
      if (inTypePos == -1) {
        call->items.insert(call->items.begin(),
                           {"", N<EllipsisExpr>(EllipsisExpr::PARTIAL)});
        inTypePos = 0;
      }
    } else {
      // Case: not a call. Convert it to a call with a single ellipsis
      (*expr)[pi].expr =
          N<CallExpr>((*expr)[pi].expr, N<EllipsisExpr>(EllipsisExpr::PARTIAL));
      ec = &(*expr)[pi].expr;
      inTypePos = 0;
    }

    // Set the ellipsis type
    auto el = cast<EllipsisExpr>((*cast<CallExpr>(*ec))[inTypePos].value);
    el->mode = EllipsisExpr::PIPE;
    // Don't unify unbound inType yet (it might become a generator that needs to be
    // extracted)
    if (!el->getType())
      el->setType(instantiateUnbound());
    if (inType && !inType->getUnbound())
      unify(el->getType(), inType);

    // Transform the call. Because a transformation might wrap the ellipsis in layers,
    // make sure to extract these layers and move them to the pipeline.
    // Example: `foo(...)` that is transformed to `foo(unwrap(...))` will become
    // `unwrap(...) |> foo(...)`
    *ec = transform(*ec);
    auto layers = findEllipsis(*ec);
    seqassert(!layers.empty(), "can't find the ellipsis");
    if (layers.size() > 1) {
      // Prepend layers
      for (auto &[pos, prepend] : layers) {
        (*cast<CallExpr>(prepend))[pos].value = N<EllipsisExpr>(EllipsisExpr::PIPE);
        expr->items.insert(expr->items.begin() + pi++, {"|>", prepend});
      }
      // Rewind the loop (yes, the current expression will get transformed again)
      /// TODO: avoid reevaluation
      expr->items.erase(expr->items.begin() + pi);
      pi = pi - layers.size() - 1;
      continue;
    }

    if ((*ec)->getType())
      unify((*expr)[pi].expr->getType(), (*ec)->getType());
    (*expr)[pi].expr = *ec;
    inType = (*expr)[pi].expr->getType();
    if (!realize(inType))
      done = false;
    expr->inTypes.push_back(inType->shared_from_this());

    // Do not extract the generator in the last stage of a pipeline
    if (pi + 1 < expr->items.size())
      inType = getIterableType(inType);
  }
  unify(expr->getType(), (hasGenerator ? getStdLibType("NoneType") : inType));
  if (done)
    expr->setDone();
}

/// Transform index expressions.
/// @example
///   `foo[T]`   -> Instantiate(foo, [T]) if `foo` is a type
///   `tup[1]`   -> `tup.item1` if `tup` is tuple
///   `foo[idx]` -> `foo.__getitem__(idx)`
///   expr.itemN or a sub-tuple if index is static (see transformStaticTupleIndex()),
void TypecheckVisitor::visit(IndexExpr *expr) {
  if (match(expr, M<IndexExpr>(M<IdExpr>(MOr("Literal", "Static")),
                               M<IdExpr>(MOr("int", "str", "bool"))))) {
    // Special case: static types.
    auto typ = instantiateUnbound();
    typ->isStatic = getStaticGeneric(expr);
    unify(expr->getType(), typ);
    expr->setDone();
    return;
  } else if (match(expr->expr, M<IdExpr>(MOr("Literal", "Static")))) {
    E(Error::BAD_STATIC_TYPE, expr->getIndex());
  }
  if (match(expr->expr, M<IdExpr>("tuple")))
    cast<IdExpr>(expr->expr)->setValue(TYPE_TUPLE);
  expr->expr = transform(expr->expr, true);

  // IndexExpr[i1, ..., iN] is internally represented as
  // IndexExpr[TupleExpr[i1, ..., iN]] for N > 1
  std::vector<Expr *> items;
  bool isTuple = false;
  if (auto t = cast<TupleExpr>(expr->getIndex())) {
    items = t->items;
    isTuple = true;
  } else {
    items.push_back(expr->getIndex());
  }
  auto origIndex = clone(expr->getIndex());
  for (auto &i : items) {
    if (cast<ListExpr>(i) && isTypeExpr(expr->getExpr())) {
      // Special case: `A[[A, B], C]` -> `A[Tuple[A, B], C]` (e.g., in
      // `Function[...]`)
      i = N<InstantiateExpr>(N<IdExpr>(TYPE_TUPLE), cast<ListExpr>(i)->items);
    }
    i = transform(i, true);
  }
  if (isTypeExpr(expr->getExpr())) {
    resultExpr = transform(N<InstantiateExpr>(expr->getExpr(), items));
    return;
  }

  expr->index = (!isTuple && items.size() == 1) ? items[0] : N<TupleExpr>(items);
  auto cls = expr->getExpr()->getClassType();
  if (!cls) {
    // Wait until the type becomes known
    return;
  }

  // Case: static tuple access
  // Note: needs untransformed origIndex to parse statics nicely
  auto [isStaticTuple, tupleExpr] =
      transformStaticTupleIndex(cls, expr->getExpr(), origIndex);
  if (isStaticTuple) {
    if (tupleExpr)
      resultExpr = tupleExpr;
  } else {
    // Case: normal __getitem__
    resultExpr = transform(
        N<CallExpr>(N<DotExpr>(expr->getExpr(), "__getitem__"), expr->getIndex()));
  }
}

/// Transform an instantiation to canonical realized name.
/// @example
///   Instantiate(foo, [bar]) -> Id("foo[bar]")
void TypecheckVisitor::visit(InstantiateExpr *expr) {
  expr->expr = transformType(expr->getExpr());

  TypePtr typ = nullptr;
  size_t typeParamsSize = expr->size();
  if (extractType(expr->expr)->is(TYPE_TUPLE)) {
    if (!expr->empty()) {
      expr->items.front() = transform(expr->front());
      if (expr->front()->getType()->isStaticType() == 1) {
        auto et = N<InstantiateExpr>(
            N<IdExpr>("Tuple"),
            std::vector<Expr *>(expr->items.begin() + 1, expr->items.end()));
        resultExpr = transform(N<InstantiateExpr>(N<IdExpr>("__NTuple__"),
                                                  std::vector<Expr *>{(*expr)[0], et}));
        return;
      }
    }
    auto t = generateTuple(typeParamsSize);
    typ = instantiateType(t);
  } else {
    typ = instantiateType(expr->getExpr()->getSrcInfo(), extractType(expr->getExpr()));
  }
  seqassert(typ->getClass(), "unknown type: {}", *(expr->getExpr()));

  auto &generics = typ->getClass()->generics;
  bool isUnion = typ->getUnion() != nullptr;
  if (!isUnion && typeParamsSize != generics.size())
    E(Error::GENERICS_MISMATCH, expr, getUnmangledName(typ->getClass()->name),
      generics.size(), typeParamsSize);

  if (isId(expr->getExpr(), TRAIT_CALLABLE)) {
    // Case: CallableTrait[...] trait instantiation

    // CallableTrait error checking.
    std::vector<TypePtr> types;
    for (auto &typeParam : *expr) {
      typeParam = transformType(typeParam);
      if (typeParam->getType()->isStaticType())
        E(Error::INST_CALLABLE_STATIC, typeParam);
      types.push_back(extractType(typeParam)->shared_from_this());
    }
    auto typ = instantiateUnbound();
    // Set up the CallableTrait
    typ->getLink()->trait = std::make_shared<CallableTrait>(ctx->cache, types);
    unify(expr->getType(), instantiateTypeVar(typ.get()));
  } else if (isId(expr->getExpr(), TRAIT_TYPE)) {
    // Case: TypeTrait[...] trait instantiation
    (*expr)[0] = transformType((*expr)[0]);
    auto typ = instantiateUnbound();
    typ->getLink()->trait =
        std::make_shared<TypeTrait>(extractType(expr->front())->shared_from_this());
    unify(expr->getType(), typ);
  } else {
    for (size_t i = 0; i < expr->size(); i++) {
      (*expr)[i] = transformType((*expr)[i]);
      auto t = instantiateType((*expr)[i]->getSrcInfo(), extractType((*expr)[i]));
      if (isUnion || (*expr)[i]->getType()->isStaticType() !=
                         generics[i].getType()->isStaticType()) {
        if (cast<NoneExpr>((*expr)[i])) // `None` -> `NoneType`
          (*expr)[i] = transformType((*expr)[i]);
        if (!isTypeExpr((*expr)[i]))
          E(Error::EXPECTED_TYPE, (*expr)[i], "type");
      }
      if (isUnion) {
        if (!typ->getUnion()->addType(t.get()))
          E(error::Error::UNION_TOO_BIG, (*expr)[i],
            typ->getUnion()->pendingTypes.size());
      } else {
        unify(t.get(), generics[i].getType());
      }
    }
    if (isUnion) {
      typ->getUnion()->seal();
    }

    unify(expr->getType(), instantiateTypeVar(typ.get()));
    // If the type is realizable, use the realized name instead of instantiation
    // (e.g. use Id("Ptr[byte]") instead of Instantiate(Ptr, {byte}))
    if (auto rt = realize(expr->getType())) {
      auto t = extractType(rt);
      resultExpr = N<IdExpr>(t->realizedName());
      resultExpr->setType(rt->shared_from_this());
      resultExpr->setDone();
    }
  }

  // Handle side effects
  if (!ctx->simpleTypes) {
    std::vector<Stmt *> prepends;
    for (auto &t : *expr) {
      if (hasSideEffect(t)) {
        auto name = getTemporaryVar("call");
        auto front =
            transform(N<AssignStmt>(N<IdExpr>(name), t, getParamType(t->getType())));
        auto swap = transformType(N<IdExpr>(name));
        t = swap;
        prepends.emplace_back(front);
      }
    }
    if (!prepends.empty()) {
      resultExpr = transform(N<StmtExpr>(prepends, resultExpr ? resultExpr : expr));
    }
  }
}

/// Transform a slice expression.
/// @example
///   `start::step` -> `Slice(start, Optional.__new__(), step)`
void TypecheckVisitor::visit(SliceExpr *expr) {
  Expr *none = N<CallExpr>(N<DotExpr>(N<IdExpr>(TYPE_OPTIONAL), "__new__"));
  resultExpr = transform(N<CallExpr>(N<IdExpr>(getStdLibType("Slice")->name),
                                     expr->getStart() ? expr->getStart() : clone(none),
                                     expr->getStop() ? expr->getStop() : clone(none),
                                     expr->getStep() ? expr->getStep() : clone(none)));
}

/// Evaluate a static unary expression and return the resulting static expression.
/// If the expression cannot be evaluated yet, return nullptr.
/// Supported operators: (strings) not (ints) not, -, +
Expr *TypecheckVisitor::evaluateStaticUnary(UnaryExpr *expr) {
  // Case: static strings
  if (expr->getExpr()->getType()->isStaticType() == 2) {
    if (expr->getOp() == "!") {
      if (expr->getExpr()->getType()->canRealize()) {
        bool value = getStrLiteral(expr->getExpr()->getType()).empty();
        LOG_TYPECHECK("[cond::un] {}: {}", getSrcInfo(), value);
        return transform(N<IntExpr>(value));
      } else {
        // Cannot be evaluated yet: just set the type
        expr->getType()->getUnbound()->isStatic = 1;
      }
    }
    return nullptr;
  }

  // Case: static bools
  if (expr->getExpr()->getType()->isStaticType() == 3) {
    if (expr->getOp() == "!") {
      if (expr->getExpr()->getType()->canRealize()) {
        bool value = getBoolLiteral(expr->getExpr()->getType());
        LOG_TYPECHECK("[cond::un] {}: {}", getSrcInfo(), value);
        return transform(N<BoolExpr>(!value));
      } else {
        // Cannot be evaluated yet: just set the type
        expr->getType()->getUnbound()->isStatic = 3;
      }
    }
    return nullptr;
  }

  // Case: static integers
  if (expr->getOp() == "-" || expr->getOp() == "+" || expr->getOp() == "!" ||
      expr->getOp() == "~") {
    if (expr->getExpr()->getType()->canRealize()) {
      int64_t value = getIntLiteral(expr->getExpr()->getType());
      if (expr->getOp() == "+")
        ;
      else if (expr->getOp() == "-")
        value = -value;
      else if (expr->getOp() == "~")
        value = ~value;
      else
        value = !bool(value);
      LOG_TYPECHECK("[cond::un] {}: {}", getSrcInfo(), value);
      if (expr->getOp() == "!")
        return transform(N<BoolExpr>(value));
      else
        return transform(N<IntExpr>(value));
    } else {
      // Cannot be evaluated yet: just set the type
      expr->getType()->getUnbound()->isStatic = expr->getOp() == "!" ? 3 : 1;
    }
  }

  return nullptr;
}

/// Division and modulus implementations.
std::pair<int64_t, int64_t> divMod(const std::shared_ptr<TypeContext> &ctx, int64_t a,
                                   int64_t b) {
  if (!b) {
    E(Error::STATIC_DIV_ZERO, ctx->getSrcInfo());
    return {0, 0};
  } else if (ctx->cache->pythonCompat) {
    // Use Python implementation.
    int64_t d = a / b;
    int64_t m = a - d * b;
    if (m && ((b ^ m) < 0)) {
      m += b;
      d -= 1;
    }
    return {d, m};
  } else {
    // Use C implementation.
    return {a / b, a % b};
  }
}

/// Evaluate a static binary expression and return the resulting static expression.
/// If the expression cannot be evaluated yet, return nullptr.
/// Supported operators: (strings) +, ==, !=
///                      (ints) <, <=, >, >=, ==, !=, and, or, +, -, *, //, %, ^, |, &
Expr *TypecheckVisitor::evaluateStaticBinary(BinaryExpr *expr) {
  // Case: static strings
  if (expr->getRhs()->getType()->isStaticType() == 2) {
    if (expr->getOp() == "+") {
      // `"a" + "b"` -> `"ab"`
      if (expr->getLhs()->getType()->getStrStatic() &&
          expr->getRhs()->getType()->getStrStatic()) {
        auto value = getStrLiteral(expr->getLhs()->getType()) +
                     getStrLiteral(expr->getRhs()->getType());
        LOG_TYPECHECK("[cond::bin] {}: {}", getSrcInfo(), value);
        return transform(N<StringExpr>(value));
      } else {
        // Cannot be evaluated yet: just set the type
        expr->getType()->getUnbound()->isStatic = 2;
      }
    } else {
      // `"a" == "b"` -> `False` (also handles `!=`)
      if (expr->getLhs()->getType()->getStrStatic() &&
          expr->getRhs()->getType()->getStrStatic()) {
        bool eq = getStrLiteral(expr->getLhs()->getType()) ==
                  getStrLiteral(expr->getRhs()->getType());
        bool value = expr->getOp() == "==" ? eq : !eq;
        LOG_TYPECHECK("[cond::bin] {}: {}", getSrcInfo(), value);
        return transform(N<BoolExpr>(value));
      } else {
        // Cannot be evaluated yet: just set the type
        expr->getType()->getUnbound()->isStatic = 3;
      }
    }
    return nullptr;
  }

  // Case: static integers
  if (expr->getLhs()->getType()->getStatic() &&
      expr->getRhs()->getType()->getStatic()) {
    int64_t lvalue = expr->getLhs()->getType()->getIntStatic()
                         ? getIntLiteral(expr->getLhs()->getType())
                         : getBoolLiteral(expr->getLhs()->getType());
    int64_t rvalue = expr->getRhs()->getType()->getIntStatic()
                         ? getIntLiteral(expr->getRhs()->getType())
                         : getBoolLiteral(expr->getRhs()->getType());
    if (expr->getOp() == "<")
      lvalue = lvalue < rvalue;
    else if (expr->getOp() == "<=")
      lvalue = lvalue <= rvalue;
    else if (expr->getOp() == ">")
      lvalue = lvalue > rvalue;
    else if (expr->getOp() == ">=")
      lvalue = lvalue >= rvalue;
    else if (expr->getOp() == "==")
      lvalue = lvalue == rvalue;
    else if (expr->getOp() == "!=")
      lvalue = lvalue != rvalue;
    else if (expr->getOp() == "&&")
      lvalue = lvalue ? rvalue : lvalue;
    else if (expr->getOp() == "||")
      lvalue = lvalue ? lvalue : rvalue;
    else if (expr->getOp() == "+")
      lvalue = lvalue + rvalue;
    else if (expr->getOp() == "-")
      lvalue = lvalue - rvalue;
    else if (expr->getOp() == "*")
      lvalue = lvalue * rvalue;
    else if (expr->getOp() == "^")
      lvalue = lvalue ^ rvalue;
    else if (expr->getOp() == "&")
      lvalue = lvalue & rvalue;
    else if (expr->getOp() == "|")
      lvalue = lvalue | rvalue;
    else if (expr->getOp() == ">>")
      lvalue = lvalue >> rvalue;
    else if (expr->getOp() == "<<")
      lvalue = lvalue << rvalue;
    else if (expr->getOp() == "//")
      lvalue = divMod(ctx, lvalue, rvalue).first;
    else if (expr->getOp() == "%")
      lvalue = divMod(ctx, lvalue, rvalue).second;
    else
      seqassert(false, "unknown static operator {}", expr->getOp());
    LOG_TYPECHECK("[cond::bin] {}: {}", getSrcInfo(), lvalue);
    if (in(std::set<std::string>{"==", "!=", "<", "<=", ">", ">="}, expr->getOp())) {
      return transform(N<BoolExpr>(lvalue));
    } else if ((expr->getOp() == "&&" || expr->getOp() == "||") &&
               expr->getLhs()->getType()->getBoolStatic() &&
               expr->getLhs()->getType()->getBoolStatic()) {
      return transform(N<BoolExpr>(lvalue));
    } else {
      return transform(N<IntExpr>(lvalue));
    }
  } else {
    // Cannot be evaluated yet: just set the type
    if (in(std::set<std::string>{"==", "!=", "<", "<=", ">", ">="}, expr->getOp())) {
      expr->getType()->getUnbound()->isStatic = 3;
    } else if ((expr->getOp() == "&&" || expr->getOp() == "||") &&
               expr->getLhs()->getType()->getBoolStatic() &&
               expr->getLhs()->getType()->getBoolStatic()) {
      expr->getType()->getUnbound()->isStatic = 3;
    } else {
      expr->getType()->getUnbound()->isStatic = 1;
    }
  }

  return nullptr;
}

/// Transform a simple binary expression.
/// @example
///   `a and b`    -> `b if a else False`
///   `a or b`     -> `True if a else b`
///   `a in b`     -> `a.__contains__(b)`
///   `a not in b` -> `not (a in b)`
///   `a is not b` -> `not (a is b)`
Expr *TypecheckVisitor::transformBinarySimple(BinaryExpr *expr) {
  // Case: simple transformations
  if (expr->getOp() == "&&") {
    if (ctx->expectedType && ctx->expectedType->is("bool")) {
      return transform(N<IfExpr>(expr->getLhs(),
                                 N<CallExpr>(N<DotExpr>(expr->getRhs(), "__bool__")),
                                 N<BoolExpr>(false)));
    } else {
      return transform(N<CallExpr>(N<IdExpr>("__internal__.and_union"), expr->getLhs(),
                                   expr->getRhs()));
    }
  } else if (expr->getOp() == "||") {
    if (ctx->expectedType && ctx->expectedType->is("bool")) {
      return transform(N<IfExpr>(expr->getLhs(), N<BoolExpr>(true),
                                 N<CallExpr>(N<DotExpr>(expr->getRhs(), "__bool__"))));
    } else {
      return transform(N<CallExpr>(N<IdExpr>("__internal__.or_union"), expr->getLhs(),
                                   expr->getRhs()));
    }
  } else if (expr->getOp() == "not in") {
    return transform(N<CallExpr>(N<DotExpr>(
        N<CallExpr>(N<DotExpr>(expr->getRhs(), "__contains__"), expr->getLhs()),
        "__invert__")));
  } else if (expr->getOp() == "in") {
    return transform(
        N<CallExpr>(N<DotExpr>(expr->getRhs(), "__contains__"), expr->getLhs()));
  } else if (expr->getOp() == "is") {
    if (cast<NoneExpr>(expr->getLhs()) && cast<NoneExpr>(expr->getRhs()))
      return transform(N<BoolExpr>(true));
    else if (cast<NoneExpr>(expr->getLhs()))
      return transform(N<BinaryExpr>(expr->getRhs(), "is", expr->getLhs()));
  } else if (expr->getOp() == "is not") {
    return transform(
        N<UnaryExpr>("!", N<BinaryExpr>(expr->getLhs(), "is", expr->getRhs())));
  }
  return nullptr;
}

/// Transform a binary `is` expression by checking for type equality. Handle special `is
/// None` cаses as well. See inside for details.
Expr *TypecheckVisitor::transformBinaryIs(BinaryExpr *expr) {
  seqassert(expr->op == "is", "not an is binary expression");

  // Case: `is None` expressions
  if (cast<NoneExpr>(expr->getRhs())) {
    if (extractClassType(expr->getLhs())->is("NoneType"))
      return transform(N<BoolExpr>(true));
    if (!extractClassType(expr->getLhs())->is(TYPE_OPTIONAL)) {
      // lhs is not optional: `return False`
      return transform(N<BoolExpr>(false));
    } else {
      // Special case: Optional[Optional[... Optional[NoneType]]...] == NoneType
      auto g = extractClassType(expr->getLhs());
      for (; extractClassGeneric(g)->is("Optional");
           g = extractClassGeneric(g)->getClass())
        ;
      if (!extractClassGeneric(g)->getClass()) {
        auto typ = instantiateUnbound();
        typ->isStatic = 3;
        unify(expr->getType(), typ);
        return nullptr;
      }
      if (extractClassGeneric(g)->is("NoneType"))
        return transform(N<BoolExpr>(true));

      // lhs is optional: `return lhs.__has__().__invert__()`
      if (expr->getType()->getUnbound() && expr->getType()->isStaticType())
        expr->getType()->getUnbound()->isStatic = 0;
      return transform(N<CallExpr>(N<DotExpr>(
          N<CallExpr>(N<DotExpr>(expr->getLhs(), "__has__")), "__invert__")));
    }
  }

  // Check the type equality (operand types and __raw__ pointers must match).
  auto lc = realize(expr->getLhs()->getType());
  auto rc = realize(expr->getRhs()->getType());
  if (!lc || !rc) {
    // Types not known: return early
    unify(expr->getType(), getStdLibType("bool"));
    return nullptr;
  }
  if (isTypeExpr(expr->getLhs()) && isTypeExpr(expr->getRhs()))
    return transform(N<BoolExpr>(lc->realizedName() == rc->realizedName()));
  if (!lc->getClass()->isRecord() && !rc->getClass()->isRecord()) {
    // Both reference types: `return lhs.__raw__() == rhs.__raw__()`
    return transform(
        N<BinaryExpr>(N<CallExpr>(N<DotExpr>(expr->getLhs(), "__raw__")),
                      "==", N<CallExpr>(N<DotExpr>(expr->getRhs(), "__raw__"))));
  }
  if (lc->is(TYPE_OPTIONAL)) {
    // lhs is optional: `return lhs.__is_optional__(rhs)`
    return transform(
        N<CallExpr>(N<DotExpr>(expr->getLhs(), "__is_optional__"), expr->getRhs()));
  }
  if (rc->is(TYPE_OPTIONAL)) {
    // rhs is optional: `return rhs.__is_optional__(lhs)`
    return transform(
        N<CallExpr>(N<DotExpr>(expr->getRhs(), "__is_optional__"), expr->getLhs()));
  }
  if (lc->realizedName() != rc->realizedName()) {
    // tuple names do not match: `return False`
    return transform(N<BoolExpr>(false));
  }
  // Same tuple types: `return lhs == rhs`
  return transform(N<BinaryExpr>(expr->getLhs(), "==", expr->getRhs()));
}

/// Return a binary magic opcode for the provided operator.
std::pair<std::string, std::string> TypecheckVisitor::getMagic(const std::string &op) {
  // Table of supported binary operations and the corresponding magic methods.
  static auto magics = std::unordered_map<std::string, std::string>{
      {"+", "add"},     {"-", "sub"},       {"*", "mul"},     {"**", "pow"},
      {"/", "truediv"}, {"//", "floordiv"}, {"@", "matmul"},  {"%", "mod"},
      {"<", "lt"},      {"<=", "le"},       {">", "gt"},      {">=", "ge"},
      {"==", "eq"},     {"!=", "ne"},       {"<<", "lshift"}, {">>", "rshift"},
      {"&", "and"},     {"|", "or"},        {"^", "xor"},
  };
  auto mi = magics.find(op);
  if (mi == magics.end())
    seqassert(false, "invalid binary operator '{}'", op);

  static auto rightMagics = std::unordered_map<std::string, std::string>{
      {"<", "gt"}, {"<=", "ge"}, {">", "lt"}, {">=", "le"}, {"==", "eq"}, {"!=", "ne"},
  };
  auto rm = in(rightMagics, op);
  return {mi->second, rm ? *rm : "r" + mi->second};
}

/// Transform an in-place binary expression.
/// @example
///   `a op= b` -> `a.__iopmagic__(b)`
/// @param isAtomic if set, use atomic magics if available.
Expr *TypecheckVisitor::transformBinaryInplaceMagic(BinaryExpr *expr, bool isAtomic) {
  auto [magic, _] = getMagic(expr->getOp());
  auto lt = expr->getLhs()->getClassType();
  seqassert(lt, "lhs type not known");

  FuncType *method = nullptr;

  // Atomic operations: check if `lhs.__atomic_op__(Ptr[lhs], rhs)` exists
  if (isAtomic) {
    auto ptr = instantiateType(getStdLibType("Ptr"), std::vector<types::Type *>{lt});
    if ((method = findBestMethod(lt, format("__atomic_{}__", magic),
                                 {ptr.get(), expr->getRhs()->getType()}))) {
      expr->lexpr = N<CallExpr>(N<IdExpr>("__ptr__"), expr->getLhs());
    }
  }

  // In-place operations: check if `lhs.__iop__(lhs, rhs)` exists
  if (!method && expr->isInPlace()) {
    method = findBestMethod(lt, format("__i{}__", magic),
                            std::vector<Expr *>{expr->getLhs(), expr->getRhs()});
  }

  if (method)
    return transform(
        N<CallExpr>(N<IdExpr>(method->getFuncName()), expr->getLhs(), expr->getRhs()));
  return nullptr;
}

/// Transform a magic binary expression.
/// @example
///   `a op b` -> `a.__opmagic__(b)`
Expr *TypecheckVisitor::transformBinaryMagic(BinaryExpr *expr) {
  auto [magic, rightMagic] = getMagic(expr->getOp());
  auto lt = expr->getLhs()->getType();
  auto rt = expr->getRhs()->getType();

  if (!lt->is("pyobj") && rt->is("pyobj")) {
    // Special case: `obj op pyobj` -> `rhs.__rmagic__(lhs)` on lhs
    // Assumes that pyobj implements all left and right magics
    auto l = getTemporaryVar("l");
    auto r = getTemporaryVar("r");
    return transform(
        N<StmtExpr>(N<AssignStmt>(N<IdExpr>(l), expr->getLhs()),
                    N<AssignStmt>(N<IdExpr>(r), expr->getRhs()),
                    N<CallExpr>(N<DotExpr>(N<IdExpr>(r), format("__{}__", rightMagic)),
                                N<IdExpr>(l))));
  }
  if (lt->getUnion()) {
    // Special case: `union op obj` -> `union.__magic__(rhs)`
    return transform(N<CallExpr>(N<DotExpr>(expr->getLhs(), format("__{}__", magic)),
                                 expr->getRhs()));
  }

  // Normal operations: check if `lhs.__magic__(lhs, rhs)` exists
  if (auto method =
          findBestMethod(lt->getClass(), format("__{}__", magic),
                         std::vector<Expr *>{expr->getLhs(), expr->getRhs()})) {
    // Normal case: `__magic__(lhs, rhs)`
    return transform(
        N<CallExpr>(N<IdExpr>(method->getFuncName()), expr->getLhs(), expr->getRhs()));
  }

  // Right-side magics: check if `rhs.__rmagic__(rhs, lhs)` exists
  if (auto method =
          findBestMethod(rt->getClass(), format("__{}__", rightMagic),
                         std::vector<Expr *>{expr->getRhs(), expr->getLhs()})) {
    auto l = getTemporaryVar("l");
    auto r = getTemporaryVar("r");
    return transform(N<StmtExpr>(
        N<AssignStmt>(N<IdExpr>(l), expr->getLhs()),
        N<AssignStmt>(N<IdExpr>(r), expr->getRhs()),
        N<CallExpr>(N<IdExpr>(method->getFuncName()), N<IdExpr>(r), N<IdExpr>(l))));
  }

  return nullptr;
}

/// Given a tuple type and the expression `expr[index]`, check if an `index` is static
/// (integer or slice). If so, statically extract the specified tuple item or a
/// sub-tuple (if the index is a slice).
/// Works only on normal tuples and partial functions.
std::pair<bool, Expr *>
TypecheckVisitor::transformStaticTupleIndex(ClassType *tuple, Expr *expr, Expr *index) {
  bool isStaticString = expr->getType()->isStaticType() == 2;
  if (isStaticString && !expr->getType()->canRealize()) {
    return {true, nullptr};
  } else if (!isStaticString) {
    if (!tuple->isRecord())
      return {false, nullptr};
    if (!tuple->is(TYPE_TUPLE)) {
      if (tuple->is(TYPE_OPTIONAL)) {
        if (auto newTuple = extractClassGeneric(tuple)->getClass()) {
          return transformStaticTupleIndex(
              newTuple, transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr)), index);
        } else {
          return {true, nullptr};
        }
      }
      return {false, nullptr};
    }
  }

  // Extract the static integer value from expression
  auto getInt = [&](int64_t *o, Expr *e) {
    if (!e)
      return true;

    auto ore = transform(clone(e));
    if (auto s = ore->getType()->getIntStatic()) {
      *o = s->value;
      return true;
    }
    return false;
  };

  std::string str = isStaticString ? getStrLiteral(expr->getType()) : "";
  auto sz = int64_t(isStaticString ? str.size() : getClassFields(tuple).size());
  int64_t start = 0, stop = sz, step = 1, multiple = 0;
  if (getInt(&start, index)) {
    // Case: `tuple[int]`
    auto i = translateIndex(start, stop);
    if (i < 0 || i >= stop)
      E(Error::TUPLE_RANGE_BOUNDS, index, stop - 1, i);
    start = i;
  } else if (auto slice = cast<SliceExpr>(index)) {
    // Case: `tuple[int:int:int]`
    if (!getInt(&start, slice->getStart()) || !getInt(&stop, slice->getStop()) ||
        !getInt(&step, slice->getStep()))
      return {false, nullptr};

    // Adjust slice indices (Python slicing rules)
    if (slice->getStep() && !slice->getStart())
      start = step > 0 ? 0 : (sz - 1);
    if (slice->getStep() && !slice->getStop())
      stop = step > 0 ? sz : -(sz + 1);
    sliceAdjustIndices(sz, &start, &stop, step);
    multiple = 1;
  } else {
    return {false, nullptr};
  }

  if (isStaticString) {
    if (!multiple) {
      return {true, transform(N<StringExpr>(str.substr(start, 1)))};
    } else {
      std::string newStr;
      for (auto i = start; (step > 0) ? (i < stop) : (i > stop); i += step)
        newStr += str[i];
      return {true, transform(N<StringExpr>(newStr))};
    }
  } else {
    auto classFields = getClassFields(tuple);
    if (!multiple) {
      return {true, transform(N<DotExpr>(expr, classFields[start].name))};
    } else {
      // Generate a sub-tuple
      auto var = N<IdExpr>(getTemporaryVar("tup"));
      auto ass = N<AssignStmt>(var, expr);
      std::vector<Expr *> te;
      for (auto i = start; (step > 0) ? (i < stop) : (i > stop); i += step) {
        if (i < 0 || i >= sz)
          E(Error::TUPLE_RANGE_BOUNDS, index, sz - 1, i);
        te.push_back(N<DotExpr>(clone(var), classFields[i].name));
      }
      auto s = generateTuple(te.size());
      Expr *e = transform(N<StmtExpr>(std::vector<Stmt *>{ass},
                                      N<CallExpr>(N<IdExpr>(TYPE_TUPLE), te)));
      return {true, e};
    }
  }
}

/// Follow Python indexing rules for static tuple indices.
/// Taken from https://github.com/python/cpython/blob/main/Objects/sliceobject.c.
int64_t TypecheckVisitor::translateIndex(int64_t idx, int64_t len, bool clamp) {
  if (idx < 0)
    idx += len;
  if (clamp) {
    if (idx < 0)
      idx = 0;
    if (idx > len)
      idx = len;
  } else if (idx < 0 || idx >= len) {
    E(Error::TUPLE_RANGE_BOUNDS, getSrcInfo(), len - 1, idx);
  }
  return idx;
}

/// Follow Python slice indexing rules for static tuple indices.
/// Taken from https://github.com/python/cpython/blob/main/Objects/sliceobject.c.
/// Quote (sliceobject.c:269): "this is harder to get right than you might think"
int64_t TypecheckVisitor::sliceAdjustIndices(int64_t length, int64_t *start,
                                             int64_t *stop, int64_t step) {
  if (step == 0)
    E(Error::SLICE_STEP_ZERO, getSrcInfo());

  if (*start < 0) {
    *start += length;
    if (*start < 0) {
      *start = (step < 0) ? -1 : 0;
    }
  } else if (*start >= length) {
    *start = (step < 0) ? length - 1 : length;
  }

  if (*stop < 0) {
    *stop += length;
    if (*stop < 0) {
      *stop = (step < 0) ? -1 : 0;
    }
  } else if (*stop >= length) {
    *stop = (step < 0) ? length - 1 : length;
  }

  if (step < 0) {
    if (*stop < *start) {
      return (*start - *stop - 1) / (-step) + 1;
    }
  } else {
    if (*start < *stop) {
      return (*stop - *start - 1) / step + 1;
    }
  }
  return 0;
}

} // namespace codon::ast
