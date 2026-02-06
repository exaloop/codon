// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/type.h"

namespace codon::ast::types {

struct FuncType;

/**
 * A generic class reference type. All Seq types inherit from this class.
 */
struct ClassType : public Type {
  /**
   * A generic type declaration.
   * Each generic is defined by its unique ID.
   */
  struct Generic {
    // Generic name.
    std::string name;
    // Unique generic ID.
    int id;
    // Pointer to realized type (or generic LinkType).
    TypePtr type;
    // Set if this is a static generic
    LiteralKind staticKind;

    Generic(std::string name, TypePtr type, int id, LiteralKind staticKind)
        : name(std::move(name)), id(id), type(std::move(type)), staticKind(staticKind) {
    }

    types::Type *getType() const { return type.get(); }
    Generic generalize(int atLevel) const;
    Generic instantiate(int atLevel, int *unboundCount,
                        std::unordered_map<int, TypePtr> *cache) const;
    std::string debugString(char mode) const;
    std::string realizedName() const;
  };

  /// Canonical type name.
  std::string name;
  /// List of generics, if present.
  std::vector<Generic> generics;

  std::vector<Generic> hiddenGenerics;

  bool isTuple = false;
  std::string _rn;

  explicit ClassType(Cache *cache, std::string name, std::vector<Generic> generics = {},
                     std::vector<Generic> hiddenGenerics = {});
  explicit ClassType(const ClassType *base);

public:
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) const override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) const override;

public:
  bool hasUnbounds(bool) const override;
  std::vector<Type *> getUnbounds(bool) const override;
  bool canRealize() const override;
  bool isInstantiated() const override;
  std::string debugString(char mode) const override;
  std::string realizedName() const override;
  ClassType *getClass() override { return this; }
  ClassType *getPartial() override { return name == "Partial" ? getClass() : nullptr; }
  bool isRecord() const { return isTuple; }

  size_t size() const { return generics.size(); }
  Type *operator[](size_t i) const { return generics[i].getType(); }

public:
  enum PartialFlag { Missing = '0', Included, Default };

  FuncType *getPartialFunc() const;
  std::string getPartialMask() const;
  bool isPartialEmpty() const;
};

} // namespace codon::ast::types

template <>
struct fmt::formatter<codon::ast::types::ClassType::Generic>
    : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const codon::ast::types::ClassType::Generic &p, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "({}{})",
                          p.name.empty() ? "" : fmt::format("{} = ", p.name), p.type);
  }
};
