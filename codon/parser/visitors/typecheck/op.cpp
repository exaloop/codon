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

void TypecheckVisitor::visit(UnaryExpr *expr) {
  expr->expr = transform(expr->expr);
  if (expr->expr->isStatic()) {
    if (expr->expr->staticValue.type == StaticValue::STRING) {
      if (expr->op == "!") {
        if (expr->expr->staticValue.evaluated) {
          resultExpr =
              transform(N<BoolExpr>(expr->expr->staticValue.getString().empty()));
        } else {
          unify(expr->type, ctx->getType("bool"));
          if (!expr->isStatic())
            expr->staticValue.type = StaticValue::INT;
        }
        return;
      }
    } else if (expr->op == "-" || expr->op == "+" || expr->op == "!") {
      if (expr->expr->staticValue.evaluated) {
        int value = expr->expr->staticValue.getInt();
        if (expr->op == "+")
          ;
        else if (expr->op == "-")
          value = -value;
        else
          value = !bool(value);
        if (expr->op == "!")
          resultExpr = transform(N<BoolExpr>(bool(value)));
        else
          resultExpr = transform(N<IntExpr>(value));
      } else {
        unify(expr->type, ctx->getType("int"));
        if (!expr->isStatic())
          expr->staticValue.type = StaticValue::INT;
      }
      return;
    }
  }
  if (expr->op == "!") {
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
    magic = format("__{}__", magic);
    resultExpr = transform(N<CallExpr>(N<DotExpr>(clone(expr->expr), magic)));
  }
}

void TypecheckVisitor::visit(BinaryExpr *expr) {
  static std::unordered_map<StaticValue::Type, std::unordered_set<std::string>>
      staticOps = {
          {StaticValue::INT,
           {"<", "<=", ">", ">=", "==", "!=", "&&", "||", "+", "-", "*", "//", "%"}},
          {StaticValue::STRING, {"==", "!=", "+"}}};
  if (!(startswith(expr->op, "is") && expr->lexpr->getNone()))
    expr->lexpr = transform(expr->lexpr);
  if (!(startswith(expr->op, "is") && expr->rexpr->getNone()))
    expr->rexpr = transform(expr->rexpr);
  if (expr->lexpr->isStatic() && expr->rexpr->isStatic() &&
      expr->lexpr->staticValue.type == expr->rexpr->staticValue.type &&
      in(staticOps[expr->rexpr->staticValue.type], expr->op)) {
    if (expr->rexpr->staticValue.type == StaticValue::STRING) {
      if (expr->op == "+") {
        if (expr->lexpr->staticValue.evaluated && expr->rexpr->staticValue.evaluated) {
          resultExpr = transform(N<StringExpr>(expr->lexpr->staticValue.getString() +
                                               expr->rexpr->staticValue.getString()));
        } else {
          if (!expr->isStatic())
            expr->staticValue.type = StaticValue::STRING;
          unify(expr->type, ctx->getType("str"));
        }
      } else {
        if (expr->lexpr->staticValue.evaluated && expr->rexpr->staticValue.evaluated) {
          bool eq = expr->lexpr->staticValue.getString() ==
                    expr->rexpr->staticValue.getString();
          resultExpr = transform(N<BoolExpr>(expr->op == "==" ? eq : !eq));
        } else {
          if (!expr->isStatic())
            expr->staticValue.type = StaticValue::INT;
          unify(expr->type, ctx->getType("bool"));
        }
      }
    } else {
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
          seqassert(false, "unknown static operator {}", expr->op); // TODO!
        }
        if (in(std::set<std::string>{"==", "!=", "<", "<=", ">", ">=", "&&", "||"},
               expr->op))
          resultExpr = transform(N<BoolExpr>(bool(lvalue)));
        else
          resultExpr = transform(N<IntExpr>(lvalue));
      } else {
        if (!expr->isStatic())
          expr->staticValue.type = StaticValue::INT;
        unify(expr->type, ctx->getType("int"));
      }
    }
  } else {
    resultExpr = transformBinary(expr);
  }
}

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
  expr->items[0].expr = transform(expr->items[0].expr);

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

