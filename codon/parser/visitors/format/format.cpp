// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <vector>

#include "codon/parser/visitors/format/format.h"

using fmt::format;

namespace codon {
namespace ast {

FormatVisitor::FormatVisitor(bool html, Cache *cache)
    : renderType(false), renderHTML(html), indent(0), cache(cache) {
  if (renderHTML) {
    header = "<html><head><link rel=stylesheet href=code.css/></head>\n<body>";
    header += "<div class=code>\n";
    footer = "\n</div></body></html>";
    nl = "<hr>";
    typeStart = "<type>";
    typeEnd = "</type>";
    nodeStart = "<node>";
    nodeEnd = "</node>";
    exprStart = "<expr>";
    exprEnd = "</expr>";
    commentStart = "<b>";
    commentEnd = "</b>";
    keywordStart = "<b class=comment>";
    keywordEnd = "</b>";
    space = "&nbsp";
    renderType = true;
  } else {
    space = " ";
  }
}

std::string FormatVisitor::transform(const ExprPtr &expr) {
  return transform(expr.get());
}

std::string FormatVisitor::transform(const Expr *expr) {
  FormatVisitor v(renderHTML, cache);
  if (expr)
    const_cast<Expr *>(expr)->accept(v);
  return v.result;
}

std::string FormatVisitor::transform(const StmtPtr &stmt) {
  return transform(stmt.get(), 0);
}

std::string FormatVisitor::transform(Stmt *stmt, int indent) {
  FormatVisitor v(renderHTML, cache);
  v.indent = this->indent + indent;
  if (stmt)
    stmt->accept(v);
  return (stmt && stmt->getSuite() ? "" : pad(indent)) + v.result + newline();
}

std::string FormatVisitor::pad(int indent) const {
  std::string s;
  for (int i = 0; i < (this->indent + indent) * 2; i++)
    s += space;
  return s;
}

std::string FormatVisitor::newline() const { return nl + "\n"; }

std::string FormatVisitor::keyword(const std::string &s) const {
  return fmt::format("{}{}{}", keywordStart, s, keywordEnd);
}

/*************************************************************************************/

void FormatVisitor::visit(NoneExpr *expr) { result = renderExpr(expr, "None"); }

void FormatVisitor::visit(BoolExpr *expr) {
  result = renderExpr(expr, "{}", expr->value ? "True" : "False");
}

void FormatVisitor::visit(IntExpr *expr) {
  result = renderExpr(expr, "{}{}", expr->value, expr->suffix);
}

void FormatVisitor::visit(FloatExpr *expr) {
  result = renderExpr(expr, "{}{}", expr->value, expr->suffix);
}

void FormatVisitor::visit(StringExpr *expr) {
  result = renderExpr(expr, "\"{}\"", escape(expr->getValue()));
}

void FormatVisitor::visit(IdExpr *expr) {
  result = renderExpr(expr, "{}", expr->value);
}

void FormatVisitor::visit(StarExpr *expr) {
  result = renderExpr(expr, "*{}", transform(expr->what));
}

void FormatVisitor::visit(KeywordStarExpr *expr) {
  result = renderExpr(expr, "**{}", transform(expr->what));
}

void FormatVisitor::visit(TupleExpr *expr) {
  result = renderExpr(expr, "({})", transform(expr->items));
}

void FormatVisitor::visit(ListExpr *expr) {
  result = renderExpr(expr, "[{}]", transform(expr->items));
}

void FormatVisitor::visit(InstantiateExpr *expr) {
  result = renderExpr(expr, "{}[{}]", transform(expr->typeExpr),
                      transform(expr->typeParams));
}

void FormatVisitor::visit(SetExpr *expr) {
  result = renderExpr(expr, "{{{}}}", transform(expr->items));
}

void FormatVisitor::visit(DictExpr *expr) {
  std::vector<std::string> s;
  for (auto &i : expr->items)
    s.push_back(fmt::format("{}: {}", transform(i->getTuple()->items[0]),
                            transform(i->getTuple()->items[1])));
  result = renderExpr(expr, "{{{}}}", join(s, ", "));
}

void FormatVisitor::visit(GeneratorExpr *expr) {
  std::string s;
  for (auto &i : expr->loops) {
    std::string cond;
    for (auto &k : i.conds)
      cond += fmt::format(" if {}", transform(k));
    s += fmt::format("for {} in {}{}", i.vars->toString(), i.gen->toString(), cond);
  }
  if (expr->kind == GeneratorExpr::ListGenerator)
    result = renderExpr(expr, "[{} {}]", transform(expr->expr), s);
  else if (expr->kind == GeneratorExpr::SetGenerator)
    result = renderExpr(expr, "{{{} {}}}", transform(expr->expr), s);
  else
    result = renderExpr(expr, "({} {})", transform(expr->expr), s);
}

void FormatVisitor::visit(DictGeneratorExpr *expr) {
  std::string s;
  for (auto &i : expr->loops) {
    std::string cond;
    for (auto &k : i.conds)
      cond += fmt::format(" if {}", transform(k));

    s += fmt::format("for {} in {}{}", i.vars->toString(), i.gen->toString(), cond);
  }
  result =
      renderExpr(expr, "{{{}: {} {}}}", transform(expr->key), transform(expr->expr), s);
}

void FormatVisitor::visit(IfExpr *expr) {
  result = renderExpr(expr, "{} if {} else {}", transform(expr->ifexpr),
                      transform(expr->cond), transform(expr->elsexpr));
}

void FormatVisitor::visit(UnaryExpr *expr) {
  result = renderExpr(expr, "{}{}", expr->op, transform(expr->expr));
}

void FormatVisitor::visit(BinaryExpr *expr) {
  result = renderExpr(expr, "({} {} {})", transform(expr->lexpr), expr->op,
                      transform(expr->rexpr));
}

void FormatVisitor::visit(PipeExpr *expr) {
  std::vector<std::string> items;
  for (auto &l : expr->items) {
    if (!items.size())
      items.push_back(transform(l.expr));
    else
      items.push_back(l.op + " " + transform(l.expr));
  }
  result = renderExpr(expr, "({})", join(items, " "));
}

void FormatVisitor::visit(IndexExpr *expr) {
  result = renderExpr(expr, "{}[{}]", transform(expr->expr), transform(expr->index));
}

void FormatVisitor::visit(CallExpr *expr) {
  std::vector<std::string> args;
  for (auto &i : expr->args) {
    if (i.name == "")
      args.push_back(transform(i.value));
    else
      args.push_back(fmt::format("{}: {}", i.name, transform(i.value)));
  }
  result = renderExpr(expr, "{}({})", transform(expr->expr), join(args, ", "));
}

void FormatVisitor::visit(DotExpr *expr) {
  result = renderExpr(expr, "{} . {}", transform(expr->expr), expr->member);
}

void FormatVisitor::visit(SliceExpr *expr) {
  std::string s;
  if (expr->start)
    s += transform(expr->start);
  s += ":";
  if (expr->stop)
    s += transform(expr->stop);
  s += ":";
  if (expr->step)
    s += transform(expr->step);
  result = renderExpr(expr, "{}", s);
}

void FormatVisitor::visit(EllipsisExpr *expr) { result = renderExpr(expr, "..."); }

void FormatVisitor::visit(LambdaExpr *expr) {
  result = renderExpr(expr, "{} {}: {}", keyword("lambda"), join(expr->vars, ", "),
                      transform(expr->expr));
}

void FormatVisitor::visit(YieldExpr *expr) { result = renderExpr(expr, "(yield)"); }

void FormatVisitor::visit(StmtExpr *expr) {
  std::string s;
  for (int i = 0; i < expr->stmts.size(); i++)
    s += format("{}{}", pad(2), transform(expr->stmts[i].get(), 2));
  result = renderExpr(expr, "({}{}{}{}{})", newline(), s, newline(), pad(2),
                      transform(expr->expr));
}

void FormatVisitor::visit(AssignExpr *expr) {
  result = renderExpr(expr, "({} := {})", transform(expr->var), transform(expr->expr));
}

void FormatVisitor::visit(SuiteStmt *stmt) {
  for (int i = 0; i < stmt->stmts.size(); i++)
    result += transform(stmt->stmts[i]);
}

void FormatVisitor::visit(BreakStmt *stmt) { result = keyword("break"); }

void FormatVisitor::visit(ContinueStmt *stmt) { result = keyword("continue"); }

void FormatVisitor::visit(ExprStmt *stmt) { result = transform(stmt->expr); }

void FormatVisitor::visit(AssignStmt *stmt) {
  if (stmt->type) {
    result = fmt::format("{}: {} = {}", transform(stmt->lhs), transform(stmt->type),
                         transform(stmt->rhs));
  } else {
    result = fmt::format("{} = {}", transform(stmt->lhs), transform(stmt->rhs));
  }
}

void FormatVisitor::visit(AssignMemberStmt *stmt) {
  result = fmt::format("{}.{} = {}", transform(stmt->lhs), stmt->member,
                       transform(stmt->rhs));
}

void FormatVisitor::visit(DelStmt *stmt) {
  result = fmt::format("{} {}", keyword("del"), transform(stmt->expr));
}

void FormatVisitor::visit(PrintStmt *stmt) {
  result = fmt::format("{} {}", keyword("print"), transform(stmt->items));
}

void FormatVisitor::visit(ReturnStmt *stmt) {
  result = fmt::format("{}{}", keyword("return"),
                       stmt->expr ? " " + transform(stmt->expr) : "");
}

void FormatVisitor::visit(YieldStmt *stmt) {
  result = fmt::format("{}{}", keyword("yield"),
                       stmt->expr ? " " + transform(stmt->expr) : "");
}

void FormatVisitor::visit(AssertStmt *stmt) {
  result = fmt::format("{} {}", keyword("assert"), transform(stmt->expr));
}

void FormatVisitor::visit(WhileStmt *stmt) {
  result = fmt::format("{} {}:{}{}", keyword("while"), transform(stmt->cond), newline(),
                       transform(stmt->suite.get(), 1));
}

void FormatVisitor::visit(ForStmt *stmt) {
  result = fmt::format("{} {} {} {}:{}{}", keyword("for"), transform(stmt->var),
                       keyword("in"), transform(stmt->iter), newline(),
                       transform(stmt->suite.get(), 1));
}

void FormatVisitor::visit(IfStmt *stmt) {
  result = fmt::format("{} {}:{}{}{}", keyword("if"), transform(stmt->cond), newline(),
                       transform(stmt->ifSuite.get(), 1),
                       stmt->elseSuite ? format("{}:{}{}", keyword("else"), newline(),
                                                transform(stmt->elseSuite.get(), 1))
                                       : "");
}

void FormatVisitor::visit(MatchStmt *stmt) {
  std::string s;
  for (auto &c : stmt->cases)
    s += fmt::format("{}{}{}{}:{}{}", pad(1), keyword("case"), transform(c.pattern),
                     c.guard ? " " + (keyword("case") + " " + transform(c.guard)) : "",
                     newline(), transform(c.suite.get(), 2));
  result =
      fmt::format("{} {}:{}{}", keyword("match"), transform(stmt->what), newline(), s);
}

void FormatVisitor::visit(ImportStmt *stmt) {
  auto as = stmt->as.empty() ? "" : fmt::format(" {} {} ", keyword("as"), stmt->as);
  if (!stmt->what)
    result += fmt::format("{} {}{}", keyword("import"), transform(stmt->from), as);
  else
    result += fmt::format("{} {} {} {}{}", keyword("from"), transform(stmt->from),
                          keyword("import"), transform(stmt->what), as);
}

void FormatVisitor::visit(TryStmt *stmt) {
  std::vector<std::string> catches;
  for (auto &c : stmt->catches) {
    catches.push_back(
        fmt::format("{} {}{}:{}{}", keyword("catch"), transform(c.exc),
                    c.var == "" ? "" : fmt::format("{} {}", keyword("as"), c.var),
                    newline(), transform(c.suite.get(), 1)));
  }
  result =
      fmt::format("{}:{}{}{}{}", keyword("try"), newline(),
                  transform(stmt->suite.get(), 1), fmt::join(catches, ""),
                  stmt->finally ? fmt::format("{}:{}{}", keyword("finally"), newline(),
                                              transform(stmt->finally.get(), 1))
                                : "");
}

void FormatVisitor::visit(GlobalStmt *stmt) {
  result = fmt::format("{} {}", keyword("global"), stmt->var);
}

void FormatVisitor::visit(ThrowStmt *stmt) {
  result = fmt::format("{} {}", keyword("raise"), transform(stmt->expr));
}

void FormatVisitor::visit(FunctionStmt *fstmt) {
  if (cache) {
    if (in(cache->functions, fstmt->name)) {
      if (!cache->functions[fstmt->name].realizations.empty()) {
        for (auto &real : cache->functions[fstmt->name].realizations) {
          if (real.first != fstmt->name) {
            result += transform(real.second->ast.get(), 0);
          }
        }
        return;
      }
      fstmt = cache->functions[fstmt->name].ast.get();
    }
  }
  //  if (cache && cache->functions.find(fstmt->name) != cache->realizationAsts.end())
  //  {
  //    fstmt = (const FunctionStmt *)(cache->realizationAsts[fstmt->name].get());
  //  } else if (cache && cache->functions[fstmt->name].realizations.size()) {
  //    for (auto &real : cache->functions[fstmt->name].realizations)
  //      result += transform(real.second.ast);
  //    return;
  //  } else if (cache) {
  //    fstmt = cache->functions[fstmt->name].ast.get();
  //  }

  std::vector<std::string> attrs;
  for (auto &a : fstmt->decorators)
    attrs.push_back(fmt::format("@{}", transform(a)));
  if (!fstmt->attributes.module.empty())
    attrs.push_back(fmt::format("@module:{}", fstmt->attributes.parentClass));
  if (!fstmt->attributes.parentClass.empty())
    attrs.push_back(fmt::format("@parent:{}", fstmt->attributes.parentClass));
  std::vector<std::string> args;
  for (auto &a : fstmt->args)
    args.push_back(fmt::format(
        "{}{}{}", a.name, a.type ? fmt::format(": {}", transform(a.type)) : "",
        a.defaultValue ? fmt::format(" = {}", transform(a.defaultValue)) : ""));
  auto body = transform(fstmt->suite.get(), 1);
  auto name = fmt::format("{}{}{}", typeStart, fstmt->name, typeEnd);
  name = fmt::format("{}{}{}", exprStart, name, exprEnd);
  result += fmt::format(
      "{}{} {}({}){}:{}{}",
      attrs.size() ? join(attrs, newline() + pad()) + newline() + pad() : "",
      keyword("def"), name, fmt::join(args, ", "),
      fstmt->ret ? fmt::format(" -> {}", transform(fstmt->ret)) : "", newline(),
      body.empty() ? fmt::format("{}", keyword("pass")) : body);
}

void FormatVisitor::visit(ClassStmt *stmt) {
  std::vector<std::string> attrs;

  if (!stmt->attributes.has(Attr::Extend))
    attrs.push_back("@extend");
  if (!stmt->attributes.has(Attr::Tuple))
    attrs.push_back("@tuple");
  std::vector<std::string> args;
  std::string key = stmt->isRecord() ? "type" : "class";
  for (auto &a : stmt->args)
    args.push_back(fmt::format("{}: {}", a.name, transform(a.type)));
  result = fmt::format("{}{} {}({})",
                       attrs.size() ? join(attrs, newline() + pad()) + newline() + pad()
                                    : "",
                       keyword(key), stmt->name, fmt::join(args, ", "));
  if (stmt->suite)
    result += fmt::format(":{}{}", newline(), transform(stmt->suite.get(), 1));
}

void FormatVisitor::visit(YieldFromStmt *stmt) {
  result = fmt::format("{} {}", keyword("yield from"), transform(stmt->expr));
}

void FormatVisitor::visit(WithStmt *stmt) {}

} // namespace ast
} // namespace codon
