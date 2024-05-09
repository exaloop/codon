// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/type.h"

namespace codon::ast::types {

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

    Generic(std::string name, std::string niceName, TypePtr type, int id)
        : name(std::move(name)), niceName(std::move(niceName)), id(id),
          type(std::move(type)) {}
  };

  /// Canonical type name.
  std::string name;
  /// Name used for pretty-printing.
  std::string niceName;
  /// List of generics, if present.
  std::vector<Generic> generics;

  std::vector<Generic> hiddenGenerics;

  std::string _rn;

  explicit ClassType(Cache *cache, std::string name, std::string niceName,
                     std::vector<Generic> generics = {},
                     std::vector<Generic> hiddenGenerics = {});
  explicit ClassType(const std::shared_ptr<ClassType> &base);

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
  /// Convenience function to get the name of realized type
  /// (needed if a subclass realizes something else as well).
  virtual std::string realizedTypeName() const;
  std::shared_ptr<ClassType> getClass() override {
    return std::static_pointer_cast<ClassType>(shared_from_this());
  }
};
using ClassTypePtr = std::shared_ptr<ClassType>;

/**
 * A generic class tuple (record) type. All Seq tuples inherit from this class.
 */
struct RecordType : public ClassType {
  /// List of tuple arguments.
  std::vector<TypePtr> args;
  bool noTuple;
  std::shared_ptr<StaticType> repeats = nullptr;

  explicit RecordType(
      Cache *cache, std::string name, std::string niceName,
      std::vector<ClassType::Generic> generics = std::vector<ClassType::Generic>(),
      std::vector<TypePtr> args = std::vector<TypePtr>(), bool noTuple = false,
      const std::shared_ptr<StaticType> &repeats = nullptr);
  RecordType(const ClassTypePtr &base, std::vector<TypePtr> args, bool noTuple = false,
             const std::shared_ptr<StaticType> &repeats = nullptr);

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
  std::string realizedTypeName() const override;

  std::shared_ptr<RecordType> getRecord() override {
    return std::static_pointer_cast<RecordType>(shared_from_this());
  }
  std::shared_ptr<RecordType> getHeterogenousTuple() override;

  int64_t getRepeats() const;
  void flatten();
};

} // namespace codon::ast::types

template <>
struct fmt::formatter<codon::ast::types::ClassType::Generic>
    : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const codon::ast::types::ClassType::Generic &p,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "({}{})",
                          p.name.empty() ? "" : fmt::format("{} = ", p.name), p.type);
  }
};
