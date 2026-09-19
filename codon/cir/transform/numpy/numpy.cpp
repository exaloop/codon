// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "numpy.h"

#include "codon/cir/analyze/dataflow/reaching.h"
#include "codon/cir/analyze/module/global_vars.h"
#include "codon/cir/analyze/module/side_effect.h"
#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"

#include "llvm/Support/CommandLine.h"

#include <algorithm>
#include <complex>
#include <sstream>
#include <utility>

#define XLOG(c, ...)                                                                   \
  do {                                                                                 \
    if (Verbose)                                                                       \
      LOG(c, ##__VA_ARGS__);                                                           \
  } while (false)

namespace codon {
namespace ir {
namespace transform {
namespace numpy {
// Pipeline: expose small array helpers, extract expressions, forward eligible
// temporaries, then lower to fused loops or specialized runtime helpers.
// forward.cpp handles cross-assignment lifetimes; expr.cpp emits expression
// evaluation, layouts and reductions. This file coordinates those pieces.
namespace {
llvm::cl::opt<int> AlwaysFuseCostThreshold(
    "npfuse-always", llvm::cl::desc("Expression cost below which (<=) to always fuse"),
    llvm::cl::init(10));

llvm::cl::opt<int> NeverFuseCostThreshold(
    "npfuse-never", llvm::cl::desc("Expression cost above which (>) to never fuse"),
    llvm::cl::init(50));

llvm::cl::opt<bool> FuseMatmul(
    "npfuse-matmul",
    llvm::cl::desc("Fuse compatible matmul additions and subtractions into BLAS"),
    llvm::cl::init(true));

llvm::cl::opt<bool> Verbose("npfuse-verbose",
                            llvm::cl::desc("Print information about fused expressions"),
                            llvm::cl::init(false));

bool isArrayType(Type *t) {
  return t && isA<RecordType>(t) &&
         t->getName().rfind(ast::getMangledClass("std.numpy.ndarray", "ndarray") + "[",
                            0) == 0;
}

bool isUFuncType(Type *t) {
  return t &&
         (t->getName().rfind(
              ast::getMangledClass("std.numpy.ufunc", "UnaryUFunc") + "[", 0) == 0 ||
          t->getName().rfind(
              ast::getMangledClass("std.numpy.ufunc", "BinaryUFunc") + "[", 0) == 0);
}

bool isNoneType(Type *t, NumPyPrimitiveTypes &T) {
  return t && (t->is(T.none) || t->is(T.optnone));
}
} // namespace

const std::string FUSION_MODULE = "std.numpy.fusion";

NumPyPrimitiveTypes::NumPyPrimitiveTypes(Module *M)
    : none(M->getNoneType()), optnone(M->getOptionalType(none)),
      bool_(M->getBoolType()), i8(M->getIntType(8, true)), u8(M->getIntType(8, false)),
      i16(M->getIntType(16, true)), u16(M->getIntType(16, false)),
      i32(M->getIntType(32, true)), u32(M->getIntType(32, false)),
      i64(M->getIntType(64, true)), u64(M->getIntType(64, false)),
      f16(M->getFloat16Type()), f32(M->getFloat32Type()), f64(M->getFloatType()),
      c64(M->getType(ast::getMangledClass("std.internal.types.complex", "complex64"))),
      c128(M->getType(ast::getMangledClass("std.internal.types.complex", "complex"))) {}

NumPyType::NumPyType(TypeCode dtype, int64_t ndim) : dtype(dtype), ndim(ndim) {
  seqassertn(ndim >= 0, "ndim must be non-negative");
}

NumPyType::NumPyType() : NumPyType(NP_TYPE_NONE) {}

NumPyType NumPyType::get(Type *t, NumPyPrimitiveTypes &T) {
  if (t->is(T.bool_))
    return {NumPyType::NP_TYPE_BOOL};
  if (t->is(T.i8))
    return {NumPyType::NP_TYPE_I8};
  if (t->is(T.u8))
    return {NumPyType::NP_TYPE_U8};
  if (t->is(T.i16))
    return {NumPyType::NP_TYPE_I16};
  if (t->is(T.u16))
    return {NumPyType::NP_TYPE_U16};
  if (t->is(T.i32))
    return {NumPyType::NP_TYPE_I32};
  if (t->is(T.u32))
    return {NumPyType::NP_TYPE_U32};
  if (t->is(T.i64))
    return {NumPyType::NP_TYPE_I64};
  if (t->is(T.u64))
    return {NumPyType::NP_TYPE_U64};
  if (t->is(T.f16))
    return {NumPyType::NP_TYPE_F16};
  if (t->is(T.f32))
    return {NumPyType::NP_TYPE_F32};
  if (t->is(T.f64))
    return {NumPyType::NP_TYPE_F64};
  if (t->is(T.c64))
    return {NumPyType::NP_TYPE_C64};
  if (t->is(T.c128))
    return {NumPyType::NP_TYPE_C128};
  if (isArrayType(t)) {
    auto generics = t->getGenerics();
    seqassertn(generics.size() == 2 && generics[0].isType() && generics[1].isStatic(),
               "unrecognized ndarray generics");
    auto *dtype = generics[0].getTypeValue();
    auto ndim = generics[1].getStaticValue();
    if (dtype->is(T.bool_))
      return {NumPyType::NP_TYPE_ARR_BOOL, ndim};
    if (dtype->is(T.i8))
      return {NumPyType::NP_TYPE_ARR_I8, ndim};
    if (dtype->is(T.u8))
      return {NumPyType::NP_TYPE_ARR_U8, ndim};
    if (dtype->is(T.i16))
      return {NumPyType::NP_TYPE_ARR_I16, ndim};
    if (dtype->is(T.u16))
      return {NumPyType::NP_TYPE_ARR_U16, ndim};
    if (dtype->is(T.i32))
      return {NumPyType::NP_TYPE_ARR_I32, ndim};
    if (dtype->is(T.u32))
      return {NumPyType::NP_TYPE_ARR_U32, ndim};
    if (dtype->is(T.i64))
      return {NumPyType::NP_TYPE_ARR_I64, ndim};
    if (dtype->is(T.u64))
      return {NumPyType::NP_TYPE_ARR_U64, ndim};
    if (dtype->is(T.f16))
      return {NumPyType::NP_TYPE_ARR_F16, ndim};
    if (dtype->is(T.f32))
      return {NumPyType::NP_TYPE_ARR_F32, ndim};
    if (dtype->is(T.f64))
      return {NumPyType::NP_TYPE_ARR_F64, ndim};
    if (dtype->is(T.c64))
      return {NumPyType::NP_TYPE_ARR_C64, ndim};
    if (dtype->is(T.c128))
      return {NumPyType::NP_TYPE_ARR_C128, ndim};
  }
  return {};
}

Type *NumPyType::getIRBaseType(NumPyPrimitiveTypes &T) const {
  switch (dtype) {
  case NP_TYPE_NONE:
    seqassertn(false, "unexpected type code (NONE)");
    return nullptr;
  case NP_TYPE_BOOL:
    return T.bool_;
  case NP_TYPE_I8:
    return T.i8;
  case NP_TYPE_U8:
    return T.u8;
  case NP_TYPE_I16:
    return T.i16;
  case NP_TYPE_U16:
    return T.u16;
  case NP_TYPE_I32:
    return T.i32;
  case NP_TYPE_U32:
    return T.u32;
  case NP_TYPE_I64:
    return T.i64;
  case NP_TYPE_U64:
    return T.u64;
  case NP_TYPE_F16:
    return T.f16;
  case NP_TYPE_F32:
    return T.f32;
  case NP_TYPE_F64:
    return T.f64;
  case NP_TYPE_C64:
    return T.c64;
  case NP_TYPE_C128:
    return T.c128;
  case NP_TYPE_SCALAR_END:
    seqassertn(false, "unexpected type code (SCALAR_END)");
    return nullptr;
  case NP_TYPE_ARR_BOOL:
    return T.bool_;
  case NP_TYPE_ARR_I8:
    return T.i8;
  case NP_TYPE_ARR_U8:
    return T.u8;
  case NP_TYPE_ARR_I16:
    return T.i16;
  case NP_TYPE_ARR_U16:
    return T.u16;
  case NP_TYPE_ARR_I32:
    return T.i32;
  case NP_TYPE_ARR_U32:
    return T.u32;
  case NP_TYPE_ARR_I64:
    return T.i64;
  case NP_TYPE_ARR_U64:
    return T.u64;
  case NP_TYPE_ARR_F16:
    return T.f16;
  case NP_TYPE_ARR_F32:
    return T.f32;
  case NP_TYPE_ARR_F64:
    return T.f64;
  case NP_TYPE_ARR_C64:
    return T.c64;
  case NP_TYPE_ARR_C128:
    return T.c128;
  default:
    seqassertn(false, "unexpected type code (?)");
    return nullptr;
  }
}

std::ostream &operator<<(std::ostream &os, NumPyType const &type) {
  static const std::unordered_map<NumPyType::TypeCode, std::string> typestrings = {
      {NumPyType::NP_TYPE_NONE, "none"},     {NumPyType::NP_TYPE_BOOL, "bool"},
      {NumPyType::NP_TYPE_I8, "i8"},         {NumPyType::NP_TYPE_U8, "u8"},
      {NumPyType::NP_TYPE_I16, "i16"},       {NumPyType::NP_TYPE_U16, "u16"},
      {NumPyType::NP_TYPE_I32, "i32"},       {NumPyType::NP_TYPE_U32, "u32"},
      {NumPyType::NP_TYPE_I64, "i64"},       {NumPyType::NP_TYPE_U64, "u64"},
      {NumPyType::NP_TYPE_F16, "f16"},       {NumPyType::NP_TYPE_F32, "f32"},
      {NumPyType::NP_TYPE_F64, "f64"},       {NumPyType::NP_TYPE_C64, "c64"},
      {NumPyType::NP_TYPE_C128, "c128"},     {NumPyType::NP_TYPE_SCALAR_END, ""},
      {NumPyType::NP_TYPE_ARR_BOOL, "bool"}, {NumPyType::NP_TYPE_ARR_I8, "i8"},
      {NumPyType::NP_TYPE_ARR_U8, "u8"},     {NumPyType::NP_TYPE_ARR_I16, "i16"},
      {NumPyType::NP_TYPE_ARR_U16, "u16"},   {NumPyType::NP_TYPE_ARR_I32, "i32"},
      {NumPyType::NP_TYPE_ARR_U32, "u32"},   {NumPyType::NP_TYPE_ARR_I64, "i64"},
      {NumPyType::NP_TYPE_ARR_U64, "u64"},   {NumPyType::NP_TYPE_ARR_F16, "f16"},
      {NumPyType::NP_TYPE_ARR_F32, "f32"},   {NumPyType::NP_TYPE_ARR_F64, "f64"},
      {NumPyType::NP_TYPE_ARR_C64, "c64"},   {NumPyType::NP_TYPE_ARR_C128, "c128"},
  };

  auto it = typestrings.find(type.dtype);
  seqassertn(it != typestrings.end(), "type not found");
  auto s = it->second;
  if (type.isArray())
    os << "array[" << s << ", " << type.ndim << "]";
  else
    os << s;
  return os;
}

std::string NumPyType::str() const {
  std::stringstream buffer;
  buffer << *this;
  return buffer.str();
}

CodegenContext::CodegenContext(Module *M, SeriesFlow *series, BodiedFunc *func,
                               NumPyPrimitiveTypes &T)
    : M(M), series(series), func(func), vars(), T(T) {}

std::unique_ptr<NumPyExpr> parse(Value *v,
                                 std::vector<std::pair<NumPyExpr *, Value *>> &leaves,
                                 NumPyPrimitiveTypes &T, bool allowReduction,
                                 Value **destination) {
  struct NumPyMagicMethod {
    std::string name;
    NumPyExpr::Op op;
    int args;
    bool right;
  };

  struct NumPyUFunc {
    std::string name;
    NumPyExpr::Op op;
    int args;
  };

  static std::vector<NumPyMagicMethod> magics = {
      {Module::POS_MAGIC_NAME, NumPyExpr::NP_OP_POS, 1, false},
      {Module::NEG_MAGIC_NAME, NumPyExpr::NP_OP_NEG, 1, false},
      {Module::INVERT_MAGIC_NAME, NumPyExpr::NP_OP_INVERT, 1, false},
      {Module::ABS_MAGIC_NAME, NumPyExpr::NP_OP_ABS, 1, false},

      {Module::ADD_MAGIC_NAME, NumPyExpr::NP_OP_ADD, 2, false},
      {Module::SUB_MAGIC_NAME, NumPyExpr::NP_OP_SUB, 2, false},
      {Module::MUL_MAGIC_NAME, NumPyExpr::NP_OP_MUL, 2, false},
      {Module::MATMUL_MAGIC_NAME, NumPyExpr::NP_OP_MATMUL, 2, false},
      {Module::TRUE_DIV_MAGIC_NAME, NumPyExpr::NP_OP_TRUE_DIV, 2, false},
      {Module::FLOOR_DIV_MAGIC_NAME, NumPyExpr::NP_OP_FLOOR_DIV, 2, false},
      {Module::MOD_MAGIC_NAME, NumPyExpr::NP_OP_MOD, 2, false},
      {Module::POW_MAGIC_NAME, NumPyExpr::NP_OP_POW, 2, false},
      {Module::LSHIFT_MAGIC_NAME, NumPyExpr::NP_OP_LSHIFT, 2, false},
      {Module::RSHIFT_MAGIC_NAME, NumPyExpr::NP_OP_RSHIFT, 2, false},
      {Module::AND_MAGIC_NAME, NumPyExpr::NP_OP_AND, 2, false},
      {Module::OR_MAGIC_NAME, NumPyExpr::NP_OP_OR, 2, false},
      {Module::XOR_MAGIC_NAME, NumPyExpr::NP_OP_XOR, 2, false},

      {Module::RADD_MAGIC_NAME, NumPyExpr::NP_OP_ADD, 2, true},
      {Module::RSUB_MAGIC_NAME, NumPyExpr::NP_OP_SUB, 2, true},
      {Module::RMUL_MAGIC_NAME, NumPyExpr::NP_OP_MUL, 2, true},
      {Module::RMATMUL_MAGIC_NAME, NumPyExpr::NP_OP_MATMUL, 2, true},
      {Module::RTRUE_DIV_MAGIC_NAME, NumPyExpr::NP_OP_TRUE_DIV, 2, true},
      {Module::RFLOOR_DIV_MAGIC_NAME, NumPyExpr::NP_OP_FLOOR_DIV, 2, true},
      {Module::RMOD_MAGIC_NAME, NumPyExpr::NP_OP_MOD, 2, true},
      {Module::RPOW_MAGIC_NAME, NumPyExpr::NP_OP_POW, 2, true},
      {Module::RLSHIFT_MAGIC_NAME, NumPyExpr::NP_OP_LSHIFT, 2, true},
      {Module::RRSHIFT_MAGIC_NAME, NumPyExpr::NP_OP_RSHIFT, 2, true},
      {Module::RAND_MAGIC_NAME, NumPyExpr::NP_OP_AND, 2, true},
      {Module::ROR_MAGIC_NAME, NumPyExpr::NP_OP_OR, 2, true},
      {Module::RXOR_MAGIC_NAME, NumPyExpr::NP_OP_XOR, 2, true},

      {Module::EQ_MAGIC_NAME, NumPyExpr::NP_OP_EQ, 2, false},
      {Module::NE_MAGIC_NAME, NumPyExpr::NP_OP_NE, 2, false},
      {Module::LT_MAGIC_NAME, NumPyExpr::NP_OP_LT, 2, false},
      {Module::LE_MAGIC_NAME, NumPyExpr::NP_OP_LE, 2, false},
      {Module::GT_MAGIC_NAME, NumPyExpr::NP_OP_GT, 2, false},
      {Module::GE_MAGIC_NAME, NumPyExpr::NP_OP_GE, 2, false},
  };

  static std::vector<NumPyUFunc> ufuncs = {
      {"positive", NumPyExpr::NP_OP_POS, 1},
      {"negative", NumPyExpr::NP_OP_NEG, 1},
      {"invert", NumPyExpr::NP_OP_INVERT, 1},
      {"abs", NumPyExpr::NP_OP_ABS, 1},
      {"absolute", NumPyExpr::NP_OP_ABS, 1},
      {"add", NumPyExpr::NP_OP_ADD, 2},
      {"subtract", NumPyExpr::NP_OP_SUB, 2},
      {"multiply", NumPyExpr::NP_OP_MUL, 2},
      {"divide", NumPyExpr::NP_OP_TRUE_DIV, 2},
      {"floor_divide", NumPyExpr::NP_OP_FLOOR_DIV, 2},
      {"remainder", NumPyExpr::NP_OP_MOD, 2},
      {"fmod", NumPyExpr::NP_OP_FMOD, 2},
      {"power", NumPyExpr::NP_OP_POW, 2},
      {"left_shift", NumPyExpr::NP_OP_LSHIFT, 2},
      {"right_shift", NumPyExpr::NP_OP_RSHIFT, 2},
      {"bitwise_and", NumPyExpr::NP_OP_AND, 2},
      {"bitwise_or", NumPyExpr::NP_OP_OR, 2},
      {"bitwise_xor", NumPyExpr::NP_OP_XOR, 2},
      {"logical_and", NumPyExpr::NP_OP_LOGICAL_AND, 2},
      {"logical_or", NumPyExpr::NP_OP_LOGICAL_OR, 2},
      {"logical_xor", NumPyExpr::NP_OP_LOGICAL_XOR, 2},
      {"equal", NumPyExpr::NP_OP_EQ, 2},
      {"not_equal", NumPyExpr::NP_OP_NE, 2},
      {"less", NumPyExpr::NP_OP_LT, 2},
      {"less_equal", NumPyExpr::NP_OP_LE, 2},
      {"greater", NumPyExpr::NP_OP_GT, 2},
      {"greater_equal", NumPyExpr::NP_OP_GE, 2},
      {"minimum", NumPyExpr::NP_OP_MIN, 2},
      {"maximum", NumPyExpr::NP_OP_MAX, 2},
      {"fmin", NumPyExpr::NP_OP_FMIN, 2},
      {"fmax", NumPyExpr::NP_OP_FMAX, 2},
      {"sin", NumPyExpr::NP_OP_SIN, 1},
      {"cos", NumPyExpr::NP_OP_COS, 1},
      {"tan", NumPyExpr::NP_OP_TAN, 1},
      {"arcsin", NumPyExpr::NP_OP_ARCSIN, 1},
      {"arccos", NumPyExpr::NP_OP_ARCCOS, 1},
      {"arctan", NumPyExpr::NP_OP_ARCTAN, 1},
      {"arctan2", NumPyExpr::NP_OP_ARCTAN2, 2},
      {"hypot", NumPyExpr::NP_OP_HYPOT, 2},
      {"sinh", NumPyExpr::NP_OP_SINH, 1},
      {"cosh", NumPyExpr::NP_OP_COSH, 1},
      {"tanh", NumPyExpr::NP_OP_TANH, 1},
      {"arcsinh", NumPyExpr::NP_OP_ARCSINH, 1},
      {"arccosh", NumPyExpr::NP_OP_ARCCOSH, 1},
      {"arctanh", NumPyExpr::NP_OP_ARCTANH, 1},
      {"conjugate", NumPyExpr::NP_OP_CONJ, 1},
      {"exp", NumPyExpr::NP_OP_EXP, 1},
      {"exp2", NumPyExpr::NP_OP_EXP2, 1},
      {"log", NumPyExpr::NP_OP_LOG, 1},
      {"log2", NumPyExpr::NP_OP_LOG2, 1},
      {"log10", NumPyExpr::NP_OP_LOG10, 1},
      {"expm1", NumPyExpr::NP_OP_EXPM1, 1},
      {"log1p", NumPyExpr::NP_OP_LOG1P, 1},
      {"sqrt", NumPyExpr::NP_OP_SQRT, 1},
      {"square", NumPyExpr::NP_OP_SQUARE, 1},
      {"cbrt", NumPyExpr::NP_OP_CBRT, 1},
      {"logaddexp", NumPyExpr::NP_OP_LOGADDEXP, 2},
      {"logaddexp2", NumPyExpr::NP_OP_LOGADDEXP2, 2},
      {"reciprocal", NumPyExpr::NP_OP_RECIPROCAL, 1},
      {"rint", NumPyExpr::NP_OP_RINT, 1},
      {"floor", NumPyExpr::NP_OP_FLOOR, 1},
      {"ceil", NumPyExpr::NP_OP_CEIL, 1},
      {"trunc", NumPyExpr::NP_OP_TRUNC, 1},
      {"isnan", NumPyExpr::NP_OP_ISNAN, 1},
      {"isinf", NumPyExpr::NP_OP_ISINF, 1},
      {"isfinite", NumPyExpr::NP_OP_ISFINITE, 1},
      {"sign", NumPyExpr::NP_OP_SIGN, 1},
      {"signbit", NumPyExpr::NP_OP_SIGNBIT, 1},
      {"copysign", NumPyExpr::NP_OP_COPYSIGN, 2},
      {"spacing", NumPyExpr::NP_OP_SPACING, 1},
      {"nextafter", NumPyExpr::NP_OP_NEXTAFTER, 2},
      {"deg2rad", NumPyExpr::NP_OP_DEG2RAD, 1},
      {"radians", NumPyExpr::NP_OP_DEG2RAD, 1},
      {"rad2deg", NumPyExpr::NP_OP_RAD2DEG, 1},
      {"degrees", NumPyExpr::NP_OP_RAD2DEG, 1},
      {"heaviside", NumPyExpr::NP_OP_HEAVISIDE, 2},
  };

  auto getNumPyExprType = [](Type *t, NumPyPrimitiveTypes &T) -> NumPyType {
    if (t->is(T.bool_))
      return {NumPyType::NP_TYPE_BOOL};
    if (t->is(T.i8))
      return {NumPyType::NP_TYPE_I8};
    if (t->is(T.u8))
      return {NumPyType::NP_TYPE_U8};
    if (t->is(T.i16))
      return {NumPyType::NP_TYPE_I16};
    if (t->is(T.u16))
      return {NumPyType::NP_TYPE_U16};
    if (t->is(T.i32))
      return {NumPyType::NP_TYPE_I32};
    if (t->is(T.u32))
      return {NumPyType::NP_TYPE_U32};
    if (t->is(T.i64))
      return {NumPyType::NP_TYPE_I64};
    if (t->is(T.u64))
      return {NumPyType::NP_TYPE_U64};
    if (t->is(T.f16))
      return {NumPyType::NP_TYPE_F16};
    if (t->is(T.f32))
      return {NumPyType::NP_TYPE_F32};
    if (t->is(T.f64))
      return {NumPyType::NP_TYPE_F64};
    if (t->is(T.c64))
      return {NumPyType::NP_TYPE_C64};
    if (t->is(T.c128))
      return {NumPyType::NP_TYPE_C128};
    if (isArrayType(t)) {
      auto generics = t->getGenerics();
      seqassertn(generics.size() == 2 && generics[0].isType() && generics[1].isStatic(),
                 "unrecognized ndarray generics");
      auto *dtype = generics[0].getTypeValue();
      auto ndim = generics[1].getStaticValue();
      if (dtype->is(T.bool_))
        return {NumPyType::NP_TYPE_ARR_BOOL, ndim};
      if (dtype->is(T.i8))
        return {NumPyType::NP_TYPE_ARR_I8, ndim};
      if (dtype->is(T.u8))
        return {NumPyType::NP_TYPE_ARR_U8, ndim};
      if (dtype->is(T.i16))
        return {NumPyType::NP_TYPE_ARR_I16, ndim};
      if (dtype->is(T.u16))
        return {NumPyType::NP_TYPE_ARR_U16, ndim};
      if (dtype->is(T.i32))
        return {NumPyType::NP_TYPE_ARR_I32, ndim};
      if (dtype->is(T.u32))
        return {NumPyType::NP_TYPE_ARR_U32, ndim};
      if (dtype->is(T.i64))
        return {NumPyType::NP_TYPE_ARR_I64, ndim};
      if (dtype->is(T.u64))
        return {NumPyType::NP_TYPE_ARR_U64, ndim};
      if (dtype->is(T.f16))
        return {NumPyType::NP_TYPE_ARR_F16, ndim};
      if (dtype->is(T.f32))
        return {NumPyType::NP_TYPE_ARR_F32, ndim};
      if (dtype->is(T.f64))
        return {NumPyType::NP_TYPE_ARR_F64, ndim};
      if (dtype->is(T.c64))
        return {NumPyType::NP_TYPE_ARR_C64, ndim};
      if (dtype->is(T.c128))
        return {NumPyType::NP_TYPE_ARR_C128, ndim};
    }
    return {};
  };

  auto type = getNumPyExprType(v->getType(), T);
  if (!type)
    return {};

  if (allowReduction) {
    auto *call = cast<CallInstr>(v);
    auto *function = call ? util::getFunc(call->getCallee()) : nullptr;
    if (function) {
      auto name = function->getUnmangledName();
      static const std::unordered_map<std::string, NumPyExpr::Op> reductions = {
          {"sum", NumPyExpr::NP_OP_SUM},   {"prod", NumPyExpr::NP_OP_PROD},
          {"any", NumPyExpr::NP_OP_ANY},   {"all", NumPyExpr::NP_OP_ALL},
          {"min", NumPyExpr::NP_OP_AMIN},  {"max", NumPyExpr::NP_OP_AMAX},
          {"amin", NumPyExpr::NP_OP_AMIN}, {"amax", NumPyExpr::NP_OP_AMAX}};
      auto found = reductions.find(name);
      bool logical = name == "any" || name == "all";
      bool extrema = name == "min" || name == "max" || name == "amin" || name == "amax";
      unsigned expectedArgs = logical ? 4 : extrema ? 6 : 5;
      if (found != reductions.end() && call->numArgs() == expectedArgs &&
          (isArrayType(function->getParentType()) ||
           function->getName().rfind(
               ast::getMangledFunc("std.numpy.reductions", name) + "[", 0) == 0)) {
        std::vector<Value *> args(call->begin(), call->end());
        auto noValue = ast::getMangledClass("std.numpy.util", "_NoValue");
        auto *initialValue = logical ? nullptr : args[extrema ? 4 : 3];
        bool hasInitial = initialValue && initialValue->getType()->getName() != noValue;
        if (isNoneType(args[2]->getType(), T) &&
            args.back()->getType()->getName() == noValue &&
            (!hasInitial || isA<Const>(initialValue) || isA<VarValue>(initialValue))) {
          auto operand = parse(args[0], leaves, T);
          auto initial = hasInitial ? parse(initialValue, leaves, T) : nullptr;
          if (operand && operand->type.isArray() && operand->type.ndim > 0 &&
              (!hasInitial || (initial && !initial->type.isArray())))
            return std::make_unique<NumPyExpr>(type, v, found->second,
                                               std::move(operand), std::move(initial));
          leaves.clear();
        }
      }
    }
  }

  // Don't break up expressions that result in scalars or 0-dim arrays since those
  // should only be computed once
  if (type.ndim == 0) {
    auto res = std::make_unique<NumPyExpr>(type, v);
    leaves.emplace_back(res.get(), v);
    return std::move(res);
  }

  if (auto *c = cast<CallInstr>(v)) {
    auto *f = util::getFunc(c->getCallee());

    if (f && c->numArgs() == 2 && isArrayType(c->front()->getType())) {
      auto name = f->getUnmangledName();
      auto *order = cast<StringConst>(c->back());
      if ((name == "zeros_like" || name == "ones_like") &&
          f->getName().rfind(ast::getMangledFunc("std.numpy.routines", name) + "[",
                             0) == 0 &&
          order &&
          (order->getVal() == "C" || order->getVal() == "F" || order->getVal() == "A" ||
           order->getVal() == "K")) {
        auto operand = parse(c->front(), leaves, T);
        if (operand)
          return std::make_unique<NumPyExpr>(type, v,
                                             name == "zeros_like"
                                                 ? NumPyExpr::NP_OP_ZEROS_LIKE
                                                 : NumPyExpr::NP_OP_ONES_LIKE,
                                             std::move(operand));
      }
    }

    if (f && isArrayType(f->getParentType())) {
      auto name = f->getUnmangledName();
      if ((name == "astype" && c->numArgs() == 3 && isA<BoolConst>(c->back())) ||
          (name == "copy" && c->numArgs() == 2)) {
        auto *order = cast<StringConst>(*std::next(c->begin()));
        bool ownsStorage = name == "copy" || cast<BoolConst>(c->back())->getVal() ||
                           !c->front()->getType()->is(v->getType());
        if (ownsStorage && order &&
            (order->getVal() == "C" || order->getVal() == "F" ||
             order->getVal() == "A" || order->getVal() == "K")) {
          auto operand = parse(c->front(), leaves, T);
          if (operand)
            return std::make_unique<NumPyExpr>(type, v, NumPyExpr::NP_OP_CAST,
                                               std::move(operand));
        }
      }
    }

    // Check for matmul
    if (f && c->numArgs() == 3 && isNoneType(c->back()->getType(), T) &&
        (f->getName().rfind(ast::getMangledFunc("std.numpy.linalg_sym", "matmul") + "[",
                            0) == 0 ||
         (f->getName().rfind(ast::getMangledFunc("std.numpy.linalg_sym", "dot") + "[]",
                             0) == 0 &&
          type.ndim == 2))) {
      std::vector<Value *> args(c->begin(), c->end());
      auto op = NumPyExpr::NP_OP_MATMUL;
      auto lhs = parse(args[0], leaves, T);
      if (!lhs)
        return {};

      auto rhs = parse(args[1], leaves, T);
      if (!rhs)
        return {};

      return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs), std::move(rhs));
    }

    // Check for builtin abs()
    if (f && c->numArgs() == 1 &&
        (f->getName().rfind(ast::getMangledFunc("std.internal.builtin", "abs") + "[",
                            0) == 0)) {
      auto op = NumPyExpr::NP_OP_ABS;
      auto lhs = parse(c->front(), leaves, T);
      if (!lhs)
        return {};

      return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs));
    }

    // Check for transpose
    if (f && isArrayType(f->getParentType()) && c->numArgs() == 1 &&
        f->getUnmangledName() == "T") {
      auto op = NumPyExpr::NP_OP_TRANSPOSE;
      auto lhs = parse(c->front(), leaves, T);
      if (!lhs)
        return {};

      return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs));
    }

    // Check for ufunc (e.g. "np.exp()") call
    if (f && f->getUnmangledName() == Module::CALL_MAGIC_NAME &&
        isUFuncType(f->getParentType())) {

      auto ufuncGenerics = f->getParentType()->getGenerics();
      seqassertn(!ufuncGenerics.empty() && ufuncGenerics[0].isStaticStr(),
                 "unrecognized ufunc class generics");
      auto ufunc = ufuncGenerics[0].getStaticStringValue();

      auto callGenerics = f->getType()->getGenerics();
      seqassertn(!callGenerics.empty() && callGenerics[0].isType(),
                 "unrecognized ufunc call generics");
      auto *dtype = callGenerics[0].getTypeValue();

      if (dtype->is(T.none)) {
        for (auto &u : ufuncs) {
          if (u.name == ufunc) {
            seqassertn(u.args == 1 || u.args == 2,
                       "unexpected number of arguments (ufunc)");

            // Argument order:
            //   - ufunc self
            //   - operand 1
            //   - (if binary) operand 2
            //   - 'out'
            //   - 'where'
            std::vector<Value *> args(c->begin(), c->end());
            seqassertn(args.size() == u.args + 3, "unexpected call of {}", u.name);
            auto *where = args[args.size() - 1];
            auto *out = args[args.size() - 2];

            if (auto *whereConst = cast<BoolConst>(where)) {
              if (!whereConst->getVal())
                break;
            } else {
              break;
            }

            if (!isNoneType(out->getType(), T) && !destination)
              break;

            auto op = u.op;
            auto lhs = parse(args[1], leaves, T);
            if (!lhs)
              return {};

            if (u.args == 1) {
              if (destination && !isNoneType(out->getType(), T))
                *destination = out;
              return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs));
            }

            auto rhs = parse(args[2], leaves, T);
            if (!rhs)
              return {};

            if (destination && !isNoneType(out->getType(), T))
              *destination = out;
            return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs),
                                               std::move(rhs));
          }
        }
      }
    }

    // Check for magic method call
    if (f && isArrayType(f->getParentType())) {
      for (auto &m : magics) {
        if (f->getUnmangledName() == m.name && c->numArgs() == m.args) {
          seqassertn(m.args == 1 || m.args == 2,
                     "unexpected number of arguments (magic)");
          std::vector<Value *> args(c->begin(), c->end());
          auto op = m.op;
          auto lhs = parse(args[0], leaves, T);
          if (!lhs)
            return {};

          if (m.args == 1)
            return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs));

          auto rhs = parse(args[1], leaves, T);
          if (!rhs)
            return {};

          return m.right ? std::make_unique<NumPyExpr>(type, v, op, std::move(rhs),
                                                       std::move(lhs))
                         : std::make_unique<NumPyExpr>(type, v, op, std::move(lhs),
                                                       std::move(rhs));
        }
      }
    }
  }

  // Check for right-hand-side magic method call
  // Right-hand-side magics (e.g. __radd__) are compiled into FlowInstr:
  //   <lhs_expr> + <rhs_expr>
  // becomes:
  //   { v1 = <lhs expr> ; v2 = <rhs expr> ; return rhs_class.__radd__(v2, v1) }
  // So we need to check for this to detect r-magics.
  if (auto *flow = cast<FlowInstr>(v)) {
    auto *series = cast<SeriesFlow>(flow->getFlow());
    auto *value = cast<CallInstr>(flow->getValue());
    auto *f = value ? util::getFunc(value->getCallee()) : nullptr;

    if (series && f && value->numArgs() == 2) {
      std::vector<Value *> assignments(series->begin(), series->end());
      auto *arg1 = value->front();
      auto *arg2 = value->back();
      auto *vv1 = cast<VarValue>(arg1);
      auto *vv2 = cast<VarValue>(arg2);
      auto *arg1Var = vv1 ? vv1->getVar() : nullptr;
      auto *arg2Var = vv2 ? vv2->getVar() : nullptr;

      for (auto &m : magics) {
        if (f->getUnmangledName() == m.name && value->numArgs() == m.args && m.right) {
          auto op = m.op;

          if (assignments.size() == 0) {
            // Case 1: Degenerate flow instruction
            return parse(value, leaves, T);
          } else if (assignments.size() == 1) {
            // Case 2: One var -- check if it's either of the r-magic operands
            auto *a1 = cast<AssignInstr>(assignments.front());
            if (a1 && a1->getLhs() == arg1Var) {
              auto rhs = parse(a1->getRhs(), leaves, T);
              if (!rhs)
                return {};

              auto lhs = parse(arg2, leaves, T);
              if (!lhs)
                return {};

              return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs),
                                                 std::move(rhs));
            } else if (a1 && a1->getLhs() == arg2Var) {
              auto lhs = parse(a1->getRhs(), leaves, T);
              if (!lhs)
                return {};

              auto rhs = parse(arg1, leaves, T);
              if (!rhs)
                return {};

              return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs),
                                                 std::move(rhs));
            }
          } else if (assignments.size() == 2) {
            // Case 2: Two vars -- check both permutations
            auto *a1 = cast<AssignInstr>(assignments.front());
            auto *a2 = cast<AssignInstr>(assignments.back());

            if (a1 && a2 && a1->getLhs() == arg1Var && a2->getLhs() == arg2Var) {
              auto rhs = parse(a1->getRhs(), leaves, T);
              if (!rhs)
                return {};

              auto lhs = parse(a2->getRhs(), leaves, T);
              if (!lhs)
                return {};

              return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs),
                                                 std::move(rhs));
            } else if (a1 && a2 && a2->getLhs() == arg1Var && a1->getLhs() == arg2Var) {
              auto lhs = parse(a1->getRhs(), leaves, T);
              if (!lhs)
                return {};

              auto rhs = parse(a2->getRhs(), leaves, T);
              if (!rhs)
                return {};

              return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs),
                                                 std::move(rhs));
            }
          }
          break;
        }
      }
    }
  }

  auto res = std::make_unique<NumPyExpr>(type, v);
  leaves.emplace_back(res.get(), v);
  return std::move(res);
}

