// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "numpy.h"

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace numpy {
namespace {
Type *coerceScalarArray(NumPyType &scalar, NumPyType &array, NumPyPrimitiveTypes &T) {
  auto xtype = scalar.dtype;
  auto atype = array.dtype;
  bool aIsInt = false;
  bool xIsInt = false;
  bool aIsFloat = false;
  bool xIsFloat = false;
  bool aIsComplex = false;
  bool xIsComplex = false;

  switch (atype) {
  case NumPyType::NP_TYPE_ARR_BOOL:
    break;
  case NumPyType::NP_TYPE_ARR_I8:
  case NumPyType::NP_TYPE_ARR_U8:
  case NumPyType::NP_TYPE_ARR_I16:
  case NumPyType::NP_TYPE_ARR_U16:
  case NumPyType::NP_TYPE_ARR_I32:
  case NumPyType::NP_TYPE_ARR_U32:
  case NumPyType::NP_TYPE_ARR_I64:
  case NumPyType::NP_TYPE_ARR_U64:
    aIsInt = true;
    break;
  case NumPyType::NP_TYPE_ARR_F16:
  case NumPyType::NP_TYPE_ARR_F32:
  case NumPyType::NP_TYPE_ARR_F64:
    aIsFloat = true;
    break;
  case NumPyType::NP_TYPE_ARR_C64:
  case NumPyType::NP_TYPE_ARR_C128:
    aIsComplex = true;
    break;
  default:
    seqassertn(false, "unexpected type");
  }

  xIsInt = (xtype == NumPyType::NP_TYPE_BOOL || xtype == NumPyType::NP_TYPE_I64);
  xIsFloat = (xtype == NumPyType::NP_TYPE_F64);
  xIsComplex = (xtype == NumPyType::NP_TYPE_C128);

  bool shouldCast =
      ((xIsInt && (aIsInt || aIsFloat || aIsComplex)) ||
       (xIsFloat && (aIsFloat || aIsComplex)) || (xIsComplex && aIsComplex));

  if ((atype == NumPyType::NP_TYPE_ARR_F16 || atype == NumPyType::NP_TYPE_ARR_F32) &&
      xtype == NumPyType::NP_TYPE_C128)
    return T.c64;
  else if (shouldCast)
    return array.getIRBaseType(T);
  else
    return scalar.getIRBaseType(T);
}

bool isPythonScalar(NumPyType &t) {
  if (t.isArray())
    return false;
  auto dt = t.dtype;
  return (dt == NumPyType::NP_TYPE_BOOL || dt == NumPyType::NP_TYPE_I64 ||
          dt == NumPyType::NP_TYPE_F64 || dt == NumPyType::NP_TYPE_C128);
}

template <typename E>
Type *decideTypes(E *expr, NumPyType &lhs, NumPyType &rhs, NumPyPrimitiveTypes &T) {
  // Special case(s)
  if (expr->op == E::NP_OP_COPYSIGN)
    return expr->type.getIRBaseType(T);

  if (lhs.isArray() && isPythonScalar(rhs))
    return coerceScalarArray(rhs, lhs, T);

  if (isPythonScalar(lhs) && rhs.isArray())
    return coerceScalarArray(lhs, rhs, T);

  auto *t1 = lhs.getIRBaseType(T);
  auto *t2 = rhs.getIRBaseType(T);
  auto *M = t1->getModule();
  auto *coerceFunc = M->getOrRealizeFunc("_coerce", {}, {t1, t2}, FUSION_MODULE);
  seqassertn(coerceFunc, "coerce func not found");
  return util::getReturnType(coerceFunc);
}
} // namespace

void NumPyExpr::replace(NumPyExpr &e) {
  type = e.type;
  val = e.val;
  op = e.op;
  lhs = std::move(e.lhs);
  rhs = std::move(e.rhs);
  third = std::move(e.third);
  ownedLastUse = e.ownedLastUse;

  e.type = {};
  e.val = nullptr;
  e.op = NP_OP_NONE;
  e.lhs = {};
  e.rhs = {};
  e.third = {};
  e.ownedLastUse = false;
}

bool NumPyExpr::haveVectorizedLoop() const {
  if (lhs &&
      (lhs->type.dtype == NumPyType::NP_TYPE_ARR_C64 ||
       lhs->type.dtype == NumPyType::NP_TYPE_ARR_C128) &&
      opstring() == "abs")
    return true;

  if (lhs && !(lhs->type.dtype == NumPyType::NP_TYPE_ARR_F32 ||
               lhs->type.dtype == NumPyType::NP_TYPE_ARR_F64))
    return false;

  if (rhs && !(rhs->type.dtype == NumPyType::NP_TYPE_ARR_F32 ||
               rhs->type.dtype == NumPyType::NP_TYPE_ARR_F64))
    return false;

  if (lhs && rhs && lhs->type.dtype != rhs->type.dtype)
    return false;

  // These are the loops available in the runtime library.
  static const std::vector<std::string> VecLoops = {
      "arccos", "arccosh", "arcsin", "arcsinh", "arctan", "arctanh", "arctan2",
      "cos",    "exp",     "exp2",   "expm1",   "log",    "log10",   "log1p",
      "log2",   "sin",     "sinh",   "tanh",    "hypot"};
  return std::find(VecLoops.begin(), VecLoops.end(), opstring()) != VecLoops.end();
}

int64_t NumPyExpr::opcost() const {
  switch (op) {
  case NP_OP_CAST:
  case NP_OP_ZEROS_LIKE:
  case NP_OP_ONES_LIKE:
  case NP_OP_WHERE:
  case NP_OP_SUM:
  case NP_OP_MEAN:
  case NP_OP_PROD:
  case NP_OP_ANY:
  case NP_OP_ALL:
  case NP_OP_AMIN:
  case NP_OP_AMAX:
    return 1;
  case NP_OP_NONE:
    return 0;
  case NP_OP_POS:
    return 0;
  case NP_OP_NEG:
    return 0;
  case NP_OP_INVERT:
    return 0;
  case NP_OP_ABS:
    return 1;
  case NP_OP_TRANSPOSE:
    return 0;
  case NP_OP_ADD:
    return 1;
  case NP_OP_SUB:
    return 1;
  case NP_OP_MUL:
    return 1;
  case NP_OP_MATMUL:
    return 20;
  case NP_OP_TRUE_DIV:
    return 8;
  case NP_OP_FLOOR_DIV:
    return 8;
  case NP_OP_MOD:
    return 8;
  case NP_OP_FMOD:
    return 8;
  case NP_OP_POW:
    return 8;
  case NP_OP_LSHIFT:
    return 1;
  case NP_OP_RSHIFT:
    return 1;
  case NP_OP_AND:
    return 1;
  case NP_OP_OR:
    return 1;
  case NP_OP_XOR:
    return 1;
  case NP_OP_LOGICAL_AND:
    return 1;
  case NP_OP_LOGICAL_OR:
    return 1;
  case NP_OP_LOGICAL_XOR:
    return 1;
  case NP_OP_EQ:
    return 1;
  case NP_OP_NE:
    return 1;
  case NP_OP_LT:
    return 1;
  case NP_OP_LE:
    return 1;
  case NP_OP_GT:
    return 1;
  case NP_OP_GE:
    return 1;
  case NP_OP_MIN:
    return 3;
  case NP_OP_MAX:
    return 3;
  case NP_OP_FMIN:
    return 3;
  case NP_OP_FMAX:
    return 3;
  case NP_OP_SIN:
    return 10;
  case NP_OP_COS:
    return 10;
  case NP_OP_TAN:
    return 10;
  case NP_OP_ARCSIN:
    return 20;
  case NP_OP_ARCCOS:
    return 20;
  case NP_OP_ARCTAN:
    return 20;
  case NP_OP_ARCTAN2:
    return 35;
  case NP_OP_HYPOT:
    return 5;
  case NP_OP_SINH:
    return 10;
  case NP_OP_COSH:
    return 10;
  case NP_OP_TANH:
    return 10;
  case NP_OP_ARCSINH:
    return 10;
  case NP_OP_ARCCOSH:
    return 10;
  case NP_OP_ARCTANH:
    return 10;
  case NP_OP_CONJ:
    return 1;
  case NP_OP_EXP:
    return 5;
  case NP_OP_EXP2:
    return 5;
  case NP_OP_LOG:
    return 5;
  case NP_OP_LOG2:
    return 5;
  case NP_OP_LOG10:
    return 5;
  case NP_OP_EXPM1:
    return 5;
  case NP_OP_LOG1P:
    return 5;
  case NP_OP_SQRT:
    return 2;
  case NP_OP_SQUARE:
    return 1;
  case NP_OP_CBRT:
    return 5;
  case NP_OP_LOGADDEXP:
    return 10;
  case NP_OP_LOGADDEXP2:
    return 10;
  case NP_OP_RECIPROCAL:
    return 1;
  case NP_OP_RINT:
    return 1;
  case NP_OP_FLOOR:
    return 1;
  case NP_OP_CEIL:
    return 1;
  case NP_OP_TRUNC:
    return 1;
  case NP_OP_ISNAN:
    return 1;
  case NP_OP_ISINF:
    return 1;
  case NP_OP_ISFINITE:
    return 1;
  case NP_OP_SIGN:
    return 1;
  case NP_OP_SIGNBIT:
    return 1;
  case NP_OP_COPYSIGN:
    return 1;
  case NP_OP_SPACING:
    return 1;
  case NP_OP_NEXTAFTER:
    return 1;
  case NP_OP_DEG2RAD:
    return 2;
  case NP_OP_RAD2DEG:
    return 2;
  case NP_OP_HEAVISIDE:
    return 3;
  }
}

int64_t NumPyExpr::cost() const {
  auto c = opcost();
  if (c == -1)
    return -1;

  // Special-case for abs(complex)
  if (op == NP_OP_ABS && lhs &&
      (lhs->type.dtype == NumPyType::NP_TYPE_ARR_C128 ||
       lhs->type.dtype == NumPyType::NP_TYPE_ARR_C64))
    c = 50;

  // Account for the fact that the vectorized loops are much faster.
  if (haveVectorizedLoop()) {
    c *= 3;
    if (lhs->type.dtype == NumPyType::NP_TYPE_ARR_F32)
      c *= 2;
    c = std::max<int64_t>(c, 64);
  }

  bool lhsIntConst = (lhs && lhs->isLeaf() && isA<IntConst>(lhs->val));
  bool rhsIntConst = (rhs && rhs->isLeaf() && isA<IntConst>(rhs->val));
  bool lhsFloatConst = (lhs && lhs->isLeaf() && isA<FloatConst>(lhs->val));
  bool rhsFloatConst = (rhs && rhs->isLeaf() && isA<FloatConst>(rhs->val));
  bool lhsConst = lhsIntConst || lhsFloatConst;
  bool rhsConst = rhsIntConst || rhsFloatConst;

  if (rhsConst || lhsConst) {
    switch (op) {
    case NP_OP_TRUE_DIV:
    case NP_OP_FLOOR_DIV:
    case NP_OP_MOD:
    case NP_OP_FMOD:
      c = 1;
      break;
    case NP_OP_POW:
      if (rhsIntConst)
        c = (cast<IntConst>(rhs->val)->getVal() == 2) ? 1 : 5;
      break;
    default:
      break;
    }
  }

  if (lhs) {
    auto cl = lhs->cost();
    if (cl == -1)
      return -1;
    c += cl;
  }

  if (rhs) {
    auto cr = rhs->cost();
    if (cr == -1)
      return -1;
    c += cr;
  }

  if (third) {
    auto thirdCost = third->cost();
    if (thirdCost == -1)
      return -1;
    c += thirdCost;
  }

  return c;
}

std::string NumPyExpr::opstring() const {
  static const std::unordered_map<Op, std::string> m = {
      {NP_OP_NONE, "a"},
      {NP_OP_POS, "pos"},
      {NP_OP_NEG, "neg"},
      {NP_OP_INVERT, "invert"},
      {NP_OP_ABS, "abs"},
      {NP_OP_TRANSPOSE, "transpose"},
      {NP_OP_ADD, "add"},
      {NP_OP_SUB, "sub"},
      {NP_OP_MUL, "mul"},
      {NP_OP_MATMUL, "matmul"},
      {NP_OP_TRUE_DIV, "true_div"},
      {NP_OP_FLOOR_DIV, "floor_div"},
      {NP_OP_MOD, "mod"},
      {NP_OP_FMOD, "fmod"},
      {NP_OP_POW, "pow"},
      {NP_OP_LSHIFT, "lshift"},
      {NP_OP_RSHIFT, "rshift"},
      {NP_OP_AND, "and"},
      {NP_OP_OR, "or"},
      {NP_OP_XOR, "xor"},
      {NP_OP_LOGICAL_AND, "logical_and"},
      {NP_OP_LOGICAL_OR, "logical_or"},
      {NP_OP_LOGICAL_XOR, "logical_xor"},
      {NP_OP_EQ, "eq"},
      {NP_OP_NE, "ne"},
      {NP_OP_LT, "lt"},
      {NP_OP_LE, "le"},
      {NP_OP_GT, "gt"},
      {NP_OP_GE, "ge"},
      {NP_OP_MIN, "minimum"},
      {NP_OP_MAX, "maximum"},
      {NP_OP_FMIN, "fmin"},
      {NP_OP_FMAX, "fmax"},
      {NP_OP_SIN, "sin"},
      {NP_OP_COS, "cos"},
      {NP_OP_TAN, "tan"},
      {NP_OP_ARCSIN, "arcsin"},
      {NP_OP_ARCCOS, "arccos"},
      {NP_OP_ARCTAN, "arctan"},
      {NP_OP_ARCTAN2, "arctan2"},
      {NP_OP_HYPOT, "hypot"},
      {NP_OP_SINH, "sinh"},
      {NP_OP_COSH, "cosh"},
      {NP_OP_TANH, "tanh"},
      {NP_OP_ARCSINH, "arcsinh"},
      {NP_OP_ARCCOSH, "arccosh"},
      {NP_OP_ARCTANH, "arctanh"},
      {NP_OP_CONJ, "conj"},
      {NP_OP_EXP, "exp"},
      {NP_OP_EXP2, "exp2"},
      {NP_OP_LOG, "log"},
      {NP_OP_LOG2, "log2"},
      {NP_OP_LOG10, "log10"},
      {NP_OP_EXPM1, "expm1"},
      {NP_OP_LOG1P, "log1p"},
      {NP_OP_SQRT, "sqrt"},
      {NP_OP_SQUARE, "square"},
      {NP_OP_CBRT, "cbrt"},
      {NP_OP_LOGADDEXP, "logaddexp"},
      {NP_OP_LOGADDEXP2, "logaddexp2"},
      {NP_OP_RECIPROCAL, "reciprocal"},
      {NP_OP_RINT, "rint"},
      {NP_OP_FLOOR, "floor"},
      {NP_OP_CEIL, "ceil"},
      {NP_OP_TRUNC, "trunc"},
      {NP_OP_ISNAN, "isnan"},
      {NP_OP_ISINF, "isinf"},
      {NP_OP_ISFINITE, "isfinite"},
      {NP_OP_SIGN, "sign"},
      {NP_OP_SIGNBIT, "signbit"},
      {NP_OP_COPYSIGN, "copysign"},
      {NP_OP_SPACING, "spacing"},
      {NP_OP_NEXTAFTER, "nextafter"},
      {NP_OP_DEG2RAD, "deg2rad"},
      {NP_OP_RAD2DEG, "rad2deg"},
      {NP_OP_HEAVISIDE, "heaviside"},
      {NP_OP_CAST, "cast"},
      {NP_OP_ZEROS_LIKE, "zeros_like"},
      {NP_OP_ONES_LIKE, "ones_like"},
      {NP_OP_WHERE, "where"},
      {NP_OP_SUM, "sum"},
      {NP_OP_MEAN, "mean"},
      {NP_OP_PROD, "prod"},
      {NP_OP_ANY, "any"},
      {NP_OP_ALL, "all"},
      {NP_OP_AMIN, "amin"},
      {NP_OP_AMAX, "amax"},
  };

  auto it = m.find(op);
  seqassertn(it != m.end(), "op not found");
  return it->second;
}

void NumPyExpr::dump(std::ostream &os, int level, int &leafId) const {
  auto indent = [&]() {
    for (int i = 0; i < level; i++)
      os << "  ";
  };

  indent();
  if (op == NP_OP_NONE) {
    os << "\033[1;36m" << opstring() << leafId;
    ++leafId;
  } else {
    os << "\033[1;33m" << opstring();
  }
  os << "\033[0m <" << type << ">";
  if (op != NP_OP_NONE)
    os << " \033[1;35m[cost=" << cost() << "]\033[0m";
  os << "\n";
  if (lhs)
    lhs->dump(os, level + 1, leafId);
  if (rhs)
    rhs->dump(os, level + 1, leafId);
  if (third)
    third->dump(os, level + 1, leafId);
}

std::ostream &operator<<(std::ostream &os, NumPyExpr const &expr) {
  int leafId = 0;
  expr.dump(os, 0, leafId);
  return os;
}

std::string NumPyExpr::str() const {
  std::stringstream buffer;
  buffer << *this;
  return buffer.str();
}

void NumPyExpr::apply(std::function<void(NumPyExpr &)> f) {
  f(*this);
  if (lhs)
    lhs->apply(f);
  if (rhs)
    rhs->apply(f);
  if (third)
    third->apply(f);
}

Value *NumPyExpr::codegenBroadcasts(CodegenContext &C) {
  auto *M = C.M;
  auto &vars = C.vars;

  Value *targetShape = nullptr;
  Value *result = nullptr;

  apply([&](NumPyExpr &e) {
    if (e.isLeaf() && e.type.isArray()) {
      auto it = vars.find(&e);
      seqassertn(it != vars.end(),
                 "NumPyExpr not found in vars map (codegen broadcasts)");
      auto *var = it->second;
      auto *shape = M->getOrRealizeFunc("_shape", {var->getType()}, {}, FUSION_MODULE);
      seqassertn(shape, "shape function not found");
      auto *leafShape = util::call(shape, {M->Nr<VarValue>(var)});

      if (!targetShape) {
        targetShape = leafShape;
      } else {
        auto *diff = (*targetShape != *leafShape);
        if (result) {
          result = *result | *diff;
        } else {
          result = diff;
        }
      }
    }
  });

  return result ? result : M->getBool(false);
}

// Plan shapes and strides without materializing producer arrays. Fusion needs
// these descriptors to preserve layout-dependent traversal and reduction order.
Var *NumPyExpr::codegenLayout(CodegenContext &C) {
  if (isLeaf())
    return C.vars.at(this);
  auto *M = C.M;
  auto *baseType = type.getIRBaseType(C.T);
  std::vector<Value *> operands;
  if (op == NP_OP_WHERE) {
    std::vector<Type *> operandTypes;
    for (auto *operand : {lhs.get(), rhs.get(), third.get()}) {
      auto *layout = operand->codegenLayout(C);
      operands.push_back(M->Nr<VarValue>(layout));
      operandTypes.push_back(layout->getType());
    }
    auto *layoutFunc =
        M->getOrRealizeFunc("_where_layout", operandTypes, {baseType}, FUSION_MODULE);
    seqassertn(layoutFunc, "where layout func not found");
    return util::makeVar(util::call(layoutFunc, operands), C.series, C.func);
  }
  if (lhs && lhs->type.isArray())
    operands.push_back(M->Nr<VarValue>(lhs->codegenLayout(C)));
  if (rhs && rhs->type.isArray())
    operands.push_back(M->Nr<VarValue>(rhs->codegenLayout(C)));
  Func *layoutFunc = nullptr;
  if (op == NP_OP_CAST || op == NP_OP_ZEROS_LIKE || op == NP_OP_ONES_LIKE) {
    auto *call = cast<CallInstr>(val);
    auto *order = cast<StringConst>(*std::next(call->begin()));
    auto *copy = call->numArgs() == 3 ? cast<BoolConst>(call->back()) : nullptr;
    operands.push_back(M->getBool(!copy || copy->getVal()));
    layoutFunc = M->getOrRealizeFunc(
        "_producer_layout", {operands[0]->getType(), M->getBoolType()},
        {baseType, opstring(), order->getVal()}, FUSION_MODULE);
  } else {
    auto *arrays = util::makeTuple(operands);
    operands = {arrays};
    layoutFunc =
        M->getOrRealizeFunc("_layout", {arrays->getType()}, {baseType}, FUSION_MODULE);
  }
  seqassertn(layoutFunc, "fusion layout func not found for {}", opstring());
  return util::makeVar(util::call(layoutFunc, operands), C.series, C.func);
}

// Build a scalar callback for one element, then let a stdlib loop helper handle
// broadcasting, strides and allocation (or reduction / writing into a destination).
Var *NumPyExpr::codegenFusedEval(CodegenContext &C, Var *destination) {
  auto *M = C.M;
  auto *series = C.series;
  auto *func = C.func;
  auto &vars = C.vars;
  auto &T = C.T;
  auto *element = isReduction() ? lhs.get() : this;

  std::vector<std::pair<NumPyExpr *, Var *>> leaves;
  element->apply([&](NumPyExpr &e) {
    if (e.isLeaf()) {
      auto it = vars.find(&e);
      seqassertn(it != vars.end(), "NumPyExpr not found in vars map (fused eval)");
      auto *var = it->second;
      leaves.emplace_back(&e, var);
    }
  });

  // Arrays for scalar expression function
  std::vector<Value *> arrays;
  std::vector<std::string> scalarFuncArgNames;
  std::vector<Type *> scalarFuncArgTypes;
  std::unordered_map<NumPyExpr *, Var *> scalarFuncArgMap;

  // Scalars passed through 'extra' arg of ndarray._loop()
  std::vector<Value *> extra;
  std::unordered_map<NumPyExpr *, unsigned> extraMap;

  auto *baseType = element->type.getIRBaseType(T);
  if (!isReduction()) {
    scalarFuncArgNames.push_back("out");
    scalarFuncArgTypes.push_back(M->getPointerType(baseType));
  }

  unsigned argIdx = 0;
  unsigned extraIdx = 0;

  for (auto &e : leaves) {
    if (e.first->type.isArray()) {
      arrays.push_back(M->Nr<VarValue>(e.second));
      scalarFuncArgNames.push_back("in" + std::to_string(argIdx++));
      scalarFuncArgTypes.push_back(M->getPointerType(e.first->type.getIRBaseType(T)));
    } else {
      extra.push_back(M->Nr<VarValue>(e.second));
      extraMap.emplace(e.first, extraIdx++);
    }
  }

  auto *extraTuple = util::makeTuple(extra, M);
  scalarFuncArgNames.push_back("extra");
  scalarFuncArgTypes.push_back(extraTuple->getType());
  auto *scalarFuncType =
      M->getFuncType(isReduction() ? baseType : M->getNoneType(), scalarFuncArgTypes);
  auto *scalarFunc = M->Nr<BodiedFunc>("__numpy_fusion_scalar_fn");
  scalarFunc->realize(scalarFuncType, scalarFuncArgNames);
  std::vector<Var *> scalarFuncArgVars(scalarFunc->arg_begin(), scalarFunc->arg_end());

  argIdx = isReduction() ? 0 : 1;
  for (auto &e : leaves) {
    if (e.first->type.isArray()) {
      scalarFuncArgMap.emplace(e.first, scalarFuncArgVars[argIdx++]);
    }
  }
  auto *scalarExpr = element->codegenScalarExpr(C, scalarFuncArgMap, extraMap,
                                                scalarFuncArgVars.back());
  if (isReduction()) {
    auto *castFunc = M->getOrRealizeFunc("_cast", {scalarExpr->getType()}, {baseType},
                                         FUSION_MODULE);
    scalarFunc->setBody(
        util::series(M->Nr<ReturnInstr>(util::call(castFunc, {scalarExpr}))));
  } else {
    auto *ptrsetFunc = M->getOrRealizeFunc("_ptrset", {scalarFuncArgTypes[0], baseType},
                                           {}, FUSION_MODULE);
    seqassertn(ptrsetFunc, "ptrset func not found");
    scalarFunc->setBody(util::series(
        util::call(ptrsetFunc, {M->Nr<VarValue>(scalarFuncArgVars[0]), scalarExpr})));
  }

  auto *arraysTuple = util::makeTuple(arrays);
  std::vector<Value *> loopArgs = {arraysTuple, M->Nr<VarValue>(scalarFunc),
                                   extraTuple};
  std::vector<Type *> loopTypes = {arraysTuple->getType(), scalarFunc->getType(),
                                   extraTuple->getType()};
  std::vector<Generic> loopGenerics = {baseType};
  if (isReduction()) {
    Value *initial = nullptr;
    if (rhs)
      initial = M->Nr<VarValue>(vars.at(rhs.get()));
    else if (op == NP_OP_MEAN) {
      auto *castFunc = M->getOrRealizeFunc("_cast", {M->getIntType()},
                                           {type.getIRBaseType(T)}, FUSION_MODULE);
      initial = util::call(castFunc, {M->getInt(0)});
    } else
      initial = util::makeTuple({}, M);
    loopArgs.push_back(initial);
    loopTypes.push_back(initial->getType());
    loopGenerics = {baseType, type.getIRBaseType(T), opstring()};
  }
  bool needsLayout = false;
  element->apply([&](NumPyExpr &expr) {
    if (expr.op == NP_OP_WHERE ||
        (expr.type.ndim > 1 && (expr.op == NP_OP_CAST || expr.op == NP_OP_ZEROS_LIKE ||
                                expr.op == NP_OP_ONES_LIKE)))
      needsLayout = true;
  });
  // Dispatch this callback to an allocating loop, a reduction, or a destination
  // write. Add layout planning where shapes and traversal order must be preserved.
  std::string loopName = isReduction() ? "_loop_reduce" : "_loop_alloc";
  auto *reductionCall = isReduction() ? cast<CallInstr>(val) : nullptr;
  auto *axis = reductionCall ? *std::next(reductionCall->begin()) : nullptr;
  bool axisReduction =
      isReduction() && (type.isArray() || !axis->getType()->is(T.none));
  if (needsLayout || axisReduction) {
    auto *layout = element->codegenLayout(C);
    loopArgs.push_back(M->Nr<VarValue>(layout));
    loopTypes.push_back(layout->getType());
    loopName += "_layout";
  }
  if (axisReduction) {
    util::CloneVisitor clone(M);
    loopArgs.push_back(clone.clone(axis));
    loopTypes.push_back(axis->getType());
    loopGenerics.push_back(int64_t(type.ndim));
    loopName = "_loop_reduce_axis";
  }
  if (destination) {
    loopArgs.push_back(M->Nr<VarValue>(destination));
    loopTypes.push_back(destination->getType());
    loopGenerics.clear();
    loopName = "_loop_into";
  }
  auto *loopFunc =
      M->getOrRealizeFunc(loopName, loopTypes, loopGenerics, FUSION_MODULE);
  seqassertn(loopFunc, "fusion loop func not found");

  auto *result = util::makeVar(util::call(loopFunc, loopArgs), series, func);

  // Release only after the entire fused loop. Ownership proofs must exclude
  // duplicate owners: this walks expression leaves, not unique data pointers.
  apply([&](NumPyExpr &e) {
    if (e.isLeaf() && e.ownedLastUse) {
      auto it = vars.find(&e);
      seqassertn(it != vars.end(), "NumPyExpr not found in vars map (fused eval)");
      auto *var = it->second;
      std::vector<Value *> freeArgs = {M->Nr<VarValue>(var)};
      std::vector<Type *> freeTypes = {var->getType()};
      if (needsLayout && !isReduction()) {
        freeArgs.push_back(M->Nr<VarValue>(result));
        freeTypes.push_back(result->getType());
      }
      auto *freeFunc = M->getOrRealizeFunc(
          needsLayout && !isReduction() ? "_free_copy_source" : "_free", freeTypes, {},
          FUSION_MODULE);
      seqassertn(freeFunc, "free func not found");
      series->push_back(util::call(freeFunc, freeArgs));
    }
  });

  return result;
}

// Evaluate whole-array operations one node at a time when fusion is not chosen.
// Owned temporaries can still be recycled if their dtype, rank and runtime shape fit.
Var *NumPyExpr::codegenSequentialEval(CodegenContext &C) {
  auto *M = C.M;
  auto *series = C.series;
  auto *func = C.func;
  auto &vars = C.vars;
  auto &T = C.T;

  auto freeArray = [&](Var *arr) {
    auto *freeFunc = M->getOrRealizeFunc("_free", {arr->getType()}, {}, FUSION_MODULE);
    seqassertn(freeFunc, "free func not found");
    return util::call(freeFunc, {M->Nr<VarValue>(arr)});
  };

  if (isLeaf()) {
    auto it = vars.find(this);
    seqassertn(it != vars.end(),
               "NumPyExpr not found in vars map (codegen sequential eval)");
    return it->second;
  }

  Var *lv = lhs->codegenSequentialEval(C);
  Var *rv = rhs ? rhs->codegenSequentialEval(C) : nullptr;
  Var *like = nullptr;
  Value *outShapeVal = nullptr;

  // Ownership permits release; matching dtype/rank and the shape checks below
  // additionally permit reuse. Borrowed leaves remain read-only even at last use.
  bool lfreeable = lhs && lhs->type.isArray() && (lhs->ownedLastUse || !lhs->isLeaf());
  bool rfreeable = rhs && rhs->type.isArray() && (rhs->ownedLastUse || !rhs->isLeaf());
  bool ltmp = lfreeable && lhs->type.dtype == type.dtype && lhs->type.ndim == type.ndim;
  bool rtmp = rfreeable && rhs->type.dtype == type.dtype && rhs->type.ndim == type.ndim;

  if (op == NP_OP_WHERE) {
    auto *thirdValue = third->codegenSequentialEval(C);
    auto *call = cast<CallInstr>(val);
    auto *result = util::makeVar(util::call(util::getFunc(call->getCallee()),
                                            {M->Nr<VarValue>(lv), M->Nr<VarValue>(rv),
                                             M->Nr<VarValue>(thirdValue)}),
                                 series, func);
    if (lfreeable)
      series->push_back(freeArray(lv));
    if (rfreeable)
      series->push_back(freeArray(rv));
    if (third->type.isArray() && (third->ownedLastUse || !third->isLeaf()))
      series->push_back(freeArray(thirdValue));
    return result;
  }

  if (type.ndim > 1 &&
      (op == NP_OP_CAST || op == NP_OP_ZEROS_LIKE || op == NP_OP_ONES_LIKE)) {
    auto *call = cast<CallInstr>(val);
    std::vector<Value *> args(call->begin(), call->end());
    args[0] = M->Nr<VarValue>(lv);
    auto *result =
        util::makeVar(util::call(util::getFunc(call->getCallee()), args), series, func);
    if (lfreeable) {
      auto *freeSource = M->getOrRealizeFunc(
          "_free_copy_source", {lv->getType(), result->getType()}, {}, FUSION_MODULE);
      seqassertn(freeSource, "free copy source func not found");
      series->push_back(
          util::call(freeSource, {M->Nr<VarValue>(lv), M->Nr<VarValue>(result)}));
    }
    return result;
  }

  if (rv) {
    // Can't do anything special with matmul here...
    if (op == NP_OP_MATMUL) {
      auto *matmul = M->getOrRealizeFunc("_matmul", {lv->getType(), rv->getType()}, {},
                                         FUSION_MODULE);
      auto *ret = util::makeVar(
          util::call(matmul, {M->Nr<VarValue>(lv), M->Nr<VarValue>(rv)}), series, func);
      if (lfreeable)
        series->push_back(freeArray(lv));
      if (rfreeable)
        series->push_back(freeArray(rv));
      return ret;
    }

    auto *lshape = M->getOrRealizeFunc("_shape", {lv->getType()}, {}, FUSION_MODULE);
    seqassertn(lshape, "shape func not found for left arg");
    auto *rshape = M->getOrRealizeFunc("_shape", {rv->getType()}, {}, FUSION_MODULE);
    seqassertn(rshape, "shape func not found for right arg");
    auto *leftShape = util::call(lshape, {M->Nr<VarValue>(lv)});
    auto *rightShape = util::call(rshape, {M->Nr<VarValue>(rv)});
    auto *shape = M->getOrRealizeFunc(
        "_broadcast", {leftShape->getType(), rightShape->getType()}, {}, FUSION_MODULE);
    seqassertn(shape, "output shape func not found");
    like = rhs->type.ndim > lhs->type.ndim ? rv : lv;
    outShapeVal = util::call(shape, {leftShape, rightShape});
  } else {
    auto *shape = M->getOrRealizeFunc("_shape", {lv->getType()}, {}, FUSION_MODULE);
    seqassertn(shape, "shape func not found");
    like = lv;
    outShapeVal = util::call(shape, {M->Nr<VarValue>(lv)});
  }

  auto *outShape = util::makeVar(outShapeVal, series, func);
  Var *result = nullptr;

  auto *t = type.getIRBaseType(T);
  auto newArray = [&]() {
    auto *create = M->getOrRealizeFunc(
        "_create", {like->getType(), outShape->getType()}, {t}, FUSION_MODULE);
    seqassertn(create, "create func not found");
    return util::call(create, {M->Nr<VarValue>(like), M->Nr<VarValue>(outShape)});
  };

  bool freeLeftStatic = false;
  bool freeRightStatic = false;
  Var *lcond = nullptr;
  Var *rcond = nullptr;

  if (rv) {
    if (ltmp && rhs->type.ndim == 0) {
      // We are adding lhs temp array to const or 0-dim array, so reuse lhs array.
      result = lv;
    } else if (rtmp && lhs->type.ndim == 0) {
      // We are adding rhs temp array to const or 0-dim array, so reuse rhs array.
      result = rv;
    } else if (!ltmp && !rtmp) {
      // Neither operand is a temp array, so we must allocate a new array.
      result = util::makeVar(newArray(), series, func);
      freeLeftStatic = lfreeable;
      freeRightStatic = rfreeable;
    } else if (ltmp && rtmp) {
      // We won't know until runtime if we can reuse the temp array(s) since they
      // might broadcast.
      auto *lshape = M->getOrRealizeFunc("_shape", {lv->getType()}, {}, FUSION_MODULE);
      seqassertn(lshape, "shape function func not found for left arg");
      auto *rshape = M->getOrRealizeFunc("_shape", {rv->getType()}, {}, FUSION_MODULE);
      seqassertn(rshape, "shape function func not found for right arg");
      auto *leftShape = util::call(lshape, {M->Nr<VarValue>(lv)});
      auto *rightShape = util::call(rshape, {M->Nr<VarValue>(rv)});
      lcond = util::makeVar(*leftShape == *M->Nr<VarValue>(outShape), series, func);
      rcond = util::makeVar(*rightShape == *M->Nr<VarValue>(outShape), series, func);
      auto *arr = M->Nr<TernaryInstr>(
          M->Nr<VarValue>(lcond), M->Nr<VarValue>(lv),
          M->Nr<TernaryInstr>(M->Nr<VarValue>(rcond), M->Nr<VarValue>(rv), newArray()));
      result = util::makeVar(arr, series, func);
    } else if (ltmp && !rtmp) {
      // We won't know until runtime if we can reuse the temp array(s) since they
      // might broadcast.
      auto *lshape = M->getOrRealizeFunc("_shape", {lv->getType()}, {}, FUSION_MODULE);
      seqassertn(lshape, "shape function func not found for left arg");
      auto *leftShape = util::call(lshape, {M->Nr<VarValue>(lv)});
      lcond = util::makeVar(*leftShape == *M->Nr<VarValue>(outShape), series, func);
      auto *arr =
          M->Nr<TernaryInstr>(M->Nr<VarValue>(lcond), M->Nr<VarValue>(lv), newArray());
      result = util::makeVar(arr, series, func);
      freeRightStatic = rfreeable;
    } else if (!ltmp && rtmp) {
      // We won't know until runtime if we can reuse the temp array(s) since they
      // might broadcast.
      auto *rshape = M->getOrRealizeFunc("_shape", {rv->getType()}, {}, FUSION_MODULE);
      seqassertn(rshape, "shape function func not found for right arg");
      auto *rightShape = util::call(rshape, {M->Nr<VarValue>(rv)});
      rcond = util::makeVar(*rightShape == *M->Nr<VarValue>(outShape), series, func);
      auto *arr =
          M->Nr<TernaryInstr>(M->Nr<VarValue>(rcond), M->Nr<VarValue>(rv), newArray());
      result = util::makeVar(arr, series, func);
      freeLeftStatic = lfreeable;
    }
  } else {
    if (ltmp) {
      result = lv;
    } else {
      result = util::makeVar(newArray(), series, func);
      freeLeftStatic = lfreeable;
    }
  }

  auto opstr = opstring();

  if (haveVectorizedLoop()) {
    // We have a vectorized loop available for this operations.
    if (rv) {
      auto *vecloop = M->getOrRealizeFunc(
          "_apply_vectorized_loop_binary",
          {lv->getType(), rv->getType(), result->getType()}, {opstr}, FUSION_MODULE);
      seqassertn(vecloop, "binary vec loop func not found ({})", opstr);
      series->push_back(util::call(vecloop, {M->Nr<VarValue>(lv), M->Nr<VarValue>(rv),
                                             M->Nr<VarValue>(result)}));
    } else {
      auto *vecloop = M->getOrRealizeFunc("_apply_vectorized_loop_unary",
                                          {lv->getType(), result->getType()}, {opstr},
                                          FUSION_MODULE);
      seqassertn(vecloop, "unary vec loop func not found ({})", opstr);
      series->push_back(
          util::call(vecloop, {M->Nr<VarValue>(lv), M->Nr<VarValue>(result)}));
    }
  } else {
    // Arrays for scalar expression function
    std::vector<Value *> arrays = {M->Nr<VarValue>(result)};
    std::vector<std::string> scalarFuncArgNames;
    std::vector<Type *> scalarFuncArgTypes;
    std::unordered_map<NumPyExpr *, Var *> scalarFuncArgMap;

    // Scalars passed through 'extra' arg of ndarray._loop()
    std::vector<Value *> extra;

    auto *baseType = type.getIRBaseType(T);
    scalarFuncArgNames.push_back("out");
    scalarFuncArgTypes.push_back(M->getPointerType(baseType));

    if (lhs->type.isArray()) {
      if (result != lv) {
        scalarFuncArgNames.push_back("in0");
        scalarFuncArgTypes.push_back(M->getPointerType(lhs->type.getIRBaseType(T)));
        arrays.push_back(M->Nr<VarValue>(lv));
      }
    } else {
      extra.push_back(M->Nr<VarValue>(lv));
    }

    if (rv) {
      if (rhs->type.isArray()) {
        if (result != rv) {
          scalarFuncArgNames.push_back("in1");
          scalarFuncArgTypes.push_back(M->getPointerType(rhs->type.getIRBaseType(T)));
          arrays.push_back(M->Nr<VarValue>(rv));
        }
      } else {
        extra.push_back(M->Nr<VarValue>(rv));
      }
    }

    auto *extraTuple = util::makeTuple(extra, M);
    scalarFuncArgNames.push_back("extra");
    scalarFuncArgTypes.push_back(extraTuple->getType());
    auto *scalarFuncType = M->getFuncType(M->getNoneType(), scalarFuncArgTypes);
    auto *scalarFunc = M->Nr<BodiedFunc>("__numpy_fusion_scalar_fn");
    scalarFunc->realize(scalarFuncType, scalarFuncArgNames);
    std::vector<Var *> scalarFuncArgVars(scalarFunc->arg_begin(),
                                         scalarFunc->arg_end());
    auto *body = M->Nr<SeriesFlow>();
    auto name = "_" + opstr;

    auto deref = [&](unsigned idx) {
      return (*M->Nr<VarValue>(scalarFuncArgVars[idx]))[*M->getInt(0)];
    };

    if (rv) {
      Value *litem = nullptr;
      Value *ritem = nullptr;

      if (lhs->type.isArray() && rhs->type.isArray()) {
        if (result == lv) {
          litem = deref(0);
          ritem = deref(1);
        } else if (result == rv) {
          litem = deref(1);
          ritem = deref(0);
        } else {
          litem = deref(1);
          ritem = deref(2);
        }
      } else if (lhs->type.isArray()) {
        if (result == lv) {
          litem = deref(0);
        } else {
          litem = deref(1);
        }
        ritem = util::tupleGet(M->Nr<VarValue>(scalarFuncArgVars.back()), 0);
      } else if (rhs->type.isArray()) {
        if (result == rv) {
          ritem = deref(0);
        } else {
          ritem = deref(1);
        }
        litem = util::tupleGet(M->Nr<VarValue>(scalarFuncArgVars.back()), 0);
      } else {
        seqassertn(false, "both lhs are rhs are scalars");
      }

      auto *commonType = decideTypes(this, lhs->type, rhs->type, T);

      auto *lcast =
          M->getOrRealizeFunc("_cast", {litem->getType()}, {commonType}, FUSION_MODULE);
      seqassertn(lcast, "cast func not found for left arg");
      litem = util::call(lcast, {litem});

      auto *rcast =
          M->getOrRealizeFunc("_cast", {ritem->getType()}, {commonType}, FUSION_MODULE);
      seqassertn(rcast, "cast func not found for left arg");
      ritem = util::call(rcast, {ritem});

      auto *op = M->getOrRealizeFunc(name, {litem->getType(), ritem->getType()}, {},
                                     FUSION_MODULE);
      seqassertn(op, "2-op func '{}' not found", name);
      auto *oitem = util::call(op, {litem, ritem});
      auto *ptrsetFunc = M->getOrRealizeFunc(
          "_ptrset", {scalarFuncArgTypes[0], oitem->getType()}, {}, FUSION_MODULE);
      seqassertn(ptrsetFunc, "ptrset func not found");
      body->push_back(
          util::call(ptrsetFunc, {M->Nr<VarValue>(scalarFuncArgVars[0]), oitem}));
    } else {
      auto *litem = deref(result == lv ? 0 : 1);
      std::vector<Generic> generics;
      if (op == NP_OP_CAST || op == NP_OP_ZEROS_LIKE || op == NP_OP_ONES_LIKE)
        generics.emplace_back(baseType);
      auto *operation =
          M->getOrRealizeFunc(name, {litem->getType()}, generics, FUSION_MODULE);
      seqassertn(operation, "1-op func '{}' not found", name);
      auto *oitem = util::call(operation, {litem});
      auto *ptrsetFunc = M->getOrRealizeFunc(
          "_ptrset", {scalarFuncArgTypes[0], oitem->getType()}, {}, FUSION_MODULE);
      seqassertn(ptrsetFunc, "ptrset func not found");
      body->push_back(
          util::call(ptrsetFunc, {M->Nr<VarValue>(scalarFuncArgVars[0]), oitem}));
    }

    scalarFunc->setBody(body);
    auto *arraysTuple = util::makeTuple(arrays);
    auto *loopFunc = M->getOrRealizeFunc(
        "_loop_basic",
        {arraysTuple->getType(), scalarFunc->getType(), extraTuple->getType()}, {},
        FUSION_MODULE);
    seqassertn(loopFunc, "loop_basic func not found");
    series->push_back(
        util::call(loopFunc, {arraysTuple, M->Nr<VarValue>(scalarFunc), extraTuple}));
  }

  seqassertn(!(freeLeftStatic && lcond), "unexpected free conditions for left arg");
  seqassertn(!(freeRightStatic && rcond), "unexpected free conditions for right arg");

  if (lcond && rcond) {
    series->push_back(M->Nr<IfFlow>(
        M->Nr<VarValue>(lcond), util::series(freeArray(rv)),
        util::series(freeArray(lv),
                     M->Nr<IfFlow>(M->Nr<VarValue>(rcond), M->Nr<SeriesFlow>(),
                                   util::series(freeArray(rv))))));
  } else {
    if (freeLeftStatic) {
      series->push_back(freeArray(lv));
    } else if (lcond) {
      series->push_back(M->Nr<IfFlow>(M->Nr<VarValue>(lcond), M->Nr<SeriesFlow>(),
                                      util::series(freeArray(lv))));
    }

    if (freeRightStatic) {
      series->push_back(freeArray(rv));
    } else if (rcond) {
      series->push_back(M->Nr<IfFlow>(M->Nr<VarValue>(rcond), M->Nr<SeriesFlow>(),
                                      util::series(freeArray(rv))));
    }
  }

  return result;
}

BroadcastInfo NumPyExpr::getBroadcastInfo() {
  int64_t arrDim = -1;
  Var *varLeaf = nullptr;
  bool multipleLeafVars = false;
  int numNonVarLeafArrays = 0;
  bool definitelyBroadcasts = false;

  apply([&](NumPyExpr &e) {
    if (e.isLeaf() && e.type.isArray()) {
      if (arrDim == -1) {
        arrDim = e.type.ndim;
      } else if (arrDim != e.type.ndim) {
        definitelyBroadcasts = true;
      }

      if (auto *v = cast<VarValue>(e.val)) {
        if (varLeaf) {
          if (varLeaf != v->getVar())
            multipleLeafVars = true;
        } else {
          varLeaf = v->getVar();
        }
      } else {
        ++numNonVarLeafArrays;
      }
    }
  });

  bool mightBroadcast = numNonVarLeafArrays > 1 || multipleLeafVars ||
                        (numNonVarLeafArrays == 1 && varLeaf);
  if (definitelyBroadcasts) {
    return BroadcastInfo::YES;
  } else if (mightBroadcast) {
    return BroadcastInfo::MAYBE;
  } else {
    return BroadcastInfo::NO;
  }
}

Value *NumPyExpr::codegenScalarExpr(
    CodegenContext &C, const std::unordered_map<NumPyExpr *, Var *> &args,
    const std::unordered_map<NumPyExpr *, unsigned> &scalarMap, Var *scalars) {
  auto *M = C.M;
  auto &T = C.T;

  Value *lv = lhs ? lhs->codegenScalarExpr(C, args, scalarMap, scalars) : nullptr;
  Value *rv = rhs ? rhs->codegenScalarExpr(C, args, scalarMap, scalars) : nullptr;
  auto name = "_" + opstring();

  if (op == NP_OP_WHERE) {
    auto *thirdValue = third->codegenScalarExpr(C, args, scalarMap, scalars);
    auto *select =
        M->getOrRealizeFunc(name, {lv->getType(), rv->getType(), thirdValue->getType()},
                            {type.getIRBaseType(T)}, FUSION_MODULE);
    seqassertn(select, "where scalar func not found");
    return util::call(select, {lv, rv, thirdValue});
  }

  if (lv && rv) {
    auto *t = type.getIRBaseType(T);
    auto *commonType = decideTypes(this, lhs->type, rhs->type, T);
    auto *cast1 =
        M->getOrRealizeFunc("_cast", {lv->getType()}, {commonType}, FUSION_MODULE);
    auto *cast2 =
        M->getOrRealizeFunc("_cast", {rv->getType()}, {commonType}, FUSION_MODULE);
    lv = util::call(cast1, {lv});
    rv = util::call(cast2, {rv});
    auto *f =
        M->getOrRealizeFunc(name, {lv->getType(), rv->getType()}, {}, FUSION_MODULE);
    seqassertn(f, "2-op func '{}' not found", name);
    return util::call(f, {lv, rv});
  } else if (lv) {
    auto *t = type.getIRBaseType(T);
    std::vector<Generic> generics;
    if (op == NP_OP_CAST || op == NP_OP_ZEROS_LIKE || op == NP_OP_ONES_LIKE)
      generics.emplace_back(t);
    auto *f = M->getOrRealizeFunc(name, {lv->getType()}, generics, FUSION_MODULE);
    seqassertn(f, "1-op func '{}' not found", name);
    return util::call(f, {lv});
  } else {
    if (type.isArray()) {
      auto it = args.find(this);
      seqassertn(it != args.end(), "NumPyExpr not found in args map (codegen expr)");
      auto *var = it->second;
      return (*M->Nr<VarValue>(var))[*M->getInt(0)];
    } else {
      auto it = scalarMap.find(this);
      seqassertn(it != scalarMap.end(),
                 "NumPyExpr not found in scalar map (codegen expr)");
      auto idx = it->second;
      return util::tupleGet(M->Nr<VarValue>(scalars), idx);
    }
  }
}

} // namespace numpy
} // namespace transform
} // namespace ir
} // namespace codon
