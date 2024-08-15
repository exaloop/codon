// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Replace unary operators with the appropriate magic calls.
/// Also evaluate static expressions. See @c evaluateStaticUnary for details.
void TypecheckVisitor::visit(UnaryExpr *expr) {
  expr->expr = transform(expr->expr);

  static std::unordered_map<int, std::unordered_set<std::string>> staticOps = {
      {1, {"-", "+", "!", "~"}}, {2, {"@"}}, {3, {"!"}}};
  // Handle static expressions
  if (auto s = expr->expr->getType()->isStaticType()) {
    if (in(staticOps[s], expr->op)) {
      resultExpr = evaluateStaticUnary(expr);
      return;
    }
  } else if (expr->expr->getType()->getUnbound()) {
    return;
  }

  if (expr->op == "!") {
    // `not expr` -> `expr.__bool__().__invert__()`
    resultExpr = transform(N<CallExpr>(
        N<DotExpr>(N<CallExpr>(N<DotExpr>(expr->expr, "__bool__")), "__invert__")));
  } else {
    std::string magic;
    if (expr->op == "~")
      magic = "invert";
    else if (expr->op == "+")
      magic = "pos";
    else if (expr->op == "-")
      magic = "neg";
    else
      seqassert(false, "invalid unary operator '{}'", expr->op);
    resultExpr =
        transform(N<CallExpr>(N<DotExpr>(expr->expr, format("__{}__", magic))));
  }
}