namespace {
// Match only the static operator/dtype/rank contract. The runtime helper checks
// shapes and BLAS-compatible strides, falling back to the original operations.
bool isMatmulAdd(NumPyExpr &expr) {
  if (!FuseMatmul ||
      (expr.op != NumPyExpr::NP_OP_ADD && expr.op != NumPyExpr::NP_OP_SUB) ||
      !expr.lhs || !expr.rhs ||
      (expr.type.dtype != NumPyType::NP_TYPE_ARR_F32 &&
       expr.type.dtype != NumPyType::NP_TYPE_ARR_F64))
    return false;
  bool productLeft = expr.lhs->op == NumPyExpr::NP_OP_MATMUL;
  auto &product = *(productLeft ? expr.lhs : expr.rhs);
  auto &bias = *(productLeft ? expr.rhs : expr.lhs);
  if (product.op != NumPyExpr::NP_OP_MATMUL || !bias.isLeaf() ||
      !isA<VarValue>(bias.val) || !bias.type.isArray())
    return false;
  auto operand = [&](NumPyExpr &input) {
    auto *leaf = input.op == NumPyExpr::NP_OP_TRANSPOSE ? input.lhs.get() : &input;
    return leaf && leaf->isLeaf() && isA<VarValue>(leaf->val) &&
           input.type.dtype == expr.type.dtype;
  };
  return product.type.dtype == expr.type.dtype && bias.type.dtype == expr.type.dtype &&
         expr.type.ndim == product.type.ndim && bias.type.ndim <= product.type.ndim &&
         product.lhs->type.ndim == 2 &&
         (product.rhs->type.ndim == 1 || product.rhs->type.ndim == 2) &&
         operand(*product.lhs) && operand(*product.rhs);
}

Var *codegenMatmulAdd(NumPyExpr &expr, CodegenContext &context,
                      Var *destination = nullptr) {
  auto *module = context.M;
  auto operand = [&](NumPyExpr &input) -> Value * {
    if (input.isLeaf())
      return module->Nr<VarValue>(context.vars.at(&input));
    auto *value = module->Nr<VarValue>(context.vars.at(input.lhs.get()));
    auto *transpose =
        module->getOrRealizeFunc("_transpose", {value->getType()}, {}, FUSION_MODULE);
    return util::call(transpose, {value});
  };
  bool productLeft = expr.lhs->op == NumPyExpr::NP_OP_MATMUL;
  auto &product = *(productLeft ? expr.lhs : expr.rhs);
  auto &bias = *(productLeft ? expr.rhs : expr.lhs);
  std::vector<Value *> args{operand(*product.lhs), operand(*product.rhs),
                            operand(bias)};
  args.push_back(destination ? module->Nr<VarValue>(destination)
                             : (*module->getNoneType())());
  std::vector<Type *> types;
  for (auto *arg : args)
    types.push_back(arg->getType());
  // Static modes: product+bias, bias+product, product-bias, bias-product.
  // Specialization keeps the signed BLAS coefficients constant at each call site.
  int64_t mode = (expr.op == NumPyExpr::NP_OP_SUB ? 2 : 0) + (productLeft ? 0 : 1);
  auto *helper = module->getOrRealizeFunc("_matmul_add", types, {mode}, FUSION_MODULE);
  seqassertn(helper, "matmul-add func not found");
  auto *result = util::makeVar(util::call(helper, args), context.series, context.func);
  expr.apply([&](NumPyExpr &leaf) {
    if (leaf.isLeaf() && leaf.freeable) {
      auto *value = module->Nr<VarValue>(context.vars.at(&leaf));
      auto *free =
          module->getOrRealizeFunc("_free", {value->getType()}, {}, FUSION_MODULE);
      context.series->push_back(util::call(free, {value}));
    }
  });
  return result;
}

Var *optimizeHelper(NumPyOptimizationUnit &unit, NumPyExpr *expr, CodegenContext &C) {
  auto *M = unit.value->getModule();
  auto *series = C.series;

  auto freeArray = [&](Var *arr) {
    auto *freeFunc = M->getOrRealizeFunc("_free", {arr->getType()}, {}, FUSION_MODULE);
    seqassertn(freeFunc, "free func not found");
    return util::call(freeFunc, {M->Nr<VarValue>(arr)});
  };

  // Handle non-elementwise operations before building scalar loops. Match a
  // compound matmul first, before materializing its product loses that pattern.
  expr->apply([&](NumPyExpr &e) {
    if (!e.type.isArray())
      return;

    if (isMatmulAdd(e)) {
      XLOG("-> BLAS matmul-add fuse:\n{}", e.str());
      auto *result = codegenMatmulAdd(e, C);
      NumPyExpr replacement(e.type, M->Nr<VarValue>(result));
      replacement.freeable = true;
      e.replace(replacement);
      C.vars[&e] = result;
      return;
    }

    if (e.op == NumPyExpr::NP_OP_TRANSPOSE) {
      auto *lv = optimizeHelper(unit, e.lhs.get(), C);
      auto *transposeFunc =
          M->getOrRealizeFunc("_transpose", {lv->getType()}, {}, FUSION_MODULE);
      seqassertn(transposeFunc, "transpose func not found");
      auto *var = util::makeVar(util::call(transposeFunc, {M->Nr<VarValue>(lv)}),
                                C.series, C.func);
      C.vars[&e] = var;
      NumPyExpr replacement(e.type, M->Nr<VarValue>(var));
      replacement.freeable = e.lhs->freeable;
      e.replace(replacement);
    }

    if (e.op == NumPyExpr::NP_OP_MATMUL) {
      auto *lv = optimizeHelper(unit, e.lhs.get(), C);
      auto *rv = optimizeHelper(unit, e.rhs.get(), C);
      auto *matmulFunc = M->getOrRealizeFunc("_matmul", {lv->getType(), rv->getType()},
                                             {}, FUSION_MODULE);
      seqassertn(matmulFunc, "matmul func not found");
      auto *var = util::makeVar(
          util::call(matmulFunc, {M->Nr<VarValue>(lv), M->Nr<VarValue>(rv)}), C.series,
          C.func);

      bool lfreeable = e.lhs->type.isArray() && (e.lhs->freeable || !e.lhs->isLeaf());
      bool rfreeable = e.rhs->type.isArray() && (e.rhs->freeable || !e.rhs->isLeaf());

      if (lfreeable)
        series->push_back(freeArray(lv));
      if (rfreeable)
        series->push_back(freeArray(rv));

      C.vars[&e] = var;
      NumPyExpr replacement(e.type, M->Nr<VarValue>(var));
      replacement.freeable = true;
      e.replace(replacement);
    }
  });

  // Collapse profitable subtrees to result leaves. When broadcasting is unknown,
  // emit a runtime choice between fusion and sequential array operations.
  bool changed;
  do {
    changed = false;
    expr->apply([&](NumPyExpr &e) {
      if (e.depth() <= 2)
        return;

      auto cost = e.cost();
      auto bcinfo = e.getBroadcastInfo();
      Var *result = nullptr;

      if (cost <= AlwaysFuseCostThreshold ||
          (cost <= NeverFuseCostThreshold && bcinfo == BroadcastInfo::NO)) {
        // Don't care about broadcasting; just fuse.
        XLOG("-> static fuse:\n{}", e.str());
        result = e.codegenFusedEval(C);
      } else if (cost <= NeverFuseCostThreshold && bcinfo != BroadcastInfo::YES) {
        // Check at runtime if we're broadcasting and fuse conditionally.
        XLOG("-> conditional fuse:\n{}", e.str());
        auto *broadcasts = e.codegenBroadcasts(C);
        auto *seqtSeries = M->Nr<SeriesFlow>();
        auto *fuseSeries = M->Nr<SeriesFlow>();
        auto *branch = M->Nr<IfFlow>(broadcasts, seqtSeries, fuseSeries);

        C.series = seqtSeries;
        auto *seqtResult = e.codegenSequentialEval(C);
        C.series = fuseSeries;
        auto *fuseResult = e.codegenFusedEval(C);
        seqassertn(seqtResult->getType()->is(fuseResult->getType()),
                   "types are not the same: {} {}", seqtResult->getType()->getName(),
                   fuseResult->getType()->getName());

        result = M->Nr<Var>(seqtResult->getType(), false);
        unit.func->push_back(result);
        seqtSeries->push_back(M->Nr<AssignInstr>(result, M->Nr<VarValue>(seqtResult)));
        fuseSeries->push_back(M->Nr<AssignInstr>(result, M->Nr<VarValue>(fuseResult)));
        C.series = series;
        series->push_back(branch);
      }

      if (result) {
        NumPyExpr tmp(e.type, M->Nr<VarValue>(result));
        e.replace(tmp);
        e.freeable = true;
        C.vars[&e] = result;
        changed = true;
      }
    });
  } while (changed);

  XLOG("-> sequential eval:\n{}", expr->str());
  return expr->codegenSequentialEval(C);
}

// Ufunc receivers and out=None expressions are absent from the arithmetic tree,
// but removing their calls must not discard observable argument evaluation.
bool hasUFuncArgumentEffects(NumPyExpr &expr,
                             analyze::module::SideEffectResult *sideEffects) {
  bool effects = false;
  expr.apply([&](NumPyExpr &element) {
    if (element.isLeaf())
      return;
    auto *call = cast<CallInstr>(element.val);
    auto *callee = call ? util::getFunc(call->getCallee()) : nullptr;
    if (callee && isUFuncType(callee->getParentType()))
      effects |= sideEffects->hasSideEffect(call->front()) ||
                 sideEffects->hasSideEffect(*std::prev(call->end(), 2));
  });
  return effects;
}
} // namespace

