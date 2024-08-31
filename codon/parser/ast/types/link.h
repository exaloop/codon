// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast/types/traits.h"
#include "codon/parser/ast/types/type.h"

namespace codon::ast::types {

struct LinkType : public Type {
  /// Enumeration describing the current state.
  enum Kind { Unbound, Generic, Link } kind;
  /// The unique identifier of an unbound or generic type.
  int id;
  /// The type-checking level of an unbound type.
  int level;
  /// The type to which LinkType points to. nullptr if unknown (unbound or generic).
  TypePtr type;
  /// >0 if a type is a static type (e.g. N in Int[N: int]); 0 otherwise.
  char isStatic;
  /// Optional trait that unbound type requires prior to unification.
  std::shared_ptr<Trait> trait;
  /// The generic name of a generic type, if applicable. Used for pretty-printing.
  std::string genericName;
  /// Type that will be used if an unbound is not resolved.
  TypePtr defaultType;

public:
  LinkType(Cache *cache, Kind kind, int id, int level = 0, TypePtr type = nullptr,
           char isStatic = 0, std::shared_ptr<Trait> trait = nullptr,
           TypePtr defaultType = nullptr, std::string genericName = "");
  /// Convenience constructor for linked types.
  explicit LinkType(TypePtr type);

public:
  int unify(Type *typ, Unification *undodo) override;
  TypePtr generalize(int atLevel) override;
  TypePtr instantiate(int atLevel, int *unboundCount,
                      std::unordered_map<int, TypePtr> *cache) override;

public:
  TypePtr follow() override;
  bool hasUnbounds(bool = false) const override;
  std::vector<Type*> getUnbounds() const override;
  bool canRealize() const override;
  bool isInstantiated() const override;
  std::string debugString(char mode) const override;
  std::string realizedName() const override;

  LinkType * getLink() override;
  FuncType * getFunc() override;
  ClassType * getPartial() override;
  ClassType * getClass() override;
  StaticType * getStatic() override;
  IntStaticType * getIntStatic() override;
  StrStaticType * getStrStatic() override;
  BoolStaticType * getBoolStatic() override;
  UnionType * getUnion() override;
  LinkType * getUnbound() override;

private:
  /// Checks if a current (unbound) type occurs within a given type.
  /// Needed to prevent a recursive unification (e.g. ?1 with list[?1]).
  bool occurs(Type *typ, Type::Unification *undo);
};

} // namespace codon::ast::types
