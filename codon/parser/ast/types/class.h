// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
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
    // Name used for pretty-printing.
    std::string niceName;
    // Unique generic ID.
    int id;
    // Pointer to realized type (or generic LinkType).
    TypePtr type;
    // Set if this is a static generic
    char isStatic;

    Generic(std::string name, std::string niceName, TypePtr type, int id, char isStatic)
        : name(std::move(name)), niceName(std::move(niceName)), type(std::move(type)),
          id(id), isStatic(isStatic) {}

    types::Type *getType() const { return type.get(); }
    Generic generalize(int atLevel);
    Generic instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache);
    std::string debugString(char mode) const;
    std::string realizedName() const;
  };

  /// Canonical type name.
  std::string name;
  /// Name used for pretty-printing.
  std::string niceName;
  /// List of generics, if present.
  std::vector<Generic> generics;

  std::vector<Generic> hiddenGenerics;

  bool isTuple = false;
  std::string _rn;

  explicit ClassType(Cache *cache, std::string name, std::string niceName,
                     std::vector<Generic> generics = {},
                     std::vector<Generic> hiddenGenerics = {});
  explicit ClassType(ClassType *base);

public:
  int unify(Type *typ, Unification *undo) override;
  TypePtr generalize(int atLevel) override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) override;

public:
  bool hasUnbounds(bool = false) const override;
  std::vector<Type *> getUnbounds() const override;
  bool canRealize() const override;
  bool isInstantiated() const override;
  std::string debugString(char mode) const override;
  std::string realizedName() const override;
  ClassType *getClass() override { return this; }
  ClassType *getPartial() override { return name == "Partial" ? getClass() : nullptr; }
  bool isRecord() const { return isTuple; }

  size_t size() const { return generics.size(); }
  Type *operator[](int i) const { return generics[i].getType(); }

public:
  ClassType *getHeterogenousTuple() override;
  FuncType *getPartialFunc() const;
  std::vector<char> getPartialMask() const;
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
