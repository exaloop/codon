// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/class.h"

namespace codon::ast::types {

struct UnionType : public RecordType {
  std::vector<TypePtr> pendingTypes;

  explicit UnionType(Cache *cache);
  UnionType(Cache *, const std::vector<ClassType::Generic> &,
            const std::vector<TypePtr> &);

public:
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) override;

public:
  bool canRealize() const override;
  std::string debugString(char mode) const override;
  std::string realizedName() const override;
  std::string realizedTypeName() const override;
  bool isSealed() const;

  std::shared_ptr<UnionType> getUnion() override {
    return std::static_pointer_cast<UnionType>(shared_from_this());
  }

  void addType(TypePtr typ);
  void seal();
  std::vector<types::TypePtr> getRealizationTypes();
};

} // namespace codon::ast::types
