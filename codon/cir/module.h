// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>

#include "codon/cir/func.h"
#include "codon/cir/util/iterators.h"
#include "codon/cir/value.h"
#include "codon/cir/var.h"
#include "codon/util/common.h"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace codon {

namespace ast {
struct Cache;
class TranslateVisitor;
class TypecheckVisitor;
} // namespace ast

namespace ir {

/// CIR object representing a program.
class Module : public AcceptorExtend<Module, Node> {
public:
  static const std::string VOID_NAME;
  static const std::string BOOL_NAME;
  static const std::string BYTE_NAME;
  static const std::string INT_NAME;
  static const std::string FLOAT_NAME;
  static const std::string FLOAT32_NAME;
  static const std::string FLOAT16_NAME;
  static const std::string BFLOAT16_NAME;
  static const std::string FLOAT128_NAME;
  static const std::string STRING_NAME;

  static const std::string EQ_MAGIC_NAME;
  static const std::string NE_MAGIC_NAME;
  static const std::string LT_MAGIC_NAME;
  static const std::string GT_MAGIC_NAME;
  static const std::string LE_MAGIC_NAME;
  static const std::string GE_MAGIC_NAME;

  static const std::string POS_MAGIC_NAME;
  static const std::string NEG_MAGIC_NAME;
  static const std::string INVERT_MAGIC_NAME;
  static const std::string ABS_MAGIC_NAME;

  static const std::string ADD_MAGIC_NAME;
  static const std::string SUB_MAGIC_NAME;
  static const std::string MUL_MAGIC_NAME;
  static const std::string MATMUL_MAGIC_NAME;
  static const std::string TRUE_DIV_MAGIC_NAME;
  static const std::string FLOOR_DIV_MAGIC_NAME;
  static const std::string MOD_MAGIC_NAME;
  static const std::string POW_MAGIC_NAME;
  static const std::string LSHIFT_MAGIC_NAME;
  static const std::string RSHIFT_MAGIC_NAME;
  static const std::string AND_MAGIC_NAME;
  static const std::string OR_MAGIC_NAME;
  static const std::string XOR_MAGIC_NAME;

  static const std::string IADD_MAGIC_NAME;
  static const std::string ISUB_MAGIC_NAME;
  static const std::string IMUL_MAGIC_NAME;
  static const std::string IMATMUL_MAGIC_NAME;
  static const std::string ITRUE_DIV_MAGIC_NAME;
  static const std::string IFLOOR_DIV_MAGIC_NAME;
  static const std::string IMOD_MAGIC_NAME;
  static const std::string IPOW_MAGIC_NAME;
  static const std::string ILSHIFT_MAGIC_NAME;
  static const std::string IRSHIFT_MAGIC_NAME;
  static const std::string IAND_MAGIC_NAME;
  static const std::string IOR_MAGIC_NAME;
  static const std::string IXOR_MAGIC_NAME;

  static const std::string RADD_MAGIC_NAME;
  static const std::string RSUB_MAGIC_NAME;
  static const std::string RMUL_MAGIC_NAME;
  static const std::string RMATMUL_MAGIC_NAME;
  static const std::string RTRUE_DIV_MAGIC_NAME;
  static const std::string RFLOOR_DIV_MAGIC_NAME;
  static const std::string RMOD_MAGIC_NAME;
  static const std::string RPOW_MAGIC_NAME;
  static const std::string RLSHIFT_MAGIC_NAME;
  static const std::string RRSHIFT_MAGIC_NAME;
  static const std::string RAND_MAGIC_NAME;
  static const std::string ROR_MAGIC_NAME;
  static const std::string RXOR_MAGIC_NAME;

  static const std::string INT_MAGIC_NAME;
  static const std::string FLOAT_MAGIC_NAME;
  static const std::string BOOL_MAGIC_NAME;
  static const std::string STR_MAGIC_NAME;
  static const std::string REPR_MAGIC_NAME;
  static const std::string CALL_MAGIC_NAME;

  static const std::string GETITEM_MAGIC_NAME;
  static const std::string SETITEM_MAGIC_NAME;
  static const std::string ITER_MAGIC_NAME;
  static const std::string LEN_MAGIC_NAME;

