#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

/// Replace unary operators with the appropriate magic calls.
/// Also evaluate static expressions. See @c evaluateStaticUnary for details.
void TypecheckVisitor::visit(UnaryExpr *expr) {
  transform(expr->expr);

  // Handle static expressions
  if (expr->expr->isStatic()) {
    resultExpr = evaluateStaticUnary(expr);
    return;
  }

  if (expr->op == "!") {
    // `not expr` -> `expr.__bool__().__invert__()`
    resultExpr = transform(N<CallExpr>(N<DotExpr>(
        N<CallExpr>(N<DotExpr>(clone(expr->expr), "__bool__")), "__invert__")));
  } else {
    std::string magic;
    if (expr->op == "~")
      magic = "invert";
    else if (expr->op == "+")
      magic = "pos";
    else if (expr->op == "-")
      magic = "neg";
    else
      error("invalid unary operator '{}'", expr->op);
    resultExpr =
        transform(N<CallExpr>(N<DotExpr>(clone(expr->expr), format("__{}__", magic))));
  }
}

/// Replace binary operators with the appropriate magic calls.
/// See @c transformBinarySimple , @c transformBinaryIs , @c transformBinaryMagic and
/// @c transformBinaryInplaceMagic for details.
/// Also evaluate static expressions. See @c evaluateStaticBinary for details.
void TypecheckVisitor::visit(BinaryExpr *expr) {
  // Transform lexpr and rexpr. Ignore Nones for now
  if (!(startswith(expr->op, "is") && expr->lexpr->getNone()))
    transform(expr->lexpr);
  if (!(startswith(expr->op, "is") && expr->rexpr->getNone()))
    transform(expr->rexpr);

  static std::unordered_map<StaticValue::Type, std::unordered_set<std::string>>
      staticOps = {
          {StaticValue::INT,
           {"<", "<=", ">", ">=", "==", "!=", "&&", "||", "+", "-", "*", "//", "%"}},
          {StaticValue::STRING, {"==", "!=", "+"}}};
  if (expr->lexpr->isStatic() && expr->rexpr->isStatic() &&
      expr->lexpr->staticValue.type == expr->rexpr->staticValue.type &&
      in(staticOps[expr->rexpr->staticValue.type], expr->op)) {
    // Handle static expressions
    resultExpr = evaluateStaticBinary(expr);
  } else if (auto e = transformBinarySimple(expr)) {
    // Case: simple binary expressions
    resultExpr = e;
  } else if (expr->lexpr->getType()->getUnbound() ||
             (expr->op != "is" && expr->rexpr->getType()->getUnbound())) {
    // Case: types are unknown, so continue later
    unify(expr->type, ctx->getUnbound());
    return;
  } else if (expr->op == "is") {
    // Case: is operator
    resultExpr = transformBinaryIs(expr);
  } else if (auto e = transformBinaryInplaceMagic(expr, false)) {
    // Case: in-place magic methods
    resultExpr = e;
  } else if (auto e = transformBinaryMagic(expr)) {
    // Case: normal magic methods
    resultExpr = e;
  } else {
    // Nothing found: report an error
    error("cannot find magic '{}' in {}", getMagic(expr->op),
          expr->lexpr->type->toString());
  }
}

