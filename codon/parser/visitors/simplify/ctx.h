// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <deque>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/ctx.h"

namespace codon::ast {

/**
 * Simplification context identifier.
 * Can be either a function, a class (type), or a variable.
 */
struct SimplifyItem : public SrcObject {
  /// Type of the identifier
  enum Kind { Func, Type, Var } kind;
  /// Base name (e.g., foo.bar.baz)
  std::string baseName;
  /// Unique identifier (canonical name)
  std::string canonicalName;
  /// Full module name
  std::string moduleName;
  /// Full scope information
  std::vector<int> scope;
  /// Non-empty string if a variable is import variable
  std::string importPath;
  /// List of scopes where the identifier is accessible
  /// without __used__ check
  std::vector<std::vector<int>> accessChecked;
  /// Set if an identifier cannot be shadowed
  /// (e.g., global-marked variables)
  bool noShadow = false;
  /// Set if an identifier is a class or a function generic
  bool generic = false;
  /// Set if an identifier is a static variable.
  char staticType = 0;
  /// Set if an identifier should not be dominated
  /// (e.g., a loop variable in a comprehension).
  bool avoidDomination = false;

public:
  SimplifyItem(Kind kind, std::string baseName, std::string canonicalName,
               std::string moduleName, std::vector<int> scope,
               std::string importPath = "")
      : kind(kind), baseName(std::move(baseName)),
        canonicalName(std::move(canonicalName)), moduleName(std::move(moduleName)),
        scope(std::move(scope)), importPath(std::move(importPath)) {}

  /* Convenience getters */
  std::string getBaseName() const { return baseName; }
  std::string getModule() const { return moduleName; }
  bool isVar() const { return kind == Var; }
  bool isFunc() const { return kind == Func; }
  bool isType() const { return kind == Type; }
  bool isImport() const { return !importPath.empty(); }
  bool isGlobal() const { return scope.size() == 1 && baseName.empty(); }
  /// True if an identifier is within a conditional block
  /// (i.e., a block that might not be executed during the runtime)
  bool isConditional() const { return scope.size() > 1; }
  bool isGeneric() const { return generic; }
  char isStatic() const { return staticType; }
  /// True if an identifier is a loop variable in a comprehension
  bool canDominate() const { return !avoidDomination; }
};

/** Context class that tracks identifiers during the simplification. **/
struct SimplifyContext : public Context<SimplifyItem> {
  /// A pointer to the shared cache.
  Cache *cache;

  /// Holds the information about current scope.
  /// A scope is defined as a stack of conditional blocks
  /// (i.e., blocks that might not get executed during the runtime).
  /// Used mainly to support Python's variable scoping rules.
  struct {
    /// Scope counter. Each conditional block gets a new scope ID.
    int counter;
    /// Current hierarchy of conditional blocks.
    std::vector<int> blocks;
    /// List of statements that are to be prepended to a block
    /// after its transformation.
    std::map<int, std::vector<StmtPtr>> stmts;
  } scope;

  /// Holds the information about current base.
  /// A base is defined as a function or a class block.
  struct Base {
    /// Canonical name of a function or a class that owns this base.
    std::string name;
    /// Tracks function attributes (e.g. if it has @atomic or @test attributes).
    /// Only set for functions.
    Attr *attributes;
    /// Set if the base is class base and if class is marked with @deduce.
    /// Stores the list of class fields in the order of traversal.
    std::shared_ptr<std::vector<std::string>> deducedMembers;
    /// Canonical name of `self` parameter that is used to deduce class fields
    /// (e.g., self in self.foo).
    std::string selfName;
    /// Map of captured identifiers (i.e., identifiers not defined in a function).
    /// Captured (canonical) identifiers are mapped to the new canonical names
    /// (representing the canonical function argument names that are appended to the
    /// function after processing) and their types (indicating if they are a type, a
    /// static or a variable).
    std::unordered_map<std::string, std::pair<std::string, ExprPtr>> *captures;

    /// Map of identifiers that are to be fetched from Python.
    std::unordered_set<std::string> *pyCaptures;

