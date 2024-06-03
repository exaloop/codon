// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include <fmt/format.h>

using fmt::format;
using namespace codon::error;

namespace codon::ast {

Stmt *ScopingVisitor::apply(Cache *cache, Stmt *s) {
  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = cache;
  c->functionScope = nullptr;
  ScopingVisitor v;
  v.ctx = c;
  return v.transformBlock(s);
}

Expr *ScopingVisitor::transform(Expr *expr) {
  ScopingVisitor v(*this);
  if (expr) {
    setSrcInfo(expr->getSrcInfo());
    expr->accept(v);
  }
  return v.resultExpr ? v.resultExpr : expr;
}

Stmt *ScopingVisitor::transform(Stmt *stmt) {
  ScopingVisitor v(*this);
  if (stmt) {
    setSrcInfo(stmt->getSrcInfo());
    stmt->accept(v);
  }
  return v.resultStmt ? v.resultStmt : stmt;
}

void ScopingVisitor::switchToUpdate(Node *binding, const std::string &name,
                                    bool gotUsedVar) {
  // These bindings (and their canonical identifiers) will be replaced by the
  // dominating binding during the type checking pass.
  auto used = format("{}.__used__", name);
  if (auto s = CAST(binding, SuiteStmt)) {
    seqassert(!s->stmts.empty() && s->stmts[0]->getAssign(), "bad suite");
    auto a = s->stmts[0]->getAssign();
    if (a->rhs) {
      a->setUpdate();
      if (gotUsedVar && s->stmts.size() < 2) {
        s->stmts.push_back(N<AssignStmt>(N<IdExpr>(used), N<BoolExpr>(true), nullptr,
                                         AssignStmt::UpdateMode::Update));
      }
    } else {
      seqassert(s->stmts.size() == 2 && s->stmts[1]->getAssign(), "bad assign block");
      s->stmts = {N<AssignStmt>(N<IdExpr>(used), N<BoolExpr>(true), nullptr)};
    }
  } else if (auto f = CAST(binding, ForStmt)) {
    f->var->setAttr(ExprAttr::Dominated);
    if (gotUsedVar) {
      bool skip = false;
      if (auto s = f->suite->firstInBlock())
        skip = s->getAssign() && s->getAssign()->lhs->isId(used);
      if (!skip) {
        f->suite = N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(used), N<BoolExpr>(true),
                                              nullptr, AssignStmt::UpdateMode::Update),
                                f->suite);
      }
    }
  } else if (auto f = CAST(binding, TryStmt::Catch)) {
    f->exc->setAttr(ExprAttr::Dominated);
    if (gotUsedVar) {
      bool skip = false;
      if (auto s = f->suite->firstInBlock())
        skip = s->getAssign() && s->getAssign()->lhs->isId(used);
      if (!skip) {
        f->suite = N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(used), N<BoolExpr>(true),
                                              nullptr, AssignStmt::UpdateMode::Update),
                                f->suite);
      }
    }
  } else if (auto f = CAST(binding, WithStmt)) {
    for (auto i = f->items.size(); i-- > 0;)
      if (f->vars[i] == name)
        f->items[i]->setAttr(ExprAttr::Dominated);
  } else if (auto f = CAST(binding, ImportStmt)) {
    // TODO)) Here we will just ignore this for now
  } else if (auto f = CAST(binding, GlobalStmt)) {
    // TODO)) Here we will just ignore this for now
  } else if (binding) {
    // class; function; func-arg; comprehension-arg; catch-name; import-name[anything
    // really]
    // todo)) generators?!
    E(error::Error::ID_INVALID_BIND, binding, name);
  }
}