/// Type-checks a pipe expression.
/// Transform a stage CallExpr foo(x) without an ellipsis into:
///   foo(..., x).
/// Transform any non-CallExpr stage foo into a CallExpr stage:
///   foo(...).
/// If necessary, add stages (e.g. unwrap, float.__new__ or Optional.__new__)
/// to support function call type adjustments.
/// TODO: LATER
void TypecheckVisitor::visit(PipeExpr *expr) {
  bool hasGenerator = false;

  // Returns T if t is of type Generator[T].
  auto getIterableType = [&](TypePtr t) {
    if (t->is("Generator")) {
      hasGenerator = true;
      return t->getClass()->generics[0].type;
    }
    return t;
  };
  // List of output types (for a|>b|>c, this list is type(a), type(a|>b),
  // type(a|>b|>c). These types are raw types (i.e. generator types are preserved).
  expr->inTypes.clear();
  transform(expr->items[0].expr);

  // The input type to the next stage.
  TypePtr inType = expr->items[0].expr->getType();
  expr->inTypes.push_back(inType);
  inType = getIterableType(inType);
  expr->done = expr->items[0].expr->done;
  for (int i = 1; i < expr->items.size(); i++) {
    ExprPtr prepend = nullptr; // An optional preceding stage
                               // (e.g. prepend  (|> unwrap |>) if an optional argument
                               //  needs unpacking).
    int inTypePos = -1;

    // Get the stage expression (take heed of StmtExpr!):
    auto ec = &expr->items[i].expr; // This is a pointer to a CallExprPtr
    while ((*ec)->getStmtExpr())
      ec = &const_cast<StmtExpr *>((*ec)->getStmtExpr())->expr;
    if (auto ecc = const_cast<CallExpr *>((*ec)->getCall())) {
      // Find the input argument position (a position of ... in the argument list):
      for (int ia = 0; ia < ecc->args.size(); ia++)
        if (auto ee = ecc->args[ia].value->getEllipsis()) {
          if (inTypePos == -1) {
            const_cast<EllipsisExpr *>(ee)->isPipeArg = true;
            inTypePos = ia;
            break;
          }
        }
      // If there is no ... in the argument list, use the first argument as the input
      // argument and add an ellipsis there
      if (inTypePos == -1) {
        ecc->args.insert(ecc->args.begin(), {"", N<EllipsisExpr>(true)});
        inTypePos = 0;
      }
    } else {
      // If this is not a CallExpr, make it a call expression with a single input
      // argument:
      expr->items[i].expr = N<CallExpr>(expr->items[i].expr, N<EllipsisExpr>(true));
      ec = &expr->items[i].expr;
      inTypePos = 0;
    }

    if (auto nn = transformCall((CallExpr *)(ec->get()), inType, &prepend))
      *ec = nn;
    if (prepend) { // Prepend the stage and rewind the loop (yes, the current
                   // expression will get parsed twice).
      expr->items.insert(expr->items.begin() + i, {"|>", prepend});
      i--;
      continue;
    }
    if ((*ec)->type)
      unify(expr->items[i].expr->type, (*ec)->type);
    expr->items[i].expr = *ec;
    inType = expr->items[i].expr->getType();
    if (auto rt = realize(inType))
      unify(inType, rt);
    else
      expr->done = false;
    expr->inTypes.push_back(inType);
    // Do not extract the generator type in the last stage of a pipeline.
    if (i < expr->items.size() - 1)
      inType = getIterableType(inType);
  }
  unify(expr->type, (hasGenerator ? ctx->getType("NoneType") : inType));
}

/// Transform index expressions.
/// @example
///   `foo[T]`   -> Instantiate(foo, [T]) if `foo` is a type
///   `tup[1]`   -> `tup.item1` if `tup` is tuple
///   `foo[idx]` -> `foo.__getitem__(idx)`
///   expr.itemN or a sub-tuple if index is static (see transformStaticTupleIndex()),
void TypecheckVisitor::visit(IndexExpr *expr) {
  // Handle `Static[T]` constructs
  if (expr->expr->isId("Static")) {
    auto typ = ctx->getUnbound();
    typ->isStatic = getStaticGeneric(expr);
    unify(expr->type, typ);
    expr->setDone();
    return;
  }

  transform(expr->expr, true);
  seqassert(!expr->expr->isType(), "index not converted to instantiate");
  auto cls = expr->expr->getType()->getClass();
  if (!cls) {
    // Wait until the type becomes known
    unify(expr->type, ctx->getUnbound());
    return;
  }

  // Case: static tuple access
  resultExpr = transformStaticTupleIndex(cls.get(), expr->expr, expr->index);

  // Case: normal __getitem__
  if (!resultExpr)
    resultExpr =
        transform(N<CallExpr>(N<DotExpr>(expr->expr, "__getitem__"), expr->index));
}

