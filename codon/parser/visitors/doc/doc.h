// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/ctx.h"
#include "codon/parser/visitors/visitor.h"

namespace codon::ast {

struct json {
  // values={str -> null} -> string value
  // values={i -> json} -> list (if list=true)
  // values={...} -> dictionary
  std::unordered_map<std::string, std::shared_ptr<json>> values;
  bool list;

  json();
  json(const std::string &s);
  json(const std::string &s, const std::string &v);
  json(const std::vector<std::shared_ptr<json>> &vs);
  json(const std::vector<std::string> &vs);
  json(const std::unordered_map<std::string, std::string> &vs);
  std::string toString();
  std::shared_ptr<json> get(const std::string &s);
  std::shared_ptr<json> set(const std::string &s, const std::string &value);
  std::shared_ptr<json> set(const std::string &s, const std::shared_ptr<json> &value);
};

struct DocContext;
struct DocShared {
  int itemID = 1;
  std::shared_ptr<json> j;
  std::unordered_map<std::string, std::shared_ptr<DocContext>> modules;
  std::string argv0;
  Cache *cache = nullptr;
  std::unordered_map<int, std::vector<std::string>> generics;
  DocShared() {}
};

struct DocContext : public Context<int> {
  std::shared_ptr<DocShared> shared;
  explicit DocContext(std::shared_ptr<DocShared> shared)
      : Context<int>(""), shared(std::move(shared)) {}
  std::shared_ptr<int> find(const std::string &s) const override;
};

struct DocVisitor : public CallbackASTVisitor<std::shared_ptr<json>, std::string> {
  std::shared_ptr<DocContext> ctx;
  std::shared_ptr<json> resultExpr;
  std::string resultStmt;

public:
  explicit DocVisitor(std::shared_ptr<DocContext> ctx) : ctx(std::move(ctx)) {}
  static std::shared_ptr<json> apply(const std::string &argv0,
                                     const std::vector<std::string> &files);

  std::shared_ptr<json> transform(const ExprPtr &e) override;
  std::string transform(const StmtPtr &e) override;

  void transformModule(StmtPtr stmt);
  std::shared_ptr<json> jsonify(const codon::SrcInfo &s);
  std::vector<StmtPtr> flatten(StmtPtr stmt, std::string *docstr = nullptr,
                               bool deep = true);

public:
  void visit(IntExpr *) override;
  void visit(IdExpr *) override;
  void visit(IndexExpr *) override;
  void visit(FunctionStmt *) override;
  void visit(ClassStmt *) override;
  void visit(AssignStmt *) override;
  void visit(ImportStmt *) override;
};

} // namespace codon::ast
