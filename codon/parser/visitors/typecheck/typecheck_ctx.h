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
    /// Map of locally realized types and functions.
    std::unordered_map<std::string, std::pair<TypecheckItem::Kind, types::TypePtr>>
        visitedAsts;
  };
  std::vector<RealizationBase> bases;

  /// The current type-checking level (for type instantiation and generalization).
  int typecheckLevel;
  /// Map of active unbound types. Each type points to its name that is reported in case
  /// something goes wrong.
  /// If type checking is successful, all of them should be  resolved.
  std::map<types::TypePtr, std::string> activeUnbounds;
  /// If set, no type will be activated. Useful for temporary instantiations.
  bool allowActivation;
  /// The age of the currently parsed statement.
  int age;
  /// Number of nested realizations. Used to prevent infinite instantiations.
  int realizationDepth;
  /// Nested default argument calls. Used to prevent infinite CallExpr chains
  /// (e.g. class A: def __init__(a: A = A())).
  std::set<std::string> defaultCallDepth;

  /// Number of nested blocks (0 for toplevel)
  int blockLevel;
  /// True if early return is sounds (anything afterwards won't be typechecked)
  bool returnEarly;

public:
  explicit TypeContext(Cache *cache);

  using Context<TypecheckItem>::add;
  /// Convenience method for adding an object to the context.
  std::shared_ptr<TypecheckItem> add(TypecheckItem::Kind kind, const std::string &name,
                                     types::TypePtr type = nullptr);
  std::shared_ptr<TypecheckItem> find(const std::string &name) const override;
  /// Find an internal type. Assumes that it exists.
  types::TypePtr findInternal(const std::string &name) const;
  /// Find a type or a function instantiation in the base stack.
  std::pair<TypecheckItem::Kind, types::TypePtr>
  findInVisited(const std::string &name) const;

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
  std::shared_ptr<types::LinkType>
  addUnbound(const Expr *expr, int level, bool setActive = true, char staticType = 0);
  /// Call `type->instantiate`.
  /// Prepare the generic instantiation table with the given generics parameter.
  /// Example: when instantiating List[T].foo, generics=List[int].foo will ensure that
  ///          T=int.
  /// @param expr Expression that needs the type. Used to set type's srcInfo.
  /// @param setActive If True, add unbounds to activeUnbounds.
  types::TypePtr instantiate(const Expr *expr, types::TypePtr type,
                             types::ClassType *generics = nullptr,
                             bool setActive = true);
  /// Instantiate the generic type root with the provided generics.
  /// @param expr Expression that needs the type. Used to set type's srcInfo.
  types::TypePtr instantiateGeneric(const Expr *expr, types::TypePtr root,
                                    const std::vector<types::TypePtr> &generics);

  /// Returns the list of generic methods that correspond to typeName.method.
  std::vector<types::FuncTypePtr> findMethod(const std::string &typeName,
                                             const std::string &method) const;
  /// Returns the generic type of typeName.member, if it exists (nullptr otherwise).
  /// Special cases: __elemsize__ and __atomic__.
  types::TypePtr findMember(const std::string &typeName,
                            const std::string &member) const;


  typedef std::function<int(int, int, const std::vector<std::vector<int>> &, bool)>
      ReorderDoneFn;
  typedef std::function<int(std::string)> ReorderErrorFn;
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
