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
 * Simplification context object description.
 * This represents an identifier that can be either a function, a class (type), a
 * variable or an import.
 */
struct SimplifyItem : public SrcObject {
  /// Object kind (function, class, variable, or import).
  enum Kind { Func, Type, Var } kind;
  /// Object's base function
  std::string base;
  /// Object's unique identifier (canonical name)
  std::string canonicalName;
  /// Full module name
  std::string moduleName;
  /// Full scope information
  std::vector<int> scope;
  /// Non-empty string if a variable is import variable
  std::string importPath;
  bool accessChecked;
  bool noShadow;
  ExprPtr replacement;

public:
  SimplifyItem(Kind kind, std::string base, std::string canonicalName,
               std::string moduleName, std::vector<int> scope,
               std::string importPath = "")
      : kind(kind), base(std::move(base)), canonicalName(std::move(canonicalName)),
        moduleName(std::move(moduleName)), scope(std::move(scope)),
        importPath(std::move(importPath)), accessChecked(true), noShadow(false) {}

  /// Convenience getters.
  std::string getBase() const { return base; }
  std::string getModule() const { return moduleName; }
  bool isVar() const { return kind == Var; }
  bool isFunc() const { return kind == Func; }
  bool isType() const { return kind == Type; }
  bool isImport() const { return !importPath.empty(); }

  bool isGlobal() const { return scope.size() == 1 && base.empty(); }
};

/**
 * A variable table (context) for simplification stage.
 */
struct SimplifyContext : public Context<SimplifyItem> {
  /// A pointer to the shared cache.
  Cache *cache;

  int scopeCnt;
  /// Sorted hierarchy of scopes!
  std::vector<int> scope;

  std::map<int, std::vector<StmtPtr>> scopeStmts;
  std::map<std::string, std::pair<std::string, bool>> scopeRenames;
  std::vector<std::set<std::string>> scopeOutsides;

  /// A base scope definition. Each function or a class defines a new base scope.
  struct Base {
    /// Canonical name of a base-defining function or a class.
    std::string name;
    /// Declaration AST of a base-defining class (or nullptr otherwise) for
    /// automatically annotating "self" and other parameters. For example, for
    /// class Foo[T, N: int]: ..., AST is Foo[T, N: int]
    ExprPtr ast;
    /// Tracks function attributes (e.g. if it has @atomic or @test attributes).
    int attributes;

    std::shared_ptr<std::vector<std::string>> deducedMembers;
    std::string selfName;

    explicit Base(std::string name, ExprPtr ast = nullptr, int attributes = 0);
    bool isType() const { return ast != nullptr; }
  };
  /// A stack of bases enclosing the current statement (the topmost base is the last
  /// base). Top-level has no base.
  std::vector<Base> bases;

  // set of seen variables
  std::unordered_map<std::string, std::unordered_map<std::string, ExprPtr>>
      seenGlobalIdentifiers;

  /// A stack of nested loops enclosing the current statement used for transforming
  /// "break" statement in loop-else constructs. Each loop is defined by a "break"
  /// variable created while parsing a loop-else construct. If a loop has no else block,
  /// the corresponding loop variable is empty.
  struct Loop {
    std::string breakVar;
    std::vector<int> scope;
    std::unordered_set<std::string> seenVars;
  };
  std::vector<Loop> loops;
  bool inLoop() const { return !loops.empty(); }
  Loop *getLoop() { return inLoop() ? &(loops.back()) : nullptr; }

  /// A stack of nested maps that capture variables defined in enclosing bases (e.g.
  /// variables not defined in a function). Maps captured canonical names to new
  /// canonical names (used in the inner function). Used for capturing outer variables
  /// in generators and lambda functions. A stack is needed because there might be
  /// nested generator or lambda constructs.
  std::vector<std::map<std::string, std::string>> captures;
  /// True if standard library is being loaded.
  bool isStdlibLoading;
  /// Current module name (Python's __name__) and its source. The default module is
  /// __main__.
  ImportFile moduleName;
  /// Tracks if we are in a dependent part of a short-circuiting expression (e.g. b in a
  /// and b) to disallow assignment expressions there.
  bool shortCircuit;
  /// Allow type() expressions.
  bool allowTypeOf;
  /// Replacement expressions.
  std::unordered_map<std::string, ExprPtr> *substitutions;

  std::vector<SrcInfo> srcInfos;

public:
  SimplifyContext(std::string filename, Cache *cache);

  /// Convenience method for adding an object to the context.
  std::shared_ptr<SimplifyItem> addVar(const std::string &name,
                                       const std::string &canonicalName,
                                       const SrcInfo &srcInfo = SrcInfo());
  std::shared_ptr<SimplifyItem> addType(const std::string &name,
                                        const std::string &canonicalName,
                                        const SrcInfo &srcInfo = SrcInfo());
  std::shared_ptr<SimplifyItem> addFunc(const std::string &name,
                                        const std::string &canonicalName,
                                        const SrcInfo &srcInfo = SrcInfo());
  std::shared_ptr<SimplifyItem>
  addAlwaysVisible(const std::shared_ptr<SimplifyItem> &item);

  std::shared_ptr<SimplifyItem> find(const std::string &name) const override;
  std::shared_ptr<SimplifyItem> findDominatingBinding(const std::string &name);

  /// Return a canonical name of the top-most base, or an empty string if this is a
  /// top-level base.
  std::string getBase() const;
  /// Return the current module.
  std::string getModule() const;
  /// Return the current base nesting level (note: bases, not blocks).
  int getLevel() const { return int(bases.size()); }
  /// Pretty-print the current context state.
  void dump() override { dump(0); }

  /// Generate a unique identifier (name) for a given string.
  std::string generateCanonicalName(const std::string &name, bool includeBase = false,
                                    bool zeroId = false) const;

  bool inFunction() const { return getLevel() && !bases.back().isType(); }
  bool inClass() const { return getLevel() && bases.back().isType(); }

  void addBlock() override;
  void popBlock() override;

  void addScope() { scope.push_back(++scopeCnt); }
  void popScope(std::vector<StmtPtr> *stmts = nullptr) {
    if (stmts && in(scopeStmts, scope.back()))
      stmts->insert(stmts->begin(), scopeStmts[scope.back()].begin(),
                    scopeStmts[scope.back()].end());
    scope.pop_back();
  }

  void pushSrcInfo(SrcInfo s) { srcInfos.emplace_back(std::move(s)); }
  void popSrcInfo() { srcInfos.pop_back(); }
  SrcInfo getSrcInfo() const { return srcInfos.back(); }

private:
  /// Pretty-print the current context state.
  void dump(int pad);
};

} // namespace codon::ast