bool NumPyOptimizationUnit::optimize(NumPyPrimitiveTypes &T,
                                     analyze::module::SideEffectResult *sideEffects) {
  bool reduction = expr->isReduction();
  bool ownedLeaf = false;
  expr->apply(
      [&](NumPyExpr &element) { ownedLeaf |= element.isLeaf() && element.freeable; });
  if ((!expr->type.isArray() && !reduction) ||
      (expr->depth() <= 2 && (reduction || !ownedLeaf)) ||
      hasUFuncArgumentEffects(*expr, sideEffects))
    return false;

  for (auto &leaf : leaves) {
    if (sideEffects->hasSideEffect(leaf.second))
      return false;
  }

  if (reduction) {
    auto *call = cast<CallInstr>(expr->val);
    if (!call)
      return false;
    std::vector<Value *> args(call->begin(), call->end());
    for (size_t index = 1; index < args.size(); ++index) {
      if (sideEffects->hasSideEffect(args[index]))
        return false;
    }
    bool supported = expr->lhs->cost() <= NeverFuseCostThreshold;
    expr->lhs->apply([&](NumPyExpr &element) {
      if ((element.op == NumPyExpr::NP_OP_TRANSPOSE && !element.lhs->isLeaf()) ||
          element.op == NumPyExpr::NP_OP_MATMUL || element.isReduction())
        supported = false;
    });
    if (!supported)
      return false;
  }

  XLOG("Optimizing expression at {}\n{}", value->getSrcInfo(), expr->str());

  auto *M = value->getModule();
  auto *series = M->Nr<SeriesFlow>();
  CodegenContext C(M, series, func, T);
  util::CloneVisitor cv(M);

  // Bind leaves once in their recorded evaluation order; every generated path
  // uses these bindings rather than reevaluating the original operands.
  for (auto &p : leaves) {
    auto *var = util::makeVar(cv.clone(p.second), series, func);
    C.vars.emplace(p.first, var);
  }

  if (reduction) {
    expr->lhs->apply([&](NumPyExpr &element) {
      if (element.op == NumPyExpr::NP_OP_TRANSPOSE)
        optimizeHelper(*this, &element, C);
    });
  }

  auto *result =
      reduction ? expr->codegenFusedEval(C) : optimizeHelper(*this, expr.get(), C);
  auto *replacement = M->Nr<FlowInstr>(C.series, M->Nr<VarValue>(result));
  value->replaceAll(replacement);
  return true;
}

