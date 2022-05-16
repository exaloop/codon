#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(CallExpr *expr) {
  // Special calls
  // 1. __ptr__(v)
  if (expr->expr->isId("__ptr__")) {
    if (expr->args.size() == 1 && expr->args[0].value->getId()) {
      auto v = ctx->findDominatingBinding(expr->args[0].value->getId()->value);
      if (v && v->isVar()) {
        resultExpr = N<PtrExpr>(transform(expr->args[0].value));
        return;
      }
    }
    error("__ptr__ only accepts a single argument (variable identifier)");
  }
  // 2. __array__[T](n)
  if (expr->expr->getIndex() && expr->expr->getIndex()->expr->isId("__array__")) {
    if (expr->args.size() != 1)
      error("__array__ only accepts a single argument (size)");
    resultExpr = N<StackAllocExpr>(transformType(expr->expr->getIndex()->index),
                                   transform(expr->args[0].value));
    return;
  }
  // 3. isinstance(v, T)
  if (expr->expr->isId("isinstance")) {
    if (expr->args.size() != 2 || !expr->args[0].name.empty() ||
        !expr->args[1].name.empty())
      error("isinstance only accepts two arguments");
    auto lhs = transform(expr->args[0].value, true);
    ExprPtr type;
    if (expr->args[1].value->isId("Tuple") || expr->args[1].value->isId("tuple") ||
        (lhs->isType() && expr->args[1].value->getNone()) ||
        expr->args[1].value->isId("ByVal") || expr->args[1].value->isId("ByRef"))
      type = expr->args[1].value->clone();
    else
      type = transformType(expr->args[1].value);
    resultExpr = N<CallExpr>(clone(expr->expr), lhs, type);
    return;
  }
  // 4. staticlen(v)
  if (expr->expr->isId("staticlen")) {
    if (expr->args.size() != 1)
      error("staticlen only accepts a single arguments");
    resultExpr = N<CallExpr>(clone(expr->expr), transform(expr->args[0].value));
    return;
  }
  // 5. hasattr(v, "id")
  if (expr->expr->isId("hasattr")) {
    if (expr->args.size() < 2 || !expr->args[0].name.empty() ||
        !expr->args[1].name.empty())
      error("hasattr accepts at least two arguments");
    auto s = transform(expr->args[1].value);
    auto arg = N<CallExpr>(N<IdExpr>("type"), expr->args[0].value);
    std::vector<ExprPtr> args{transformType(arg), transform(s)};
    for (int i = 2; i < expr->args.size(); i++)
      args.push_back(transformType(expr->args[i].value));
    resultExpr = N<CallExpr>(clone(expr->expr), args);
    return;
  }
  // 6. compile_error("msg")
  if (expr->expr->isId("compile_error")) {
    if (expr->args.size() != 1)
      error("compile_error accepts a single argument");
    auto s = transform(expr->args[0].value);
    resultExpr = N<CallExpr>(clone(expr->expr), transform(s));
    return;
  }
  // 7. tuple(i for i in j)
  if (expr->expr->isId("tuple")) {
    GeneratorExpr *g = nullptr;
    if (expr->args.size() != 1 || !(g = CAST(expr->args[0].value, GeneratorExpr)) ||
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
      auto head =
          transform(N<AssignStmt>(clone(g->loops[0].vars), clone(var), nullptr, true));
      ex = N<StmtExpr>(head, transform(ex));
    }
    std::vector<GeneratorBody> body;
    body.push_back({var, transform(g->loops[0].gen), {}});
    resultExpr = N<GeneratorExpr>(GeneratorExpr::Generator, ex, body);
    ctx->popBlock();
    return;
  }
  // 8. type(i)
  if (expr->expr->isId("type")) {
    if (expr->args.size() != 1 || !expr->args[0].name.empty())
      error("type only accepts two arguments");
    if (!ctx->allowTypeOf)
      error("type() not allowed in definitions");
    resultExpr = N<CallExpr>(clone(expr->expr), transform(expr->args[0].value, true));
    resultExpr->markType();
    return;
  }
  // 9. getattr(v, "id")
  if (expr->expr->isId("getattr")) {
    if (expr->args.size() != 2 || !expr->args[0].name.empty() ||
        !expr->args[1].name.empty())
      error("getattr accepts at least two arguments");
    auto s = transform(expr->args[1].value);
    std::vector<ExprPtr> args{transform(expr->args[0].value), transform(s)};
    resultExpr = N<CallExpr>(clone(expr->expr), args);
    return;
  }

  auto e = transform(expr->expr, true);
  // 8. namedtuple
  if (e->isId("std.collections.namedtuple")) {
    if (expr->args.size() != 2 || !expr->args[0].value->getString() ||
        !expr->args[1].value->getList())
      error("invalid namedtuple arguments");
    std::vector<Param> generics, args;
    int ti = 1;
    for (auto &i : expr->args[1].value->getList()->items)
      if (auto s = i->getString()) {
        generics.emplace_back(
            Param{format("T{}", ti), N<IdExpr>("type"), nullptr, true});
        args.emplace_back(
            Param{s->getValue(), N<IdExpr>(format("T{}", ti++)), nullptr});
      } else if (i->getTuple() && i->getTuple()->items.size() == 2 &&
                 i->getTuple()->items[0]->getString()) {
        args.emplace_back(Param{i->getTuple()->items[0]->getString()->getValue(),
                                transformType(i->getTuple()->items[1]), nullptr});
      } else {
        error("invalid namedtuple arguments");
      }
    for (auto &g : generics)
      args.push_back(g);
    auto name = expr->args[0].value->getString()->getValue();
    transform(N<ClassStmt>(name, args, nullptr, Attr({Attr::Tuple})));
    auto i = N<IdExpr>(name);
    resultExpr = transformType(i);
    return;
  }
  // 9. partial
  if (e->isId("std.functools.partial")) {
    if (expr->args.empty())
      error("invalid namedtuple arguments");
    std::vector<CallExpr::Arg> args = clone_nop(expr->args);
    args.erase(args.begin());
    args.push_back({"", N<EllipsisExpr>()});
    resultExpr = transform(N<CallExpr>(clone(expr->args[0].value), args));
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

} // namespace codon::ast