/**
 * format.h
 * Format AST walker.
 *
 * Generates HTML representation of a given AST node.
 * Useful for debugging types.
 */

#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "parser/ast.h"
#include "parser/cache.h"
#include "parser/common.h"
#include "parser/visitors/visitor.h"

namespace seq {
namespace ast {

class FormatVisitor : public CallbackASTVisitor<string, string> {
  string result;
  string space;
  bool renderType, renderHTML;
  int indent;

  string header, footer, nl;
  string typeStart, typeEnd;
  string nodeStart, nodeEnd;
  string exprStart, exprEnd;
  string commentStart, commentEnd;
  string keywordStart, keywordEnd;

  shared_ptr<Cache> cache;

private:
  template <typename T, typename... Ts> string renderExpr(T &&t, Ts &&...args) {
    string s;
    // if (renderType)
    // s += fmt::format("{}{}{}", typeStart,
    //  t->getType() ? t->getType()->toString() : "-", typeEnd);
    return fmt::format("{}{}{}{}{}{}", exprStart, s, nodeStart, fmt::format(args...),
                       nodeEnd, exprEnd);
  }
  template <typename... Ts> string renderComment(Ts &&...args) {
    return fmt::format("{}{}{}", commentStart, fmt::format(args...), commentEnd);
  }
  string pad(int indent = 0) const;
  string newline() const;
  string keyword(const string &s) const;

public:
  FormatVisitor(bool html, shared_ptr<Cache> cache = nullptr);
  string transform(const ExprPtr &e) override;
  string transform(const Expr *expr);
  string transform(const StmtPtr &stmt) override;
  string transform(Stmt *stmt, int indent);

  template <typename T>
  static string apply(const T &stmt, shared_ptr<Cache> cache = nullptr,
                      bool html = false, bool init = false) {
    auto t = FormatVisitor(html, cache);
    return fmt::format("{}{}{}", t.header, t.transform(stmt), t.footer);
  }

  void defaultVisit(Expr *e) override { error("cannot format {}", *e); }
  void defaultVisit(Stmt *e) override { error("cannot format {}", *e); }

public:
  void visit(NoneExpr *) override;
  void visit(BoolExpr *) override;
  void visit(IntExpr *) override;
  void visit(FloatExpr *) override;
  void visit(StringExpr *) override;
  void visit(IdExpr *) override;
  void visit(StarExpr *) override;
  void visit(KeywordStarExpr *) override;
  void visit(TupleExpr *) override;
  void visit(ListExpr *) override;
  void visit(SetExpr *) override;
  void visit(DictExpr *) override;
  void visit(GeneratorExpr *) override;
  void visit(DictGeneratorExpr *) override;
  void visit(InstantiateExpr *expr) override;
  void visit(StackAllocExpr *expr) override;
  void visit(IfExpr *) override;
  void visit(UnaryExpr *) override;
  void visit(BinaryExpr *) override;
  void visit(PipeExpr *) override;
  void visit(IndexExpr *) override;
  void visit(CallExpr *) override;
  void visit(DotExpr *) override;
  void visit(SliceExpr *) override;
  void visit(EllipsisExpr *) override;
  void visit(PtrExpr *) override;
  void visit(LambdaExpr *) override;
  void visit(YieldExpr *) override;
  void visit(StmtExpr *expr) override;
  void visit(AssignExpr *expr) override;

  void visit(SuiteStmt *) override;
  void visit(BreakStmt *) override;
  void visit(UpdateStmt *) override;
  void visit(ContinueStmt *) override;
  void visit(ExprStmt *) override;
  void visit(AssignStmt *) override;
  void visit(AssignMemberStmt *) override;
  void visit(DelStmt *) override;
  void visit(PrintStmt *) override;
  void visit(ReturnStmt *) override;
  void visit(YieldStmt *) override;
  void visit(AssertStmt *) override;
  void visit(WhileStmt *) override;
  void visit(ForStmt *) override;
  void visit(IfStmt *) override;
  void visit(MatchStmt *) override;
  void visit(ImportStmt *) override;
  void visit(TryStmt *) override;
  void visit(GlobalStmt *) override;
  void visit(ThrowStmt *) override;
  void visit(FunctionStmt *) override;
  void visit(ClassStmt *) override;
  void visit(YieldFromStmt *) override;
  void visit(WithStmt *) override;

public:
  friend std::ostream &operator<<(std::ostream &out, const FormatVisitor &c) {
    return out << c.result;
  }

  using CallbackASTVisitor<string, string>::transform;
  template <typename T> string transform(const vector<T> &ts) {
    vector<string> r;
    for (auto &e : ts)
      r.push_back(transform(e));
    return fmt::format("{}", fmt::join(r, ", "));
  }
};

} // namespace ast
} // namespace seq