// Destination writes bypass the forwarding DAG: prove the output is safe first,
// then lower into that buffer without taking ownership of it or freeing it.
struct DestinationExpression {
  NumPyOptimizationUnit unit;
  Value *destination;
  bool assignment;

  bool eligible(NumPyPrimitiveTypes &types, analyze::dataflow::RDInspector *rd,
                analyze::dataflow::CFGraph *cfg,
                analyze::module::SideEffectResult *sideEffects) {
    auto *output = cast<VarValue>(destination);
    if (!output || output->getVar()->isGlobal() || !unit.expr->type.isArray() ||
        (!assignment && unit.expr->depth() <= 2) ||
        hasUFuncArgumentEffects(*unit.expr, sideEffects))
      return false;
    auto outputType = NumPyType::get(destination->getType(), types);
    if (!outputType.isArray() || outputType.dtype != unit.expr->type.dtype)
      return false;
    if (assignment &&
        (outputType.ndim == 0 ||
         sideEffects->hasSideEffect(*std::next(cast<CallInstr>(unit.value)->begin()))))
      return false;
    auto *base = outputType.getIRBaseType(types);
    bool matmulAdd = isMatmulAdd(*unit.expr);
    if (matmulAdd && outputType.ndim != unit.expr->type.ndim)
      return false;
    bool supported = true;
    // Exact pointwise self-reads are safe, but BLAS must not overwrite either
    // multiplicative input. This applies whichever side contains the product.
    if (matmulAdd) {
      auto &product = unit.expr->lhs->op == NumPyExpr::NP_OP_MATMUL ? unit.expr->lhs
                                                                    : unit.expr->rhs;
      product->apply([&](NumPyExpr &element) {
        auto *read = cast<VarValue>(element.val);
        if (read && read->getVar() == output->getVar())
          supported = false;
      });
    }
    std::unordered_set<id_t> allowed{destination->getId()};
    unit.expr->apply([&](NumPyExpr &element) {
      if (element.type.getIRBaseType(types) != base ||
          (!matmulAdd && element.type.ndim > outputType.ndim))
        supported = false;
      if (element.isLeaf()) {
        if ((!isA<VarValue>(element.val) && !isA<Const>(element.val)) ||
            sideEffects->hasSideEffect(element.val))
          supported = false;
        allowed.insert(element.val->getId());
        return;
      }
      switch (element.op) {
      case NumPyExpr::NP_OP_ADD:
      case NumPyExpr::NP_OP_SUB:
      case NumPyExpr::NP_OP_MUL:
      case NumPyExpr::NP_OP_NEG:
      case NumPyExpr::NP_OP_POS:
        break;
      case NumPyExpr::NP_OP_MATMUL:
      case NumPyExpr::NP_OP_TRANSPOSE:
        if (!matmulAdd)
          supported = false;
        break;
      default:
        supported = false;
      }
    });
    if (!supported)
      return false;

    // Prove disjointness from a single fresh allocation in this block, not from
    // general alias analysis. Earlier or cross-block uses may expose the buffer.
    auto definitions = rd->getReachingDefinitions(output->getVar(), destination);
    if (definitions.size() != 1)
      return false;
    auto *definition = cast<AssignInstr>(definitions[0].assignment);
    auto *allocation = definition ? cast<CallInstr>(definition->getRhs()) : nullptr;
    auto *allocator = allocation ? util::getFunc(allocation->getCallee()) : nullptr;
    if (!allocator)
      return false;
    auto name = allocator->getUnmangledName();
    if ((name != "empty" && name != "zeros" && name != "ones" && name != "full" &&
         name != "empty_like" && name != "zeros_like" && name != "ones_like" &&
         name != "full_like") ||
        allocator->getName().rfind(
            ast::getMangledFunc("std.numpy.routines", name) + "[", 0) != 0)
      return false;
    auto *block = cfg->getBlock(unit.value);
    if (!block || cfg->getBlock(definition) != block)
      return false;
    std::unordered_set<id_t> preceding;
    for (const auto *value : *block) {
      if (value == unit.value)
        break;
      preceding.insert(value->getId());
    }
    if (!preceding.count(definition->getId()))
      return false;

    struct CheckUses : public util::Operator {
      Var *output;
      const AssignInstr *definition;
      analyze::dataflow::RDInspector *rd;
      analyze::dataflow::CFGraph *cfg;
      analyze::dataflow::CFBlock *block;
      const std::unordered_set<id_t> &allowed;
      const std::unordered_set<id_t> &preceding;
      bool safe = true;

      CheckUses(Var *output, const AssignInstr *definition,
                analyze::dataflow::RDInspector *rd, analyze::dataflow::CFGraph *cfg,
                analyze::dataflow::CFBlock *block,
                const std::unordered_set<id_t> &allowed,
                const std::unordered_set<id_t> &preceding)
          : output(output), definition(definition), rd(rd), cfg(cfg), block(block),
            allowed(allowed), preceding(preceding) {}

      void preHook(Node *node) override {
        auto *value = cast<Value>(node);
        if (!safe || !value || value == definition || allowed.count(value->getId()))
          return;
        auto variables = value->getUsedVariables();
        if (std::find(variables.begin(), variables.end(), output) == variables.end())
          return;
        for (auto &reaching : rd->getReachingDefinitions(output, value)) {
          if (reaching.assignment->getId() == definition->getId() &&
              (cfg->getBlock(value) != block || preceding.count(value->getId())))
            safe = false;
        }
      }
    } uses(output->getVar(), definition, rd, cfg, block, allowed, preceding);
    unit.func->accept(uses);
    return uses.safe;
  }