  static const std::string NEW_MAGIC_NAME;
  static const std::string INIT_MAGIC_NAME;

private:
  /// the module's "main" function
  std::unique_ptr<Func> mainFunc;
  /// the module's argv variable
  std::unique_ptr<Var> argVar;
  /// the global variables list
  std::list<std::unique_ptr<Var>> vars;
  /// the global variables map
  std::unordered_map<id_t, std::list<std::unique_ptr<Var>>::iterator> varMap;
  /// the global value list
  std::list<std::unique_ptr<Value>> values;
  /// the global value map
  std::unordered_map<id_t, std::list<std::unique_ptr<Value>>::iterator> valueMap;
  /// the global types list
  std::list<std::unique_ptr<types::Type>> types;
  /// the global types map
  std::unordered_map<std::string, std::list<std::unique_ptr<types::Type>>::iterator>
      typesMap;

  /// the type-checker cache
  ast::Cache *cache = nullptr;

public:
  static const char NodeId;

  /// Constructs an CIR module.
  /// @param name the module name
  explicit Module(const std::string &name = "");

  virtual ~Module() noexcept = default;

  /// @return the main function
  Func *getMainFunc() { return mainFunc.get(); }
  /// @return the main function
  const Func *getMainFunc() const { return mainFunc.get(); }

  /// @return the arg var
  Var *getArgVar() { return argVar.get(); }
  /// @return the arg var
  const Var *getArgVar() const { return argVar.get(); }

  /// @return iterator to the first symbol
  auto begin() { return util::raw_ptr_adaptor(vars.begin()); }
  /// @return iterator beyond the last symbol
  auto end() { return util::raw_ptr_adaptor(vars.end()); }
  /// @return iterator to the first symbol
  auto begin() const { return util::const_raw_ptr_adaptor(vars.begin()); }
  /// @return iterator beyond the last symbol
  auto end() const { return util::const_raw_ptr_adaptor(vars.end()); }
  /// @return a pointer to the first symbol
  Var *front() { return vars.front().get(); }
  /// @return a pointer to the last symbol
  Var *back() { return vars.back().get(); }
  /// @return a pointer to the first symbol
  const Var *front() const { return vars.front().get(); }
  /// @return a pointer to the last symbol
  const Var *back() const { return vars.back().get(); }
  /// Gets a var by id.
  /// @param id the id
  /// @return the variable or nullptr
  Var *getVar(id_t id) {
    auto it = varMap.find(id);
    return it != varMap.end() ? it->second->get() : nullptr;
  }
  /// Gets a var by id.
  /// @param id the id
  /// @return the variable or nullptr
  const Var *getVar(id_t id) const {
    auto it = varMap.find(id);
    return it != varMap.end() ? it->second->get() : nullptr;
  }
  /// Removes a given var.
  /// @param v the var
  void remove(const Var *v) {
    auto it = varMap.find(v->getId());
    vars.erase(it->second);
    varMap.erase(it);
  }

  /// @return iterator to the first value
  auto values_begin() { return util::raw_ptr_adaptor(values.begin()); }
  /// @return iterator beyond the last value
  auto values_end() { return util::raw_ptr_adaptor(values.end()); }
  /// @return iterator to the first value
  auto values_begin() const { return util::const_raw_ptr_adaptor(values.begin()); }
  /// @return iterator beyond the last value
  auto values_end() const { return util::const_raw_ptr_adaptor(values.end()); }
  /// @return a pointer to the first value
  Value *values_front() { return values.front().get(); }
  /// @return a pointer to the last value
  Value *values_back() { return values.back().get(); }
  /// @return a pointer to the first value
  const Value *values_front() const { return values.front().get(); }
  /// @return a pointer to the last value
  const Value *values_back() const { return values.back().get(); }
  /// Gets a value by id.
  /// @param id the id
  /// @return the value or nullptr
  Value *getValue(id_t id) {
    auto it = valueMap.find(id);
    return it != valueMap.end() ? it->second->get() : nullptr;
  }
  /// Gets a value by id.
  /// @param id the id
  /// @return the value or nullptr
  const Value *getValue(id_t id) const {
    auto it = valueMap.find(id);
    return it != valueMap.end() ? it->second->get() : nullptr;
  }
  /// Removes a given value.
  /// @param v the value
  void remove(const Value *v) {
    auto it = valueMap.find(v->getId());
    values.erase(it->second);
    valueMap.erase(it);
  }

