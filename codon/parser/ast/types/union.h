// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/class.h"

namespace codon::ast::types {

struct UnionType : public ClassType {
  // std::vector<TypePtr> pendingTypes;

  explicit UnionType(Cache *cache);
  UnionType(Cache *, const std::vector<ClassType::Generic> &);

public:
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) const override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) const override;

public:
  std::string debugString(char mode) const override;
  std::string realizedName() const override;

  UnionType *getUnion() override { return this; }

  std::vector<Type *> getRealizationTypes() const;
};

} // namespace codon::ast::types