/// Transform an instantiation to canonical realized name.
/// @example
///   Instantiate(foo, [bar]) -> Id("foo[bar]")
void TypecheckVisitor::visit(InstantiateExpr *expr) {
  // Infer the expression type
  if (expr->typeExpr->isId(TYPE_CALLABLE)) {
    // Case: Callable[...] instantiation
    std::vector<TypePtr> types;

    // Callable error checking.
    /// TODO: move to Codon?
    if (expr->typeParams.size() != 2)
      error("invalid Callable type declaration");
    for (size_t i = 0; i < expr->typeParams.size(); i++) {
      transformType(expr->typeParams[i]);
      if (expr->typeParams[i]->type->isStaticType())
        error("unexpected static type");
      types.push_back(expr->typeParams[i]->type);
    }
    auto typ = ctx->getUnbound();
    // Set up the Callable trait
    typ->getLink()->trait = std::make_shared<CallableTrait>(types);
    unify(expr->type, typ);
  } else {
    transformType(expr->typeExpr);
    TypePtr typ =
        ctx->instantiate(expr->typeExpr->getSrcInfo(), expr->typeExpr->getType());
    seqassert(typ->getClass(), "unknown type");

    auto &generics = typ->getClass()->generics;
    if (expr->typeParams.size() != generics.size())
      error("expected {} generics and/or statics", generics.size());

    for (size_t i = 0; i < expr->typeParams.size(); i++) {
      transform(expr->typeParams[i], true);
      TypePtr t = nullptr;
      if (expr->typeParams[i]->isStatic()) {
        t = std::make_shared<StaticType>(expr->typeParams[i], ctx);
      } else {
        if (expr->typeParams[i]->getNone()) // `None` -> `NoneType`
          transformType(expr->typeParams[i]);
        if (!expr->typeParams[i]->isType())
          error("expected type or static parameters");
        t = ctx->instantiate(expr->typeParams[i]->getSrcInfo(),
                             expr->typeParams[i]->getType());
      }
      unify(generics[i].type, t);
    }
    unify(expr->type, typ);
  }
  expr->markType();

  // If the type is realizable, use the realized name instead of instantiation
  // (e.g. use Id("Ptr[byte]") instead of Instantiate(Ptr, {byte}))
  if (realize(expr->type)) {
    resultExpr = N<IdExpr>(expr->type->realizedName());
    resultExpr->setType(expr->type);
    resultExpr->setDone();
    if (expr->typeExpr->isType())
      resultExpr->markType();
  }
}

/// Transform a slice expression.
/// @example
///   `start::step` -> `Slice(start, Optional.__new__(), step)`
void TypecheckVisitor::visit(SliceExpr *expr) {
  ExprPtr none = N<CallExpr>(N<DotExpr>(TYPE_OPTIONAL, "__new__"));
  resultExpr = transform(N<CallExpr>(
      N<IdExpr>(TYPE_SLICE), expr->start ? expr->start : clone(none),
      expr->stop ? expr->stop : clone(none), expr->step ? expr->step : clone(none)));
}