  /// @return iterator to the first type
  auto types_begin() { return util::raw_ptr_adaptor(types.begin()); }
  /// @return iterator beyond the last type
  auto types_end() { return util::raw_ptr_adaptor(types.end()); }
  /// @return iterator to the first type
  auto types_begin() const { return util::const_raw_ptr_adaptor(types.begin()); }
  /// @return iterator beyond the last type
  auto types_end() const { return util::const_raw_ptr_adaptor(types.end()); }
  /// @return a pointer to the first type
  types::Type *types_front() const { return types.front().get(); }
  /// @return a pointer to the last type
  types::Type *types_back() const { return types.back().get(); }
  /// @param name the type's name
  /// @return the type with the given name
  types::Type *getType(const std::string &name) {
    auto it = typesMap.find(name);
    return it == typesMap.end() ? nullptr : it->second->get();
  }
  /// @param name the type's name
  /// @return the type with the given name
  types::Type *getType(const std::string &name) const {
    auto it = typesMap.find(name);
    return it == typesMap.end() ? nullptr : it->second->get();
  }
  /// Removes a given type.
  /// @param t the type
  void remove(types::Type *t) {
    auto it = typesMap.find(t->getName());
    types.erase(it->second);
    typesMap.erase(it);
  }

  /// Constructs and registers an IR node with provided source information.
  /// @param s the source information
  /// @param args the arguments
  /// @return the new node
  template <typename DesiredType, typename... Args>
  DesiredType *N(codon::SrcInfo s, Args &&...args) {
    auto *ret = new DesiredType(std::forward<Args>(args)...);
    ret->setModule(this);
    ret->setSrcInfo(s);

    store(ret);
    return ret;
  }
  /// Constructs and registers an IR node with provided source node.
  /// @param s the source node
  /// @param args the arguments
  /// @return the new node
  template <typename DesiredType, typename... Args>
  DesiredType *N(const codon::SrcObject *s, Args &&...args) {
    return N<DesiredType>(s->getSrcInfo(), std::forward<Args>(args)...);
  }
  /// Constructs and registers an IR node with provided source node.
  /// @param s the source node
  /// @param args the arguments
  /// @return the new node
  template <typename DesiredType, typename... Args>
  DesiredType *N(const Node *s, Args &&...args) {
    return N<DesiredType>(s->getSrcInfo(), std::forward<Args>(args)...);
  }
  /// Constructs and registers an IR node with no source information.
  /// @param args the arguments
  /// @return the new node
  template <typename DesiredType, typename... Args> DesiredType *Nr(Args &&...args) {
    return N<DesiredType>(codon::SrcInfo(), std::forward<Args>(args)...);
  }

  /// @return the type-checker cache
  ast::Cache *getCache() const { return cache; }
  /// Sets the type-checker cache.
  /// @param c the cache
  void setCache(ast::Cache *c) { cache = c; }

  /// Parse a codon code block.
  void parseCode(const std::string &code);

  /// Gets or realizes a method.
  /// @param parent the parent class
  /// @param methodName the method name
  /// @param args the argument types
  /// @param generics the generics
  /// @return the method or nullptr
  Func *getOrRealizeMethod(types::Type *parent, const std::string &methodName,
                           std::vector<types::Type *> args,
                           std::vector<types::Generic> generics = {});

  /// Gets or realizes a function.
  /// @param funcName the function name
  /// @param args the argument types
  /// @param generics the generics
  /// @param module the module of the function
  /// @return the function or nullptr
  Func *getOrRealizeFunc(const std::string &funcName, std::vector<types::Type *> args,
                         std::vector<types::Generic> generics = {},
                         const std::string &module = "");

  /// Gets or realizes a type.
  /// @param typeName the type name
  /// @param generics the generics
  /// @param module the module of the type
  /// @return the function or nullptr
  types::Type *getOrRealizeType(const std::string &typeName,
                                std::vector<types::Generic> generics = {},
                                const std::string &module = "");

