// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "module.h"

#include <algorithm>
#include <memory>

#include "codon/cir/func.h"
#include "codon/parser/cache.h"

namespace codon {
namespace ir {
namespace {
std::vector<codon::ast::types::TypePtr>
translateGenerics(codon::ast::Cache *cache, std::vector<types::Generic> &generics) {
  std::vector<codon::ast::types::TypePtr> ret;
  for (auto &g : generics) {
    seqassertn(g.isStatic() || g.getTypeValue(), "generic must be static or a type");
    ret.push_back(std::make_shared<codon::ast::types::LinkType>(
        g.isStatic()
            ? std::make_shared<codon::ast::types::StaticType>(cache, g.getStaticValue())
            : (g.isStaticStr() ? std::make_shared<codon::ast::types::StaticType>(
                                     cache, g.getStaticStringValue())
                               : g.getTypeValue()->getAstType())));
  }
  return ret;
}

std::vector<codon::ast::types::TypePtr>
generateDummyNames(std::vector<types::Type *> &types) {
  std::vector<codon::ast::types::TypePtr> ret;
  for (auto *t : types) {
    seqassertn(t->getAstType(), "{} must have an ast type", *t);
    ret.emplace_back(t->getAstType());
  }
  return ret;
}

std::vector<codon::ast::types::TypePtr>
translateArgs(codon::ast::Cache *cache, std::vector<types::Type *> &types) {
  std::vector<codon::ast::types::TypePtr> ret = {
      std::make_shared<codon::ast::types::LinkType>(
          cache, codon::ast::types::LinkType::Kind::Unbound, 0)};
  for (auto *t : types) {
    seqassertn(t->getAstType(), "{} must have an ast type", *t);
    if (auto f = t->getAstType()->getFunc()) {
      auto *irType = cast<types::FuncType>(t);
      std::vector<char> mask(std::distance(irType->begin(), irType->end()), 0);
      ret.push_back(std::make_shared<codon::ast::types::PartialType>(
          t->getAstType()->getRecord(), f, mask));
    } else {
      ret.push_back(t->getAstType());
    }
  }
  return ret;
}
} // namespace

const std::string Module::VOID_NAME = "void";
const std::string Module::BOOL_NAME = "bool";
const std::string Module::BYTE_NAME = "byte";
const std::string Module::INT_NAME = "int";
const std::string Module::FLOAT_NAME = "float";
const std::string Module::FLOAT32_NAME = "float32";
const std::string Module::FLOAT16_NAME = "float16";
const std::string Module::BFLOAT16_NAME = "bfloat16";
const std::string Module::FLOAT128_NAME = "float128";
const std::string Module::STRING_NAME = "str";

const std::string Module::EQ_MAGIC_NAME = "__eq__";
const std::string Module::NE_MAGIC_NAME = "__ne__";
const std::string Module::LT_MAGIC_NAME = "__lt__";
const std::string Module::GT_MAGIC_NAME = "__gt__";
const std::string Module::LE_MAGIC_NAME = "__le__";
const std::string Module::GE_MAGIC_NAME = "__ge__";

const std::string Module::POS_MAGIC_NAME = "__pos__";
const std::string Module::NEG_MAGIC_NAME = "__neg__";
const std::string Module::INVERT_MAGIC_NAME = "__invert__";
const std::string Module::ABS_MAGIC_NAME = "__abs__";

const std::string Module::ADD_MAGIC_NAME = "__add__";
const std::string Module::SUB_MAGIC_NAME = "__sub__";
const std::string Module::MUL_MAGIC_NAME = "__mul__";
const std::string Module::MATMUL_MAGIC_NAME = "__matmul__";
const std::string Module::TRUE_DIV_MAGIC_NAME = "__truediv__";
const std::string Module::FLOOR_DIV_MAGIC_NAME = "__floordiv__";
const std::string Module::MOD_MAGIC_NAME = "__mod__";
const std::string Module::POW_MAGIC_NAME = "__pow__";
const std::string Module::LSHIFT_MAGIC_NAME = "__lshift__";
const std::string Module::RSHIFT_MAGIC_NAME = "__rshift__";
const std::string Module::AND_MAGIC_NAME = "__and__";
const std::string Module::OR_MAGIC_NAME = "__or__";
const std::string Module::XOR_MAGIC_NAME = "__xor__";

const std::string Module::IADD_MAGIC_NAME = "__iadd__";
const std::string Module::ISUB_MAGIC_NAME = "__isub__";
const std::string Module::IMUL_MAGIC_NAME = "__imul__";
const std::string Module::IMATMUL_MAGIC_NAME = "__imatmul__";
const std::string Module::ITRUE_DIV_MAGIC_NAME = "__itruediv__";
const std::string Module::IFLOOR_DIV_MAGIC_NAME = "__ifloordiv__";
const std::string Module::IMOD_MAGIC_NAME = "__imod__";
const std::string Module::IPOW_MAGIC_NAME = "__ipow__";
const std::string Module::ILSHIFT_MAGIC_NAME = "__ilshift__";
const std::string Module::IRSHIFT_MAGIC_NAME = "__irshift__";
const std::string Module::IAND_MAGIC_NAME = "__iand__";
const std::string Module::IOR_MAGIC_NAME = "__ior__";
const std::string Module::IXOR_MAGIC_NAME = "__ixor__";

const std::string Module::RADD_MAGIC_NAME = "__radd__";
const std::string Module::RSUB_MAGIC_NAME = "__rsub__";
const std::string Module::RMUL_MAGIC_NAME = "__rmul__";
const std::string Module::RMATMUL_MAGIC_NAME = "__rmatmul__";
const std::string Module::RTRUE_DIV_MAGIC_NAME = "__rtruediv__";
const std::string Module::RFLOOR_DIV_MAGIC_NAME = "__rfloordiv__";
const std::string Module::RMOD_MAGIC_NAME = "__rmod__";
const std::string Module::RPOW_MAGIC_NAME = "__rpow__";
const std::string Module::RLSHIFT_MAGIC_NAME = "__rlshift__";
const std::string Module::RRSHIFT_MAGIC_NAME = "__rrshift__";
const std::string Module::RAND_MAGIC_NAME = "__rand__";
const std::string Module::ROR_MAGIC_NAME = "__ror__";
const std::string Module::RXOR_MAGIC_NAME = "__rxor__";

const std::string Module::INT_MAGIC_NAME = "__int__";
const std::string Module::FLOAT_MAGIC_NAME = "__float__";
const std::string Module::BOOL_MAGIC_NAME = "__bool__";
const std::string Module::STR_MAGIC_NAME = "__str__";
const std::string Module::REPR_MAGIC_NAME = "__repr__";
const std::string Module::CALL_MAGIC_NAME = "__call__";

const std::string Module::GETITEM_MAGIC_NAME = "__getitem__";
const std::string Module::SETITEM_MAGIC_NAME = "__setitem__";
const std::string Module::ITER_MAGIC_NAME = "__iter__";
const std::string Module::LEN_MAGIC_NAME = "__len__";

const std::string Module::NEW_MAGIC_NAME = "__new__";
const std::string Module::INIT_MAGIC_NAME = "__init__";

const char Module::NodeId = 0;

Module::Module(const std::string &name) : AcceptorExtend(name) {
  mainFunc = std::make_unique<BodiedFunc>("main");
  mainFunc->realize(cast<types::FuncType>(unsafeGetDummyFuncType()), {});
  mainFunc->setModule(this);
  mainFunc->setReplaceable(false);
  argVar = std::make_unique<Var>(unsafeGetArrayType(getStringType()), /*global=*/true,
                                 /*external=*/false, ".argv");
  argVar->setModule(this);
  argVar->setReplaceable(false);
}

void Module::parseCode(const std::string &code) { cache->parseCode(code); }

Func *Module::getOrRealizeMethod(types::Type *parent, const std::string &methodName,
                                 std::vector<types::Type *> args,
                                 std::vector<types::Generic> generics) {

  auto cls =
      std::const_pointer_cast<ast::types::Type>(parent->getAstType())->getClass();
  auto method = cache->findMethod(cls.get(), methodName, generateDummyNames(args));
  if (!method)
    return nullptr;
  try {
    return cache->realizeFunction(method, translateArgs(cache, args),
                                  translateGenerics(cache, generics), cls);
  } catch (const exc::ParserException &e) {
    for (int i = 0; i < e.messages.size(); i++)
      LOG_IR("getOrRealizeMethod parser error at {}: {}", e.locations[i],
             e.messages[i]);
    return nullptr;
  }
}

Func *Module::getOrRealizeFunc(const std::string &funcName,
                               std::vector<types::Type *> args,
                               std::vector<types::Generic> generics,
                               const std::string &module) {
  auto fqName =
      module.empty() ? funcName : fmt::format(FMT_STRING("{}.{}"), module, funcName);
  auto func = cache->findFunction(fqName);
  if (!func)
    return nullptr;
  auto arg = translateArgs(cache, args);
  auto gens = translateGenerics(cache, generics);
  try {
    return cache->realizeFunction(func, arg, gens);
  } catch (const exc::ParserException &e) {
    for (int i = 0; i < e.messages.size(); i++)
      LOG_IR("getOrRealizeFunc parser error at {}: {}", e.locations[i], e.messages[i]);
    return nullptr;
  }
}

types::Type *Module::getOrRealizeType(const std::string &typeName,
                                      std::vector<types::Generic> generics,
                                      const std::string &module) {
  auto fqName =
      module.empty() ? typeName : fmt::format(FMT_STRING("{}.{}"), module, typeName);
  auto type = cache->findClass(fqName);
  if (!type)
    return nullptr;
  try {
    return cache->realizeType(type, translateGenerics(cache, generics));
  } catch (const exc::ParserException &e) {
    for (int i = 0; i < e.messages.size(); i++)
      LOG_IR("getOrRealizeType parser error at {}: {}", e.locations[i], e.messages[i]);
    return nullptr;
  }
}

types::Type *Module::getVoidType() {
  if (auto *rVal = getType(VOID_NAME))
    return rVal;
  return Nr<types::VoidType>();
}

types::Type *Module::getBoolType() {
  if (auto *rVal = getType(BOOL_NAME))
    return rVal;
  return Nr<types::BoolType>();
}

types::Type *Module::getByteType() {
  if (auto *rVal = getType(BYTE_NAME))
    return rVal;
  return Nr<types::ByteType>();
}

types::Type *Module::getIntType() {
  if (auto *rVal = getType(INT_NAME))
    return rVal;
  return Nr<types::IntType>();
}

types::Type *Module::getFloatType() {
  if (auto *rVal = getType(FLOAT_NAME))
    return rVal;
  return Nr<types::FloatType>();
}

types::Type *Module::getFloat32Type() {
  if (auto *rVal = getType(FLOAT32_NAME))
    return rVal;
  return Nr<types::Float32Type>();
}

types::Type *Module::getFloat16Type() {
  if (auto *rVal = getType(FLOAT16_NAME))
    return rVal;
  return Nr<types::Float16Type>();
}

types::Type *Module::getBFloat16Type() {
  if (auto *rVal = getType(BFLOAT16_NAME))
    return rVal;
  return Nr<types::BFloat16Type>();
}

types::Type *Module::getFloat128Type() {
  if (auto *rVal = getType(FLOAT128_NAME))
    return rVal;
  return Nr<types::Float128Type>();
}

types::Type *Module::getStringType() {
  if (auto *rVal = getType(STRING_NAME))
    return rVal;
  return Nr<types::RecordType>(
      STRING_NAME,
      std::vector<types::Type *>{getIntType(), unsafeGetPointerType(getByteType())},
      std::vector<std::string>{"len", "ptr"});
}

types::Type *Module::getPointerType(types::Type *base) {
  return getOrRealizeType("Ptr", {base});
}

types::Type *Module::getArrayType(types::Type *base) {
  return getOrRealizeType("Array", {base});
}

types::Type *Module::getGeneratorType(types::Type *base) {
  return getOrRealizeType("Generator", {base});
}

types::Type *Module::getOptionalType(types::Type *base) {
  return getOrRealizeType("Optional", {base});
}

types::Type *Module::getFuncType(types::Type *rType,
                                 std::vector<types::Type *> argTypes, bool variadic) {
  auto args = translateArgs(cache, argTypes);
  args[0] = std::make_shared<codon::ast::types::LinkType>(rType->getAstType());
  auto *result = cache->makeFunction(args);
  if (variadic) {
    // Type checker types have no concept of variadic functions, so we will
    // create a new IR type here with the same AST type.
    auto *f = cast<types::FuncType>(result);
    result = unsafeGetFuncType(f->getName() + "$variadic", f->getReturnType(),
                               std::vector<types::Type *>(f->begin(), f->end()),
                               /*variadic=*/true);
    result->setAstType(f->getAstType());
  }
  return result;
}

types::Type *Module::getIntNType(unsigned int len, bool sign) {
  return getOrRealizeType(sign ? "Int" : "UInt", {len});
}

types::Type *Module::getVectorType(unsigned count, types::Type *base) {
  return getOrRealizeType("Vec", {base, count});
}

types::Type *Module::getTupleType(std::vector<types::Type *> args) {
  std::vector<ast::types::TypePtr> argTypes;
  for (auto *t : args) {
    seqassertn(t->getAstType(), "{} must have an ast type", *t);
    argTypes.push_back(t->getAstType());
  }
  return cache->makeTuple(argTypes);
}

types::Type *Module::getUnionType(std::vector<types::Type *> types) {
  std::vector<ast::types::TypePtr> argTypes;
  for (auto *t : types) {
    seqassertn(t->getAstType(), "{} must have an ast type", *t);
    argTypes.push_back(t->getAstType());
  }
  return cache->makeUnion(argTypes);
}

types::Type *Module::getNoneType() { return getOrRealizeType("NoneType"); }

Value *Module::getInt(int64_t v) { return Nr<IntConst>(v, getIntType()); }

Value *Module::getFloat(double v) { return Nr<FloatConst>(v, getFloatType()); }

Value *Module::getBool(bool v) { return Nr<BoolConst>(v, getBoolType()); }

Value *Module::getString(std::string v) {
  return Nr<StringConst>(std::move(v), getStringType());
}

types::Type *Module::unsafeGetDummyFuncType() {
  return unsafeGetFuncType("<internal_func_type>", getVoidType(), {});
}

types::Type *Module::unsafeGetPointerType(types::Type *base) {
  auto name = types::PointerType::getInstanceName(base);
  if (auto *rVal = getType(name))
    return rVal;
  return Nr<types::PointerType>(base);
}

types::Type *Module::unsafeGetArrayType(types::Type *base) {
  auto name = fmt::format(FMT_STRING(".Array[{}]"), base->referenceString());
  if (auto *rVal = getType(name))
    return rVal;
  std::vector<types::Type *> members = {getIntType(), unsafeGetPointerType(base)};
  std::vector<std::string> names = {"len", "ptr"};
  return Nr<types::RecordType>(name, members, names);
}

types::Type *Module::unsafeGetGeneratorType(types::Type *base) {
  auto name = types::GeneratorType::getInstanceName(base);
  if (auto *rVal = getType(name))
    return rVal;
  return Nr<types::GeneratorType>(base);
}

types::Type *Module::unsafeGetOptionalType(types::Type *base) {
  auto name = types::OptionalType::getInstanceName(base);
  if (auto *rVal = getType(name))
    return rVal;
  return Nr<types::OptionalType>(base);
}

types::Type *Module::unsafeGetFuncType(const std::string &name, types::Type *rType,
                                       std::vector<types::Type *> argTypes,
                                       bool variadic) {
  if (auto *rVal = getType(name))
    return rVal;
  return Nr<types::FuncType>(name, rType, std::move(argTypes), variadic);
}

types::Type *Module::unsafeGetMemberedType(const std::string &name, bool ref) {
  auto *rVal = getType(name);

  if (!rVal) {
    if (ref) {
      auto contentName = name + ".contents";
      auto *record = getType(contentName);
      if (!record) {
        record = Nr<types::RecordType>(contentName);
      }
      rVal = Nr<types::RefType>(name, cast<types::RecordType>(record));
    } else {
      rVal = Nr<types::RecordType>(name);
    }
  }

  return rVal;
}

types::Type *Module::unsafeGetIntNType(unsigned int len, bool sign) {
  auto name = types::IntNType::getInstanceName(len, sign);
  if (auto *rVal = getType(name))
    return rVal;
  return Nr<types::IntNType>(len, sign);
}

types::Type *Module::unsafeGetVectorType(unsigned int count, types::Type *base) {
  auto *primitive = cast<types::PrimitiveType>(base);
  auto name = types::VectorType::getInstanceName(count, primitive);
  if (auto *rVal = getType(name))
    return rVal;
  seqassertn(primitive, "base type must be a primitive type");
  return Nr<types::VectorType>(count, primitive);
}

types::Type *Module::unsafeGetUnionType(const std::vector<types::Type *> &types) {
  auto name = types::UnionType::getInstanceName(types);
  if (auto *rVal = getType(name))
    return rVal;
  return Nr<types::UnionType>(types);
}

} // namespace ir
} // namespace codon
