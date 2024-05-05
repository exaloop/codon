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

class ScopingVisitor : public CallbackASTVisitor<ExprPtr, StmtPtr> {
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
      std::vector<StmtPtr> stmts;

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
      std::shared_ptr<SrcObject> binding = nullptr;

      /// List of scopes where the identifier is accessible
      /// without __used__ check
      std::vector<std::vector<int>> accessChecked;

      Item(const codon::SrcInfo &src, std::vector<int> scope,
           std::shared_ptr<SrcObject> binding = nullptr,
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
    std::shared_ptr<SrcObject> root = nullptr;
    FunctionStmt *functionScope = nullptr;
    bool inClass = false;
    bool isConditional = false;

    std::vector<std::unordered_map<std::string, std::string>> renames = {{}};
    bool tempScope = false;
  };
  std::shared_ptr<Context> ctx = nullptr;
  ExprPtr resultExpr = nullptr;
  StmtPtr resultStmt = nullptr;

public:
  static void apply(Cache *, StmtPtr &s);
  ExprPtr transform(const std::shared_ptr<Expr> &expr) override;
  StmtPtr transform(const std::shared_ptr<Stmt> &stmt) override;
  ExprPtr transform(std::shared_ptr<Expr> &expr) override;
  StmtPtr transform(std::shared_ptr<Stmt> &stmt) override;

  void visitName(const std::string &name, bool = false,
                 const std::shared_ptr<SrcObject> & = nullptr,
                 const SrcInfo & = SrcInfo());
  void transformAdding(ExprPtr &e, std::shared_ptr<SrcObject>);
  ExprPtr transformFString(const std::string &);

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
  void unpackAssignments(const ExprPtr &, ExprPtr, std::vector<StmtPtr> &);
  /// Enter a conditional block.
  void enterConditionalBlock();
  /// Leave a conditional block. Populate stmts (if set) with the declarations of
  /// newly added identifiers that dominate the children blocks.
  void leaveConditionalBlock();
  void leaveConditionalBlock(StmtPtr &);

  void transformBlock(StmtPtr &s);
  ExprPtr makeAnonFn(std::vector<StmtPtr>, const std::vector<std::string> & = {});
  void switchToUpdate(std::shared_ptr<SrcObject> binding, const std::string &, bool);
};

} // namespace codon::ast