  void optimize(NumPyPrimitiveTypes &types) {
    auto *module = unit.value->getModule();
    auto *series = module->Nr<SeriesFlow>();
    CodegenContext context(module, series, unit.func, types);
    util::CloneVisitor clone(module);
    for (auto &leaf : unit.leaves)
      context.vars.emplace(leaf.first,
                           util::makeVar(clone.clone(leaf.second), series, unit.func));
    auto *output = util::makeVar(clone.clone(destination), series, unit.func);
    XLOG("-> destination fuse at {}:\n{}", unit.value->getSrcInfo(), unit.expr->str());
    auto *result = isMatmulAdd(*unit.expr)
                       ? codegenMatmulAdd(*unit.expr, context, output)
                       : unit.expr->codegenFusedEval(context, output);
    Value *returned =
        assignment ? (*module->getNoneType())() : module->Nr<VarValue>(result);
    unit.value->replaceAll(module->Nr<FlowInstr>(series, returned));
  }
};

// Consider store-like expressions before their RHS. Claim their nodes only after
// the destination proof succeeds, so rejected stores still allow ordinary fusion.
struct ExtractArrayExpressions : public util::Operator {
  BodiedFunc *func;
  NumPyPrimitiveTypes types;
  analyze::dataflow::RDInspector *rd;
  analyze::dataflow::CFGraph *cfg;
  analyze::module::SideEffectResult *sideEffects;
  std::vector<NumPyOptimizationUnit> exprs;
  std::vector<DestinationExpression> destinations;
  std::unordered_set<id_t> extracted;

