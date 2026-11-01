// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <set>
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
  InternedString baseName;
  /// Full module name
  InternedString moduleName;
  /// Type
  types::TypePtr type = nullptr;

  /// Information about number of nested conditionals (blocks).
  int blockLevel = 0;
  /// Specifies at which time the name was added to the context.
  /// Used to prevent using later definitions early (can happen in
  /// advanced type checking iterations).
  int64_t time = 0;

  /// Set if an identifier is a class or a function generic
  bool generic = false;

  /// Points to another type in case this type is not useful
  /// [used within isinstance blocks].
  std::shared_ptr<TypecheckItem> alternative = nullptr;

  TypecheckItem(std::string, InternedString, InternedString, types::TypePtr, int = 0);
  TypecheckItem(std::string, std::string_view, InternedString, types::TypePtr, int = 0);

  /* Convenience getters */
  const std::string &getBaseName() const { return baseName.str(); }
  const std::string &getModule() const { return moduleName.str(); }
  bool isVar() const { return !generic && !isFunc() && !isType(); }
  bool isFunc() const { return type->getFunc() != nullptr; }
  bool isType() const { return type->is(StdlibTypes::Type); }

  bool isGlobal() const { return blockLevel == 0 && baseName.empty(); }
  /// True if an identifier is within a conditional block
  /// (i.e., a block that might not be executed during the runtime)
  bool isConditional() const { return blockLevel > 0; }
  bool isGeneric() const { return generic; }
  types::LiteralKind getStaticKind() const { return type->getStaticKind(); }

  types::Type *getType() const { return type.get(); }
  const std::string &getName() const { return canonicalName; }

  int64_t getTime() const { return time; }
};

/** Context class that tracks identifiers during the typechecking. **/
struct TypeContext : public Context<TypecheckItem> {
  /// A pointer to the shared cache.
  Cache *cache;

  /// Holds the information about current base.
  /// A base is defined as a function or a class block.
  struct Base {
    /// Canonical name of a function or a class that owns this base.
    InternedString name;
    /// Function type
    types::TypePtr type;
    /// The return type of currently realized function
    types::TypePtr returnType;
    /// Typechecking iteration
    int iteration = 0;
    /// Only set for functions.
    FunctionStmt *func = nullptr;
    Stmt *suite = nullptr;
    /// Collect reached returns before committing to a result type or wrapping them.
    bool inferReturnType = false;
    /// Reached returns and whether each was originally bare, for nullable joining.
    std::vector<std::pair<ReturnStmt *, bool>> returnStatements;
    /// Index of the parent base
    int parent = 0;

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

    static constexpr int NONE_ARGUMENT_DEFAULT_PRIORITY = -1;
    std::map<int, std::set<types::TypePtr>> pendingDefaults;
    std::set<types::TypePtr> recursiveDependencies;

  public:
    Base() = default;
    Base(std::string_view name, types::TypePtr type = nullptr,
         types::TypePtr returnType = nullptr)
        : name(name), type(std::move(type)), returnType(std::move(returnType)) {}
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
      holder->addBlock();
    }
    ~BaseGuard() {
      holder->bases.pop_back();
      holder->popBlock();
    }
  };

private:
  /// Module information and its normalized identifier are assigned together.
  ImportFile moduleInfo = {ImportFile::PACKAGE, "", ""};
  InternedString moduleIdentifier;

public:
  /// Set if the standard library is currently being loaded.
  bool isStdlibLoading = false;

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

  /// Current statement time.
  int64_t time = 0;

  /// @brief  Type to be expected upon completed typechecking.
  types::TypePtr expectedType = nullptr;

  bool autoPython = false;
  /// True if no side-effects allowed
  bool simpleTypes = false;

  std::unordered_map<std::string, size_t> globalShadows;

  std::unordered_map<std::string, double> _itime;

public:
  explicit TypeContext(Cache *cache, std::string filename = "");

  TypeContext saveInferenceState();
  void restoreInferenceState(TypeContext &&saved);

  void add(const std::string &name, const Item &var) override;
  /// Convenience method for adding an object to the context.
  Item addVar(const std::string &name, const std::string &canonicalName,
              const types::TypePtr &type, int64_t time = 0,
              const SrcInfo &srcInfo = SrcInfo());
  Item addType(const std::string &name, const std::string &canonicalName,
               const types::TypePtr &type, const SrcInfo &srcInfo = SrcInfo());
  Item addFunc(const std::string &name, const std::string &canonicalName,
               const types::TypePtr &type, const SrcInfo &srcInfo = SrcInfo());
  /// Add the item to the standard library module, thus ensuring its visibility from all
  /// modules.
  Item addAlwaysVisible(const Item &item, bool = false);

  /// Get an item from the context before given srcInfo. If the item does not exist,
  /// nullptr is returned.
  Item find(const std::string &name, int64_t time = 0, const char * = nullptr) const;
  /// Get an item that exists in the context. If the item does not exist, assertion is
  /// raised.
  Item forceFind(const std::string &name) const;

  /// Return a canonical name of the current base.
  /// An empty string represents the toplevel base.
  InternedString getBaseName() const;
  /// Return the current module.
  InternedString getModule() const { return moduleIdentifier; }
  /// Assign module information and compute its normalized identifier once.
  void setModule(ImportFile module);
  const ImportFile &getModuleInfo() const { return moduleInfo; }
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
    recordBinding(name);
    auto &bindings = map[name];
    bindings.insert(bindings.begin(), item);
    return item;
  }

public:
  /// Get the current realization depth (i.e., the number of nested realizations).
  size_t getRealizationDepth() const;
  /// Get the name of the current realization stack (e.g., `fn1:fn2:...`).
  std::string getRealizationStackName() const;

private:
  /// Pretty-print the current context state.
  void dump(int pad);
  /// Pretty-print the current realization context.
  std::string debugInfo();

protected:
  void removeFromMap(const std::string &name) override;
};

} // namespace codon::ast
