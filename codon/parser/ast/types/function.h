// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
 * A generic type that represents a Seq function instantiation.
 * It inherits ClassType that realizes Callable[...].
 *
 * ⚠️ This is not a function pointer (Function[...]) type.
 */
struct FuncType : public ClassType {
  /// Canonical AST node.
  FunctionStmt *ast;
  /// Function generics (e.g. T in def foo[T](...)).
  std::vector<ClassType::Generic> funcGenerics;
  /// Enclosing class or a function.
  TypePtr funcParent;

public:
  FuncType(
      const std::shared_ptr<ClassType> &baseType, FunctionStmt *ast,
      std::vector<ClassType::Generic> funcGenerics = std::vector<ClassType::Generic>(),
      TypePtr funcParent = nullptr);

public:
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) override;

public:
  bool hasUnbounds(bool = false) const override;
  std::vector<TypePtr> getUnbounds() const override;
  bool canRealize() const override;
  bool isInstantiated() const override;
  std::string debugString(char mode) const override;
  std::string realizedName() const override;

  std::shared_ptr<FuncType> getFunc() override {
    return std::static_pointer_cast<FuncType>(shared_from_this());
  }

  std::vector<TypePtr> getArgTypes() const;
  TypePtr getRetType() const;
};
using FuncTypePtr = std::shared_ptr<FuncType>;

} // namespace codon::ast::types
