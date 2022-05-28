#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(CallExpr *expr) {
  auto e = transform(expr->expr, true);
  if (auto ret = transformSpecialCall(e, expr->args)) {
    resultExpr = ret;
    return;
  }

  std::vector<CallExpr::Arg> args;
  bool namesStarted = false;
  bool foundEllispis = false;
  for (auto &i : expr->args) {
    if (i.name.empty() && namesStarted &&
        !(CAST(i.value, KeywordStarExpr) || i.value->getEllipsis()))
      error("unnamed argument after a named argument");
    if (!i.name.empty() && (i.value->getStar() || CAST(i.value, KeywordStarExpr)))
      error("named star-expressions not allowed");
    namesStarted |= !i.name.empty();
    if (auto ee = i.value->getEllipsis()) {
      if (foundEllispis ||
          (!ee->isPipeArg && i.value.get() != expr->args.back().value.get()))
        error("unexpected ellipsis expression");
      foundEllispis = true;
      args.push_back({i.name, clone(i.value)});
    } else if (auto es = i.value->getStar())
      args.push_back({i.name, N<StarExpr>(transform(es->what))});
    else if (auto ek = CAST(i.value, KeywordStarExpr))
      args.push_back({i.name, N<KeywordStarExpr>(transform(ek->what))});
    else
      args.push_back({i.name, transform(i.value, true)});
  }
  resultExpr = N<CallExpr>(e, args);
}

ExprPtr SimplifyVisitor::transformSpecialCall(const ExprPtr &callee,
                                              const std::vector<CallExpr::Arg> &args) {
  if (!callee->getId())
    return nullptr;
  auto val = callee->getId()->value;
  if (val == "tuple") { // tuple(i for i in j)
    GeneratorExpr *g = nullptr;
    if (args.size() != 1 || !(g = CAST(args[0].value, GeneratorExpr)) ||
        g->kind != GeneratorExpr::Generator || g->loops.size() != 1 ||
        !g->loops[0].conds.empty())
      error("tuple only accepts a simple comprehension over a tuple");

    ctx->addBlock();
    auto var = clone(g->loops[0].vars);
    auto ex = clone(g->expr);
    if (auto i = var->getId()) {
      ctx->addVar(i->value, ctx->generateCanonicalName(i->value), var->getSrcInfo());
      var = transform(var);
      ex = transform(ex);
    } else {
      std::string varName = ctx->cache->getTemporaryVar("for");
      ctx->addVar(varName, varName, var->getSrcInfo());
      var = N<IdExpr>(varName);
      auto head = transform(N<AssignStmt>(clone(g->loops[0].vars), clone(var)));
      ex = N<StmtExpr>(head, transform(ex));
    }
    std::vector<GeneratorBody> body;
    body.push_back({var, transform(g->loops[0].gen), {}});
    auto e = N<GeneratorExpr>(GeneratorExpr::Generator, ex, body);
    ctx->popBlock();
    return e;
  } else if (val == "type" && !ctx->allowTypeOf) { // type(i)
    error("type() not allowed in definitions");
  } else if (val == "std.collections.namedtuple") { // namedtuple
    if (args.size() != 2 || !args[0].value->getString() || !args[1].value->getList())
      error("invalid namedtuple arguments");
    std::vector<Param> generics, params;
    int ti = 1;
    for (auto &i : args[1].value->getList()->items)
      if (auto s = i->getString()) {
        generics.emplace_back(
            Param{format("T{}", ti), N<IdExpr>("type"), nullptr, true});
        params.emplace_back(
            Param{s->getValue(), N<IdExpr>(format("T{}", ti++)), nullptr});
      } else if (i->getTuple() && i->getTuple()->items.size() == 2 &&
                 i->getTuple()->items[0]->getString()) {
        params.emplace_back(Param{i->getTuple()->items[0]->getString()->getValue(),
                                  transformType(i->getTuple()->items[1]), nullptr});
      } else {
        error("invalid namedtuple arguments");
      }
    for (auto &g : generics)
      params.push_back(g);
    auto name = args[0].value->getString()->getValue();
    prependStmts->push_back(
        transform(N<ClassStmt>(name, params, nullptr, Attr({Attr::Tuple}))));
    auto i = N<IdExpr>(name);
    return transformType(i);
  } else if (val == "std.functools.partial") { // partial
    if (args.empty())
      error("invalid partial arguments");
    std::vector<CallExpr::Arg> nargs = clone_nop(args);
    nargs.erase(nargs.begin());
    nargs.push_back({"", N<EllipsisExpr>()});
    return transform(N<CallExpr>(clone(args[0].value), nargs));
  }
  return nullptr;
}

} // namespace codon::ast