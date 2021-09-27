/*
 * simplify_expr.cpp --- AST expression simplifications.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#include <deque>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "parser/ast.h"
#include "parser/cache.h"
#include "parser/common.h"
#include "parser/peg/peg.h"
#include "parser/visitors/simplify/simplify.h"

using fmt::format;

namespace seq {
namespace ast {

ExprPtr SimplifyVisitor::transform(const ExprPtr &expr) {
  return transform(expr, false, true);
}

ExprPtr SimplifyVisitor::transform(const ExprPtr &expr, bool allowTypes,
                                   bool allowAssign) {
  if (!expr)
    return nullptr;
  SimplifyVisitor v(ctx, preamble);
  v.setSrcInfo(expr->getSrcInfo());
  auto oldAssign = ctx->canAssign;
  ctx->canAssign = allowAssign;
  const_cast<Expr *>(expr.get())->accept(v);
  ctx->canAssign = oldAssign;
  if (!allowTypes && v.resultExpr && v.resultExpr->isType())
    error("unexpected type expression");
  return v.resultExpr;
}

ExprPtr SimplifyVisitor::transformType(const ExprPtr &expr, bool allowTypeOf) {
  auto oldTypeOf = ctx->allowTypeOf;
  ctx->allowTypeOf = allowTypeOf;
  auto e = transform(expr, true);
  ctx->allowTypeOf = oldTypeOf;
  if (e && !e->isType())
    error("expected type expression");
  return e;
}

void SimplifyVisitor::defaultVisit(Expr *e) { resultExpr = e->clone(); }

/**************************************************************************************/

void SimplifyVisitor::visit(NoneExpr *expr) {
  resultExpr = transform(N<CallExpr>(N<IdExpr>(TYPE_OPTIONAL)));
}

void SimplifyVisitor::visit(IntExpr *expr) {
  resultExpr = transformInt(expr->value, expr->suffix);
}

void SimplifyVisitor::visit(FloatExpr *expr) {
  resultExpr = transformFloat(expr->value, expr->suffix);
}

void SimplifyVisitor::visit(StringExpr *expr) {
  vector<ExprPtr> exprs;
  string concat;
  int realStrings = 0;
  for (auto &p : expr->strings) {
    if (p.second == "f" || p.second == "F") {
      /// F-strings
      exprs.push_back(transformFString(p.first));
    } else if (!p.second.empty()) {
      /// Custom-prefix strings
      exprs.push_back(
          transform(N<CallExpr>(N<DotExpr>("str", format("__prefix_{}__", p.second)),
                                N<StringExpr>(p.first), N<IntExpr>(p.first.size()))));
    } else {
      exprs.push_back(N<StringExpr>(p.first));
      concat += p.first;
      realStrings++;
    }
  }
  if (realStrings == expr->strings.size())
    resultExpr = N<StringExpr>(concat);
  else if (exprs.size() == 1)
    resultExpr = exprs[0];
  else
    resultExpr = transform(N<CallExpr>(N<DotExpr>("str", "cat"), exprs));
}

