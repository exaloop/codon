// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/class.h"

namespace codon::ast::types {

/**
 * A static integer type (e.g. N in def foo[N: int]). Usually an integer, but can point
 * to a static expression.
 */
struct StaticType : public Type {
  enum Kind { Int = 1, String = 2, Bool = 3 };

private:
  Kind kind;
  void *value;

public:
  /// Convenience function for static types whose evaluation is already known.
  explicit StaticType(Cache *cache, int64_t i);
  explicit StaticType(Cache *cache, const std::string &s);
  ~StaticType() override;

public:
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) override;

public:
  std::vector<TypePtr> getUnbounds() const override;
  bool canRealize() const override;
  bool isInstantiated() const override { return true; }
  std::string debugString(char mode) const override;
  std::string realizedName() const override;

  std::shared_ptr<StaticType> getStatic() override {
    return std::static_pointer_cast<StaticType>(shared_from_this());
  }

  bool isString() const;
  std::string getString() const;
  bool isInt() const;
  int64_t getInt() const;
  std::string getTypeName() const;
  std::shared_ptr<Expr> getStaticExpr() const;

  static std::string getTypeName(StaticType::Kind);
  static std::string getTypeName(char);
};
using StaticTypePtr = std::shared_ptr<StaticType>;

} // namespace codon::ast::types
