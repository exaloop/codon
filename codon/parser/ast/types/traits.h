// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/type.h"

namespace codon::ast::types {

struct Trait : public Type {
  bool canRealize() const override;
  bool isInstantiated() const override;
  std::string realizedName() const override;

protected:
  explicit Trait(const std::shared_ptr<Type> &);
  explicit Trait(Cache *);
};

struct CallableTrait : public Trait {
  std::vector<TypePtr> args; // tuple with arg types, ret type

public:
  explicit CallableTrait(Cache *cache, std::vector<TypePtr> args);
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) const override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) const override;
  std::string debugString(char mode) const override;
};

struct TypeTrait : public Trait {
  TypePtr type;

public:
  explicit TypeTrait(TypePtr type);
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) const override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) const override;
  std::string debugString(char mode) const override;
};

} // namespace codon::ast::types