void SimplifyVisitor::visit(IdExpr *expr) {
  if (ctx->substitutions) {
    auto it = ctx->substitutions->find(expr->value);
    if (it != ctx->substitutions->end()) {
      resultExpr = transform(it->second, true);
      return;
    }
  }

  if (in(set<string>{"type", "TypeVar", "Callable"}, expr->value)) {
    resultExpr = N<IdExpr>(expr->value == "TypeVar" ? "type" : expr->value);
    resultExpr->markType();
    return;
  }
  auto val = ctx->find(expr->value);
  if (!val)
    error("identifier '{}' not found", expr->value);
  auto canonicalName = val->canonicalName;

  // If we are accessing an outer non-global variable, raise an error unless
  // we are capturing variables (in that case capture it).
  bool captured = false;
  auto newName = canonicalName;
  if (val->isVar()) {
    if (ctx->getBase() != val->getBase() && !val->isGlobal()) {
      if (!ctx->captures.empty()) {
        captured = true;
        if (!in(ctx->captures.back(), canonicalName)) {
          ctx->captures.back()[canonicalName] = newName =
              ctx->generateCanonicalName(canonicalName);
          ctx->cache->reverseIdentifierLookup[newName] = newName;
        }
        newName = ctx->captures.back()[canonicalName];
      } else {
        error("cannot access non-global variable '{}'",
              ctx->cache->reverseIdentifierLookup[expr->value]);
      }
    }
  }

  // Replace the variable with its canonical name. Do not canonize captured
  // variables (they will be later passed as argument names).
  resultExpr = N<IdExpr>(newName);
  // Flag the expression as a type expression if it points to a class name or a generic.
  if (val->isType())
    resultExpr->markType();

  // The only variables coming from the enclosing base must be class generics.
  seqassert(!val->isFunc() || val->getBase().empty(), "{} has invalid base ({})",
            expr->value, val->getBase());
  if (!val->getBase().empty() && ctx->getBase() != val->getBase()) {
    // Assumption: only 2 bases are available (class -> function)
    if (ctx->bases.size() == 2 && ctx->bases[0].isType() &&
        ctx->bases[0].name == val->getBase()) {
      ctx->bases.back().attributes |= FLAG_METHOD;
      return;
    }
  }
  // If that is not the case, we are probably having a class accessing its enclosing
  // function variable (generic or other identifier). We do not like that!
  if (!captured && ctx->getBase() != val->getBase() && !val->getBase().empty())
    error("identifier '{}' not found (cannot access outer function identifiers)",
          expr->value);
}

void SimplifyVisitor::visit(StarExpr *expr) {
  error("cannot use star-expression here");
}

void SimplifyVisitor::visit(TupleExpr *expr) {
  vector<ExprPtr> items;
  for (auto &i : expr->items) {
    if (auto es = i->getStar())
      items.emplace_back(N<StarExpr>(transform(es->what)));
    else
      items.emplace_back(transform(i));
  }
  resultExpr = N<TupleExpr>(items);
}

void SimplifyVisitor::visit(ListExpr *expr) {
  vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("list"));
  stmts.push_back(transform(N<AssignStmt>(
      clone(var),
      N<CallExpr>(N<IdExpr>("List"),
                  !expr->items.empty() ? N<IntExpr>(expr->items.size()) : nullptr),
      nullptr, true)));
  for (const auto &it : expr->items) {
    if (auto star = it->getStar()) {
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("it"));
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), star->what->clone(),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "append"), clone(forVar))))));
    } else {
      stmts.push_back(transform(
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "append"), clone(it)))));
    }
  }
  resultExpr = N<StmtExpr>(stmts, transform(var));
}

void SimplifyVisitor::visit(SetExpr *expr) {
  vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("set"));
  stmts.push_back(transform(
      N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Set")), nullptr, true)));
  for (auto &it : expr->items)
    if (auto star = it->getStar()) {
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("it"));
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), star->what->clone(),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "add"), clone(forVar))))));
    } else {
      stmts.push_back(transform(
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "add"), clone(it)))));
    }
  resultExpr = N<StmtExpr>(stmts, transform(var));
}

void SimplifyVisitor::visit(DictExpr *expr) {
  vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("dict"));
  stmts.push_back(transform(
      N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")), nullptr, true)));
  for (auto &it : expr->items)
    if (auto star = CAST(it.value, KeywordStarExpr)) {
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("it"));
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), N<CallExpr>(N<DotExpr>(star->what->clone(), "items")),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(0)),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(1)))))));
    } else {
      stmts.push_back(transform(N<ExprStmt>(N<CallExpr>(
          N<DotExpr>(clone(var), "__setitem__"), clone(it.key), clone(it.value)))));
    }
  resultExpr = N<StmtExpr>(stmts, transform(var));
}