/// Evaluate a static unary expression and return the resulting static expression.
/// If the expression cannot be evaluated yet, return nullptr.
/// Supported operators: (strings) not (ints) not, -, +
ExprPtr TypecheckVisitor::evaluateStaticUnary(UnaryExpr *expr) {
  // Case: static strings
  if (expr->expr->staticValue.type == StaticValue::STRING) {
    if (expr->op == "!") {
      if (expr->expr->staticValue.evaluated) {
        return transform(N<BoolExpr>(expr->expr->staticValue.getString().empty()));
      } else {
        // Cannot be evaluated yet: just set the type
        unify(expr->type, ctx->getType("bool"));
        if (!expr->isStatic())
          expr->staticValue.type = StaticValue::INT;
      }
    }
    return nullptr;
  }

  // Case: static integers
  if (expr->op == "-" || expr->op == "+" || expr->op == "!") {
    if (expr->expr->staticValue.evaluated) {
      int value = expr->expr->staticValue.getInt();
      if (expr->op == "+")
        ;
      else if (expr->op == "-")
        value = -value;
      else
        value = !bool(value);
      if (expr->op == "!")
        return transform(N<BoolExpr>(bool(value)));
      else
        return transform(N<IntExpr>(value));
    } else {
      // Cannot be evaluated yet: just set the type
      unify(expr->type, ctx->getType("int"));
      if (!expr->isStatic())
        expr->staticValue.type = StaticValue::INT;
    }
  }

  return nullptr;
}

