// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/common.h"

namespace codon::ast {
struct Cache;
struct Expr;
} // namespace codon::ast

namespace codon::ast::types {

/// Forward declarations
struct FuncType;
struct ClassType;
struct LinkType;
struct RecordType;
struct PartialType;
struct StaticType;
struct UnionType;

/**
 * An abstract type class that describes methods needed for the type inference.
 * (Hindley-Milner's Algorithm W inference; see
 * https://github.com/tomprimozic/type-systems).
 *
 * Type instances are mutable and each type is intended to be instantiated and
 * manipulated as a shared_ptr.
 */
struct Type : public codon::SrcObject, public std::enable_shared_from_this<Type> {
  /// A structure that keeps the list of unification steps that can be undone later.
  /// Needed because the unify() is destructive.
  struct Unification {
    /// List of unbound types that have been changed.
    std::vector<LinkType *> linked;
    /// List of unbound types whose level has been changed.
    std::vector<std::pair<LinkType *, int>> leveled;
    /// List of assigned traits.
    std::vector<LinkType *> traits;
    /// List of pointers that are owned by unification process
    /// (to avoid memory issues with undoing).
    std::vector<std::shared_ptr<Type>> ownedTypes;

  public:
    /// Undo the unification step.
    void undo();
  };

public:
  /// Unifies a given type with the current type.
  /// @param typ A given type.
  /// @param undo A reference to Unification structure to track the unification steps
  ///             and allow later undoing of the unification procedure.
  /// @return Unification score: -1 for failure, anything >= 0 for success.
  ///         Higher score translates to a "better" unification.
  /// ⚠️ Destructive operation if undo is not null!
  ///    (both the current and a given type are modified).
  virtual int unify(Type *typ, Unification *undo) = 0;
  /// Generalize all unbound types whose level is below the provided level.
  /// This method replaces all unbound types with a generic types (e.g. ?1 -> T1).
  /// Note that the generalized type keeps the unbound type's ID.
  virtual std::shared_ptr<Type> generalize(int atLevel) = 0;
  /// Instantiate all generic types. Inverse of generalize(): it replaces all
  /// generic types with new unbound types (e.g. T1 -> ?1234).
  /// Note that the instantiated type has a distinct and unique ID.
  /// @param level Level of the instantiation.
  /// @param unboundCount A reference of the unbound counter to ensure that no two
  ///                     unbound types share the same ID.
  /// @param cache A reference to a lookup table to ensure that all instances of a
  ///              generic point to the same unbound type (e.g. dict[T, list[T]] should
  ///              be instantiated as dict[?1, list[?1]]).
  virtual std::shared_ptr<Type>
  instantiate(int atLevel, int *unboundCount,
              std::unordered_map<int, std::shared_ptr<Type>> *cache) = 0;

public:
  /// Get the final type (follow through all LinkType links).
  /// For example, for (a->b->c->d) it returns d.
  virtual std::shared_ptr<Type> follow();
  /// Obtain the list of internal unbound types.
  virtual std::vector<std::shared_ptr<Type>> getUnbounds() const;
  /// True if a type is realizable.
  virtual bool canRealize() const = 0;
  /// True if a type is completely instantiated (has no unbounds or generics).
  virtual bool isInstantiated() const = 0;
  /// Pretty-print facility.
  std::string toString() const;
  /// Pretty-print facility.
  std::string prettyString() const;
  /// Pretty-print facility. mode is [0: pretty, 1: llvm, 2: debug]
  virtual std::string debugString(char mode) const = 0;
  /// Print the realization string.
  /// Similar to toString, but does not print the data unnecessary for realization
  /// (e.g. the function return type).
  virtual std::string realizedName() const = 0;

  /// Convenience virtual functions to avoid unnecessary dynamic_cast calls.
  virtual std::shared_ptr<FuncType> getFunc() { return nullptr; }
  virtual std::shared_ptr<PartialType> getPartial() { return nullptr; }
  virtual std::shared_ptr<ClassType> getClass() { return nullptr; }
  virtual std::shared_ptr<RecordType> getRecord() { return nullptr; }
  virtual std::shared_ptr<LinkType> getLink() { return nullptr; }
  virtual std::shared_ptr<LinkType> getUnbound() { return nullptr; }
  virtual std::shared_ptr<StaticType> getStatic() { return nullptr; }
  virtual std::shared_ptr<UnionType> getUnion() { return nullptr; }
  virtual std::shared_ptr<RecordType> getHeterogenousTuple() { return nullptr; }

  virtual bool is(const std::string &s);
  char isStaticType();

public:
  static std::shared_ptr<Type> makeType(Cache *, const std::string &,
                                        const std::string &, bool = false);
  static std::shared_ptr<StaticType> makeStatic(Cache *, const std::shared_ptr<Expr> &);

protected:
  Cache *cache;
  explicit Type(const std::shared_ptr<Type> &);
  explicit Type(Cache *, const SrcInfo & = SrcInfo());
};
using TypePtr = std::shared_ptr<Type>;

} // namespace codon::ast::types

template <typename T>
struct fmt::formatter<
    T, std::enable_if_t<std::is_base_of<codon::ast::types::Type, T>::value, char>>
    : fmt::ostream_formatter {};

template <typename T>
struct fmt::formatter<
    T,
    std::enable_if_t<
        std::is_convertible<T, std::shared_ptr<codon::ast::types::Type>>::value, char>>
    : fmt::formatter<std::string_view> {
  char presentation = 'd';

  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'p' || *it == 'd' || *it == 'D'))
      presentation = *it++;
    return it;
  }

  template <typename FormatContext>
  auto format(const T &p, FormatContext &ctx) const -> decltype(ctx.out()) {
    if (presentation == 'p')
      return fmt::format_to(ctx.out(), "{}", p ? p->debugString(0) : "<nullptr>");
    else if (presentation == 'd')
      return fmt::format_to(ctx.out(), "{}", p ? p->debugString(1) : "<nullptr>");
    else
      return fmt::format_to(ctx.out(), "{}", p ? p->debugString(2) : "<nullptr>");
  }
};
