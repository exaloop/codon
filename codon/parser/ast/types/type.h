// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/common.h"

namespace codon::ast {
struct Cache;
struct Expr;
struct TypeContext;
} // namespace codon::ast

namespace codon::ast::types {

/// Forward declarations
struct ClassType;
struct FuncType;
struct LinkType;
struct StaticType;
struct IntStaticType;
struct StrStaticType;
struct BoolStaticType;
struct UnionType;

enum LiteralKind { Runtime, Int, String, Bool };

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
    std::vector<std::shared_ptr<Type>> linked;
    /// List of unbound types whose level has been changed.
    std::vector<std::pair<std::shared_ptr<Type>, int>> leveled;
    /// List of assigned traits.
    std::vector<std::shared_ptr<Type>> traits;
    /// List of unbound types whose static status has been changed.
    std::vector<std::shared_ptr<Type>> statics;

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
  virtual std::shared_ptr<Type> generalize(int atLevel) const = 0;
  /// Instantiate all generic types. Inverse of generalize(): it replaces all
  /// generic types with new unbound types (e.g. T1 -> ?1234).
  /// Note that the instantiated type has a distinct and unique ID.
  /// @param atLevel Level of the instantiation.
  /// @param unboundCount A reference of the unbound counter to ensure that no two
  ///                     unbound types share the same ID.
  /// @param cache A reference to a lookup table to ensure that all instances of a
  ///              generic point to the same unbound type (e.g. dict[T, list[T]] should
  ///              be instantiated as dict[?1, list[?1]]).
  virtual std::shared_ptr<Type>
  instantiate(int atLevel, int *unboundCount,
              std::unordered_map<int, std::shared_ptr<Type>> *cache) const = 0;

public:
  /// Get the final type (follow through all LinkType links).
  /// For example, for (a->b->c->d) it returns d.
  virtual Type *follow();
  /// Check if type has unbound/generic types.
  virtual bool hasUnbounds(bool includeGenerics) const;
  /// Obtain the list of internal unbound types.
  virtual std::vector<Type *> getUnbounds(bool includeGenerics) const;
  /// True if a type is realizable.
  virtual bool canRealize() const = 0;
  /// True if a type is completely instantiated (has no unbounds or generics).
  virtual bool isInstantiated() const = 0;
  /// Debug print facility.
  std::string toString() const;
  /// Pretty-print facility.
  std::string prettyString() const;
  /// Pretty-print facility. mode is [0: pretty, 1: llvm, 2: debug]
  virtual std::string debugString(char mode) const = 0;
  /// Print the realization string.
  /// Similar to toString, but does not print the data unnecessary for realization
  /// (e.g. the function return type).
  virtual std::string realizedName() const = 0;
  LiteralKind getStaticKind();

  /// Convenience virtual functions to avoid unnecessary casts.
  virtual FuncType *getFunc() { return nullptr; }
  virtual ClassType *getPartial() { return nullptr; }
  virtual ClassType *getClass() { return nullptr; }
  virtual LinkType *getLink() { return nullptr; }
  virtual LinkType *getUnbound() { return nullptr; }
  virtual StaticType *getStatic() { return nullptr; }
  virtual IntStaticType *getIntStatic() { return nullptr; }
  virtual StrStaticType *getStrStatic() { return nullptr; }
  virtual BoolStaticType *getBoolStatic() { return nullptr; }
  virtual UnionType *getUnion() { return nullptr; }
  virtual ClassType *getHeterogenousTuple() { return nullptr; }

  virtual bool is(const std::string &s);

  Type *operator<<(Type *t);

  static LiteralKind literalFromString(const std::string &s);
  static std::string stringFromLiteral(LiteralKind k);

protected:
  Cache *cache;
  explicit Type(const std::shared_ptr<Type> &);
  explicit Type(Cache *, const SrcInfo & = SrcInfo());
};
using TypePtr = std::shared_ptr<Type>;

} // namespace codon::ast::types

template <typename T>
struct fmt::formatter<
    T, std::enable_if_t<std::is_base_of_v<codon::ast::types::Type, T>, char>>
    : fmt::formatter<std::string_view> {
  char presentation = 'b';

  constexpr auto parse(const format_parse_context &ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    if (const auto end = ctx.end();
        it != end && (*it == 'a' || *it == 'b' || *it == 'c'))
      presentation = *it++;
    return it;
  }

  template <typename FormatContext>
  auto format(const T &p, FormatContext &ctx) const -> decltype(ctx.out()) {
    if (presentation == 'a')
      return fmt::format_to(ctx.out(), "{}", p.debugString(0));
    else if (presentation == 'b')
      return fmt::format_to(ctx.out(), "{}", p.debugString(1));
    else
      return fmt::format_to(ctx.out(), "{}", p.debugString(2));
  }
};