void ScopingVisitor::visitName(const std::string &name, bool adding, Node *root,
                               const SrcInfo &src) {
  if (adding && ctx->inClass)
    return;
  if (adding) {
    if (auto p = in(ctx->captures, name)) {
      if (*p == Attr::CaptureType::Read)
        E(error::Error::ASSIGN_LOCAL_REFERENCE, ctx->firstSeen[name], name, src);
      else if (root) // global, nonlocal
        switchToUpdate(root, name, false);
    } else {
      if (auto i = in(ctx->childCaptures, name)) {
        if (*i != Attr::CaptureType::Global && ctx->functionScope) {
          auto newScope = std::vector<int>{ctx->scope[0].id};
          auto b = N<AssignStmt>(N<IdExpr>(name), nullptr, nullptr);
          auto newItem = ScopingVisitor::Context::Item(src, newScope, b);
          ctx->scope.front().stmts.emplace_back(b);
          ctx->map[name].push_back(newItem);
        }
      }
      ctx->map[name].emplace_front(src, ctx->getScope(), root);
    }
  } else {
    if (!in(ctx->firstSeen, name))
      ctx->firstSeen[name] = src;
    if (!in(ctx->map, name))
      ctx->captures[name] = Attr::CaptureType::Read;
  }
  if (auto val = findDominatingBinding(name)) {
    // Track loop variables to dominate them later. Example:
    // x = 1
    // while True:
    //   if x > 10: break
    //   x = x + 1  # x must be dominated after the loop to ensure that it gets updated
    auto scope = ctx->getScope();
    for (size_t li = ctx->scope.size(); li-- > 0;) {
      if (ctx->scope[li].seenVars) {
        bool inside = val->scope.size() >= scope.size() &&
                      val->scope[scope.size() - 1] == scope.back();
        if (!inside)
          ctx->scope[li].seenVars->insert(name);
        else
          break;
      }
      scope.pop_back();
    }

    // Variable binding check for variables that are defined within conditional blocks
    if (!val->accessChecked.empty()) {
      // LOG("var {}: {} vs {}", val->canonicalName, val->accessChecked,
      //     ctx->getScope());
      bool checked = false;
      for (size_t ai = val->accessChecked.size(); ai-- > 0;) {
        auto &a = val->accessChecked[ai];
        if (a.size() <= ctx->scope.size() &&
            a[a.size() - 1] == ctx->scope[a.size() - 1].id) {
          checked = true;
          break;
        }
      }
      if (!checked) {
        // Prepend access with __internal__.undef([var]__used__, "[var name]")
        auto checkStmt = N<ExprStmt>(N<CallExpr>(
            N<DotExpr>(N<IdExpr>("__internal__"), "undef"),
            N<IdExpr>(fmt::format("{}.__used__", name)), N<StringExpr>(name)));

        resultExpr = N<StmtExpr>(checkStmt, N<IdExpr>(name));
        if (!ctx->isConditional) {
          // If the expression is not conditional, we can just do the check once
          val->accessChecked.push_back(ctx->getScope());
        }
      }
    }
  }
}

Expr *ScopingVisitor::transformAdding(Expr *e, Node *root) {
  seqassert(e, "bad call to transformAdding");
  if (e->getIndex() || e->getDot()) {
    return transform(e);
  } else if (e->getList() || e->getTuple() || e->getId()) {
    auto adding = true;
    std::swap(adding, ctx->adding);
    std::swap(root, ctx->root);
    e = transform(e);
    std::swap(root, ctx->root);
    std::swap(adding, ctx->adding);
    return e;
  } else {
    E(Error::ASSIGN_INVALID, e);
    return nullptr;
  }
}

void ScopingVisitor::visit(IdExpr *expr) {
  if (ctx->adding && ctx->tempScope)
    ctx->renames.back()[expr->value] = ctx->cache->getTemporaryVar(expr->value);
  for (size_t i = ctx->renames.size(); i-- > 0;)
    if (auto v = in(ctx->renames[i], expr->value)) {
      expr->value = *v;
      break;
    }
  visitName(expr->value, ctx->adding, ctx->root, expr->getSrcInfo());
}