void SimplifyVisitor::visit(GeneratorExpr *expr) {
  SuiteStmt *prev;
  vector<StmtPtr> stmts;

  auto loops = clone_nop(expr->loops);
  // List comprehension optimization: pass iter.__len__() if we only have a single for
  // loop without any conditions.
  string optimizeVar;
  if (expr->kind == GeneratorExpr::ListGenerator && loops.size() == 1 &&
      loops[0].conds.empty()) {
    optimizeVar = ctx->cache->getTemporaryVar("iter");
    stmts.push_back(
        transform(N<AssignStmt>(N<IdExpr>(optimizeVar), loops[0].gen, nullptr, true)));
    loops[0].gen = N<IdExpr>(optimizeVar);
  }

  auto suite = transformGeneratorBody(loops, prev);
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  if (expr->kind == GeneratorExpr::ListGenerator) {
    vector<ExprPtr> args;
    // Use special List.__init__(bool, T) constructor.
    if (!optimizeVar.empty())
      args = {N<BoolExpr>(true), N<IdExpr>(optimizeVar)};
    stmts.push_back(transform(N<AssignStmt>(
        clone(var), N<CallExpr>(N<IdExpr>("List"), args), nullptr, true)));
    prev->stmts.push_back(
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "append"), clone(expr->expr))));
    stmts.push_back(transform(suite));
    resultExpr = N<StmtExpr>(stmts, transform(var));
  } else if (expr->kind == GeneratorExpr::SetGenerator) {
    stmts.push_back(transform(
        N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Set")), nullptr, true)));
    prev->stmts.push_back(
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "add"), clone(expr->expr))));
    stmts.push_back(transform(suite));
    resultExpr = N<StmtExpr>(stmts, transform(var));
  } else {
    prev->stmts.push_back(N<YieldStmt>(clone(expr->expr)));
    stmts.push_back(suite);
    resultExpr = N<CallExpr>(N<DotExpr>(N<CallExpr>(makeAnonFn(stmts)), "__iter__"));
  }
}

void SimplifyVisitor::visit(DictGeneratorExpr *expr) {
  SuiteStmt *prev;
  auto suite = transformGeneratorBody(expr->loops, prev);

  vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  stmts.push_back(transform(
      N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")), nullptr, true)));
  prev->stmts.push_back(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                                clone(expr->key), clone(expr->expr))));
  stmts.push_back(transform(suite));
  resultExpr = N<StmtExpr>(stmts, transform(var));
}

void SimplifyVisitor::visit(IfExpr *expr) {
  auto cond = transform(expr->cond);
  auto newExpr = N<IfExpr>(cond, transform(expr->ifexpr, false, /*allowAssign*/ false),
                           transform(expr->elsexpr, false, /*allowAssign*/ false));
  resultExpr = newExpr;
}

void SimplifyVisitor::visit(UnaryExpr *expr) {
  resultExpr = N<UnaryExpr>(expr->op, transform(expr->expr));
}

void SimplifyVisitor::visit(BinaryExpr *expr) {
  auto lhs = (startswith(expr->op, "is") && expr->lexpr->getNone())
                 ? clone(expr->lexpr)
                 : transform(expr->lexpr);
  auto rhs = (startswith(expr->op, "is") && expr->rexpr->getNone())
                 ? clone(expr->rexpr)
                 : transform(expr->rexpr, false,
                             /*allowAssign*/ expr->op != "&&" && expr->op != "||");
  resultExpr = N<BinaryExpr>(lhs, expr->op, rhs, expr->inPlace);
}

void SimplifyVisitor::visit(ChainBinaryExpr *expr) {
  seqassert(expr->exprs.size() >= 2, "not enough expressions in ChainBinaryExpr");
  vector<ExprPtr> e;
  string prev;
  for (int i = 1; i < expr->exprs.size(); i++) {
    auto l = prev.empty() ? clone(expr->exprs[i - 1].second) : N<IdExpr>(prev);
    prev = ctx->generateCanonicalName("chain");
    auto r =
        (i + 1 == expr->exprs.size())
            ? clone(expr->exprs[i].second)
            : N<StmtExpr>(N<AssignStmt>(N<IdExpr>(prev), clone(expr->exprs[i].second)),
                          N<IdExpr>(prev));
    e.emplace_back(N<BinaryExpr>(l, expr->exprs[i].first, r));
  }

  int i = int(e.size()) - 1;
  ExprPtr b = e[i];
  for (i -= 1; i >= 0; i--)
    b = N<BinaryExpr>(e[i], "&&", b);
  resultExpr = transform(b);
}

void SimplifyVisitor::visit(PipeExpr *expr) {
  vector<PipeExpr::Pipe> p;
  for (auto &i : expr->items) {
    auto e = clone(i.expr);
    if (auto ec = const_cast<CallExpr *>(e->getCall())) {
      for (auto &a : ec->args)
        if (auto ee = const_cast<EllipsisExpr *>(a.value->getEllipsis()))
          ee->isPipeArg = true;
    }
    p.push_back({i.op, transform(e)});
  }
  resultExpr = N<PipeExpr>(p);
}