/// Replace binary operators with the appropriate magic calls.
/// See @c transformBinarySimple , @c transformBinaryIs , @c transformBinaryMagic and
/// @c transformBinaryInplaceMagic for details.
/// Also evaluate static expressions. See @c evaluateStaticBinary for details.
void TypecheckVisitor::visit(BinaryExpr *expr) {
  expr->lexpr = transform(expr->lexpr, true);
  expr->rexpr = transform(expr->rexpr, true);

  static std::unordered_map<int, std::unordered_set<std::string>> staticOps = {
      {1,
       {"<", "<=", ">", ">=", "==", "!=", "&&", "||", "+", "-", "*", "//", "%", "&",
        "|", "^", ">>", "<<"}},
      {2, {"==", "!=", "+"}},
      {3, {"<", "<=", ">", ">=", "==", "!=", "&&", "||"}}};
  if (expr->lexpr->getType()->isStaticType() &&
      expr->rexpr->getType()->isStaticType()) {
    auto l = expr->lexpr->getType()->isStaticType();
    auto r = expr->rexpr->getType()->isStaticType();
    bool isStatic = l == r && in(staticOps[l], expr->op);
    if (!isStatic && ((l == 1 && r == 3) || (r == 1 && l == 3)) &&
        in(staticOps[1], expr->op))
      isStatic = true;
    if (isStatic) {
      resultExpr = evaluateStaticBinary(expr);
      return;
    }
  }

  if (auto e = transformBinarySimple(expr)) {
    // Case: simple binary expressions
    resultExpr = e;
  } else if (expr->lexpr->getType()->getUnbound() ||
             (expr->op != "is" && expr->rexpr->getType()->getUnbound())) {
    // Case: types are unknown, so continue later
    return;
  } else if (expr->op == "is") {
    // Case: is operator
    resultExpr = transformBinaryIs(expr);
  } else {
    if (auto ei = transformBinaryInplaceMagic(expr, false)) {
      // Case: in-place magic methods
      resultExpr = ei;
    } else if (auto em = transformBinaryMagic(expr)) {
      // Case: normal magic methods
      resultExpr = em;
    } else if (expr->lexpr->getType()->is(TYPE_OPTIONAL)) {
      // Special case: handle optionals if everything else fails.
      // Assumes that optionals have no relevant magics (except for __eq__)
      resultExpr =
          transform(N<BinaryExpr>(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr->lexpr),
                                  expr->op, expr->rexpr, expr->inPlace));
    } else {
      // Nothing found: report an error
      E(Error::OP_NO_MAGIC, expr, expr->op, expr->lexpr->getType()->prettyString(),
        expr->rexpr->getType()->prettyString());
    }
  }
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
  resultExpr = transform(final);
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
    if (auto el = cast<EllipsisExpr>(a.value)) {
      if (el->mode == EllipsisExpr::PIPE)
        return {{ai, expr}};
    } else if (cast<CallExpr>(a.value)) {
      auto v = findEllipsis(a.value);
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
/// might become `a |> unwrap |> foo` to satisfy type constraints; see @c wrapExpr for
/// details).
void TypecheckVisitor::visit(PipeExpr *expr) {
  bool hasGenerator = false;

  // Return T if t is of type `Generator[T]`; otherwise just `type(t)`
  auto getIterableType = [&](TypePtr t) {
    if (t->is("Generator")) {
      hasGenerator = true;
      return t->getClass()->generics[0].type;
    }
    return t;
  };

  // List of output types
  // (e.g., for `a|>b|>c` it is `[type(a), type(a|>b), type(a|>b|>c)]`).
  // Note: the generator types are completely preserved (i.e., not extracted)
  expr->inTypes.clear();

  // Process the pipeline head
  expr->items[0].expr = transform(expr->items[0].expr);
  auto inType = expr->items[0].expr->getType(); // input type to the next stage
  expr->inTypes.push_back(inType);
  inType = getIterableType(inType);
  auto done = expr->items[0].expr->isDone();
  for (size_t pi = 1; pi < expr->items.size(); pi++) {
    int inTypePos = -1;                   // ellipsis position
    Expr **ec = &(expr->items[pi].expr);  // a pointer so that we can replace it
    while (auto se = cast<StmtExpr>(*ec)) // handle StmtExpr (e.g., in partial calls)
      ec = &(se->expr);

    if (auto call = cast<CallExpr>(*ec)) {
      // Case: a call. Find the position of the pipe ellipsis within it
      for (size_t ia = 0; inTypePos == -1 && ia < call->size(); ia++)
        if (cast<EllipsisExpr>((*call)[ia].value)) {
          inTypePos = int(ia);
        }
      // No ellipses found? Prepend it as the first argument
      if (inTypePos == -1) {
        call->items.insert(call->items.begin(),
                           {"", N<EllipsisExpr>(EllipsisExpr::PARTIAL)});
        inTypePos = 0;
      }
    } else {
      // Case: not a call. Convert it to a call with a single ellipsis
      expr->items[pi].expr =
          N<CallExpr>(expr->items[pi].expr, N<EllipsisExpr>(EllipsisExpr::PARTIAL));
      ec = &expr->items[pi].expr;
      inTypePos = 0;
    }

    // Set the ellipsis type
    auto el = cast<EllipsisExpr>((*cast<CallExpr>(*ec))[inTypePos].value);
    el->mode = EllipsisExpr::PIPE;
    // Don't unify unbound inType yet (it might become a generator that needs to be
    // extracted)
    if (!el->getType())
      el->setType(ctx->getUnbound());
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
      unify(expr->items[pi].expr->getType(), (*ec)->getType());
    expr->items[pi].expr = *ec;
    inType = expr->items[pi].expr->getType();
    if (!realize(inType))
      done = false;
    expr->inTypes.push_back(inType);

    // Do not extract the generator in the last stage of a pipeline
    if (pi + 1 < expr->items.size())
      inType = getIterableType(inType);
  }
  unify(expr->getType(), (hasGenerator ? ctx->getType("NoneType") : inType));
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
  auto ei = cast<IdExpr>(expr->expr);
  if (ei && ei->getValue() == "Static") {
    // Special case: static types. Ensure that static is supported
    auto eii = cast<IdExpr>(expr->index);
    if (!eii || (eii->getValue() != "int" && eii->getValue() != "str" &&
                 eii->getValue() != "bool"))
      E(Error::BAD_STATIC_TYPE, expr->index);
    auto typ = ctx->getUnbound();
    typ->isStatic = getStaticGeneric(expr);
    unify(expr->getType(), typ);
    expr->setDone();
    return;
  }
  if (ei && ei->getValue() == "tuple")
    ei->setValue(TYPE_TUPLE);
  expr->expr = transform(expr->expr, true);

  // IndexExpr[i1, ..., iN] is internally represented as
  // IndexExpr[TupleExpr[i1, ..., iN]] for N > 1
  std::vector<Expr *> items;
  bool isTuple = false;
  if (auto t = cast<TupleExpr>(expr->index)) {
    items = t->items;
    isTuple = true;
  } else {
    items.push_back(expr->index);
  }
  for (auto &i : items) {
    if (cast<ListExpr>(i) && expr->expr->getType()->is("type")) {
      // Special case: `A[[A, B], C]` -> `A[Tuple[A, B], C]` (e.g., in
      // `Function[...]`)
      i = N<InstantiateExpr>(N<IdExpr>(TYPE_TUPLE), cast<ListExpr>(i)->items);
    }
    i = transform(i, true);
  }
  if (expr->expr->getType()->is("type")) {
    resultExpr = transform(N<InstantiateExpr>(expr->expr, items));
    return;
  }

  expr->index = (!isTuple && items.size() == 1) ? items[0] : N<TupleExpr>(items);
  auto cls = expr->expr->getType()->getClass();
  if (!cls) {
    // Wait until the type becomes known
    return;
  }

  // Case: static tuple access
  auto [isStaticTuple, tupleExpr] =
      transformStaticTupleIndex(cls, expr->expr, expr->index);
  if (isStaticTuple) {
    if (tupleExpr)
      resultExpr = tupleExpr;
  } else {
    // Case: normal __getitem__
    resultExpr =
        transform(N<CallExpr>(N<DotExpr>(expr->expr, "__getitem__"), expr->index));
  }
}

/// Transform an instantiation to canonical realized name.
/// @example
///   Instantiate(foo, [bar]) -> Id("foo[bar]")
void TypecheckVisitor::visit(InstantiateExpr *expr) {
  expr->expr = transformType(expr->expr);

  TypePtr typ = nullptr;
  bool hasRepeats = false;
  size_t typeParamsSize = expr->size() - hasRepeats;
  if (getType(expr->expr)->is(TYPE_TUPLE)) {
    // if (expr->size() > 1) {
    //   expr->items[0] = transform(expr->front(), true);
    //   if (expr->front()->getType()->isStaticType()) {
    //     hasRepeats = true;
    //   }
    // }
    typ = ctx->instantiate(generateTuple(typeParamsSize - hasRepeats));
  } else {
    typ = ctx->instantiate(expr->expr->getSrcInfo(), getType(expr->expr));
  }
  seqassert(typ->getClass(), "unknown type: {}", *(expr->expr));

  auto &generics = typ->getClass()->generics;
  bool isUnion = typ->getUnion() != nullptr;
  if (!isUnion && typeParamsSize != generics.size())
    E(Error::GENERICS_MISMATCH, expr, ctx->cache->rev(typ->getClass()->name),
      generics.size(), typeParamsSize);

  if (isId(expr->expr, TYPE_CALLABLE)) {
    // Case: Callable[...] trait instantiation
    std::vector<TypePtr> types;

    // Callable error checking.
    for (auto &typeParam : *expr) {
      typeParam = transformType(typeParam);
      if (typeParam->getType()->isStaticType())
        E(Error::INST_CALLABLE_STATIC, typeParam);
      types.push_back(getType(typeParam));
    }
    auto typ = ctx->getUnbound();
    // Set up the Callable trait
    typ->getLink()->trait = std::make_shared<CallableTrait>(ctx->cache, types);
    unify(expr->getType(), ctx->instantiateGeneric(ctx->getType("type"), {typ}));
  } else if (isId(expr->expr, TYPE_TYPEVAR)) {
    // Case: TypeVar[...] trait instantiation
    (*expr)[0] = transformType((*expr)[0]);
    auto typ = ctx->getUnbound();
    typ->getLink()->trait = std::make_shared<TypeTrait>(getType((*expr)[0]));
    unify(expr->getType(), typ);
  } else {
    for (size_t i = hasRepeats; i < expr->size(); i++) {
      (*expr)[i] = transformType((*expr)[i]);
      auto t = ctx->instantiate((*expr)[i]->getSrcInfo(), getType((*expr)[i]));
      if (isUnion ||
          (*expr)[i]->getType()->isStaticType() != generics[i].type->isStaticType()) {
        if (cast<NoneExpr>((*expr)[i])) // `None` -> `NoneType`
          (*expr)[i] = transformType((*expr)[i]);
        if (!(*expr)[i]->getType()->is("type"))
          E(Error::EXPECTED_TYPE, (*expr)[i], "type");
      }
      if (isUnion)
        typ->getUnion()->addType(t);
      else
        unify(t, generics[i - hasRepeats].type);
    }
    // if (repeats)
    // typ->getRecord()->repeats = repeats;
    if (isUnion) {
      typ->getUnion()->seal();
    }

    unify(expr->getType(), ctx->instantiateGeneric(ctx->getType("type"), {typ}));
    // If the type is realizable, use the realized name instead of instantiation
    // (e.g. use Id("Ptr[byte]") instead of Instantiate(Ptr, {byte}))
    if (realize(expr->getType())) {
      auto t = getType(expr);
      resultExpr = N<IdExpr>(t->realizedName());
      resultExpr->setType(expr->getType());
      resultExpr->setDone();
    }
  }
}

/// Transform a slice expression.
/// @example
///   `start::step` -> `Slice(start, Optional.__new__(), step)`
void TypecheckVisitor::visit(SliceExpr *expr) {
  Expr *none = N<CallExpr>(N<DotExpr>(N<IdExpr>(TYPE_OPTIONAL), "__new__"));
  auto name = ctx->cache->imports[STDLIB_IMPORT].ctx->getType("Slice")->getClass();
  resultExpr = transform(N<CallExpr>(
      N<IdExpr>(name->name), expr->start ? expr->start : clone(none),
      expr->stop ? expr->stop : clone(none), expr->step ? expr->step : clone(none)));
}

/// Evaluate a static unary expression and return the resulting static expression.
/// If the expression cannot be evaluated yet, return nullptr.
/// Supported operators: (strings) not (ints) not, -, +
Expr *TypecheckVisitor::evaluateStaticUnary(UnaryExpr *expr) {
  // Case: static strings
  if (expr->expr->getType()->isStaticType() == 2) {
    if (expr->op == "!") {
      if (expr->expr->getType()->canRealize()) {
        bool value = expr->expr->getType()->getStrStatic()->value.empty();
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
  if (expr->expr->getType()->isStaticType() == 3) {
    if (expr->op == "!") {
      if (expr->expr->getType()->canRealize()) {
        bool value = expr->expr->getType()->getBoolStatic()->value;
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
  if (expr->op == "-" || expr->op == "+" || expr->op == "!") {
    if (expr->expr->getType()->canRealize()) {
      int64_t value = expr->expr->getType()->getIntStatic()->value;
      if (expr->op == "+")
        ;
      else if (expr->op == "-")
        value = -value;
      else if (expr->op == "~")
        value = ~value;
      else
        value = !bool(value);
      LOG_TYPECHECK("[cond::un] {}: {}", getSrcInfo(), value);
      if (expr->op == "!")
        return transform(N<BoolExpr>(value));
      else
        return transform(N<IntExpr>(value));
    } else {
      // Cannot be evaluated yet: just set the type
      expr->getType()->getUnbound()->isStatic = expr->op == "!" ? 3 : 1;
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
  if (expr->rexpr->getType()->isStaticType() == 2) {
    if (expr->op == "+") {
      // `"a" + "b"` -> `"ab"`
      if (expr->lexpr->getType()->getStrStatic() &&
          expr->rexpr->getType()->getStrStatic()) {
        auto value = expr->lexpr->getType()->getStrStatic()->value +
                     expr->rexpr->getType()->getStrStatic()->value;
        LOG_TYPECHECK("[cond::bin] {}: {}", getSrcInfo(), value);
        return transform(N<StringExpr>(value));
      } else {
        // Cannot be evaluated yet: just set the type
        expr->getType()->getUnbound()->isStatic = 2;
      }
    } else {
      // `"a" == "b"` -> `False` (also handles `!=`)
      if (expr->lexpr->getType()->getStrStatic() &&
          expr->rexpr->getType()->getStrStatic()) {
        bool eq = expr->lexpr->getType()->getStrStatic()->value ==
                  expr->rexpr->getType()->getStrStatic()->value;
        bool value = expr->op == "==" ? eq : !eq;
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
  if (expr->lexpr->getType()->getStatic() && expr->rexpr->getType()->getStatic()) {
    int64_t lvalue = expr->lexpr->getType()->getIntStatic()
                         ? expr->lexpr->getType()->getIntStatic()->value
                         : expr->lexpr->getType()->getBoolStatic()->value;
    int64_t rvalue = expr->rexpr->getType()->getIntStatic()
                         ? expr->rexpr->getType()->getIntStatic()->value
                         : expr->rexpr->getType()->getBoolStatic()->value;
    if (expr->op == "<")
      lvalue = lvalue < rvalue;
    else if (expr->op == "<=")
      lvalue = lvalue <= rvalue;
    else if (expr->op == ">")
      lvalue = lvalue > rvalue;
    else if (expr->op == ">=")
      lvalue = lvalue >= rvalue;
    else if (expr->op == "==")
      lvalue = lvalue == rvalue;
    else if (expr->op == "!=")
      lvalue = lvalue != rvalue;
    else if (expr->op == "&&")
      lvalue = lvalue && rvalue;
    else if (expr->op == "||")
      lvalue = lvalue || rvalue;
    else if (expr->op == "+")
      lvalue = lvalue + rvalue;
    else if (expr->op == "-")
      lvalue = lvalue - rvalue;
    else if (expr->op == "*")
      lvalue = lvalue * rvalue;
    else if (expr->op == "^")
      lvalue = lvalue ^ rvalue;
    else if (expr->op == "&")
      lvalue = lvalue & rvalue;
    else if (expr->op == "|")
      lvalue = lvalue | rvalue;
    else if (expr->op == ">>")
      lvalue = lvalue >> rvalue;
    else if (expr->op == "<<")
      lvalue = lvalue << rvalue;
    else if (expr->op == "//")
      lvalue = divMod(ctx, lvalue, rvalue).first;
    else if (expr->op == "%")
      lvalue = divMod(ctx, lvalue, rvalue).second;
    else
      seqassert(false, "unknown static operator {}", expr->op);
    LOG_TYPECHECK("[cond::bin] {}: {}", getSrcInfo(), lvalue);
    if (in(std::set<std::string>{"==", "!=", "<", "<=", ">", ">=", "&&", "||"},
           expr->op))
      return transform(N<BoolExpr>(lvalue));
    else
      return transform(N<IntExpr>(lvalue));
  } else {
    // Cannot be evaluated yet: just set the type
    if (in(std::set<std::string>{"==", "!=", "<", "<=", ">", ">=", "&&", "||"},
           expr->op))
      expr->getType()->getUnbound()->isStatic = 3;
    else
      expr->getType()->getUnbound()->isStatic = 1;
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
  if (expr->op == "&&") {
    return transform(N<IfExpr>(expr->lexpr,
                               N<CallExpr>(N<DotExpr>(expr->rexpr, "__bool__")),
                               N<BoolExpr>(false)));
  } else if (expr->op == "||") {
    return transform(N<IfExpr>(expr->lexpr, N<BoolExpr>(true),
                               N<CallExpr>(N<DotExpr>(expr->rexpr, "__bool__"))));
  } else if (expr->op == "not in") {
    return transform(N<CallExpr>(
        N<DotExpr>(N<CallExpr>(N<DotExpr>(expr->rexpr, "__contains__"), expr->lexpr),
                   "__invert__")));
  } else if (expr->op == "in") {
    return transform(N<CallExpr>(N<DotExpr>(expr->rexpr, "__contains__"), expr->lexpr));
  } else if (expr->op == "is") {
    if (cast<NoneExpr>(expr->lexpr) && cast<NoneExpr>(expr->rexpr))
      return transform(N<BoolExpr>(true));
    else if (cast<NoneExpr>(expr->lexpr))
      return transform(N<BinaryExpr>(expr->rexpr, "is", expr->lexpr));
  } else if (expr->op == "is not") {
    return transform(N<UnaryExpr>("!", N<BinaryExpr>(expr->lexpr, "is", expr->rexpr)));
  }
  return nullptr;
}

/// Transform a binary `is` expression by checking for type equality. Handle special `is
/// None` cаses as well. See inside for details.
Expr *TypecheckVisitor::transformBinaryIs(BinaryExpr *expr) {
  seqassert(expr->op == "is", "not an is binary expression");

  // Case: `is None` expressions
  if (cast<NoneExpr>(expr->rexpr)) {
    if (getType(expr->lexpr)->is("NoneType"))
      return transform(N<BoolExpr>(true));
    if (!getType(expr->lexpr)->is(TYPE_OPTIONAL)) {
      // lhs is not optional: `return False`
      return transform(N<BoolExpr>(false));
    } else {
      // Special case: Optional[Optional[... Optional[NoneType]]...] == NoneType
      auto g = getType(expr->lexpr)->getClass();
      for (; g->generics[0].type->is("Optional"); g = g->generics[0].type->getClass())
        ;
      if (!g->generics[0].type->getClass()) {
        auto typ = ctx->getUnbound();
        typ->isStatic = 3;
        unify(expr->getType(), typ);
        return nullptr;
      }
      if (g->generics[0].type->is("NoneType"))
        return transform(N<BoolExpr>(true));

      // lhs is optional: `return lhs.__has__().__invert__()`
      if (expr->getType()->getUnbound() && expr->getType()->isStaticType())
        expr->getType()->getUnbound()->isStatic = 0;
      return transform(N<CallExpr>(
          N<DotExpr>(N<CallExpr>(N<DotExpr>(expr->lexpr, "__has__")), "__invert__")));
    }
  }

  // Check the type equality (operand types and __raw__ pointers must match).
  auto lc = realize(expr->lexpr->getType());
  auto rc = realize(expr->rexpr->getType());
  if (!lc || !rc) {
    // Types not known: return early
    unify(expr->getType(), ctx->getType("bool"));
    return nullptr;
  }
  if (expr->lexpr->getType()->is("type") && expr->rexpr->getType()->is("type"))
    return transform(N<BoolExpr>(lc->realizedName() == rc->realizedName()));
  if (!lc->getClass()->isRecord() && !rc->getClass()->isRecord()) {
    // Both reference types: `return lhs.__raw__() == rhs.__raw__()`
    return transform(
        N<BinaryExpr>(N<CallExpr>(N<DotExpr>(expr->lexpr, "__raw__")),
                      "==", N<CallExpr>(N<DotExpr>(expr->rexpr, "__raw__"))));
  }
  if (lc->getClass()->is(TYPE_OPTIONAL)) {
    // lhs is optional: `return lhs.__is_optional__(rhs)`
    return transform(
        N<CallExpr>(N<DotExpr>(expr->lexpr, "__is_optional__"), expr->rexpr));
  }
  if (rc->getClass()->is(TYPE_OPTIONAL)) {
    // rhs is optional: `return rhs.__is_optional__(lhs)`
    return transform(
        N<CallExpr>(N<DotExpr>(expr->rexpr, "__is_optional__"), expr->lexpr));
  }
  if (lc->realizedName() != rc->realizedName()) {
    // tuple names do not match: `return False`
    return transform(N<BoolExpr>(false));
  }
  // Same tuple types: `return lhs == rhs`
  return transform(N<BinaryExpr>(expr->lexpr, "==", expr->rexpr));
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
  auto [magic, _] = getMagic(expr->op);
  auto lt = expr->lexpr->getClassType();
  seqassert(lt, "lhs type not known");

  FuncTypePtr method = nullptr;

  // Atomic operations: check if `lhs.__atomic_op__(Ptr[lhs], rhs)` exists
  if (isAtomic) {
    auto ptr = ctx->instantiateGeneric(ctx->getType("Ptr"), {lt});
    if ((method = findBestMethod(lt, format("__atomic_{}__", magic),
                                 {ptr, expr->rexpr->getType()}))) {
      expr->lexpr = N<CallExpr>(N<IdExpr>("__ptr__"), expr->lexpr);
    }
  }

  // In-place operations: check if `lhs.__iop__(lhs, rhs)` exists
  if (!method && expr->inPlace) {
    method = findBestMethod(lt, format("__i{}__", magic),
                            std::vector<Expr *>{expr->lexpr, expr->rexpr});
  }

  if (method)
    return transform(
        N<CallExpr>(N<IdExpr>(method->ast->name), expr->lexpr, expr->rexpr));
  return nullptr;
}

/// Transform a magic binary expression.
/// @example
///   `a op b` -> `a.__opmagic__(b)`
Expr *TypecheckVisitor::transformBinaryMagic(BinaryExpr *expr) {
  auto [magic, rightMagic] = getMagic(expr->op);
  auto lt = expr->lexpr->getType();
  auto rt = expr->rexpr->getType();

  if (!lt->is("pyobj") && rt->is("pyobj")) {
    // Special case: `obj op pyobj` -> `rhs.__rmagic__(lhs)` on lhs
    // Assumes that pyobj implements all left and right magics
    auto l = ctx->cache->getTemporaryVar("l"), r = ctx->cache->getTemporaryVar("r");
    return transform(
        N<StmtExpr>(N<AssignStmt>(N<IdExpr>(l), expr->lexpr),
                    N<AssignStmt>(N<IdExpr>(r), expr->rexpr),
                    N<CallExpr>(N<DotExpr>(N<IdExpr>(r), format("__{}__", rightMagic)),
                                N<IdExpr>(l))));
  }
  if (lt->getUnion()) {
    // Special case: `union op obj` -> `union.__magic__(rhs)`
    return transform(
        N<CallExpr>(N<DotExpr>(expr->lexpr, format("__{}__", magic)), expr->rexpr));
  }

  // Normal operations: check if `lhs.__magic__(lhs, rhs)` exists
  if (auto method = findBestMethod(lt->getClass(), format("__{}__", magic),
                                   std::vector<Expr *>{expr->lexpr, expr->rexpr})) {
    // Normal case: `__magic__(lhs, rhs)`
    return transform(
        N<CallExpr>(N<IdExpr>(method->ast->name), expr->lexpr, expr->rexpr));
  }

  // Right-side magics: check if `rhs.__rmagic__(rhs, lhs)` exists
  if (auto method = findBestMethod(rt->getClass(), format("__{}__", rightMagic),
                                   std::vector<Expr *>{expr->rexpr, expr->lexpr})) {
    auto l = ctx->cache->getTemporaryVar("l"), r = ctx->cache->getTemporaryVar("r");
    return transform(N<StmtExpr>(
        N<AssignStmt>(N<IdExpr>(l), expr->lexpr),
        N<AssignStmt>(N<IdExpr>(r), expr->rexpr),
        N<CallExpr>(N<IdExpr>(method->ast->name), N<IdExpr>(r), N<IdExpr>(l))));
  }

  return nullptr;
}

/// Given a tuple type and the expression `expr[index]`, check if an `index` is static
/// (integer or slice). If so, statically extract the specified tuple item or a
/// sub-tuple (if the index is a slice).
/// Works only on normal tuples and partial functions.
std::pair<bool, Expr *>
TypecheckVisitor::transformStaticTupleIndex(const ClassTypePtr &tuple, Expr *expr,
                                            Expr *index) {
  bool isStaticString = expr->getType()->isStaticType() == 2;
  if (isStaticString && !expr->getType()->canRealize()) {
    return {true, nullptr};
  } else if (!isStaticString) {
    if (!tuple->isRecord())
      return {false, nullptr};
    if (!tuple->is(TYPE_TUPLE)) {
      if (tuple->is(TYPE_OPTIONAL)) {
        if (auto newTuple = tuple->generics[0].type->getClass()) {
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
    auto f = transform(clean_clone(e));
    if (auto s = f->getType()->getIntStatic()) {
      *o = s->value;
      return true;
    }
    return false;
  };

  std::string str = isStaticString ? expr->getType()->getStrStatic()->value : "";
  auto sz = int64_t(isStaticString ? str.size() : getClassFields(tuple.get()).size());
  int64_t start = 0, stop = sz, step = 1, multiple = 0;
  if (getInt(&start, index)) {
    // Case: `tuple[int]`
    auto i = translateIndex(start, stop);
    if (i < 0 || i >= stop)
      E(Error::TUPLE_RANGE_BOUNDS, index, stop - 1, i);
    start = i;
  } else if (auto slice = cast<SliceExpr>(index->getOrigExpr())) {
    // Case: `tuple[int:int:int]`
    if (!getInt(&start, slice->start) || !getInt(&stop, slice->stop) ||
        !getInt(&step, slice->step))
      return {false, nullptr};

    // Adjust slice indices (Python slicing rules)
    if (slice->step && !slice->start)
      start = step > 0 ? 0 : (sz - 1);
    if (slice->step && !slice->stop)
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
    auto classFields = getClassFields(tuple.get());
    if (!multiple) {
      return {true, transform(N<DotExpr>(expr, classFields[start].name))};
    } else {
      // Generate a sub-tuple
      auto var = N<IdExpr>(ctx->cache->getTemporaryVar("tup"));
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
