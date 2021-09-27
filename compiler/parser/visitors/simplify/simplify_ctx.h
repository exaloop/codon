/*
 * simplify_ctx.h --- Context for simplification transformation.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#pragma once

#include <deque>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "parser/cache.h"
#include "parser/common.h"
#include "parser/ctx.h"

namespace seq {
namespace ast {

/**
 * Simplification context object description.
 * This represents an identifier that can be either a function, a class (type), a
 * variable or an import.
 */
struct SimplifyItem {
  /// Object kind (function, class, variable, or import).
  enum Kind { Func, Type, Var } kind;
  /// Object's base function
  string base;
  /// Object's unique identifier (canonical name)
  string canonicalName;
  /// True if an object is global.
  bool global;
  /// Non-empty string if a variable is import variable
  string importPath;

public:
  SimplifyItem(Kind k, string base, string canonicalName, bool global = false);

  /// Convenience getters.
  string getBase() const { return base; }
  bool isGlobal() const { return global; }
  bool isVar() const { return kind == Var; }
  bool isFunc() const { return kind == Func; }
  bool isType() const { return kind == Type; }
  bool isImport() const { return !importPath.empty(); }
};

/**
 * A variable table (context) for simplification stage.
 */
struct SimplifyContext : public Context<SimplifyItem> {
  /// A pointer to the shared cache.
  shared_ptr<Cache> cache;

  /// A base scope definition. Each function or a class defines a new base scope.
  struct Base {
    /// Canonical name of a base-defining function or a class.
    string name;
    /// Declaration AST of a base-defining class (or nullptr otherwise) for
    /// automatically annotating "self" and other parameters. For example, for
    /// class Foo[T, N: int]: ..., AST is Foo[T, N: int]
    ExprPtr ast;
    /// Tracks function attributes (e.g. if it has @atomic or @test attributes).
    int attributes;

    explicit Base(string name, ExprPtr ast = nullptr, int attributes = 0);
    bool isType() const { return ast != nullptr; }
  };
  /// A stack of bases enclosing the current statement (the topmost base is the last
  /// base). Top-level has no base.
  vector<Base> bases;
  /// A stack of nested loops enclosing the current statement used for transforming
  /// "break" statement in loop-else constructs. Each loop is defined by a "break"
  /// variable created while parsing a loop-else construct. If a loop has no else block,
  /// the corresponding loop variable is empty.
  vector<string> loops;
  /// A stack of nested maps that capture variables defined in enclosing bases (e.g.
  /// variables not defined in a function). Maps captured canonical names to new
  /// canonical names (used in the inner function). Used for capturing outer variables
  /// in generators and lambda functions. A stack is needed because there might be
  /// nested generator or lambda constructs.
  vector<std::map<string, string>> captures;
  /// True if standard library is being loaded.
  bool isStdlibLoading;
  /// Current module name (Python's __name__) and its source. The default module is
  /// __main__.
  ImportFile moduleName;
  /// Tracks if we are in a dependent part of a short-circuiting expression (e.g. b in a
  /// and b) to disallow assignment expressions there.
  bool canAssign;
  /// Allow type() expressions.
  bool allowTypeOf;
  /// Replacement expressions.
  unordered_map<string, ExprPtr> *substitutions;

public:
  SimplifyContext(string filename, shared_ptr<Cache> cache);

  using Context<SimplifyItem>::add;
  /// Convenience method for adding an object to the context.
  shared_ptr<SimplifyItem> add(SimplifyItem::Kind kind, const string &name,
                               const string &canonicalName = "", bool global = false);
  shared_ptr<SimplifyItem> find(const string &name) const override;

  /// Return a canonical name of the top-most base, or an empty string if this is a
  /// top-level base.
  string getBase() const;
  /// Return the current base nesting level (note: bases, not blocks).
  int getLevel() const { return bases.size(); }
  /// Pretty-print the current context state.
  void dump() override { dump(0); }

  /// Generate a unique identifier (name) for a given string.
  string generateCanonicalName(const string &name, bool includeBase = false) const;

  bool inFunction() const { return getLevel() && !bases.back().isType(); }
  bool inClass() const { return getLevel() && bases.back().isType(); }

private:
  /// Pretty-print the current context state.
  void dump(int pad);
};

} // namespace ast
} // namespace seq