  ExtractArrayExpressions(BodiedFunc *func, analyze::dataflow::RDInspector *rd,
                          analyze::dataflow::CFGraph *cfg,
                          analyze::module::SideEffectResult *sideEffects)
      : func(func), types(func->getModule()), rd(rd), cfg(cfg),
        sideEffects(sideEffects) {}

  void extract(Value *v, AssignInstr *assign = nullptr) {
    if (extracted.count(v->getId()))
      return;

    std::vector<std::pair<NumPyExpr *, Value *>> leaves;
    Value *destination = nullptr;
    auto *call = cast<CallInstr>(v);
    auto *callee = call ? util::getFunc(call->getCallee()) : nullptr;
    bool assignment = callee && isArrayType(callee->getParentType()) &&
                      callee->getUnmangledName() == Module::SETITEM_MAGIC_NAME &&
                      call->numArgs() == 3;
    if (assignment) {
      auto *slice = *std::next(call->begin());
      auto sliceName = ast::getMangledClass("std.internal.types.slice", "Slice");
      auto *constructor = cast<CallInstr>(slice);
      auto *sliceFunc = constructor ? util::getFunc(constructor->getCallee()) : nullptr;
      assignment = slice->getType()->getName().rfind(sliceName + "[", 0) == 0 &&
                   sliceFunc &&
                   sliceFunc->getName().rfind(sliceName + ".__new__:", 0) == 0 &&
                   constructor->numArgs() == 3;
      if (assignment) {
        auto *optionalInt =
            v->getModule()->getOptionalType(v->getModule()->getIntType());
        for (auto *bound : *constructor) {
          auto *empty = cast<CallInstr>(bound);
          auto *emptyFunc = empty ? util::getFunc(empty->getCallee()) : nullptr;
          if (!emptyFunc || !bound->getType()->is(optionalInt) ||
              emptyFunc->getName().rfind("Optional[Int[64]]:Optional.__new__:", 0) !=
                  0 ||
              empty->numArgs() != 0)
            assignment = false;
        }
      }
      if (assignment)
        destination = call->front();
    }
    auto expr = assignment ? parse(call->back(), leaves, types)
                           : parse(v, leaves, types, true, &destination);
    if (destination && expr && !expr->isLeaf()) {
      DestinationExpression candidate{
          {v, func, std::move(expr), std::move(leaves), nullptr},
          destination,
          assignment};
      if (candidate.eligible(types, rd, cfg, sideEffects)) {
        candidate.unit.expr->apply(
            [&](NumPyExpr &element) { extracted.insert(element.val->getId()); });
        extracted.insert(v->getId());
        destinations.push_back(std::move(candidate));
        return;
      }
      leaves.clear();
      expr = parse(v, leaves, types, true);
    }
    if (expr) {
      int64_t numArrayNodes = 0;
      expr->apply([&](NumPyExpr &e) {
        if (e.type.isArray())
          ++numArrayNodes;
        extracted.emplace(e.val->getId());
      });
      if (numArrayNodes > 0 && expr->depth() > 1) {
        exprs.push_back({v, func, std::move(expr), std::move(leaves), assign});
      }
    }
  }

