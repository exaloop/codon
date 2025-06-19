// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "codon/parser/ast/types/class.h"

namespace codon::ast::types {

struct StaticType : public ClassType {
  explicit StaticType(Cache *, const std::string &);

public:
  bool canRealize() const override;
  bool isInstantiated() const override;
  std::string realizedName() const override;
  virtual Expr *getStaticExpr() const = 0;
  virtual LiteralKind getStaticKind() = 0;
  virtual Type *getNonStaticType() const;
  StaticType *getStatic() override { return this; }
};

struct IntStaticType : public StaticType {
  int64_t value;

  explicit IntStaticType(Cache *cache, int64_t);

  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) const override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) const override;

  std::string debugString(char mode) const override;
  Expr *getStaticExpr() const override;
  LiteralKind getStaticKind() override { return LiteralKind::Int; }

  IntStaticType *getIntStatic() override { return this; }
};

struct StrStaticType : public StaticType {
  std::string value;

  explicit StrStaticType(Cache *cache, std::string);

  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) const override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) const override;

  std::string debugString(char mode) const override;
  Expr *getStaticExpr() const override;
  LiteralKind getStaticKind() override { return LiteralKind::String; }

  StrStaticType *getStrStatic() override { return this; }
};

struct BoolStaticType : public StaticType {
  bool value;

  explicit BoolStaticType(Cache *cache, bool);

  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) const override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) const override;

  std::string debugString(char mode) const override;
  Expr *getStaticExpr() const override;
  LiteralKind getStaticKind() override { return LiteralKind::Bool; }

  BoolStaticType *getBoolStatic() override { return this; }
};

using StaticTypePtr = std::shared_ptr<StaticType>;
using IntStaticTypePtr = std::shared_ptr<IntStaticType>;
using StrStaticTypePtr = std::shared_ptr<StrStaticType>;

} // namespace codon::ast::types
