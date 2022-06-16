#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

/// Transform print statement.
/// @example
///   `print a, b` -> `print(a, b)`
///   `print a, b,` -> `print(a, b, end=' ')`
void SimplifyVisitor::visit(PrintStmt *stmt) {
  std::vector<CallExpr::Arg> args;
  for (auto &i : stmt->items)
    args.emplace_back(CallExpr::Arg{"", transform(i)});
  if (stmt->isInline)
    args.emplace_back(CallExpr::Arg{"end", N<StringExpr>(" ")});
  resultStmt = N<ExprStmt>(N<CallExpr>(transform(N<IdExpr>("print")), args));
}

/// Transform calls. The real stuff happens during the type checking.
/// Here just perform some sanity checks and transform some special calls
/// (see @c transformSpecialCall for details).
void SimplifyVisitor::visit(CallExpr *expr) {
  auto callee = transform(expr->expr, true);
  if ((resultExpr = transformSpecialCall(callee, expr->args)))
    return;

  std::vector<CallExpr::Arg> args;
  bool foundEllispis = false;
  for (auto &i : expr->args) {
    if (auto ee = i.value->getEllipsis()) {
      args.push_back({i.name, clone(i.value)});
      if (foundEllispis ||
          (!ee->isPipeArg && i.value.get() != expr->args.back().value.get()))
        error("unexpected ellipsis expression");
      foundEllispis = true;
    } else {
      args.push_back({i.name, transform(i.value, true)});
    }
  }
  resultExpr = N<CallExpr>(callee, args);
}

/// Simplify the following special call expressions:
///   `tuple(i for i in tup)`      (tuple generatoris)
///   `std.collections.namedtuple` (sugar for @tuple class)
///   `std.functools.partial`      (sugar for partial calls)
/// Check validity of `type()` call. See below for more details.
ExprPtr SimplifyVisitor::transformSpecialCall(ExprPtr callee,
                                              const std::vector<CallExpr::Arg> &args) {
  if (callee->isId("tuple")) {
    // tuple(i for i in j)
    return transformTupleGenerator(args);
  } else if (callee->isId("type") && !ctx->allowTypeOf) {
    // type(i)
    error("type() not allowed in definitions");
  } else if (callee->isId("std.collections.namedtuple")) {
    // namedtuple('Foo', ['x', 'y'])
    return transformNamedTuple(args);
  } else if (callee->isId("std.functools.partial")) {
    // partial(foo, a=5)
    return transformFunctoolsPartial(args);
  }
  return nullptr;
}

/// Transform `tuple(i for i in tup)` into a GeneratorExpr that will be handled during
/// the type checking.
ExprPtr
SimplifyVisitor::transformTupleGenerator(const std::vector<CallExpr::Arg> &args) {
  GeneratorExpr *g = nullptr;
  // We currently allow only a simple iterations over tuples
  if (args.size() != 1 || !(g = CAST(args[0].value, GeneratorExpr)) ||
      g->kind != GeneratorExpr::Generator || g->loops.size() != 1 ||
      !g->loops[0].conds.empty())
    error("tuple only accepts a simple comprehension over a tuple");

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
  return e;
}

/// Transform named tuples.
/// @example
///   `namedtuple("NT", ["a", ("b", int)])` -> ```@tuple
///                                               class NT[T1]:
///                                                 a: T1
///                                                 b: int```
ExprPtr SimplifyVisitor::transformNamedTuple(const std::vector<CallExpr::Arg> &args) {
  // Ensure that namedtuple call is valid
  if (args.size() != 2 || !args[0].value->getString() || !args[1].value->getList())
    error("invalid namedtuple arguments");

  // Construct the class statement
  std::vector<Param> generics, params;
  int ti = 1;
  for (auto &i : args[1].value->getList()->items) {
    if (auto s = i->getString()) {
      generics.emplace_back(Param{format("T{}", ti), N<IdExpr>("type"), nullptr, true});
      params.emplace_back(
          Param{s->getValue(), N<IdExpr>(format("T{}", ti++)), nullptr});
    } else if (i->getTuple() && i->getTuple()->items.size() == 2 &&
               i->getTuple()->items[0]->getString()) {
      params.emplace_back(Param{i->getTuple()->items[0]->getString()->getValue(),
                                transformType(i->getTuple()->items[1]), nullptr});
    } else {
      error(i, "invalid namedtuple argument");
    }
  }
  for (auto &g : generics)
    params.push_back(g);
  auto name = args[0].value->getString()->getValue();
  prependStmts->push_back(transform(
      N<ClassStmt>(name, params, nullptr, std::vector<ExprPtr>{N<IdExpr>("tuple")})));
  auto i = N<IdExpr>(name);
  return transformType(i);
}

/// Transform partial calls (Python syntax).
/// @example
///   `partial(foo, 1, a=2)` -> `foo(1, a=2, ...)`
ExprPtr
SimplifyVisitor::transformFunctoolsPartial(const std::vector<CallExpr::Arg> &args) {
  if (args.empty())
    error("invalid partial arguments");
  std::vector<CallExpr::Arg> nargs = clone_nop(args);
  nargs.erase(nargs.begin());
  nargs.push_back({"", N<EllipsisExpr>()});
  return transform(N<CallExpr>(clone(args[0].value), nargs));
}

} // namespace codon::ast