void TypecheckVisitor::visit(InstantiateExpr *expr) {
  if (expr->typeExpr->isId(TYPE_CALLABLE)) {
    std::vector<TypePtr> types;
    if (expr->typeParams.size() != 2)
      error("invalid Callable type declaration");
    for (int i = 0; i < expr->typeParams.size(); i++) {
      expr->typeParams[i] = transformType(expr->typeParams[i]);
      if (expr->typeParams[i]->type->isStaticType())
        error("unexpected static type");
      types.push_back(expr->typeParams[i]->type);
    }
    auto typ = ctx->getUnbound();
    typ->getLink()->trait = std::make_shared<CallableTrait>(types);
    unify(expr->type, typ);
  } else {
    expr->typeExpr = transformType(expr->typeExpr);
    TypePtr typ =
        ctx->instantiate(expr->typeExpr->getSrcInfo(), expr->typeExpr->getType());
    seqassert(typ->getClass(), "unknown type");
    auto &generics = typ->getClass()->generics;
    if (expr->typeParams.size() != generics.size())
      error("expected {} generics and/or statics", generics.size());

    for (int i = 0; i < expr->typeParams.size(); i++) {
      expr->typeParams[i] = transform(expr->typeParams[i], true);
      TypePtr t = nullptr;
      if (expr->typeParams[i]->isStatic()) {
        t = std::make_shared<StaticType>(expr->typeParams[i], ctx);
      } else {
        if (expr->typeParams[i]->getNone())
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

  // If this is realizable, use the realized name (e.g. use Id("Ptr[byte]") instead of
  // Instantiate(Ptr, {byte})).
  if (auto rt = realize(expr->type)) {
    unify(expr->type, rt);
    resultExpr = N<IdExpr>(expr->type->realizedName());
    unify(resultExpr->type, rt);
    resultExpr->done = true;
    if (expr->typeExpr->isType())
      resultExpr->markType();
  }
}

void TypecheckVisitor::visit(SliceExpr *expr) {
  ExprPtr none = N<CallExpr>(N<DotExpr>(TYPE_OPTIONAL, "__new__"));
  resultExpr = transform(N<CallExpr>(
      N<IdExpr>(TYPE_SLICE), expr->start ? expr->start : clone(none),
      expr->stop ? expr->stop : clone(none), expr->step ? expr->step : clone(none)));
}

void TypecheckVisitor::visit(IndexExpr *expr) {
  if (expr->expr->isId("Static")) {
    auto typ = ctx->getUnbound();
    typ->isStatic = expr->index->isId("str") ? 1 : 2;
    unify(expr->type, typ);
    expr->done = true;
    return;
  }

  expr->expr = transform(expr->expr, true);
  auto typ = expr->expr->getType();
  seqassert(!expr->expr->isType(), "index not converted to instantiate");
  if (auto c = typ->getClass()) {
    // Case 1: check if this is a static tuple access...
    resultExpr = transformStaticTupleIndex(c.get(), expr->expr, expr->index);
    if (!resultExpr) {
      // Case 2: ... and if not, just call __getitem__.
      ExprPtr e = N<CallExpr>(N<DotExpr>(expr->expr, "__getitem__"), expr->index);
      resultExpr = transform(e, false);
    }
  } else {
    // Case 3: type is still unknown.
    // expr->index = transform(expr->index);
    unify(expr->type, ctx->getUnbound());
  }
}

ExprPtr TypecheckVisitor::transformBinary(BinaryExpr *expr, bool isAtomic,
                                          bool *noReturn) {
  // Table of supported binary operations and the corresponding magic methods.
  auto magics = std::unordered_map<std::string, std::string>{
      {"+", "add"},     {"-", "sub"},       {"*", "mul"},     {"**", "pow"},
      {"/", "truediv"}, {"//", "floordiv"}, {"@", "matmul"},  {"%", "mod"},
      {"<", "lt"},      {"<=", "le"},       {">", "gt"},      {">=", "ge"},
      {"==", "eq"},     {"!=", "ne"},       {"<<", "lshift"}, {">>", "rshift"},
      {"&", "and"},     {"|", "or"},        {"^", "xor"},
  };
  if (noReturn)
    *noReturn = false;

  // Case 0: simple transformations
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

  if (expr->lexpr->getType()->getUnbound() ||
      (expr->op != "is" && expr->rexpr->getType()->getUnbound())) {
    // Case 1: If operand types are unknown, continue later (no transformation).
    // Mark the type as unbound.
    unify(expr->type, ctx->getUnbound());
    return nullptr;
  }

  // Check if this is a "a is None" expression. If so, ...
  if (expr->op == "is" && expr->rexpr->getNone()) {
    if (expr->lexpr->getType()->getClass()->name == "NoneType")
      return transform(N<BoolExpr>(true));
    if (expr->lexpr->getType()->getClass()->name != TYPE_OPTIONAL)
      // ... return False if lhs is not an Optional...
      return transform(N<BoolExpr>(false));
    else
      // ... or return lhs.__bool__.__invert__()
      return transform(N<CallExpr>(
          N<DotExpr>(N<CallExpr>(N<DotExpr>(expr->lexpr, "__bool__")), "__invert__")));
  }

  // Check the type equality (operand types and __raw__ pointers must match).
  // TODO: more special cases needed (Optional[T] == Optional[U] if both are None)
  if (expr->op == "is") {
    auto lc = realize(expr->lexpr->getType());
    auto rc = realize(expr->rexpr->getType());
    if (!lc || !rc) {
      // We still do not know the exact types...
      unify(expr->type, ctx->getType("bool"));
      return nullptr;
    } else if (!lc->getRecord() && !rc->getRecord()) {
      return transform(
          N<BinaryExpr>(N<CallExpr>(N<DotExpr>(expr->lexpr, "__raw__")),
                        "==", N<CallExpr>(N<DotExpr>(expr->rexpr, "__raw__"))));
    } else if (lc->getClass()->name == TYPE_OPTIONAL) {
      return transform(
          N<CallExpr>(N<DotExpr>(expr->lexpr, "__is_optional__"), expr->rexpr));
    } else if (rc->getClass()->name == TYPE_OPTIONAL) {
      return transform(
          N<CallExpr>(N<DotExpr>(expr->rexpr, "__is_optional__"), expr->lexpr));
    } else if (lc->realizedName() != rc->realizedName()) {
      return transform(N<BoolExpr>(false));
    } else {
      return transform(N<BinaryExpr>(expr->lexpr, "==", expr->rexpr));
    }
  }

  // Transform a binary expression to a magic method call.
  auto mi = magics.find(expr->op);
  if (mi == magics.end())
    error("invalid binary operator '{}'", expr->op);
  auto magic = mi->second;

  auto lt = expr->lexpr->getType()->getClass();
  auto rt = expr->rexpr->getType()->getClass();
  seqassert(lt && rt, "lhs and rhs types not known");

  FuncTypePtr method = nullptr;
  // Check if lt.__atomic_op__(Ptr[lt], rt) exists.
  if (isAtomic) {
    auto ptrlt =
        ctx->instantiateGeneric(expr->lexpr->getSrcInfo(), ctx->getType("Ptr"), {lt});
    method =
        findBestMethod(expr->lexpr.get(), format("__atomic_{}__", magic), {ptrlt, rt});
    if (method) {
      expr->lexpr = N<CallExpr>(N<IdExpr>("__ptr__"), expr->lexpr);
      if (noReturn)
        *noReturn = true;
    }
  }
  // Check if lt.__iop__(lt, rt) exists.
  if (!method && expr->inPlace) {
    method = findBestMethod(expr->lexpr.get(), format("__i{}__", magic), {lt, rt});
    if (method && noReturn)
      *noReturn = true;
  }
  // Check if lt.__op__(lt, rt) exists.
  if (!method)
    method = findBestMethod(expr->lexpr.get(), format("__{}__", magic), {lt, rt});
  // Check if rt.__rop__(rt, lt) exists.
  if (!method) {
    method = findBestMethod(expr->rexpr.get(), format("__r{}__", magic), {rt, lt});
    if (method)
      swap(expr->lexpr, expr->rexpr);
  }
  if (method) {
    return transform(
        N<CallExpr>(N<IdExpr>(method->ast->name), expr->lexpr, expr->rexpr));
  } else if (lt->is("pyobj")) {
    return transform(N<CallExpr>(N<CallExpr>(N<DotExpr>(expr->lexpr, "_getattr"),
                                             N<StringExpr>(format("__{}__", magic))),
                                 expr->rexpr));
  } else if (rt->is("pyobj")) {
    return transform(N<CallExpr>(N<CallExpr>(N<DotExpr>(expr->rexpr, "_getattr"),
                                             N<StringExpr>(format("__r{}__", magic))),
                                 expr->lexpr));
  }
  error("cannot find magic '{}' in {}", magic, lt->toString());
  return nullptr;
}

ExprPtr TypecheckVisitor::transformStaticTupleIndex(ClassType *tuple, ExprPtr &expr,
                                                    ExprPtr &index) {
  if (!tuple->getRecord())
    return nullptr;
  if (!startswith(tuple->name, TYPE_TUPLE) && !startswith(tuple->name, TYPE_PARTIAL))
    return nullptr;

  // Extract a static integer value from a compatible expression.
  auto getInt = [&](int64_t *o, const ExprPtr &e) {
    if (!e)
      return true;
    auto f = transform(clone(e));
    if (f->staticValue.type == StaticValue::INT) {
      seqassert(f->staticValue.evaluated, "{} not evaluated", e->toString());
      *o = f->staticValue.getInt();
      return true;
    }
    if (auto ei = f->getInt()) {
      *o = *(ei->intValue);
      return true;
    }
    return false;
  };

  auto classItem = ctx->cache->classes.find(tuple->name);
  seqassert(classItem != ctx->cache->classes.end(), "cannot find class '{}'",
            tuple->name);
  auto sz = classItem->second.fields.size();
  int64_t start = 0, stop = sz, step = 1;
  if (getInt(&start, index)) {
    int i = translateIndex(start, stop);
    if (i < 0 || i >= stop)
      error("tuple index out of range (expected 0..{}, got {})", stop, i);
    return transform(N<DotExpr>(expr, classItem->second.fields[i].name));
  } else if (auto es = CAST(index, SliceExpr)) {
    if (!getInt(&start, es->start) || !getInt(&stop, es->stop) ||
        !getInt(&step, es->step))
      return nullptr;
    // Correct slice indices.
    if (es->step && !es->start)
      start = step > 0 ? 0 : sz;
    if (es->step && !es->stop)
      stop = step > 0 ? sz : 0;
    sliceAdjustIndices(sz, &start, &stop, step);
    // Generate new tuple.
    std::vector<ExprPtr> te;
    for (auto i = start; (step >= 0) ? (i < stop) : (i >= stop); i += step) {
      if (i < 0 || i >= sz)
        error("tuple index out of range (expected 0..{}, got {})", sz, i);
      te.push_back(N<DotExpr>(clone(expr), classItem->second.fields[i].name));
    }
    return transform(
        N<CallExpr>(N<DotExpr>(format(TYPE_TUPLE "{}", te.size()), "__new__"), te));
  }
  return nullptr;
}

} // namespace codon::ast