  void preHook(Node *n) override {
    if (auto *v = cast<AssignInstr>(n)) {
      extract(v->getRhs(), v->getLhs()->isGlobal() ? nullptr : v);
    } else if (auto *v = cast<Value>(n)) {
      extract(v);
    }
  }
};

const std::string NumPyFusionPass::KEY = "core-numpy-fusion";

const std::string NumPyInlinePass::KEY = "core-numpy-inline";

// Open bounded, single-expression user helpers so the fusion parser can see
// through calls without requiring general-purpose inlining of the NumPy library.
void NumPyInlinePass::visit(BodiedFunc *func) {
  struct Inliner : public util::Operator {
    BodiedFunc *parent;
    NumPyPrimitiveTypes types;
    unsigned remaining = 16;

    explicit Inliner(BodiedFunc *parent) : parent(parent), types(parent->getModule()) {}

    void handle(CallInstr *call) override {
      if (!remaining || !isArrayType(call->getType()))
        return;
      auto *callee = cast<BodiedFunc>(util::getFunc(call->getCallee()));
      if (!callee || callee == parent ||
          callee->getName().rfind("std.numpy.", 0) == 0 ||
          util::hasAttribute(
              callee, ast::getMangledFunc("std.internal.attributes", "noinline")))
        return;
      auto *body = cast<SeriesFlow>(callee->getBody());
      if (!body || std::distance(body->begin(), body->end()) != 1)
        return;
      auto *result = cast<ReturnInstr>(body->front());
      if (!result || !result->getValue())
        return;
      std::vector<std::pair<NumPyExpr *, Value *>> leaves;
      auto expression = parse(result->getValue(), leaves, types);
      if (!expression || expression->isLeaf() || expression->nodes() > 16)
        return;
      auto *module = call->getModule();
      util::CloneVisitor clone(module);
      auto *bindings = module->Nr<SeriesFlow>();
      bool directBindings = true;
      for (auto *value : *call) {
        auto *read = cast<VarValue>(value);
        directBindings &= read && !read->getVar()->isGlobal();
      }
      for (auto &leaf : leaves) {
        auto *read = cast<VarValue>(leaf.second);
        directBindings &= (read && !read->getVar()->isGlobal()) ||
                          isA<IntConst>(leaf.second) || isA<FloatConst>(leaf.second) ||
                          isA<BoolConst>(leaf.second);
      }
      auto argument = callee->arg_begin();
      for (auto *value : *call) {
        if (directBindings) {
          clone.forceRemap(*argument++, cast<VarValue>(value)->getVar());
        } else {
          auto *variable = clone.forceClone(*argument++);
          parent->push_back(variable);
          bindings->push_back(module->Nr<AssignInstr>(variable, value));
        }
      }
      for (auto *variable : *callee)
        parent->push_back(clone.forceClone(variable));
      auto *replacement = clone.clone(result->getValue());
      if (!directBindings)
        replacement = module->Nr<FlowInstr>(bindings, replacement);
      call->replaceAll(replacement);
      --remaining;
    }
  } inliner(func);
  func->accept(inliner);
}

