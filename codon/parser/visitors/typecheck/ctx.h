// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/ctx.h"

namespace codon::ast {

class TypecheckVisitor;

/**
 * Typecheck context identifier.
 * Can be either a function, a class (type), or a variable.
 */
struct TypecheckItem : public SrcObject {
  /// Unique identifier (canonical name)
  std::string canonicalName;
  /// Base name (e.g., foo.bar.baz)
  std::string baseName;
  /// Full module name
  std::string moduleName;
  /// Type
  types::TypePtr type = nullptr;

  /// Full base scope information
  std::vector<int> scope = {0};

  /// Set if an identifier is a class or a function generic
  bool generic = false;

  TypecheckItem(std::string, std::string, std::string, types::TypePtr,
                std::vector<int> = {0});

  /* Convenience getters */
  std::string getBaseName() const { return baseName; }
  std::string getModule() const { return moduleName; }
  bool isVar() const { return !generic && !isFunc() && !isType(); }
  bool isFunc() const { return type->getFunc() != nullptr; }
  bool isType() const { return type->is("type"); }

  bool isGlobal() const { return scope.size() == 1 && baseName.empty(); }
  /// True if an identifier is within a conditional block
  /// (i.e., a block that might not be executed during the runtime)
  bool isConditional() const { return scope.size() > 1; }
  bool isGeneric() const { return generic; }
  char isStatic() const { return type->isStaticType(); }

  types::Type *getType() const { return type.get(); }
  std::string getName() const { return canonicalName; }
};

/** Context class that tracks identifiers during the typechecking. **/
struct TypeContext : public Context<TypecheckItem> {
  /// A pointer to the shared cache.
  Cache *cache;

  /// Holds the information about current scope.
  /// A scope is defined as a stack of conditional blocks
  /// (i.e., blocks that might not get executed during the runtime).
  /// Used mainly to support Python's variable scoping rules.
  struct ScopeBlock {
    int id;
    std::unordered_map<std::string, std::pair<std::string, bool>> replacements;
    /// List of statements that are to be prepended to a block
    /// after its transformation.
    std::vector<Stmt *> stmts;
    ScopeBlock(int id) : id(id) {}
  };
  /// Current hierarchy of conditional blocks.
  std::vector<ScopeBlock> scope;
  std::vector<int> getScope() const {
    std::vector<int> result;
    result.reserve(scope.size());
    for (const auto &b : scope)
      result.emplace_back(b.id);
    return result;
  }

  /// Holds the information about current base.
  /// A base is defined as a function or a class block.
  struct Base {
    /// Canonical name of a function or a class that owns this base.
    std::string name;
    /// Function type
    types::TypePtr type;
    /// The return type of currently realized function
    types::TypePtr returnType;
    /// Typechecking iteration
    int iteration = 0;
    /// Only set for functions.
    FunctionStmt *func = nullptr;
    Stmt *suite = nullptr;

    struct {
      /// Set if the base is class base and if class is marked with @deduce.
      /// Stores the list of class fields in the order of traversal.
      std::shared_ptr<std::vector<std::string>> deducedMembers = nullptr;
      /// Canonical name of `self` parameter that is used to deduce class fields
      /// (e.g., self in self.foo).
      std::string selfName;
    } deduce;

    /// Map of captured identifiers (i.e., identifiers not defined in a function).
    /// Captured (canonical) identifiers are mapped to the new canonical names
    /// (representing the canonical function argument names that are appended to the
    /// function after processing) and their types (indicating if they are a type, a
    /// static or a variable).
    // std::unordered_set<std::string> captures;

    /// Map of identifiers that are to be fetched from Python.
    std::unordered_set<std::string> *pyCaptures = nullptr;

    /// Scope that defines the base.
    std::vector<int> scope;

    /// A stack of nested loops enclosing the current statement used for transforming
    /// "break" statement in loop-else constructs. Each loop is defined by a "break"
    /// variable created while parsing a loop-else construct. If a loop has no else
    /// block, the corresponding loop variable is empty.
    struct Loop {
      std::string breakVar;
      /// False if a loop has continue/break statement. Used for flattening static
      /// loops.
      bool flat = true;
      Loop(const std::string &breakVar) : breakVar(breakVar) {}
    };
    std::vector<Loop> loops;

    std::set<types::TypePtr> pendingDefaults;

  public:
    Loop *getLoop() { return loops.empty() ? nullptr : &(loops.back()); }
    bool isType() const { return func == nullptr; }
  };
  /// Current base stack (the last enclosing base is the last base in the stack).
  std::vector<Base> bases;

  struct BaseGuard {
    TypeContext *holder;
    BaseGuard(TypeContext *holder, const std::string &name) : holder(holder) {
      holder->bases.emplace_back();
      holder->bases.back().name = name;
      holder->bases.back().scope = holder->getScope();
      holder->addBlock();
    }
    ~BaseGuard() {
      holder->bases.pop_back();
      holder->popBlock();
    }
  };

  /// Current module. The default module is named `__main__`.
  ImportFile moduleName = {ImportFile::PACKAGE, "", ""};
  /// Set if the standard library is currently being loaded.
  bool isStdlibLoading = false;
  /// Allow type() expressions. Currently used to disallow type() in class
  /// and function definitions.
  bool allowTypeOf = true;

  /// The current type-checking level (for type instantiation and generalization).
  int typecheckLevel = 0;
  int changedNodes = 0;

  /// Number of nested realizations. Used to prevent infinite instantiations.
  int realizationDepth = 0;

