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

struct BindingsAttribute : public ir::Attribute {
  static const std::string AttributeName;

  enum CaptureType { Read, Global, Nonlocal };
  std::unordered_map<std::string, CaptureType> captures;
  std::unordered_map<std::string, size_t> bindings;

  std::unique_ptr<Attribute> clone() const override {
    auto p = std::make_unique<BindingsAttribute>();
    p->captures = captures;
    p->bindings = bindings;
    return p;
  }

private:
  std::ostream &doFormat(std::ostream &os) const override { return os << "Bindings"; }
};

class ScopingVisitor : public CallbackASTVisitor<void, void> {
  struct Context {
    /// A pointer to the shared cache.
    Cache *cache;

    /// Holds the information about current scope.
    /// A scope is defined as a stack of conditional blocks
    /// (i.e., blocks that might not get executed during the runtime).
    /// Used mainly to support Python's variable scoping rules.
    struct ScopeBlock {
      int id;
      // Associated SuiteStmt
      Stmt *suite;
      /// List of variables "seen" before their assignment within a loop.
      /// Used to dominate variables that are updated within a loop.
      std::unique_ptr<std::unordered_set<std::string>> seenVars = nullptr;
      ScopeBlock(int id, Stmt *s = nullptr) : id(id), suite(s), seenVars(nullptr) {}
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
      ASTNode *binding = nullptr;
      bool ignore = false;


      /// List of scopes where the identifier is accessible
      /// without __used__ check
      std::vector<std::vector<int>> accessChecked;
      Item(const codon::SrcInfo &src, std::vector<int> scope, ASTNode *binding = nullptr,
           std::vector<std::vector<int>> accessChecked = {})
          : scope(std::move(scope)), binding(std::move(binding)), ignore(false),
            accessChecked(std::move(accessChecked)) {
        setSrcInfo(src);
      }
    };
    std::unordered_map<std::string, std::list<Item>> map;

    std::unordered_map<std::string, BindingsAttribute::CaptureType> captures;
    std::unordered_map<std::string, BindingsAttribute::CaptureType>
        childCaptures; // for functions!
    std::map<std::string, SrcInfo> firstSeen;

    bool adding = false;
    ASTNode *root = nullptr;
    FunctionStmt *functionScope = nullptr;
    bool inClass = false;
    // bool isConditional = false;

    std::vector<std::unordered_map<std::string, std::string>> renames = {{}};
    bool tempScope = false;
  };
  std::shared_ptr<Context> ctx = nullptr;

  struct ConditionalBlock {
    Context *ctx;
    ConditionalBlock(Context *ctx, Stmt *s, int id = -1) : ctx(ctx) {
      if (s)
        seqassertn(cast<SuiteStmt>(s), "not a suite");
      ctx->scope.emplace_back(id == -1 ? ctx->cache->blockCount++ : id, s);
    }
    ~ConditionalBlock() {
      seqassertn(!ctx->scope.empty() &&
                     (ctx->scope.back().id == 0 || ctx->scope.size() > 1),
                 "empty scope");
      ctx->scope.pop_back();
    }
  };

public:
  static void apply(Cache *, Stmt *s);
  void transform(Expr *expr) override;
  void transform(Stmt *stmt) override;

  bool visitName(const std::string &name, bool = false, ASTNode * = nullptr,
                 const SrcInfo & = SrcInfo());
  void transformAdding(Expr *e, ASTNode *);
  void transformScope(Expr *);
  void transformScope(Stmt *);

  void visit(IdExpr *) override;
  void visit(StringExpr *) override;
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
  void processChildCaptures();
  void switchToUpdate(ASTNode *binding, const std::string &, bool);

  template <typename Tn, typename... Ts> Tn *N(Ts &&...args) {
    Tn *t = ctx->cache->N<Tn>(std::forward<Ts>(args)...);
    t->setSrcInfo(getSrcInfo());
    return t;
  }
};

} // namespace codon::ast