void NumPyFusionPass::visit(BodiedFunc *func) {
  auto *rdres = getAnalysisResult<analyze::dataflow::RDResult>(reachingDefKey);
  auto it = rdres->results.find(func->getId());
  if (it == rdres->results.end())
    return;
  auto *rd = it->second.get();
  auto *se = getAnalysisResult<analyze::module::SideEffectResult>(sideEffectsKey);
  auto *cfg = rdres->cfgResult->graphs.find(func->getId())->second.get();
  ExtractArrayExpressions extractor(func, rd, cfg, se);
  func->accept(extractor);
  if (extractor.exprs.empty() && extractor.destinations.empty())
    return;
  auto fwd = getForwardingDAGs(func, rd, cfg, se, extractor.exprs);
  for (auto &destination : extractor.destinations)
    destination.optimize(extractor.types);

  for (auto &dag : fwd) {
    std::vector<AssignInstr *> assignsToDelete;
    auto *e = doForwarding(dag, assignsToDelete);
    if (e->optimize(extractor.types, se)) {
      // Remove producer assignments only after their replacement was emitted;
      // rejected candidates must retain the original computation and lifetime.
      for (auto *a : assignsToDelete)
        a->replaceAll(func->getModule()->Nr<SeriesFlow>());
    }
  }
}

} // namespace numpy
} // namespace transform
} // namespace ir
} // namespace codon