  /// Number of nested blocks (0 for toplevel)
  int blockLevel = 0;
  /// True if an early return is found (anything afterwards won't be typechecked)
  bool returnEarly = false;
  /// Stack of static loop control variables (used to emulate goto statements).
  std::vector<std::string> staticLoops = {};

public:
  explicit TypeContext(Cache *cache, std::string filename = "");

  void add(const std::string &name, const Item &var) override;
  /// Convenience method for adding an object to the context.
  Item addVar(const std::string &name, const std::string &canonicalName,
              const types::TypePtr &type, const SrcInfo &srcInfo = SrcInfo());
  Item addType(const std::string &name, const std::string &canonicalName,
               const types::TypePtr &type, const SrcInfo &srcInfo = SrcInfo());
  Item addFunc(const std::string &name, const std::string &canonicalName,
               const types::TypePtr &type, const SrcInfo &srcInfo = SrcInfo());
  /// Add the item to the standard library module, thus ensuring its visibility from all
  /// modules.
  Item addAlwaysVisible(const Item &item, bool = false);

  /// Get an item from the context. If the item does not exist, nullptr is returned.
  Item find(const std::string &name) const override;
  /// Get an item that exists in the context. If the item does not exist, assertion is
  /// raised.
  Item forceFind(const std::string &name) const;

  /// Return a canonical name of the current base.
  /// An empty string represents the toplevel base.
  std::string getBaseName() const;
  /// Return the current module.
  std::string getModule() const;
  /// Return the current module path.
  std::string getModulePath() const;
  /// Pretty-print the current context state.
  void dump() override;

  /// Generate a unique identifier (name) for a given string.
  std::string generateCanonicalName(const std::string &name, bool includeBase = false,
                                    bool noSuffix = false) const;
  /// True if we are at the toplevel.
  bool isGlobal() const;
  /// True if we are within a conditional block.
  bool isConditional() const;
  /// Get the current base.
  Base *getBase();
  /// True if the current base is function.
  bool inFunction() const;
  /// True if the current base is class.
  bool inClass() const;
  /// True if an item is defined outside of the current base or a module.
  bool isOuter(const Item &val) const;
  /// Get the enclosing class base (or nullptr if such does not exist).
  Base *getClassBase();

  /// Convenience method for adding an object to the context.
  std::shared_ptr<TypecheckItem>
  addToplevel(const std::string &name, const std::shared_ptr<TypecheckItem> &item) {
    map[name].push_front(item);
    return item;
  }

public:
  /// Get the current realization depth (i.e., the number of nested realizations).
  size_t getRealizationDepth() const;
  /// Get the name of the current realization stack (e.g., `fn1:fn2:...`).
  std::string getRealizationStackName() const;

public:
  /// Create an unbound type with the provided typechecking level.
  std::shared_ptr<types::LinkType> getUnbound(const SrcInfo &info, int level) const;
  std::shared_ptr<types::LinkType> getUnbound(const SrcInfo &info) const;
  std::shared_ptr<types::LinkType> getUnbound() const;

  /// Call `type->instantiate`.
  /// Prepare the generic instantiation table with the given generics parameter.
  /// Example: when instantiating List[T].foo, generics=List[int].foo will ensure that
  ///          T=int.
  /// @param expr Expression that needs the type. Used to set type's srcInfo.
  /// @param setActive If True, add unbounds to activeUnbounds.
  types::TypePtr instantiate(const SrcInfo &info, types::Type *type,
                             types::ClassType *generics = nullptr);
  types::TypePtr instantiate(types::Type *type, types::ClassType *generics = nullptr) {
    return instantiate(getSrcInfo(), std::move(type), generics);
  }

  /// Instantiate the generic type root with the provided generics.
  /// @param expr Expression that needs the type. Used to set type's srcInfo.
  types::TypePtr instantiateGeneric(const SrcInfo &info, types::Type *root,
                                    const std::vector<types::Type *> &generics);
  types::TypePtr instantiateGeneric(types::Type *root,
                                    const std::vector<types::Type *> &generics) {
    return instantiateGeneric(getSrcInfo(), std::move(root), generics);
  }

  /// Returns the list of generic methods that correspond to typeName.method.
  std::vector<types::FuncType *> findMethod(types::ClassType *type,
                                            const std::string &method,
                                            bool hideShadowed = true);
  /// Returns the generic type of typeName.member, if it exists (nullptr otherwise).
  /// Special cases: __elemsize__ and __atomic__.
  Cache::Class::ClassField *findMember(types::ClassType *, const std::string &) const;

  using ReorderDoneFn =
      std::function<int(int, int, const std::vector<std::vector<int>> &, bool)>;
  using ReorderErrorFn = std::function<int(error::Error, const SrcInfo &, std::string)>;
  /// Reorders a given vector or named arguments (consisting of names and the
  /// corresponding types) according to the signature of a given function.
  /// Returns the reordered vector and an associated reordering score (missing
  /// default arguments' score is half of the present arguments).
  /// Score is -1 if the given arguments cannot be reordered.
  /// @param known Bitmask that indicated if an argument is already provided
  ///              (partial function) or not.
  int reorderNamedArgs(types::FuncType *func, const std::vector<CallArg> &args,
                       const ReorderDoneFn &onDone, const ReorderErrorFn &onError,
                       const std::vector<char> &known = std::vector<char>());

  bool isCanonicalName(const std::string &name) const;

private:
  /// Pretty-print the current context state.
  void dump(int pad);
  /// Pretty-print the current realization context.
  std::string debugInfo();

public:
  types::FuncType *extractFunction(types::Type *);
  types::Type *getType(const std::string &);
  types::Type *extractType(types::Type *);

protected:
  void removeFromMap(const std::string &name) override;
};

} // namespace codon::ast
