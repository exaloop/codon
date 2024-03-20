// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/class.h"

namespace codon::ast::types {

struct StaticType : public ClassType {
  explicit StaticType(Cache *, const std::string &);

public:
  TypePtr generalize(int atLevel) override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) override;
  bool canRealize() const override;
  bool isInstantiated() const override;
  std::string realizedName() const override;
  virtual std::shared_ptr<Expr> getStaticExpr() const = 0;
  virtual TypePtr getNonStaticType() const;
  std::shared_ptr<StaticType> getStatic() override {
    return std::static_pointer_cast<StaticType>(shared_from_this());
  }
};

struct IntStaticType : public StaticType {
  int64_t value;

public:
  explicit IntStaticType(Cache *cache, int64_t);

public:
  int unify(Type *typ, Unification *undo) override;

public:
  std::string debugString(char mode) const override;
  std::shared_ptr<Expr> getStaticExpr() const override;

  std::shared_ptr<IntStaticType> getIntStatic() override {
    return std::static_pointer_cast<IntStaticType>(shared_from_this());
  }
};

struct StrStaticType : public StaticType {
  std::string value;

public:
  explicit StrStaticType(Cache *cache, std::string);

public:
  int unify(Type *typ, Unification *undo) override;

public:
  std::string debugString(char mode) const override;
  std::shared_ptr<Expr> getStaticExpr() const override;

  std::shared_ptr<StrStaticType> getStrStatic() override {
    return std::static_pointer_cast<StrStaticType>(shared_from_this());
  }
};

struct BoolStaticType : public StaticType {
  bool value;

public:
  explicit BoolStaticType(Cache *cache, bool);

public:
  int unify(Type *typ, Unification *undo) override;

public:
  std::string debugString(char mode) const override;
  std::shared_ptr<Expr> getStaticExpr() const override;

  std::shared_ptr<BoolStaticType> getBoolStatic() override {
    return std::static_pointer_cast<BoolStaticType>(shared_from_this());
  }
};

using StaticTypePtr = std::shared_ptr<StaticType>;
using IntStaticTypePtr = std::shared_ptr<IntStaticType>;
using StrStaticTypePtr = std::shared_ptr<StrStaticType>;

} // namespace codon::ast::types
