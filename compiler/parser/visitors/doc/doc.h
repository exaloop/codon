/*
 * doc.h --- Seq documentation generator.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/ctx.h"
#include "parser/visitors/visitor.h"

namespace seq {
namespace ast {

struct json {
  // values={str -> null} -> string value
  // values={i -> json} -> list (if list=true)
  // values={...} -> dictionary
  unordered_map<string, shared_ptr<json>> values;
  bool list;

  json();
  json(const string &s);
  json(const string &s, const string &v);
  json(const vector<shared_ptr<json>> &vs);
  json(const vector<string> &vs);
  json(const unordered_map<string, string> &vs);
  string toString();
  shared_ptr<json> get(const string &s);
  shared_ptr<json> set(const string &s, const string &value);
  shared_ptr<json> set(const string &s, const shared_ptr<json> &value);
};

struct DocContext;
struct DocShared {
  int itemID;
  shared_ptr<json> j;
  unordered_map<string, shared_ptr<DocContext>> modules;
  string argv0;
  shared_ptr<Cache> cache;
  unordered_map<int, vector<string>> generics;
  DocShared() : itemID(1) {}
};

struct DocContext : public Context<int> {
  shared_ptr<DocShared> shared;
  explicit DocContext(shared_ptr<DocShared> shared)
      : Context<int>(""), shared(move(shared)) {
    stack.push_front(vector<string>());
  }
  shared_ptr<int> find(const string &s) const override;
};

struct DocVisitor : public CallbackASTVisitor<shared_ptr<json>, string> {
  shared_ptr<DocContext> ctx;
  shared_ptr<json> resultExpr;
  string resultStmt;

public:
  explicit DocVisitor(shared_ptr<DocContext> ctx) : ctx(move(ctx)) {}
  static shared_ptr<json> apply(const string &argv0, const vector<string> &files);

  shared_ptr<json> transform(const ExprPtr &e) override;
  string transform(const StmtPtr &e) override;

  void transformModule(StmtPtr stmt);
  shared_ptr<json> jsonify(const seq::SrcInfo &s);
  vector<StmtPtr> flatten(StmtPtr stmt, string *docstr = nullptr, bool deep = true);

public:
  void visit(IntExpr *) override;
  void visit(IdExpr *) override;
  void visit(IndexExpr *) override;
  void visit(FunctionStmt *) override;
  void visit(ClassStmt *) override;
  void visit(AssignStmt *) override;
  void visit(ImportStmt *) override;
};

} // namespace ast
} // namespace seq
