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
namespace {
llvm::cl::opt<int> AlwaysFuseCostThreshold(
    "npfuse-always", llvm::cl::desc("Expression cost below which (<=) to always fuse"),
    llvm::cl::init(10));

llvm::cl::opt<int> NeverFuseCostThreshold(
    "npfuse-never", llvm::cl::desc("Expression cost above which (>) to never fuse"),
    llvm::cl::init(50));

llvm::cl::opt<bool> Verbose("npfuse-verbose",
                            llvm::cl::desc("Print information about fused expressions"),
                            llvm::cl::init(false));

bool isArrayType(types::Type *t) {
  return t && isA<types::RecordType>(t) &&
         t->getName().rfind(ast::getMangledClass("std.numpy.ndarray", "ndarray") + "[",
                            0) == 0;
}

bool isUFuncType(types::Type *t) {
  return t &&
         (t->getName().rfind(
              ast::getMangledClass("std.numpy.ufunc", "UnaryUFunc") + "[", 0) == 0 ||
          t->getName().rfind(
              ast::getMangledClass("std.numpy.ufunc", "BinaryUFunc") + "[", 0) == 0);
}

bool isNoneType(types::Type *t, NumPyPrimitiveTypes &T) {
  return t && (t->is(T.none) || t->is(T.optnone));
}
} // namespace

const std::string FUSION_MODULE = "std.numpy.fusion";

NumPyPrimitiveTypes::NumPyPrimitiveTypes(Module *M)
    : none(M->getNoneType()), optnone(M->getOptionalType(none)),
      bool_(M->getBoolType()), i8(M->getIntNType(8, true)),
      u8(M->getIntNType(8, false)), i16(M->getIntNType(16, true)),
      u16(M->getIntNType(16, false)), i32(M->getIntNType(32, true)),
      u32(M->getIntNType(32, false)), i64(M->getIntType()),
      u64(M->getIntNType(64, false)), f16(M->getFloat16Type()),
      f32(M->getFloat32Type()), f64(M->getFloatType()),
      c64(M->getType(ast::getMangledClass("std.internal.types.complex", "complex64"))),
      c128(M->getType(ast::getMangledClass("std.internal.types.complex", "complex"))) {}

NumPyType::NumPyType(Type dtype, int64_t ndim) : dtype(dtype), ndim(ndim) {
  seqassertn(ndim >= 0, "ndim must be non-negative");
}

NumPyType::NumPyType() : NumPyType(NP_TYPE_NONE) {}

NumPyType NumPyType::get(types::Type *t, NumPyPrimitiveTypes &T) {
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

types::Type *NumPyType::getIRBaseType(NumPyPrimitiveTypes &T) const {
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
  static const std::unordered_map<NumPyType::Type, std::string> typestrings = {
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
                                 NumPyPrimitiveTypes &T) {
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

  auto getNumPyExprType = [](types::Type *t, NumPyPrimitiveTypes &T) -> NumPyType {
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

  // Don't break up expressions that result in scalars or 0-dim arrays since those
  // should only be computed once
  if (type.ndim == 0) {
    auto res = std::make_unique<NumPyExpr>(type, v);
    leaves.emplace_back(res.get(), v);
    return std::move(res);
  }

  if (auto *c = cast<CallInstr>(v)) {
    auto *f = util::getFunc(c->getCallee());

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

            if (!isNoneType(out->getType(), T))
              break;

            auto op = u.op;
            auto lhs = parse(args[1], leaves, T);
            if (!lhs)
              return {};

            if (u.args == 1)
              return std::make_unique<NumPyExpr>(type, v, op, std::move(lhs));

            auto rhs = parse(args[2], leaves, T);
            if (!rhs)
              return {};

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
Var *optimizeHelper(NumPyOptimizationUnit &unit, NumPyExpr *expr, CodegenContext &C) {
  auto *M = unit.value->getModule();
  auto *series = C.series;

  // Remove some operations that cannot be done element-wise easily by optimizing them
  // separately, recursively.
  expr->apply([&](NumPyExpr &e) {
    if (!e.type.isArray())
      return;

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
      C.vars[&e] = var;
      NumPyExpr replacement(e.type, M->Nr<VarValue>(var));
      replacement.freeable = true;
      e.replace(replacement);
    }
  });

  // Optimize the given expression
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
} // namespace

bool NumPyOptimizationUnit::optimize(NumPyPrimitiveTypes &T) {
  if (!expr->type.isArray() || expr->depth() <= 2)
    return false;

  XLOG("Optimizing expression at {}\n{}", value->getSrcInfo(), expr->str());

  auto *M = value->getModule();
  auto *series = M->Nr<SeriesFlow>();
  CodegenContext C(M, series, func, T);
  util::CloneVisitor cv(M);

  for (auto &p : leaves) {
    auto *var = util::makeVar(cv.clone(p.second), series, func);
    C.vars.emplace(p.first, var);
  }

  auto *result = optimizeHelper(*this, expr.get(), C);
  auto *replacement = M->Nr<FlowInstr>(C.series, M->Nr<VarValue>(result));
  value->replaceAll(replacement);
  return true;
}

struct ExtractArrayExpressions : public util::Operator {
  BodiedFunc *func;
  NumPyPrimitiveTypes types;
  std::vector<NumPyOptimizationUnit> exprs;
  std::unordered_set<id_t> extracted;

  explicit ExtractArrayExpressions(BodiedFunc *func)
      : util::Operator(), func(func), types(func->getModule()), exprs(), extracted() {}

  void extract(Value *v, AssignInstr *assign = nullptr) {
    if (extracted.count(v->getId()))
      return;

    std::vector<std::pair<NumPyExpr *, Value *>> leaves;
    auto expr = parse(v, leaves, types);
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

void NumPyFusionPass::visit(BodiedFunc *func) {
  ExtractArrayExpressions extractor(func);
  func->accept(extractor);

  if (extractor.exprs.empty())
    return;

  auto *rdres = getAnalysisResult<analyze::dataflow::RDResult>(reachingDefKey);
  auto it = rdres->results.find(func->getId());
  if (it == rdres->results.end())
    return;
  auto *rd = it->second.get();
  auto *se = getAnalysisResult<analyze::module::SideEffectResult>(sideEffectsKey);
  auto *cfg = rdres->cfgResult->graphs.find(func->getId())->second.get();
  auto fwd = getForwardingDAGs(func, rd, cfg, se, extractor.exprs);

  for (auto &dag : fwd) {
    std::vector<AssignInstr *> assignsToDelete;
    auto *e = doForwarding(dag, assignsToDelete);
    if (e->optimize(extractor.types)) {
      for (auto *a : assignsToDelete)
        a->replaceAll(func->getModule()->Nr<SeriesFlow>());
    }
  }
}

} // namespace numpy
} // namespace transform
} // namespace ir
} // namespace codon