void SimplifyVisitor::visit(IndexExpr *expr) {
  ExprPtr e = nullptr;
  auto index = expr->index->clone();
  // First handle the tuple[] and function[] cases.
  if (expr->expr->isId("tuple") || expr->expr->isId("Tuple")) {
    auto t = index->getTuple();
    e = N<IdExpr>(format(TYPE_TUPLE "{}", t ? t->items.size() : 1));
    e->markType();
  } else if (expr->expr->isId("function") || expr->expr->isId("Function") ||
             expr->expr->isId("Callable")) {
    auto t = const_cast<TupleExpr *>(index->getTuple());
    if (!t || t->items.size() != 2 || !t->items[0]->getList())
      error("invalid {} type declaration", expr->expr->getId()->value);
    for (auto &i : const_cast<ListExpr *>(t->items[0]->getList())->items)
      t->items.emplace_back(i);
    t->items.erase(t->items.begin());
    e = N<IdExpr>(
        format(expr->expr->isId("Callable") ? TYPE_CALLABLE "{}" : TYPE_FUNCTION "{}",
               t ? int(t->items.size()) - 1 : 0));
    e->markType();
  } else if (expr->expr->isId("Static")) {
    if (!expr->index->isId("int") && !expr->index->isId("str"))
      error("only static integers and strings are supported");
    resultExpr = expr->clone();
    resultExpr->markType();
    return;
  } else {
    e = transform(expr->expr, true);
  }
  // IndexExpr[i1, ..., iN] is internally stored as IndexExpr[TupleExpr[i1, ..., iN]]
  // for N > 1, so make sure to check that case.
  vector<ExprPtr> it;
  if (auto t = index->getTuple())
    for (auto &i : t->items)
      it.push_back(transform(i, true));
  else
    it.push_back(transform(index, true));
  if (e->isType()) {
    resultExpr = N<InstantiateExpr>(e, it);
    resultExpr->markType();
  } else {
    resultExpr = N<IndexExpr>(e, it.size() == 1 ? it[0] : N<TupleExpr>(it));
  }
}

