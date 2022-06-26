#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

void TypecheckVisitor::visit(TryStmt *stmt) {
  std::vector<TryStmt::Catch> catches;
  ctx->blockLevel++;
  stmt->suite = transform(stmt->suite);
  ctx->blockLevel--;
  stmt->done = stmt->suite->done;
  for (auto &c : stmt->catches) {
    c.exc = transformType(c.exc);
    if (!c.var.empty()) {
      bool changed = in(ctx->cache->replacements, c.var);
      while (auto s = in(ctx->cache->replacements, c.var))
        c.var = s->first;
      if (changed) {
        auto u =
            N<AssignStmt>(N<IdExpr>(format("{}.__used__", c.var)), N<BoolExpr>(true));
        u->setUpdate();
        c.suite = N<SuiteStmt>(u, c.suite);
      }

      if (c.ownVar) {
        ctx->add(TypecheckItem::Var, c.var, c.exc->getType());
      } else {
        auto val = ctx->find(c.var);
        seqassert(val, "'{}' not found", c.var);
        unify(val->type, c.exc->getType());
      }
    }
    ctx->blockLevel++;
    c.suite = transform(c.suite);
    ctx->blockLevel--;
    stmt->done &= (c.exc ? c.exc->done : true) && c.suite->done;
  }
  if (stmt->finally) {
    ctx->blockLevel++;
    stmt->finally = transform(stmt->finally);
    ctx->blockLevel--;
    stmt->done &= stmt->finally->done;
  }
}

void TypecheckVisitor::visit(ThrowStmt *stmt) {
  stmt->expr = transform(stmt->expr);
  auto tc = stmt->expr->type->getClass();
  if (!stmt->transformed && tc) {
    auto &f = ctx->cache->classes[tc->name].fields;
    if (f.empty() || !f[0].type->getClass() ||
        f[0].type->getClass()->name != TYPE_EXCHEADER)
      error("cannot throw non-exception (first object member must be of type "
            "ExcHeader)");
    auto var = ctx->cache->getTemporaryVar("exc");
    std::vector<CallExpr::Arg> args;
    args.emplace_back(CallExpr::Arg{"", N<StringExpr>(tc->name)});
    args.emplace_back(CallExpr::Arg{
        "", N<DotExpr>(N<DotExpr>(var, ctx->cache->classes[tc->name].fields[0].name),
                       "msg")});
    args.emplace_back(CallExpr::Arg{"", N<StringExpr>(ctx->bases.back().name)});
    args.emplace_back(CallExpr::Arg{"", N<StringExpr>(stmt->getSrcInfo().file)});
    args.emplace_back(CallExpr::Arg{"", N<IntExpr>(stmt->getSrcInfo().line)});
    args.emplace_back(CallExpr::Arg{"", N<IntExpr>(stmt->getSrcInfo().col)});
    resultStmt = transform(
        N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(var), stmt->expr),
                     N<AssignMemberStmt>(N<IdExpr>(var),
                                         ctx->cache->classes[tc->name].fields[0].name,
                                         N<CallExpr>(N<IdExpr>(TYPE_EXCHEADER), args)),
                     N<ThrowStmt>(N<IdExpr>(var), true)));
  } else {
    stmt->done = stmt->expr->done;
  }
}

}