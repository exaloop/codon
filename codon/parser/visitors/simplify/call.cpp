// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

/// Transform print statement.
/// @example
///   `print a, b` -> `print(a, b)`
///   `print a, b,` -> `print(a, b, end=' ')`
void SimplifyVisitor::visit(PrintStmt *stmt) {
  std::vector<CallExpr::Arg> args;
  for (auto &i : stmt->items)
    args.emplace_back("", transform(i));
  if (stmt->isInline)
    args.emplace_back("end", N<StringExpr>(" "));
  resultStmt = N<ExprStmt>(N<CallExpr>(transform(N<IdExpr>("print")), args));
}

/// Transform calls. The real stuff happens during the type checking.
/// Here just perform some sanity checks and transform some special calls
/// (see @c transformSpecialCall for details).
void SimplifyVisitor::visit(CallExpr *expr) {
  transform(expr->expr, true);
  if ((resultExpr = transformSpecialCall(expr->expr, expr->args)))
    return;

  for (auto &i : expr->args) {
    if (auto el = i.value->getEllipsis()) {
      if (&(i) == &(expr->args.back()) && i.name.empty())
        el->mode = EllipsisExpr::PARTIAL;
    }
    transform(i.value, true);
  }
}

/// Simplify the following special call expressions:
///   `tuple(i for i in tup)`      (tuple generators)
///   `std.collections.namedtuple` (sugar for @tuple class)
///   `std.functools.partial`      (sugar for partial calls)
/// Check validity of `type()` call. See below for more details.
ExprPtr SimplifyVisitor::transformSpecialCall(const ExprPtr &callee,
                                              const std::vector<CallExpr::Arg> &args) {
  if (callee->isId("tuple") && args.size() == 1 &&
      CAST(args.front().value, GeneratorExpr)) {
    // tuple(i for i in j)
    return transformTupleGenerator(args);
  } else if (callee->isId("type") && !ctx->allowTypeOf) {
    // type(i)
    E(Error::CALL_NO_TYPE, getSrcInfo());
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
    E(Error::CALL_TUPLE_COMPREHENSION, args[0].value);
  auto var = clone(g->loops[0].vars);
  auto ex = clone(g->expr);

  ctx->enterConditionalBlock();
  ctx->getBase()->loops.push_back({"", ctx->scope.blocks, {}});
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
  ctx->leaveConditionalBlock();
  // Dominate loop variables
  for (auto &var : ctx->getBase()->getLoop()->seenVars)
    ctx->findDominatingBinding(var);
  ctx->getBase()->loops.pop_back();
  return N<GeneratorExpr>(
      GeneratorExpr::Generator, ex,
      std::vector<GeneratorBody>{{var, transform(g->loops[0].gen), {}}});
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
    E(Error::CALL_NAMEDTUPLE, getSrcInfo());

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
      E(Error::CALL_NAMEDTUPLE, i);
    }
  }
  for (auto &g : generics)
    params.push_back(g);
  auto name = args[0].value->getString()->getValue();
  prependStmts->push_back(transform(
      N<ClassStmt>(name, params, nullptr, std::vector<ExprPtr>{N<IdExpr>("tuple")})));
  return transformType(N<IdExpr>(name));
}

/// Transform partial calls (Python syntax).
/// @example
///   `partial(foo, 1, a=2)` -> `foo(1, a=2, ...)`
ExprPtr SimplifyVisitor::transformFunctoolsPartial(std::vector<CallExpr::Arg> args) {
  if (args.empty())
    E(Error::CALL_PARTIAL, getSrcInfo());
  auto name = clone(args[0].value);
  args.erase(args.begin());
  args.emplace_back("", N<EllipsisExpr>(EllipsisExpr::PARTIAL));
  return transform(N<CallExpr>(name, args));
}

} // namespace codon::ast
