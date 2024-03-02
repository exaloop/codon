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
 * It inherits RecordType that realizes Callable[...].
 *
 * ⚠️ This is not a function pointer (Function[...]) type.
 */
struct FuncType : public RecordType {
  /// Canonical AST node.
  FunctionStmt *ast;
  /// Function generics (e.g. T in def foo[T](...)).
  std::vector<ClassType::Generic> funcGenerics;
  /// Enclosing class or a function.
  TypePtr funcParent;

public:
  FuncType(
      const std::shared_ptr<RecordType> &baseType, FunctionStmt *ast,
      std::vector<ClassType::Generic> funcGenerics = std::vector<ClassType::Generic>(),
      TypePtr funcParent = nullptr);

public:
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) override;

public:
  std::vector<TypePtr> getUnbounds() const override;
  bool canRealize() const override;
  bool isInstantiated() const override;
  std::string debugString(char mode) const override;
  std::string realizedName() const override;
  std::string realizedTypeName() const override;

  std::shared_ptr<FuncType> getFunc() override {
    return std::static_pointer_cast<FuncType>(shared_from_this());
  }

  std::vector<TypePtr> &getArgTypes() const {
    return generics[0].type->getRecord()->args;
  }
  TypePtr getRetType() const { return generics[1].type; }
};
using FuncTypePtr = std::shared_ptr<FuncType>;

/**
 * A generic type that represents a partial Seq function instantiation.
 * It inherits RecordType that realizes Tuple[...].
 *
 * Note: partials only work on Seq functions. Function pointer partials
 *       will become a partials of Function.__call__ Seq function.
 */
struct PartialType : public RecordType {
  /// Seq function that is being partialized. Always generic (not instantiated).
  FuncTypePtr func;
  /// Arguments that are already provided (1 for known argument, 0 for expecting).
  std::vector<char> known;

public:
  PartialType(const std::shared_ptr<RecordType> &baseType,
              std::shared_ptr<FuncType> func, std::vector<char> known);

public:
  int unify(Type *typ, Unification *us) override;
  TypePtr generalize(int atLevel) override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) override;

  std::string debugString(char mode) const override;
  std::string realizedName() const override;

public:
  std::shared_ptr<PartialType> getPartial() override {
    return std::static_pointer_cast<PartialType>(shared_from_this());
  }
};
using PartialTypePtr = std::shared_ptr<PartialType>;

} // namespace codon::ast::types
