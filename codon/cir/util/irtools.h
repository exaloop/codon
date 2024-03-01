// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/cir.h"

namespace codon {
namespace ir {
namespace util {

/// Checks whether a function has a given attribute.
/// @param func the function
/// @param attribute the attribute name
/// @return true if the function has the given attribute
bool hasAttribute(const Func *func, const std::string &attribute);

/// Checks whether a function comes from the standard library, and
/// optionally a specific module therein.
/// @param func the function
/// @param submodule module name (e.g. "std::bio"), or empty if
///                  no module check is required
/// @return true if the function is from the standard library in
///         the given module
bool isStdlibFunc(const Func *func, const std::string &submodule = "");

/// Calls a function.
/// @param func the function
/// @param args vector of call arguments
/// @return call instruction with the given function and arguments
CallInstr *call(Func *func, const std::vector<Value *> &args);

/// Checks if a value represents a call of a particular function.
/// @param value the value to check
/// @param name the function's (unmangled) name
/// @param inputs vector of input types
/// @param output output type, null for no check
/// @param method true to ensure this call is a method call
/// @return true if value is a call matching all parameters above
bool isCallOf(const Value *value, const std::string &name,
              const std::vector<types::Type *> &inputs, types::Type *output = nullptr,
              bool method = false);

/// Checks if a value represents a call of a particular function.
/// @param value the value to check
/// @param name the function's (unmangled) name
/// @param numArgs argument count, negative for no check
/// @param output output type, null for no check
/// @param method true to ensure this call is a method call
/// @return true if value is a call matching all parameters above
bool isCallOf(const Value *value, const std::string &name, int numArgs = -1,
              types::Type *output = nullptr, bool method = false);

/// Checks if a value represents a call to a magic method.
/// Magic method names start and end in "__" (two underscores).
/// @param value the value to check
/// @return true if value is a magic method call
bool isMagicMethodCall(const Value *value);

/// Constructs a new tuple.
/// @param args vector of tuple contents
/// @param M the module; inferred from elements if null
/// @return value represents a tuple with the given contents
Value *makeTuple(const std::vector<Value *> &args, Module *M = nullptr);

/// Constructs and assigns a new variable.
/// @param x the value to assign to the new variable
/// @param flow series flow in which to assign the new variable
/// @param parent function to add the new variable to, or null for global variable
/// @param prepend true to insert assignment at start of block
/// @return value containing the new variable
VarValue *makeVar(Value *x, SeriesFlow *flow, BodiedFunc *parent, bool prepend = false);

/// Dynamically allocates memory for the given type with the given
/// number of elements.
/// @param type the type
/// @param count integer value representing the number of elements
/// @return value representing a pointer to the allocated memory
Value *alloc(types::Type *type, Value *count);

/// Dynamically allocates memory for the given type with the given
/// number of elements.
/// @param type the type
/// @param count the number of elements
/// @return value representing a pointer to the allocated memory
Value *alloc(types::Type *type, int64_t count);

/// Builds a new series flow with the given contents. Returns
/// null if no contents are provided.
/// @param args contents of the series flow
/// @return new series flow
template <typename... Args> SeriesFlow *series(Args... args) {
  std::vector<Value *> vals = {args...};
  if (vals.empty())
    return nullptr;
  auto *series = vals[0]->getModule()->Nr<SeriesFlow>();
  for (auto *val : vals) {
    series->push_back(val);
  }
  return series;
}

/// Checks whether the given value is a constant of the given
/// type. Note that standard "int" corresponds to the C type
/// "int64_t", which should be used here.
/// @param x the value to check
/// @return true if the value is constant
template <typename T> bool isConst(const Value *x) { return isA<TemplatedConst<T>>(x); }

/// Checks whether the given value is a constant of the given
/// type, and that is has a particular value. Note that standard
/// "int" corresponds to the C type "int64_t", which should be used here.
/// @param x the value to check
/// @param value constant value to compare to
/// @return true if the value is constant with the given value
template <typename T> bool isConst(const Value *x, const T &value) {
  if (auto *c = cast<TemplatedConst<T>>(x)) {
    return c->getVal() == value;
  }
  return false;
}

/// Returns the constant represented by a given value. Raises an assertion
/// error if the given value is not constant. Note that standard
/// "int" corresponds to the C type "int64_t", which should be used here.
/// @param x the (constant) value
/// @return the constant represented by the given value
template <typename T> T getConst(const Value *x) {
  auto *c = cast<TemplatedConst<T>>(x);
  seqassertn(c, "{} is not a constant [{}]", *x, x->getSrcInfo());
  return c->getVal();
}

/// Gets a variable from a value.
/// @param x the value
/// @return the variable represented by the given value, or null if none
Var *getVar(Value *x);

/// Gets a variable from a value.
/// @param x the value
/// @return the variable represented by the given value, or null if none
const Var *getVar(const Value *x);

/// Gets a function from a value.
/// @param x the value
/// @return the function represented by the given value, or null if none
Func *getFunc(Value *x);

/// Gets a function from a value.
/// @param x the value
/// @return the function represented by the given value, or null if none
const Func *getFunc(const Value *x);

/// Loads value from a pointer.
/// @param ptr the pointer
/// @return the value pointed to by the argument
Value *ptrLoad(Value *ptr);

/// Stores a value into a pointer.
/// @param ptr the pointer
/// @param val the value to store
/// @return "__setitem__" call representing the store
Value *ptrStore(Value *ptr, Value *val);

/// Gets value from a tuple at the given index.
/// @param tuple the tuple
/// @param index the 0-based index
/// @return tuple element at the given index
Value *tupleGet(Value *tuple, unsigned index);

/// Stores value in a tuple at the given index. Since tuples are immutable,
/// a new instance is returned with the appropriate element replaced.
/// @param tuple the tuple
/// @param index the 0-based index
/// @param val the value to store
/// @return new tuple instance with the given value inserted
Value *tupleStore(Value *tuple, unsigned index, Value *val);

/// Gets a bodied standard library function from a value.
/// @param x the value
/// @param name name of the function
/// @param submodule optional module to check
/// @return the standard library function (with the given name, from the given
/// submodule) represented by the given value, or null if none
BodiedFunc *getStdlibFunc(Value *x, const std::string &name,
                          const std::string &submodule = "");

/// Gets a bodied standard library function from a value.
/// @param x the value
/// @param name name of the function
/// @param submodule optional module to check
/// @return the standard library function (with the given name, from the given
/// submodule) represented by the given value, or null if none
const BodiedFunc *getStdlibFunc(const Value *x, const std::string &name,
                                const std::string &submodule = "");

/// Gets the return type of a function.
/// @param func the function
/// @return the return type of the given function
types::Type *getReturnType(const Func *func);

/// Sets the return type of a function. Argument types remain unchanged.
/// @param func the function
/// @param rType the new return type
void setReturnType(Func *func, types::Type *rType);

} // namespace util
} // namespace ir
} // namespace codon
