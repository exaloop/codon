#pragma once

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/ctx.h"

namespace codon {
namespace ast {

/**
 * Type-checking context object description.
 * This represents an identifier that can be either a function, a class (type), or a
 * variable.
 */
struct TypecheckItem {
  enum Kind { Func, Type, Var } kind;
  /// Item's type.
  types::TypePtr type;

  TypecheckItem(Kind k, types::TypePtr type) : kind(k), type(move(type)) {}
  bool isType() const { return kind == Type; }
  bool isVar() const { return kind == Var; }
};

/**
 * A variable table (context) for type-checking stage.
 */
struct TypeContext : public Context<TypecheckItem> {
  /// A pointer to the shared cache.
  Cache *cache;

  /// A realization base definition. Each function realization defines a new base scope.
  /// Used to properly realize enclosed functions and to prevent mess with mutually
  /// recursive enclosed functions.
  struct RealizationBase {
    /// Function name
    std::string name;
    /// Function type
    types::TypePtr type;
    /// The return type of currently realized function
    types::TypePtr returnType;
  };
  std::vector<RealizationBase> bases;

  /// The current type-checking level (for type instantiation and generalization).
  int typecheckLevel;
  std::set<types::TypePtr> pendingDefaults;
  int changedNodes;

  /// The age of the currently parsed statement.
  int age;
  /// Number of nested realizations. Used to prevent infinite instantiations.
  int realizationDepth;
  /// Nested default argument calls. Used to prevent infinite CallExpr chains
  /// (e.g. class A: def __init__(a: A = A())).
  std::set<std::string> defaultCallDepth;

  /// Number of nested blocks (0 for toplevel)
  int blockLevel;
  /// True if an early return is found (anything afterwards won't be typechecked)
  bool returnEarly;

public:
  explicit TypeContext(Cache *cache);

  using Context<TypecheckItem>::add;
  /// Convenience method for adding an object to the context.
  std::shared_ptr<TypecheckItem> add(TypecheckItem::Kind kind, const std::string &name,
                                     types::TypePtr type = nullptr);
  std::shared_ptr<TypecheckItem>
  addToplevel(const std::string &name, const std::shared_ptr<TypecheckItem> &item) {
    map[name].push_front(item);
    return item;
  }
  std::shared_ptr<TypecheckItem> find(const std::string &name) const override;
  /// Find an internal type. Assumes that it exists.
  std::shared_ptr<TypecheckItem> forceFind(const std::string &name) const;
  types::TypePtr getType(const std::string &name) const;

  /// Pretty-print the current context state.
  void dump() override { dump(0); }

public:
  /// Find a base with a given name.
  int findBase(const std::string &b);
  /// Return the name of the current realization stack (e.g. fn1:fn2:...).
  std::string getBase() const;
  /// Return the current base nesting level (note: bases, not blocks).
  int getLevel() const { return bases.size(); }

public:
  /// Create an unbound type.
  /// @param expr Expression that needs the type. Used to set type's srcInfo.
  /// @param level Type-checking level.
  /// @param setActive If True, add it to activeUnbounds.
  /// @param isStatic True if this is a static integer unbound.
  std::shared_ptr<types::LinkType> getUnbound(const SrcInfo &info, int level) const;
  std::shared_ptr<types::LinkType> getUnbound(const SrcInfo &info) const;
  std::shared_ptr<types::LinkType> getUnbound() const;
  /// Call `type->instantiate`.
  /// Prepare the generic instantiation table with the given generics parameter.
  /// Example: when instantiating List[T].foo, generics=List[int].foo will ensure that
  ///          T=int.
  /// @param expr Expression that needs the type. Used to set type's srcInfo.
  /// @param setActive If True, add unbounds to activeUnbounds.
  types::TypePtr instantiate(const SrcInfo &info, types::TypePtr type,
                             const types::ClassTypePtr &generics = nullptr);
  types::TypePtr instantiate(types::TypePtr type,
                             const types::ClassTypePtr &generics = nullptr) {
    return instantiate(getSrcInfo(), type, generics);
  }
  /// Instantiate the generic type root with the provided generics.
  /// @param expr Expression that needs the type. Used to set type's srcInfo.
  types::TypePtr instantiateGeneric(const SrcInfo &info, types::TypePtr root,
                                    const std::vector<types::TypePtr> &generics);
  types::TypePtr instantiateGeneric(types::TypePtr root,
                                    const std::vector<types::TypePtr> &generics) {
    return instantiateGeneric(getSrcInfo(), root, generics);
  }

  /// Returns the list of generic methods that correspond to typeName.method.
  std::vector<types::FuncTypePtr> findMethod(const std::string &typeName,
                                             const std::string &method,
                                             bool hideShadowed = true) const;
  /// Returns the generic type of typeName.member, if it exists (nullptr otherwise).
  /// Special cases: __elemsize__ and __atomic__.
  types::TypePtr findMember(const std::string &typeName,
                            const std::string &member) const;

  using ReorderDoneFn = std::function<int(int, int, const std::vector<std::vector<int>> &, bool)>;
  using ReorderErrorFn = std::function<int(std::string)>;
  /// Reorders a given vector or named arguments (consisting of names and the
  /// corresponding types) according to the signature of a given function.
  /// Returns the reordered vector and an associated reordering score (missing
  /// default arguments' score is half of the present arguments).
  /// Score is -1 if the given arguments cannot be reordered.
  /// @param known Bitmask that indicated if an argument is already provided
  ///              (partial function) or not.
  int reorderNamedArgs(types::FuncType *func, const std::vector<CallExpr::Arg> &args,
                       ReorderDoneFn onDone, ReorderErrorFn onError,
                       const std::vector<char> &known = std::vector<char>());

private:
  /// Pretty-print the current context state.
  void dump(int pad);
};

} // namespace ast
} // namespace codon
