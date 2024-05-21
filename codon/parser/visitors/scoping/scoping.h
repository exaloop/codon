// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/ctx.h"
#include "codon/parser/visitors/visitor.h"

namespace codon::ast {

class ScopingVisitor : public ReplacingCallbackASTVisitor {
  struct Context {
    /// A pointer to the shared cache.
    Cache *cache;

    /// Holds the information about current scope.
    /// A scope is defined as a stack of conditional blocks
    /// (i.e., blocks that might not get executed during the runtime).
    /// Used mainly to support Python's variable scoping rules.
    struct ScopeBlock {
      int id;
      /// List of statements that are to be prepended to a block
      /// after its transformation.
      std::vector<Stmt *> stmts;

      /// List of variables "seen" before their assignment within a loop.
      /// Used to dominate variables that are updated within a loop.
      std::shared_ptr<std::unordered_set<std::string>> seenVars = nullptr;
      ScopeBlock(int id) : id(id), seenVars(nullptr) {}
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

    struct Item : public codon::SrcObject {
      std::vector<int> scope;
      Node *binding = nullptr;

      /// List of scopes where the identifier is accessible
      /// without __used__ check
      std::vector<std::vector<int>> accessChecked;

      Item(const codon::SrcInfo &src, std::vector<int> scope, Node *binding = nullptr,
           std::vector<std::vector<int>> accessChecked = {})
          : scope(std::move(scope)), binding(std::move(binding)),
            accessChecked(std::move(accessChecked)) {
        setSrcInfo(src);
      }
    };
    std::unordered_map<std::string, std::list<Item>> map;

    std::unordered_map<std::string, Attr::CaptureType> captures;
    std::unordered_map<std::string, Attr::CaptureType> childCaptures; // for functions!
    std::map<std::string, SrcInfo> firstSeen;

    bool adding = false;
    Node *root = nullptr;
    FunctionStmt *functionScope = nullptr;
    bool inClass = false;
    bool isConditional = false;

    std::vector<std::unordered_map<std::string, std::string>> renames = {{}};
    bool tempScope = false;
  };
  std::shared_ptr<Context> ctx = nullptr;
  Expr *resultExpr = nullptr;
  Stmt *resultStmt = nullptr;

public:
  static Stmt *apply(Cache *, Stmt *s);
  Expr *transform(Expr *expr) override;
  Stmt *transform(Stmt *stmt) override;

  void visitName(const std::string &name, bool = false, Node * = nullptr,
                 const SrcInfo & = SrcInfo());
  Expr *transformAdding(Expr *e, Node *);
  Expr *transformFString(const std::string &);

  void visit(IdExpr *) override;
  void visit(StringExpr *) override; // because of f-strings argh!
  void visit(GeneratorExpr *) override;
  void visit(AssignExpr *) override;
  void visit(LambdaExpr *) override;
  void visit(IfExpr *) override;
  void visit(BinaryExpr *) override;
  void visit(YieldExpr *) override;
  void visit(AssignStmt *) override;
  void visit(IfStmt *) override;
  void visit(MatchStmt *) override;
  void visit(WhileStmt *) override;
  void visit(ForStmt *) override;
  void visit(ImportStmt *) override;
  void visit(DelStmt *) override;
  void visit(TryStmt *) override;
  void visit(GlobalStmt *) override;
  void visit(YieldStmt *) override;
  void visit(FunctionStmt *) override;
  void visit(ClassStmt *) override;
  void visit(WithStmt *) override;

  Context::Item *findDominatingBinding(const std::string &, bool = true);
  void unpackAssignments(Expr *, Expr *, std::vector<Stmt *> &);
  /// Enter a conditional block.
  void enterConditionalBlock();
  /// Leave a conditional block. Populate stmts (if set) with the declarations of
  /// newly added identifiers that dominate the children blocks.
  void leaveConditionalBlock();
  void leaveConditionalBlock(Stmt **);

  Stmt *transformBlock(Stmt *);
  Expr *makeAnonFn(std::vector<Stmt *>, const std::vector<std::string> & = {});
  void switchToUpdate(Node *binding, const std::string &, bool);

  template <typename Tn, typename... Ts> Tn *N(Ts &&...args) {
    Tn *t = ctx->cache->N<Tn>(std::forward<Ts>(args)...);
    t->setSrcInfo(getSrcInfo());
    return t;
  }
};

} // namespace codon::ast