    /// Scope that defines the base.
    std::vector<int> scope;

    /// A stack of nested loops enclosing the current statement used for transforming
    /// "break" statement in loop-else constructs. Each loop is defined by a "break"
    /// variable created while parsing a loop-else construct. If a loop has no else
    /// block, the corresponding loop variable is empty.
    struct Loop {
      std::string breakVar;
      std::vector<int> scope;
      /// List of variables "seen" before their assignment within a loop.
      /// Used to dominate variables that are updated within a loop.
      std::unordered_set<std::string> seenVars;
      /// False if a loop has continue/break statement. Used for flattening static
      /// loops.
      bool flat = true;
    };
    std::vector<Loop> loops;

  public:
    explicit Base(std::string name, Attr *attributes = nullptr);
    Loop *getLoop() { return loops.empty() ? nullptr : &(loops.back()); }
    bool isType() const { return attributes == nullptr; }
  };
  /// Current base stack (the last enclosing base is the last base in the stack).
  std::vector<Base> bases;

  struct BaseGuard {
    SimplifyContext *holder;
    BaseGuard(SimplifyContext *holder, const std::string &name) : holder(holder) {
      holder->bases.emplace_back(Base(name));
      holder->bases.back().scope = holder->scope.blocks;
      holder->addBlock();
    }
    ~BaseGuard() {
      holder->bases.pop_back();
      holder->popBlock();
    }
  };

  /// Set of seen global identifiers used to prevent later creation of local variables
  /// with the same name.
  std::unordered_map<std::string, std::unordered_map<std::string, ExprPtr>>
      seenGlobalIdentifiers;

  /// Set if the standard library is currently being loaded.
  bool isStdlibLoading;
  /// Current module. The default module is named `__main__`.
  ImportFile moduleName;
  /// Tracks if we are in a dependent part of a short-circuiting expression (e.g. b in a
  /// and b) to disallow assignment expressions there.
  bool isConditionalExpr;
  /// Allow type() expressions. Currently used to disallow type() in class
  /// and function definitions.
  bool allowTypeOf;
  /// Set if all assignments should not be dominated later on.
  bool avoidDomination = false;

public:
  SimplifyContext(std::string filename, Cache *cache);

  void add(const std::string &name, const Item &var) override;
  /// Convenience method for adding an object to the context.
  Item addVar(const std::string &name, const std::string &canonicalName,
              const SrcInfo &srcInfo = SrcInfo());
  Item addType(const std::string &name, const std::string &canonicalName,
               const SrcInfo &srcInfo = SrcInfo());
  Item addFunc(const std::string &name, const std::string &canonicalName,
               const SrcInfo &srcInfo = SrcInfo());
  /// Add the item to the standard library module, thus ensuring its visibility from all
  /// modules.
  Item addAlwaysVisible(const Item &item);

  /// Get an item from the context. If the item does not exist, nullptr is returned.
  Item find(const std::string &name) const override;
  /// Get an item that exists in the context. If the item does not exist, assertion is
  /// raised.
  Item forceFind(const std::string &name) const;
  /// Get an item from the context. Perform domination analysis for accessing items
  /// defined in the conditional blocks (i.e., Python scoping).
  Item findDominatingBinding(const std::string &name);

  /// Return a canonical name of the current base.
  /// An empty string represents the toplevel base.
  std::string getBaseName() const;
  /// Return the current module.
  std::string getModule() const;
  /// Pretty-print the current context state.
  void dump() override;

  /// Generate a unique identifier (name) for a given string.
  std::string generateCanonicalName(const std::string &name, bool includeBase = false,
                                    bool zeroId = false) const;
  /// Enter a conditional block.
  void enterConditionalBlock();
  /// Leave a conditional block. Populate stmts (if set) with the declarations of newly
  /// added identifiers that dominate the children blocks.
  void leaveConditionalBlock(std::vector<StmtPtr> *stmts = nullptr);
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

private:
  /// Pretty-print the current context state.
  void dump(int pad);
};

} // namespace codon::ast