  /// @return the void type
  types::Type *getVoidType();
  /// @return the bool type
  types::Type *getBoolType();
  /// @return the byte type
  types::Type *getByteType();
  /// @return the int type
  types::Type *getIntType();
  /// @return the float type
  types::Type *getFloatType();
  /// @return the float32 type
  types::Type *getFloat32Type();
  /// @return the float16 type
  types::Type *getFloat16Type();
  /// @return the bfloat16 type
  types::Type *getBFloat16Type();
  /// @return the float128 type
  types::Type *getFloat128Type();
  /// @return the string type
  types::Type *getStringType();
  /// Gets a pointer type.
  /// @param base the base type
  /// @return a pointer type that references the base
  types::Type *getPointerType(types::Type *base);
  /// Gets an array type.
  /// @param base the base type
  /// @return an array type that contains the base
  types::Type *getArrayType(types::Type *base);
  /// Gets a generator type.
  /// @param base the base type
  /// @return a generator type that yields the base
  types::Type *getGeneratorType(types::Type *base);
  /// Gets an optional type.
  /// @param base the base type
  /// @return an optional type that contains the base
  types::Type *getOptionalType(types::Type *base);
  /// Gets a function type.
  /// @param rType the return type
  /// @param argTypes the argument types
  /// @param variadic true if variadic (e.g. "printf" in C)
  /// @return the void type
  types::Type *getFuncType(types::Type *rType, std::vector<types::Type *> argTypes,
                           bool variadic = false);
  /// Gets a variable length integer type.
  /// @param len the length
  /// @param sign true if signed
  /// @return a variable length integer type
  types::Type *getIntNType(unsigned len, bool sign);
  /// Gets a vector type.
  /// @param count the vector size
  /// @param base the vector base type (MUST be a primitive type)
  /// @return a vector type
  types::Type *getVectorType(unsigned count, types::Type *base);
  /// Gets a tuple type.
  /// @param args the arg types
  /// @return the tuple type
  types::Type *getTupleType(std::vector<types::Type *> args);
  /// Gets a union type.
  /// @param types the alternative types
  /// @return the union type
  types::Type *getUnionType(std::vector<types::Type *> types);
  /// Gets the "none" type (i.e. empty tuple).
  /// @return none type
  types::Type *getNoneType();

  /// @param v the value
  /// @return an int constant
  Value *getInt(int64_t v);
  /// @param v the value
  /// @return a float constant
  Value *getFloat(double v);
  /// @param v the value
  /// @return a bool constant
  Value *getBool(bool v);
  /// @param v the value
  /// @return a string constant
  Value *getString(std::string v);

  /// Gets a dummy function type. Should generally not be used as no type-checker
  /// information is generated.
  /// @return a func type with no args and void return type.
  types::Type *unsafeGetDummyFuncType();
  /// Gets a pointer type. Should generally not be used as no type-checker
  /// information is generated.
  /// @param base the base type
  /// @return a pointer type that references the base
  types::Type *unsafeGetPointerType(types::Type *base);
  /// Gets an array type. Should generally not be used as no type-checker
  /// information is generated.
  /// @param base the base type
  /// @return an array type that contains the base
  types::Type *unsafeGetArrayType(types::Type *base);
  /// Gets a generator type. Should generally not be used as no type-checker
  /// information is generated.
  /// @param base the base type
  /// @return a generator type that yields the base
  types::Type *unsafeGetGeneratorType(types::Type *base);
  /// Gets an optional type. Should generally not be used as no type-checker
  /// information is generated.
  /// @param base the base type
  /// @return an optional type that contains the base
  types::Type *unsafeGetOptionalType(types::Type *base);
  /// Gets a function type. Should generally not be used as no type-checker
  /// information is generated.
  /// @param rType the return type
  /// @param argTypes the argument types
  /// @param variadic true if variadic (e.g. "printf" in C)
  /// @return the void type
  types::Type *unsafeGetFuncType(const std::string &name, types::Type *rType,
                                 std::vector<types::Type *> argTypes,
                                 bool variadic = false);
  /// Gets a membered type. Should generally not be used as no type-checker
  /// information is generated.
  /// @param name the type's name
  /// @param ref whether the type should be a ref
  /// @return an empty membered/ref type
  types::Type *unsafeGetMemberedType(const std::string &name, bool ref = false);
  /// Gets a variable length integer type. Should generally not be used as no
  /// type-checker information is generated.
  /// @param len the length
  /// @param sign true if signed
  /// @return a variable length integer type
  types::Type *unsafeGetIntNType(unsigned len, bool sign);
  /// Gets a vector type. Should generally not be used as no
  /// type-checker information is generated.
  /// @param count the vector size
  /// @param base the vector base type (MUST be a primitive type)
  /// @return a vector type
  types::Type *unsafeGetVectorType(unsigned count, types::Type *base);
  /// Gets a union type. Should generally not be used as no
  /// type-checker information is generated.
  /// @param types the alternative types
  /// @return a union type
  types::Type *unsafeGetUnionType(const std::vector<types::Type *> &types);

private:
  void store(types::Type *t) {
    types.emplace_back(t);
    typesMap[t->getName()] = std::prev(types.end());
  }
  void store(Value *v) {
    values.emplace_back(v);
    valueMap[v->getId()] = std::prev(values.end());
  }
  void store(Var *v) {
    vars.emplace_back(v);
    varMap[v->getId()] = std::prev(vars.end());
  }
};

} // namespace ir
} // namespace codon

template <> struct fmt::formatter<codon::ir::Module> : fmt::ostream_formatter {};