void SimplifyVisitor::visit(CallExpr *expr) {
  // Special calls
  // 1. __ptr__(v)
  if (expr->expr->isId("__ptr__")) {
    if (expr->args.size() == 1 && expr->args[0].value->getId()) {
      auto v = ctx->find(expr->args[0].value->getId()->value);
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
    vector<ExprPtr> args{transformType(arg), transform(s)};
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
      ctx->add(SimplifyItem::Var, i->value, ctx->generateCanonicalName(i->value));
      var = transform(var);
      ex = transform(ex);
    } else {
      string varName = ctx->cache->getTemporaryVar("for");
      ctx->add(SimplifyItem::Var, varName, varName);
      var = N<IdExpr>(varName);
      ex = N<StmtExpr>(
          transform(N<AssignStmt>(clone(g->loops[0].vars), clone(var), nullptr, true)),
          transform(ex));
    }
    vector<GeneratorBody> body;
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
    vector<ExprPtr> args{transform(expr->args[0].value), transform(s)};
    resultExpr = N<CallExpr>(clone(expr->expr), args);
    return;
  }

  auto e = transform(expr->expr, true);
  // 8. namedtuple
  if (e->isId("std.collections.namedtuple")) {
    if (expr->args.size() != 2 || !expr->args[0].value->getString() ||
        !expr->args[1].value->getList())
      error("invalid namedtuple arguments");
    vector<Param> generics, args;
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
    if (expr->args.size() < 1)
      error("invalid namedtuple arguments");
    vector<CallExpr::Arg> args = clone_nop(expr->args);
    args.erase(args.begin());
    args.push_back({"", N<EllipsisExpr>()});
    resultExpr = transform(N<CallExpr>(clone(expr->args[0].value), args));
    return;
  }

  vector<CallExpr::Arg> args;
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

void SimplifyVisitor::visit(DotExpr *expr) {
  /// First flatten the imports.
  const Expr *e = expr;
  std::deque<string> chain;
  while (auto d = e->getDot()) {
    chain.push_front(d->member);
    e = d->expr.get();
  }
  if (auto d = e->getId()) {
    chain.push_front(d->value);

    /// Check if this is a import or a class access:
    /// (import1.import2...).(class1.class2...)?.method?
    int importEnd = 0, itemEnd = 0;
    string importName, itemName;
    shared_ptr<SimplifyItem> val = nullptr;
    for (int i = int(chain.size()) - 1; i >= 0; i--) {
      auto s = join(chain, "/", 0, i + 1);
      val = ctx->find(s);
      if (val && val->isImport()) {
        importName = val->importPath;
        importEnd = i + 1;
        break;
      }
    }
    // a.b.c is completely import name
    if (importEnd == chain.size()) {
      resultExpr = transform(N<IdExpr>(val->canonicalName));
      return;
    }
    auto fctx = importName.empty() ? ctx : ctx->cache->imports[importName].ctx;
    for (int i = int(chain.size()) - 1; i >= importEnd; i--) {
      auto s = join(chain, ".", importEnd, i + 1);
      val = fctx->find(s);
      // Make sure that we access only global imported variables.
      if (val && (importName.empty() || val->isGlobal())) {
        itemName = val->canonicalName;
        itemEnd = i + 1;
        if (!importName.empty())
          ctx->add(val->canonicalName, val);
        break;
      }
    }
    if (itemName.empty() && importName.empty())
      error("identifier '{}' not found", chain[importEnd]);
    if (itemName.empty())
      error("identifier '{}' not found in {}", chain[importEnd], importName);
    resultExpr = N<IdExpr>(itemName);
    if (importName.empty())
      resultExpr = transform(resultExpr, true);
    if (val->isType() && itemEnd == chain.size())
      resultExpr->markType();
    for (int i = itemEnd; i < chain.size(); i++)
      resultExpr = N<DotExpr>(resultExpr, chain[i]);
  } else {
    resultExpr = N<DotExpr>(transform(expr->expr, true), expr->member);
  }
}

void SimplifyVisitor::visit(SliceExpr *expr) {
  resultExpr = N<SliceExpr>(transform(expr->start), transform(expr->stop),
                            transform(expr->step));
}

void SimplifyVisitor::visit(EllipsisExpr *expr) {
  error("unexpected ellipsis expression");
}

void SimplifyVisitor::visit(YieldExpr *expr) {
  if (!ctx->inFunction())
    error("expected function body");
  defaultVisit(expr);
}

void SimplifyVisitor::visit(LambdaExpr *expr) {
  resultExpr =
      makeAnonFn(vector<StmtPtr>{N<ReturnStmt>(clone(expr->expr))}, expr->vars);
}

void SimplifyVisitor::visit(AssignExpr *expr) {
  seqassert(expr->var->getId(), "only simple assignment expression are supported");
  if (!ctx->canAssign)
    error("assignment expression in a short-circuiting subexpression");
  resultExpr = transform(
      N<StmtExpr>(vector<StmtPtr>{N<AssignStmt>(clone(expr->var), clone(expr->expr))},
                  clone(expr->var)));
}

void SimplifyVisitor::visit(RangeExpr *expr) {
  error("unexpected pattern range expression");
}

void SimplifyVisitor::visit(StmtExpr *expr) {
  vector<StmtPtr> stmts;
  for (auto &s : expr->stmts)
    stmts.emplace_back(transform(s));
  resultExpr = N<StmtExpr>(stmts, transform(expr->expr));
}

/**************************************************************************************/

ExprPtr SimplifyVisitor::transformInt(const string &value, const string &suffix) {
  auto to_int = [](const string &s) {
    if (startswith(s, "0b") || startswith(s, "0B"))
      return std::stoull(s.substr(2), nullptr, 2);
    return std::stoull(s, nullptr, 0);
  };
  try {
    if (suffix.empty()) {
      auto expr = N<IntExpr>(to_int(value));
      return expr;
    }
    /// Unsigned numbers: use UInt[64] for that
    if (suffix == "u")
      return transform(N<CallExpr>(N<IndexExpr>(N<IdExpr>("UInt"), N<IntExpr>(64)),
                                   N<IntExpr>(to_int(value))));
    /// Fixed-precision numbers (uXXX and iXXX)
    /// NOTE: you cannot use binary (0bXXX) format with those numbers.
    /// TODO: implement non-string constructor for these cases.
    if (suffix[0] == 'u' && isdigit(suffix.substr(1)))
      return transform(N<CallExpr>(
          N<IndexExpr>(N<IdExpr>("UInt"), N<IntExpr>(std::stoi(suffix.substr(1)))),
          N<StringExpr>(value)));
    if (suffix[0] == 'i' && isdigit(suffix.substr(1)))
      return transform(N<CallExpr>(
          N<IndexExpr>(N<IdExpr>("Int"), N<IntExpr>(std::stoi(suffix.substr(1)))),
          N<StringExpr>(value)));
  } catch (std::out_of_range &) {
    error("integer {} out of range", value);
  }
  /// Custom suffix sfx: use int.__suffix_sfx__(str) call.
  /// NOTE: you cannot neither use binary (0bXXX) format here.
  return transform(N<CallExpr>(N<DotExpr>("int", format("__suffix_{}__", suffix)),
                               N<StringExpr>(value)));
}

ExprPtr SimplifyVisitor::transformFloat(const string &value, const string &suffix) {
  try {
    if (suffix.empty()) {
      auto expr = N<FloatExpr>(std::stod(value));
      return expr;
    }
  } catch (std::out_of_range &) {
    error("integer {} out of range", value);
  }
  /// Custom suffix sfx: use float.__suffix_sfx__(str) call.
  return transform(N<CallExpr>(N<DotExpr>("float", format("__suffix_{}__", suffix)),
                               N<StringExpr>(value)));
}

ExprPtr SimplifyVisitor::transformFString(string value) {
  vector<ExprPtr> items;
  int braceCount = 0, braceStart = 0;
  for (int i = 0; i < value.size(); i++) {
    if (value[i] == '{') {
      if (braceStart < i)
        items.push_back(N<StringExpr>(value.substr(braceStart, i - braceStart)));
      if (!braceCount)
        braceStart = i + 1;
      braceCount++;
    } else if (value[i] == '}') {
      braceCount--;
      if (!braceCount) {
        string code = value.substr(braceStart, i - braceStart);
        auto offset = getSrcInfo();
        offset.col += i;
        if (!code.empty() && code.back() == '=') {
          code = code.substr(0, code.size() - 1);
          items.push_back(N<StringExpr>(format("{}=", code)));
        }
        items.push_back(
            N<CallExpr>(N<IdExpr>("str"), parseExpr(ctx->cache, code, offset)));
      }
      braceStart = i + 1;
    }
  }
  if (braceCount)
    error("f-string braces are not balanced");
  if (braceStart != value.size())
    items.push_back(N<StringExpr>(value.substr(braceStart, value.size() - braceStart)));
  return transform(N<CallExpr>(N<DotExpr>("str", "cat"), items));
}

StmtPtr SimplifyVisitor::transformGeneratorBody(const vector<GeneratorBody> &loops,
                                                SuiteStmt *&prev) {
  StmtPtr suite = N<SuiteStmt>(), newSuite = nullptr;
  prev = (SuiteStmt *)suite.get();
  for (auto &l : loops) {
    newSuite = N<SuiteStmt>();
    auto nextPrev = (SuiteStmt *)newSuite.get();

    prev->stmts.push_back(N<ForStmt>(l.vars->clone(), l.gen->clone(), newSuite));
    prev = nextPrev;
    for (auto &cond : l.conds) {
      newSuite = N<SuiteStmt>();
      nextPrev = (SuiteStmt *)newSuite.get();
      prev->stmts.push_back(N<IfStmt>(cond->clone(), newSuite));
      prev = nextPrev;
    }
  }
  return suite;
}

ExprPtr SimplifyVisitor::makeAnonFn(vector<StmtPtr> stmts,
                                    const vector<string> &argNames) {
  vector<Param> params;
  string name = ctx->cache->getTemporaryVar("lambda");
  for (auto &s : argNames)
    params.emplace_back(Param{s, nullptr, nullptr});
  auto s = transform(N<FunctionStmt>(name, nullptr, params, N<SuiteStmt>(move(stmts)),
                                     Attr({Attr::Capture})));
  if (s)
    return N<StmtExpr>(s, transform(N<IdExpr>(name)));
  return transform(N<IdExpr>(name));
}

} // namespace ast
} // namespace seq
