// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/flow.h"
#include "codon/cir/util/iterators.h"
#include "codon/cir/var.h"

namespace codon {
namespace ir {

/// CIR function
class Func : public AcceptorExtend<Func, Var> {
private:
  /// unmangled (source code) name of the function
  std::string unmangledName;
  /// whether the function is a generator
  bool generator;
  /// Parent type if func is a method, or null if not
  types::Type *parentType;

protected:
  /// list of arguments
  std::list<Var *> args;

  std::vector<Var *> doGetUsedVariables() const override;
  int doReplaceUsedVariable(id_t id, Var *newVar) override;

  std::vector<types::Type *> doGetUsedTypes() const override;
  int doReplaceUsedType(const std::string &name, types::Type *newType) override;

public:
  static const char NodeId;

  /// Constructs an unrealized CIR function.
  /// @param name the function's name
  explicit Func(std::string name = "")
      : AcceptorExtend(nullptr, true, false, std::move(name)), generator(false),
        parentType(nullptr) {}

  /// Re-initializes the function with a new type and names.
  /// @param newType the function's new type
  /// @param names the function's new argument names
  void realize(types::Type *newType, const std::vector<std::string> &names);

  /// @return iterator to the first arg
  auto arg_begin() { return args.begin(); }
  /// @return iterator beyond the last arg
  auto arg_end() { return args.end(); }
  /// @return iterator to the first arg
  auto arg_begin() const { return args.begin(); }
  /// @return iterator beyond the last arg
  auto arg_end() const { return args.end(); }

  /// @return a pointer to the last arg
  Var *arg_front() { return args.front(); }
  /// @return a pointer to the last arg
  Var *arg_back() { return args.back(); }
  /// @return a pointer to the last arg
  const Var *arg_back() const { return args.back(); }
  /// @return a pointer to the first arg
  const Var *arg_front() const { return args.front(); }

  /// @return the function's unmangled (source code) name
  std::string getUnmangledName() const { return unmangledName; }
  /// Sets the unmangled name.
  /// @param v the new value
  void setUnmangledName(std::string v) { unmangledName = std::move(v); }

  /// @return true if the function is a generator
  bool isGenerator() const { return generator; }
  /// Sets the function's generator flag.
  /// @param v the new value
  void setGenerator(bool v = true) { generator = v; }

  /// @return the variable corresponding to the given argument name
  /// @param n the argument name
  Var *getArgVar(const std::string &n);

  /// @return the parent type
  types::Type *getParentType() const { return parentType; }
  /// Sets the parent type.
  /// @param p the new parent
  void setParentType(types::Type *p) { parentType = p; }
};

class BodiedFunc : public AcceptorExtend<BodiedFunc, Func> {
private:
  /// list of variables defined and used within the function
  std::list<Var *> symbols;
  /// the function body
  Value *body = nullptr;
  /// whether the function is a JIT input
  bool jit = false;

public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

  /// @return iterator to the first symbol
  auto begin() { return symbols.begin(); }
  /// @return iterator beyond the last symbol
  auto end() { return symbols.end(); }
  /// @return iterator to the first symbol
  auto begin() const { return symbols.begin(); }
  /// @return iterator beyond the last symbol
  auto end() const { return symbols.end(); }

  /// @return a pointer to the first symbol
  Var *front() { return symbols.front(); }
  /// @return a pointer to the last symbol
  Var *back() { return symbols.back(); }
  /// @return a pointer to the first symbol
  const Var *front() const { return symbols.front(); }
  /// @return a pointer to the last symbol
  const Var *back() const { return symbols.back(); }

  /// Inserts an symbol at the given position.
  /// @param pos the position
  /// @param v the symbol
  /// @return an iterator to the newly added symbol
  template <typename It> auto insert(It pos, Var *v) { return symbols.insert(pos, v); }
  /// Appends an symbol.
  /// @param v the new symbol
  void push_back(Var *v) { symbols.push_back(v); }

  /// Erases the symbol at the given position.
  /// @param pos the position
  /// @return symbol_iterator following the removed symbol.
  template <typename It> auto erase(It pos) { return symbols.erase(pos); }

  /// @return the function body
  Flow *getBody() { return cast<Flow>(body); }
  /// @return the function body
  const Flow *getBody() const { return cast<Flow>(body); }
  /// Sets the function's body.
  /// @param b the new body
  void setBody(Flow *b) { body = b; }

  /// @return true if the function is a JIT input
  bool isJIT() const { return jit; }
  /// Changes the function's JIT input status.
  /// @param v true if JIT input, false otherwise
  void setJIT(bool v = true) { jit = v; }

protected:
  std::vector<Value *> doGetUsedValues() const override {
    return body ? std::vector<Value *>{body} : std::vector<Value *>{};
  }
  int doReplaceUsedValue(id_t id, Value *newValue) override;

  std::vector<Var *> doGetUsedVariables() const override;
  int doReplaceUsedVariable(id_t id, Var *newVar) override;
};

class ExternalFunc : public AcceptorExtend<ExternalFunc, Func> {
public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

  /// @return true if the function is variadic
  bool isVariadic() const { return cast<types::FuncType>(getType())->isVariadic(); }
};

/// Internal, LLVM-only function.
class InternalFunc : public AcceptorExtend<InternalFunc, Func> {
public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;
};

/// LLVM function defined in Seq source.
class LLVMFunc : public AcceptorExtend<LLVMFunc, Func> {
private:
  /// literals that must be formatted into the body
  std::vector<types::Generic> llvmLiterals;
  /// declares for llvm-only function
  std::string llvmDeclares;
  /// body of llvm-only function
  std::string llvmBody;

public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

  /// Sets the LLVM literals.
  /// @param v the new values.
  void setLLVMLiterals(std::vector<types::Generic> v) { llvmLiterals = std::move(v); }

  /// @return iterator to the first literal
  auto literal_begin() { return llvmLiterals.begin(); }
  /// @return iterator beyond the last literal
  auto literal_end() { return llvmLiterals.end(); }
  /// @return iterator to the first literal
  auto literal_begin() const { return llvmLiterals.begin(); }
  /// @return iterator beyond the last literal
  auto literal_end() const { return llvmLiterals.end(); }

  /// @return a reference to the first literal
  auto &literal_front() { return llvmLiterals.front(); }
  /// @return a reference to the last literal
  auto &literal_back() { return llvmLiterals.back(); }
  /// @return a reference to the first literal
  auto &literal_front() const { return llvmLiterals.front(); }
  /// @return a reference to the last literal
  auto &literal_back() const { return llvmLiterals.back(); }

  /// @return the LLVM declarations
  const std::string &getLLVMDeclarations() const { return llvmDeclares; }
  /// Sets the LLVM declarations.
  /// @param v the new value
  void setLLVMDeclarations(std::string v) { llvmDeclares = std::move(v); }
  /// @return the LLVM body
  const std::string &getLLVMBody() const { return llvmBody; }
  /// Sets the LLVM body.
  /// @param v the new value
  void setLLVMBody(std::string v) { llvmBody = std::move(v); }

protected:
  std::vector<types::Type *> doGetUsedTypes() const override;
  int doReplaceUsedType(const std::string &name, types::Type *newType) override;
};

} // namespace ir
} // namespace codon

template <> struct fmt::formatter<codon::ir::Func> : fmt::ostream_formatter {};
