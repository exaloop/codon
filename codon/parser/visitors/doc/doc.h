// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "llvm/Support/JSON.h"

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/ctx.h"
#include "codon/parser/visitors/visitor.h"

namespace codon::ast {

struct DocContext;
struct DocShared {
  int itemID = 1;
  llvm::json::Object symbols;
  std::unordered_map<std::string, std::shared_ptr<DocContext>> modules;
  std::unordered_map<int, std::shared_ptr<DocContext>> scopes;
  std::unordered_map<const Stmt *, std::unordered_map<std::string, int>> declarations;
  std::unordered_set<std::string> requested;
  std::unordered_map<std::string, int> overloads;
  std::string root;
  std::string argv0;
  Cache *cache = nullptr;
  std::unordered_map<int, std::vector<std::string>> generics;
  DocShared() {}
};

struct DocContext : public Context<int> {
  DocShared *shared;
  DocContext *parent = nullptr;
  std::string module;
  std::string owner;
  int moduleID = 0;
  std::vector<std::string> extraChildren;
  explicit DocContext(DocShared *shared) : Context<int>(""), shared(shared) {}
  std::shared_ptr<int> find(const std::string &s) const override;
};

struct DocVisitor : public CallbackASTVisitor<llvm::json::Value, std::string> {
  std::shared_ptr<DocContext> ctx;
  llvm::json::Value resultExpr = nullptr;
  std::string resultStmt;

public:
  explicit DocVisitor(std::shared_ptr<DocContext> ctx) : ctx(std::move(ctx)) {}
  static llvm::json::Object apply(const std::string &argv0,
                                  const std::vector<std::string> &files,
                                  const std::string &root = "");

  llvm::json::Value transform(Expr *e) override;
  std::string transform(Stmt *e) override;

  void transformModule(Stmt *stmt, Stmt *prelude = nullptr);
  std::shared_ptr<DocContext> loadModule(const std::string &path);
  int declare(Stmt *stmt, const std::string &kind, const std::string &name,
              bool bind = true);
  void publish(int id, llvm::json::Object value);
  std::shared_ptr<int> resolve(Expr *expr) const;
  std::string alias(Stmt *stmt, const std::string &name, int target);
  static llvm::json::Object jsonify(const codon::SrcInfo &s);
  static std::vector<Stmt *> flatten(Stmt *stmt, std::string *docstr = nullptr,
                                     bool deep = true);

public:
  void visit(IntExpr *) override;
  void visit(IdExpr *) override;
  void visit(DotExpr *) override;
  void visit(IndexExpr *) override;
  void visit(FunctionStmt *) override;
  void visit(ClassStmt *) override;
  void visit(AssignStmt *) override;
  void visit(ImportStmt *) override;
};

} // namespace codon::ast