/// Get an item from the context. Perform domination analysis for accessing items
/// defined in the conditional blocks (i.e., Python scoping).
ScopingVisitor::Context::Item *
ScopingVisitor::findDominatingBinding(const std::string &name, bool allowShadow) {
  auto it = in(ctx->map, name);
  if (!it || it->empty())
    return nullptr;

  auto lastGood = it->begin();
  int prefix = int(ctx->scope.size());
  // Iterate through all bindings with the given name and find the closest binding that
  // dominates the current scope.
  for (auto i = it->begin(); i != it->end(); i++) {
    // Find the longest block prefix between the binding and the current scope.
    int p = std::min(prefix, int(i->scope.size()));
    while (p >= 0 && i->scope[p - 1] != ctx->scope[p - 1].id)
      p--;
    // We reached the toplevel. Break.
    if (p < 0)
      break;

    bool completeDomination = i->scope.size() <= ctx->scope.size() &&
                              i->scope.back() == ctx->scope[i->scope.size() - 1].id;
    if (!completeDomination && prefix < int(ctx->scope.size()) && prefix != p)
      break;

    prefix = p;
    lastGood = i;
    // The binding completely dominates the current scope. Break.
    if (completeDomination)
      break;
  }
  seqassert(lastGood != it->end(), "corrupted scoping ({})", name);
  if (!allowShadow) { // go to the end
    lastGood = it->end();
    --lastGood;
    int p = std::min(prefix, int(lastGood->scope.size()));
    while (p >= 0 && lastGood->scope[p - 1] != ctx->scope[p - 1].id)
      p--;
    prefix = p;
  }

  bool gotUsedVar = false;
  if (lastGood->scope.size() != prefix) {
    // The current scope is potentially reachable by multiple bindings that are
    // not dominated by a common binding. Create such binding in the scope that
    // dominates (covers) all of them.
    auto scope = ctx->getScope();
    auto newScope = std::vector<int>(scope.begin(), scope.begin() + prefix);

    // Make sure to prepend a binding declaration: `var` and `var__used__ = False`
    // to the dominating scope.
    auto b = N<SuiteStmt>(
        N<AssignStmt>(N<IdExpr>(name), nullptr),
        N<AssignStmt>(N<IdExpr>(format("{}.__used__", name)), N<BoolExpr>(false)));
    ctx->scope[prefix - 1].stmts.emplace_back(b);
    auto newItem =
        ScopingVisitor::Context::Item(getSrcInfo(), newScope, b, {lastGood->scope});
    lastGood = it->insert(++lastGood, newItem);
    gotUsedVar = true;
  }
  // Remove all bindings after the dominant binding.
  for (auto i = it->begin(); i != it->end(); i++) {
    if (i == lastGood)
      break;
    switchToUpdate(i->binding, name, gotUsedVar);
  }
  it->erase(it->begin(), lastGood);
  return &(*lastGood);
}

void ScopingVisitor::visit(GeneratorExpr *expr) {
  SetInScope(ctx->tempScope, true);
  ctx->renames.emplace_back();
  expr->loops = transform(expr->loops);
  ctx->renames.pop_back();
}

void ScopingVisitor::visit(IfExpr *expr) {
  expr->cond = transform(expr->cond);

  SetInScope(ctx->isConditional, true);
  expr->ifexpr = transform(expr->ifexpr);
  expr->elsexpr = transform(expr->elsexpr);
}

void ScopingVisitor::visit(BinaryExpr *expr) {
  expr->lexpr = transform(expr->lexpr);

  SetInScope(ctx->isConditional, expr->op == "&&" || expr->op == "||");
  expr->rexpr = transform(expr->rexpr);
}

void ScopingVisitor::visit(AssignExpr *expr) {
  seqassert(expr->var->getId(), "only simple assignment expression are supported");

  Expr *s = N<StmtExpr>(N<AssignStmt>(clone(expr->var), expr->expr), expr->var);
  enterConditionalBlock();
  {
    SetInScope(ctx->tempScope, false);
    s = transform(s);
  }
  leaveConditionalBlock();
  resultExpr = s;
}

void ScopingVisitor::visit(LambdaExpr *expr) {
  resultExpr = makeAnonFn(std::vector<Stmt *>{N<ReturnStmt>(expr->expr)}, expr->vars);
}

// todo)) Globals/nonlocals cannot be shadowed in children scopes (as in Python)
// val->canShadow = false;

void ScopingVisitor::visit(AssignStmt *stmt) {
  seqassert(stmt->lhs->getId(), "assignment not unpacked");
  stmt->rhs = transform(stmt->rhs);
  stmt->type = transform(stmt->type);
  stmt->lhs = transformAdding(stmt->lhs, resultStmt);
}

void ScopingVisitor::visit(IfStmt *stmt) {
  stmt->cond = transform(stmt->cond);

  enterConditionalBlock();
  stmt->ifSuite = transform(stmt->ifSuite);
  leaveConditionalBlock(&stmt->ifSuite);

  enterConditionalBlock();
  stmt->elseSuite = transform(stmt->elseSuite);
  leaveConditionalBlock(&stmt->elseSuite);
}

void ScopingVisitor::visit(MatchStmt *stmt) {
  stmt->what = transform(stmt->what);
  for (auto &m : stmt->cases) {
    m.pattern = transform(m.pattern);
    m.guard = transform(m.guard);

    enterConditionalBlock();
    m.suite = transform(m.suite);
    leaveConditionalBlock(&m.suite);
  }
}

void ScopingVisitor::visit(WhileStmt *stmt) {
  stmt->cond = transform(stmt->cond);

  enterConditionalBlock();
  ctx->scope.back().seenVars = std::make_shared<std::unordered_set<std::string>>();
  stmt->suite = transform(stmt->suite);
  auto seen = *(ctx->scope.back().seenVars);
  leaveConditionalBlock(&stmt->suite);
  for (auto &var : seen)
    findDominatingBinding(var);

  enterConditionalBlock();
  stmt->elseSuite = transform(stmt->elseSuite);
  leaveConditionalBlock(&stmt->elseSuite);
}

