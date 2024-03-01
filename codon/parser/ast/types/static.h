// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/class.h"

namespace codon::ast {
struct StaticValue;
}

namespace codon::ast::types {

/**
 * A static integer type (e.g. N in def foo[N: int]). Usually an integer, but can point
 * to a static expression.
 */
struct StaticType : public Type {
  /// List of static variables that a type depends on
  /// (e.g. for A+B+2, generics are {A, B}).
  std::vector<ClassType::Generic> generics;
  /// A static expression that needs to be evaluated.
  /// Can be nullptr if there is no expression.
  std::shared_ptr<Expr> expr;

  StaticType(Cache *cache, std::vector<ClassType::Generic> generics,
             const std::shared_ptr<Expr> &expr);
  /// Convenience function that parses expr and populates static type generics.
  StaticType(Cache *cache, const std::shared_ptr<Expr> &expr);
  /// Convenience function for static types whose evaluation is already known.
  explicit StaticType(Cache *cache, int64_t i);
  explicit StaticType(Cache *cache, const std::string &s);

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

  StaticValue evaluate() const;
  std::shared_ptr<StaticType> getStatic() override {
    return std::static_pointer_cast<StaticType>(shared_from_this());
  }

private:
  void parseExpr(const std::shared_ptr<Expr> &e, std::unordered_set<std::string> &seen);
};

} // namespace codon::ast::types
