// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/class.h"

namespace codon::ast::types {

struct UnionType : public ClassType {
  static constexpr int MAX_UNION = 256;

  std::vector<TypePtr> pendingTypes;

  explicit UnionType(Cache *cache);
  UnionType(Cache *, const std::vector<ClassType::Generic> &,
            const std::vector<TypePtr> &);

public:
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) const override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) const override;

public:
  bool canRealize() const override;
  std::string debugString(char mode) const override;
  std::string realizedName() const override;
  bool isSealed() const;

  UnionType *getUnion() override { return this; }

  bool addType(Type *);
  void seal();
  std::vector<Type *> getRealizationTypes() const;
};

} // namespace codon::ast::types
