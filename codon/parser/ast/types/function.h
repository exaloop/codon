// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/class.h"
#include "codon/parser/ast/types/type.h"

namespace codon::ast {
struct FunctionStmt;
}

namespace codon::ast::types {

/**
 * A generic type that represents a Codon function instantiation.
 * It inherits ClassType that realizes Function[...].
 */
struct FuncType : public ClassType {
  /// Canonical AST node.
  FunctionStmt *ast;
  /// Function capture index.
  size_t index;
  /// Function generics (e.g. T in def foo[T](...)).
  std::vector<ClassType::Generic> funcGenerics;
  /// Enclosing class or a function.
  TypePtr funcParent;

public:
  FuncType(
      ClassType *baseType, FunctionStmt *ast, size_t index = 0,
      std::vector<ClassType::Generic> funcGenerics = std::vector<ClassType::Generic>(),
      TypePtr funcParent = nullptr);

public:
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) override;

public:
  bool hasUnbounds(bool = false) const override;
  std::vector<Type *> getUnbounds(bool = false) const override;
  bool canRealize() const override;
  bool isInstantiated() const override;
  std::string debugString(char mode) const override;
  std::string realizedName() const override;

  FuncType *getFunc() override { return this; }

  Type *getRetType() const;
  Type *getParentType() const { return funcParent.get(); }
  std::string getFuncName() const;

  Type *operator[](int i) const;
  std::vector<ClassType::Generic>::iterator begin() const;
  std::vector<ClassType::Generic>::iterator end() const;
  size_t size() const;
  bool empty() const;
};

} // namespace codon::ast::types