void ScopingVisitor::visit(ForStmt *stmt) {
  stmt->iter = transform(stmt->iter);
  stmt->decorator = transform(stmt->decorator);
  for (auto &a : stmt->ompArgs)
    a.value = transform(a.value);

  enterConditionalBlock();
  if (!stmt->var->getId()) {
    auto var = N<IdExpr>(ctx->cache->getTemporaryVar("for"));
    stmt->suite =
        N<SuiteStmt>(N<AssignStmt>(clone(stmt->var), clone(var)), stmt->suite);
    stmt->var = var;
  }
  stmt->var = transformAdding(stmt->var, stmt);

  ctx->scope.back().seenVars = std::make_shared<std::unordered_set<std::string>>();
  stmt->suite = transform(stmt->suite);
  auto seen = *(ctx->scope.back().seenVars);
  leaveConditionalBlock(&stmt->suite);
  for (auto &var : seen)
    if (var != stmt->var->getId()->value)
      findDominatingBinding(var);

  enterConditionalBlock();
  stmt->elseSuite = transform(stmt->elseSuite);
  leaveConditionalBlock(&stmt->elseSuite);
}

void ScopingVisitor::visit(ImportStmt *stmt) {
  if (ctx->functionScope && stmt->what && stmt->what->isId("*"))
    E(error::Error::IMPORT_STAR, stmt);

  // dylib C imports
  if (stmt->from && stmt->from->isId("C") && stmt->what->getDot()) {
    stmt->what->getDot()->expr = transform(stmt->what->getDot()->expr);
  }

  if (stmt->as.empty()) {
    if (stmt->what)
      stmt->what = transformAdding(stmt->what, stmt);
    else
      stmt->from = transformAdding(stmt->from, stmt);
  } else {
    visitName(stmt->as, true, stmt, stmt->getSrcInfo());
  }
  for (auto &a : stmt->args) {
    a.type = transform(a.type);
    a.defaultValue = transform(a.defaultValue);
  }
  stmt->ret = transform(stmt->ret);
}

void ScopingVisitor::visit(TryStmt *stmt) {
  enterConditionalBlock();
  stmt->suite = transform(stmt->suite);
  leaveConditionalBlock(&stmt->suite);

  for (auto &a : stmt->catches) {
    a->exc = transform(a->exc);
    enterConditionalBlock();
    if (!a->var.empty()) {
      auto newName = ctx->cache->getTemporaryVar(a->var);
      ctx->renames.push_back({{a->var, newName}});
      a->var = newName;
      visitName(a->var, true, a, a->exc->getSrcInfo());
    }
    a->suite = transform(a->suite);
    if (!a->var.empty()) {
      ctx->renames.pop_back();
    }
    leaveConditionalBlock(&a->suite);
  }
  stmt->finally = transform(stmt->finally);
}

void ScopingVisitor::visit(DelStmt *stmt) {
  /// TODO
  stmt->expr = transform(stmt->expr);
}

/// Process `global` statements. Remove them upon completion.
void ScopingVisitor::visit(GlobalStmt *stmt) {
  if (!ctx->functionScope)
    E(Error::FN_OUTSIDE_ERROR, stmt, stmt->nonLocal ? "nonlocal" : "global");
  if (in(ctx->map, stmt->var) || in(ctx->captures, stmt->var))
    E(Error::FN_GLOBAL_ASSIGNED, stmt, stmt->var);

  visitName(stmt->var, true, stmt, stmt->getSrcInfo());
  ctx->captures[stmt->var] =
      stmt->nonLocal ? Attr::CaptureType::Nonlocal : Attr::CaptureType::Global;

  resultStmt = N<SuiteStmt>();
}

void ScopingVisitor::visit(YieldStmt *stmt) {
  if (ctx->functionScope)
    ctx->functionScope->attributes.set(Attr::IsGenerator);
  stmt->expr = transform(stmt->expr);
}

void ScopingVisitor::visit(YieldExpr *expr) {
  if (ctx->functionScope)
    ctx->functionScope->attributes.set(Attr::IsGenerator);
}