/// Evaluate a static binary expression and return the resulting static expression.
/// If the expression cannot be evaluated yet, return nullptr.
/// Supported operators: (strings) +, ==, !=
///                      (ints) <, <=, >, >=, ==, !=, and, or, +, -, *, //, %
ExprPtr TypecheckVisitor::evaluateStaticBinary(BinaryExpr *expr) {
  // Case: static strings
  if (expr->rexpr->staticValue.type == StaticValue::STRING) {
    if (expr->op == "+") {
      // `"a" + "b"` -> `"ab"`
      if (expr->lexpr->staticValue.evaluated && expr->rexpr->staticValue.evaluated) {
        return transform(N<StringExpr>(expr->lexpr->staticValue.getString() +
                                       expr->rexpr->staticValue.getString()));
      } else {
        // Cannot be evaluated yet: just set the type
        if (!expr->isStatic())
          expr->staticValue.type = StaticValue::STRING;
        unify(expr->type, ctx->getType("str"));
      }
    } else {
      // `"a" == "b"` -> `False` (also handles `!=`)
      if (expr->lexpr->staticValue.evaluated && expr->rexpr->staticValue.evaluated) {
        bool eq = expr->lexpr->staticValue.getString() ==
                  expr->rexpr->staticValue.getString();
        return transform(N<BoolExpr>(expr->op == "==" ? eq : !eq));
      } else {
        // Cannot be evaluated yet: just set the type
        if (!expr->isStatic())
          expr->staticValue.type = StaticValue::INT;
        unify(expr->type, ctx->getType("bool"));
      }
    }
    return nullptr;
  }

  // Case: static integers
  if (expr->lexpr->staticValue.evaluated && expr->rexpr->staticValue.evaluated) {
    int64_t lvalue = expr->lexpr->staticValue.getInt();
    int64_t rvalue = expr->rexpr->staticValue.getInt();
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
    else if (expr->op == "//") {
      if (!rvalue)
        error("static division by zero");
      lvalue = lvalue / rvalue;
    } else if (expr->op == "%") {
      if (!rvalue)
        error("static division by zero");
      lvalue = lvalue % rvalue;
    } else {
      seqassert(false, "unknown static operator {}", expr->op);
    }

    if (in(std::set<std::string>{"==", "!=", "<", "<=", ">", ">=", "&&", "||"},
           expr->op))
      return transform(N<BoolExpr>(bool(lvalue)));
    else
      return transform(N<IntExpr>(lvalue));
  } else {
    // Cannot be evaluated yet: just set the type
    if (!expr->isStatic())
      expr->staticValue.type = StaticValue::INT;
    unify(expr->type, ctx->getType("int"));
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
ExprPtr TypecheckVisitor::transformBinarySimple(BinaryExpr *expr) {
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
    if (expr->lexpr->getNone() && expr->rexpr->getNone())
      return transform(N<BoolExpr>(true));
    else if (expr->lexpr->getNone())
      return transform(N<BinaryExpr>(expr->rexpr, "is", expr->lexpr));
  } else if (expr->op == "is not") {
    return transform(N<UnaryExpr>("!", N<BinaryExpr>(expr->lexpr, "is", expr->rexpr)));
  }
  return nullptr;
}

/// Transform a binary `is` expression by checking for type equality. Handle special `is
/// None` cаses as well. See inside for details.
ExprPtr TypecheckVisitor::transformBinaryIs(BinaryExpr *expr) {
  seqassert(expr->op == "is", "not an is binary expression");

  // Case: `is None` expressions
  if (expr->rexpr->getNone()) {
    if (expr->lexpr->getType()->is("NoneType"))
      return transform(N<BoolExpr>(true));
    if (!expr->lexpr->getType()->is(TYPE_OPTIONAL)) {
      // lhs is not optional: `return False`
      return transform(N<BoolExpr>(false));
    } else {
      // lhs is optional: `return lhs.__bool__.__invert__()`
      return transform(N<CallExpr>(
          N<DotExpr>(N<CallExpr>(N<DotExpr>(expr->lexpr, "__bool__")), "__invert__")));
    }
  }

  // Check the type equality (operand types and __raw__ pointers must match).
  auto lc = realize(expr->lexpr->getType());
  auto rc = realize(expr->rexpr->getType());
  if (!lc || !rc) {
    // Types not known: return early
    unify(expr->type, ctx->getType("bool"));
    return nullptr;
  }
  if (!lc->getRecord() && !rc->getRecord()) {
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
std::string TypecheckVisitor::getMagic(const std::string &op) {
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
    error("invalid binary operator '{}'", op);
  return mi->second;
}

/// Transform an in-place binary expression.
/// @example
///   `a op= b` -> `a.__iopmagic__(b)`
/// @param isAtomic if set, use atomic magics if available.
ExprPtr TypecheckVisitor::transformBinaryInplaceMagic(BinaryExpr *expr, bool isAtomic) {
  auto magic = getMagic(expr->op);
  auto lt = expr->lexpr->getType()->getClass();
  auto rt = expr->rexpr->getType()->getClass();
  seqassert(lt && rt, "lhs and rhs types not known");

  FuncTypePtr method = nullptr;

  // Atomic operations: check if `lhs.__atomic_op__(Ptr[lhs], rhs)` exists
  if (isAtomic) {
    auto ptr = ctx->instantiateGeneric(ctx->getType("Ptr"), {lt});
    if ((method = findBestMethod(expr->lexpr.get(), format("__atomic_{}__", magic),
                                 {ptr, rt}))) {
      expr->lexpr = N<CallExpr>(N<IdExpr>("__ptr__"), expr->lexpr);
    }
  }

  // In-place operations: check if `lhs.__iop__(lhs, rhs)` exists
  if (!method && expr->inPlace) {
    method = findBestMethod(expr->lexpr.get(), format("__i{}__", magic), {lt, rt});
  }

  if (method)
    return transform(
        N<CallExpr>(N<IdExpr>(method->ast->name), expr->lexpr, expr->rexpr));
  return nullptr;
}

/// Transform a magic binary expression.
/// @example
///   `a op b` -> `a.__opmagic__(b)`
ExprPtr TypecheckVisitor::transformBinaryMagic(BinaryExpr *expr) {
  auto magic = getMagic(expr->op);
  auto lt = expr->lexpr->getType()->getClass();
  auto rt = expr->rexpr->getType()->getClass();
  seqassert(lt && rt, "lhs and rhs types not known");

  // Normal operations: check if `lhs.__op__(lhs, rhs)` exists
  auto method = findBestMethod(expr->lexpr.get(), format("__{}__", magic), {lt, rt});

  // Right-side magics: check if `rhs.__rop__(rhs, lhs)` exists
  if (!method && (method = findBestMethod(expr->rexpr.get(), format("__r{}__", magic),
                                          {rt, lt}))) {
    swap(expr->lexpr, expr->rexpr);
  }

  if (method) {
    // Normal case: `__magic__(lhs, rhs)`
    return transform(
        N<CallExpr>(N<IdExpr>(method->ast->name), expr->lexpr, expr->rexpr));
  } else if (lt->is("pyobj")) {
    // Special case: call `pyobj._getattr(magic)` on lhs
    return transform(N<CallExpr>(N<CallExpr>(N<DotExpr>(expr->lexpr, "_getattr"),
                                             N<StringExpr>(format("__{}__", magic))),
                                 expr->rexpr));
  } else if (rt->is("pyobj")) {
    // Special case: call `pyobj._getattr(magic)` on rhs
    return transform(N<CallExpr>(N<CallExpr>(N<DotExpr>(expr->rexpr, "_getattr"),
                                             N<StringExpr>(format("__r{}__", magic))),
                                 expr->lexpr));
  }
  return nullptr;
}

/// Given a tuple type and the expression `expr[index]`, check if an `index` is static
/// (integer or slice). If so, statically extract the specified tuple item or a
/// sub-tuple (if the index is a slice).
/// Works only on normal tuples and partial functions.
ExprPtr TypecheckVisitor::transformStaticTupleIndex(ClassType *tuple, ExprPtr &expr,
                                                    ExprPtr &index) {
  if (!tuple->getRecord())
    return nullptr;
  if (!startswith(tuple->name, TYPE_TUPLE) && !startswith(tuple->name, TYPE_PARTIAL))
    return nullptr;

  // Extract the static integer value from expression
  auto getInt = [&](int64_t *o, const ExprPtr &e) {
    if (!e)
      return true;
    auto f = transform(clone(e));
    if (f->staticValue.type == StaticValue::INT) {
      seqassert(f->staticValue.evaluated, "{} not evaluated", e->toString());
      *o = f->staticValue.getInt();
      return true;
    } else if (auto ei = f->getInt()) {
      *o = *(ei->intValue);
      return true;
    }
    return false;
  };

  auto classItem = in(ctx->cache->classes, tuple->name);
  seqassert(classItem, "cannot find class '{}'", tuple->name);
  auto sz = classItem->fields.size();
  int64_t start = 0, stop = sz, step = 1;
  if (getInt(&start, index)) {
    // Case: `tuple[int]`
    int i = translateIndex(start, stop);
    if (i < 0 || i >= stop)
      error("tuple index out of range (expected 0..{}, got {})", stop, i);
    return transform(N<DotExpr>(expr, classItem->fields[i].name));
  } else if (auto slice = CAST(index, SliceExpr)) {
    // Case: `tuple[int:int:int]`
    if (!getInt(&start, slice->start) || !getInt(&stop, slice->stop) ||
        !getInt(&step, slice->step))
      return nullptr;

    // Adjust slice indices (Python slicing rules)
    if (slice->step && !slice->start)
      start = step > 0 ? 0 : sz;
    if (slice->step && !slice->stop)
      stop = step > 0 ? sz : 0;
    sliceAdjustIndices(sz, &start, &stop, step);

    // Generate a sub-tuple
    std::vector<ExprPtr> te;
    for (auto i = start; (step >= 0) ? (i < stop) : (i >= stop); i += step) {
      if (i < 0 || i >= sz)
        error("tuple index out of range (expected 0..{}, got {})", sz, i);
      te.push_back(N<DotExpr>(clone(expr), classItem->fields[i].name));
    }
    return transform(
        N<CallExpr>(N<DotExpr>(format(TYPE_TUPLE "{}", te.size()), "__new__"), te));
  }

  return nullptr;
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
    error("tuple index {} out of bounds (len: {})", idx, len);
  }
  return idx;
}

/// Follow Python slice indexing rules for static tuple indices.
/// Taken from https://github.com/python/cpython/blob/main/Objects/sliceobject.c.
/// Quote (sliceobject.c:269): "this is harder to get right than you might think"
int64_t TypecheckVisitor::sliceAdjustIndices(int64_t length, int64_t *start,
                                             int64_t *stop, int64_t step) {
  if (step == 0)
    error("slice step cannot be 0");

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