void ScopingVisitor::visit(FunctionStmt *stmt) {
  bool isOverload = false;
  for (auto &d : stmt->decorators)
    if (d->isId("overload"))
      isOverload = true;
  if (!isOverload)
    visitName(stmt->name, true, stmt, stmt->getSrcInfo());

  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = ctx->cache;
  c->functionScope = stmt;
  c->renames = ctx->renames;
  ScopingVisitor v;
  c->scope.emplace_back(0);
  v.ctx = c;
  v.visitName(stmt->name, true, stmt, stmt->getSrcInfo());
  for (auto &a : stmt->args) {
    v.visitName(a.name, true, stmt, a.getSrcInfo());
    if (a.defaultValue)
      a.defaultValue = transform(a.defaultValue);
  }
  c->scope.pop_back();
  stmt->suite = v.transformBlock(stmt->suite);

  stmt->attributes.captures = c->captures;
  for (const auto &n : c->captures)
    ctx->childCaptures.insert(n);

  for (auto &[u, v] : c->map)
    stmt->attributes.bindings[u] = v.size();

  // if (stmt->name=="test_omp_critical") {
  // LOG("=> {} :: cap {}", stmt->name, c->captures);
  // LOG("{}", stmt->toString(2));
  // }
}

void ScopingVisitor::visit(WithStmt *stmt) {
  enterConditionalBlock();
  for (size_t i = 0; i < stmt->items.size(); i++) {
    stmt->items[i] = transform(stmt->items[i]);
    if (!stmt->vars[i].empty())
      visitName(stmt->vars[i], true, stmt, stmt->getSrcInfo());
  }
  stmt->suite = transform(stmt->suite);
  leaveConditionalBlock(&stmt->suite);
}

Stmt *ScopingVisitor::transformBlock(Stmt *s) {
  ctx->scope.emplace_back(0);
  s = transform(s);
  if (!ctx->scope.back().stmts.empty()) {
    ctx->scope.back().stmts.push_back(s);
    s = N<SuiteStmt>(ctx->scope.back().stmts);
    ctx->scope.back().stmts.clear();
  }
  for (auto &n : ctx->childCaptures) {
    // TODO HACK!
    // if (ctx->functionScope && n.second == Attr::CaptureType::Global) {
    //   ctx->captures.insert(n); // propagate!
    //   continue;
    // }
    if (auto i = in(ctx->map, n.first)) {
      if (i->back().binding && CAST(i->back().binding, ClassStmt))
        continue;
    }
    if (!findDominatingBinding(n.first, false))
      ctx->captures.insert(n); // propagate!
  }
  if (!ctx->scope.back().stmts.empty()) {
    ctx->scope.back().stmts.push_back(s);
    s = N<SuiteStmt>(ctx->scope.back().stmts);
  }
  ctx->scope.pop_back();
  return s;
}

void ScopingVisitor::visit(ClassStmt *stmt) {
  if (stmt->hasAttr(Attr::Extend))
    visitName(stmt->name);
  else
    visitName(stmt->name, true, stmt, stmt->getSrcInfo());

  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = ctx->cache;
  c->renames = ctx->renames;
  ScopingVisitor v;
  c->scope.emplace_back(0);
  c->inClass = true;
  v.ctx = c;
  for (auto &a : stmt->args) {
    a.type = v.transform(a.type);
    a.defaultValue = v.transform(a.defaultValue);
  }
  stmt->suite = v.transform(stmt->suite);
  c->scope.pop_back();

  //   for (auto &d : stmt->decorators)
  //     transform(d);
  for (auto &d : stmt->baseClasses)
    d = transform(d);
  for (auto &d : stmt->staticBaseClasses)
    d = transform(d);
}

void ScopingVisitor::enterConditionalBlock() {
  ctx->scope.emplace_back(ctx->cache->blockCount++);
}

void ScopingVisitor::leaveConditionalBlock() {
  ctx->scope.pop_back();
  seqassert(!ctx->scope.empty(), "empty scope");
}

void ScopingVisitor::leaveConditionalBlock(Stmt **stmts) {
  if (!ctx->scope.back().stmts.empty()) {
    ctx->scope.back().stmts.push_back(*stmts);
    *stmts = N<SuiteStmt>(ctx->scope.back().stmts);
  }
  leaveConditionalBlock();
}

Expr *ScopingVisitor::makeAnonFn(std::vector<Stmt *> suite,
                                 const std::vector<std::string> &argNames) {
  std::vector<Param> params;
  std::string name = ctx->cache->getTemporaryVar("lambda");
  params.reserve(argNames.size());
  for (auto &s : argNames)
    params.emplace_back(s);
  auto f =
      transform(N<FunctionStmt>(name, nullptr, params, N<SuiteStmt>(std::move(suite))));
  return N<StmtExpr>(f, N<CallExpr>(N<IdExpr>(name), N<EllipsisExpr>()));
}

} // namespace codon::ast
