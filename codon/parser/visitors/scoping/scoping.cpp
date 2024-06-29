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

void ScopingVisitor::apply(Cache *cache, Stmt *s) {
  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = cache;
  c->functionScope = nullptr;
  ScopingVisitor v;
  v.ctx = c;

  ConditionalBlock cb(c.get(), s, 0);
  v.transform(s);
  v.processChildCaptures();
}

void ScopingVisitor::transform(Expr *expr) {
  ScopingVisitor v(*this);
  if (expr) {
    setSrcInfo(expr->getSrcInfo());
    expr->accept(v);
  }
}

void ScopingVisitor::transform(Stmt *stmt) {
  ScopingVisitor v(*this);
  if (stmt) {
    setSrcInfo(stmt->getSrcInfo());
    stmt->accept(v);
  }
}

void ScopingVisitor::transformScope(Expr *e) {
  if (e) {
    ConditionalBlock c(ctx.get(), nullptr);
    transform(e);
  }
}

void ScopingVisitor::transformScope(Stmt *s) {
  if (s) {
    ConditionalBlock c(ctx.get(), s);
    transform(s);
  }
}

void ScopingVisitor::transformAdding(Expr *e, Node *root) {
  seqassert(e, "bad call to transformAdding");
  if (e->getIndex() || e->getDot()) {
    transform(e);
  } else if (e->getList() || e->getTuple() || e->getId()) {
    SetInScope s1(&(ctx->adding), true);
    SetInScope s2(&(ctx->root), root);
    transform(e);
  } else {
    E(Error::ASSIGN_INVALID, e);
  }
}

void ScopingVisitor::visit(IdExpr *expr) {
  // LOG("-------> {} {} {}", expr->getSrcInfo(), *expr, ctx->getScope());
  if (ctx->adding && ctx->tempScope)
    ctx->renames.back()[expr->value] = ctx->cache->getTemporaryVar(expr->value);
  for (size_t i = ctx->renames.size(); i-- > 0;)
    if (auto v = in(ctx->renames[i], expr->value)) {
      expr->value = *v;
      break;
    }
  if (visitName(expr->value, ctx->adding, ctx->root, expr->getSrcInfo())) {
    expr->setAttribute(Attr::ExprDominatedUndefCheck);
  }
}

void ScopingVisitor::visit(StringExpr *expr) {
  for (auto &s : expr->strings)
    if (s.prefix == "#f") {
      auto [expr, format] = parseExpr(ctx->cache, s.value, s.getSrcInfo());
      if (!format.empty()) {
        s.expr = ctx->cache->NS<CallExpr>(
            expr, ctx->cache->NS<DotExpr>(expr, expr, "__format__"),
            ctx->cache->NS<StringExpr>(expr, format));
      } else {
        s.expr =
            ctx->cache->NS<CallExpr>(expr, ctx->cache->NS<IdExpr>(expr, "str"), expr);
      }
      transform(s.expr);
    }
}

void ScopingVisitor::visit(GeneratorExpr *expr) {
  SetInScope s(&(ctx->tempScope), true);
  ctx->renames.emplace_back();
  transform(expr->loops);
  ctx->renames.pop_back();
}

void ScopingVisitor::visit(IfExpr *expr) {
  transform(expr->cond);

  transformScope(expr->ifexpr);
  transformScope(expr->elsexpr);
}

void ScopingVisitor::visit(BinaryExpr *expr) {
  transform(expr->lexpr);

  if (expr->op == "&&" || expr->op == "||")
    transformScope(expr->rexpr);
  else
    transform(expr->rexpr);
}

void ScopingVisitor::visit(AssignExpr *expr) {
  seqassert(expr->var->getId(), "only simple assignment expression are supported");

  SetInScope s(&(ctx->tempScope), false);
  transform(expr->expr);
  transformAdding(expr->var, expr);
}

void ScopingVisitor::visit(LambdaExpr *expr) {
  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = ctx->cache;
  FunctionStmt f("lambda", nullptr, {}, nullptr);
  c->functionScope = &f;
  c->renames = ctx->renames;
  ScopingVisitor v;
  c->scope.emplace_back(0, nullptr);
  v.ctx = c;
  for (auto &a : expr->vars)
    v.visitName(a, true, expr, expr->getSrcInfo());
  c->scope.pop_back();

  SuiteStmt s;
  c->scope.emplace_back(0, &s);
  v.transform(expr->expr);
  v.processChildCaptures();
  c->scope.pop_back();

  auto b = std::make_unique<BindingsAttribute>();
  b->captures = c->captures;
  for (const auto &n : c->captures)
    ctx->childCaptures.insert(n);
  for (auto &[u, v] : c->map)
    b->bindings[u] = v.size();
  expr->setAttribute(Attr::Bindings, std::move(b));
}

// todo)) Globals/nonlocals cannot be shadowed in children scopes (as in Python)
// val->canShadow = false;

void ScopingVisitor::visit(AssignStmt *stmt) {
  // seqassert(stmt->lhs->getId(), "assignment not unpacked");
  transform(stmt->rhs);
  transform(stmt->type);
  transformAdding(stmt->lhs, stmt);
}

void ScopingVisitor::visit(IfStmt *stmt) {
  transform(stmt->cond);
  transformScope(stmt->ifSuite);
  transformScope(stmt->elseSuite);
}

void ScopingVisitor::visit(MatchStmt *stmt) {
  transform(stmt->what);
  for (auto &m : stmt->cases) {
    transform(m.pattern);
    transform(m.guard);
    transformScope(m.suite);
  }
}

void ScopingVisitor::visit(WhileStmt *stmt) {
  transform(stmt->cond);

  std::unordered_set<std::string> seen;
  {
    ConditionalBlock c(ctx.get(), stmt->suite);
    ctx->scope.back().seenVars = std::make_unique<std::unordered_set<std::string>>();
    transform(stmt->suite);
    seen = *(ctx->scope.back().seenVars);
  }
  for (auto &var : seen)
    findDominatingBinding(var);

  transformScope(stmt->elseSuite);
}

void ScopingVisitor::visit(ForStmt *stmt) {
  transform(stmt->iter);
  transform(stmt->decorator);
  for (auto &a : stmt->ompArgs)
    transform(a.value);

  std::unordered_set<std::string> seen;
  {
    ConditionalBlock c(ctx.get(), stmt->suite);
    if (!stmt->var->getId()) {
      auto var = N<IdExpr>(ctx->cache->getTemporaryVar("for"));
      auto e = N<AssignStmt>(clone(stmt->var), clone(var));
      stmt->suite = N<SuiteStmt>(e->unpack(), stmt->suite);
      stmt->var = var;
    }
    transformAdding(stmt->var, stmt);

    ctx->scope.back().seenVars = std::make_unique<std::unordered_set<std::string>>();
    transform(stmt->suite);
    seen = *(ctx->scope.back().seenVars);
  }
  for (auto &var : seen)
    if (var != stmt->var->getId()->value)
      findDominatingBinding(var);

  transformScope(stmt->elseSuite);
}

void ScopingVisitor::visit(ImportStmt *stmt) {
  if (ctx->functionScope && stmt->what && stmt->what->isId("*"))
    E(error::Error::IMPORT_STAR, stmt);

  // dylib C imports
  if (stmt->from && stmt->from->isId("C") && stmt->what->getDot())
    transform(stmt->what->getDot()->expr);

  if (stmt->as.empty()) {
    if (stmt->what)
      transformAdding(stmt->what, stmt);
    else
      transformAdding(stmt->from, stmt);
  } else {
    visitName(stmt->as, true, stmt, stmt->getSrcInfo());
  }
  for (auto &a : stmt->args) {
    transform(a.type);
    transform(a.defaultValue);
  }
  transform(stmt->ret);
}

void ScopingVisitor::visit(TryStmt *stmt) {
  transformScope(stmt->suite);
  for (auto &a : stmt->catches) {
    transform(a->exc);
    ConditionalBlock c(ctx.get(), a->suite);
    if (!a->var.empty()) {
      auto newName = ctx->cache->getTemporaryVar(a->var);
      ctx->renames.push_back({{a->var, newName}});
      a->var = newName;
      visitName(a->var, true, a, a->exc->getSrcInfo());
    }
    transform(a->suite);
    if (!a->var.empty())
      ctx->renames.pop_back();
  }
  transform(stmt->finally);
}

void ScopingVisitor::visit(DelStmt *stmt) {
  /// TODO
  transform(stmt->expr);
}

/// Process `global` statements. Remove them upon completion.
void ScopingVisitor::visit(GlobalStmt *stmt) {
  if (!ctx->functionScope)
    E(Error::FN_OUTSIDE_ERROR, stmt, stmt->nonLocal ? "nonlocal" : "global");
  if (in(ctx->map, stmt->var) || in(ctx->captures, stmt->var))
    E(Error::FN_GLOBAL_ASSIGNED, stmt, stmt->var);

  visitName(stmt->var, true, stmt, stmt->getSrcInfo());
  ctx->captures[stmt->var] = stmt->nonLocal ? BindingsAttribute::CaptureType::Nonlocal
                                            : BindingsAttribute::CaptureType::Global;
}

void ScopingVisitor::visit(YieldStmt *stmt) {
  if (ctx->functionScope)
    ctx->functionScope->setAttribute(Attr::IsGenerator);
  transform(stmt->expr);
}

void ScopingVisitor::visit(YieldExpr *expr) {
  if (ctx->functionScope)
    ctx->functionScope->setAttribute(Attr::IsGenerator);
}

void ScopingVisitor::visit(FunctionStmt *stmt) {
  bool isOverload = false;
  for (auto &d : stmt->decorators)
    if (d->isId("overload")) {
      isOverload = true;
    }
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
      transform(a.defaultValue);
  }
  c->scope.pop_back();

  c->scope.emplace_back(0, stmt->suite);
  v.transform(stmt->suite);
  v.processChildCaptures();
  c->scope.pop_back();

  auto b = std::make_unique<BindingsAttribute>();
  b->captures = c->captures;
  for (const auto &n : c->captures)
    ctx->childCaptures.insert(n);
  for (auto &[u, v] : c->map)
    b->bindings[u] = v.size();
  stmt->setAttribute(Attr::Bindings, std::move(b));
}

void ScopingVisitor::visit(WithStmt *stmt) {
  ConditionalBlock c(ctx.get(), stmt->suite);
  for (size_t i = 0; i < stmt->items.size(); i++) {
    transform(stmt->items[i]);
    if (!stmt->vars[i].empty())
      visitName(stmt->vars[i], true, stmt, stmt->getSrcInfo());
  }
  transform(stmt->suite);
}

void ScopingVisitor::visit(ClassStmt *stmt) {
  if (stmt->hasAttribute(Attr::Extend))
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
    v.transform(a.type);
    v.transform(a.defaultValue);
  }
  v.transform(stmt->suite);
  c->scope.pop_back();

  //   for (auto &d : stmt->decorators)
  //     transform(d);
  for (auto &d : stmt->baseClasses)
    transform(d);
  for (auto &d : stmt->staticBaseClasses)
    transform(d);
}

void ScopingVisitor::processChildCaptures() {
  for (auto &n : ctx->childCaptures) {
    if (auto i = in(ctx->map, n.first)) {
      if (i->back().binding && CAST(i->back().binding, ClassStmt))
        continue;
    }
    if (!findDominatingBinding(n.first, false)) {
      // LOG("-> add: {} / {}", ctx->functionScope ? ctx->functionScope->name : "-", n);
      ctx->captures.insert(n); // propagate!
    }
  }
}

void ScopingVisitor::switchToUpdate(Node *binding, const std::string &name,
                                    bool gotUsedVar) {
  // LOG("switch: {} {} : {}", name, gotUsedVar, binding ? binding->toString(0) : "-");
  if (binding && binding->hasAttribute(Attr::Bindings)) {
    binding->getAttribute<BindingsAttribute>(Attr::Bindings)->bindings.erase(name);
  }
  if (binding) {
    if (!gotUsedVar && binding->hasAttribute(Attr::ExprDominatedUsed))
      binding->attributes.erase(Attr::ExprDominatedUsed);
    binding->setAttribute(gotUsedVar ? Attr::ExprDominatedUsed : Attr::ExprDominated);
  }
  if (CAST(binding, FunctionStmt))
    E(error::Error::ID_INVALID_BIND, binding, name);
  if (CAST(binding, ClassStmt))
    E(error::Error::ID_INVALID_BIND, binding, name);
}

bool ScopingVisitor::visitName(const std::string &name, bool adding, Node *root,
                               const SrcInfo &src) {
  if (adding && ctx->inClass)
    return false;
  if (adding) {
    if (auto p = in(ctx->captures, name)) {
      if (*p == BindingsAttribute::CaptureType::Read)
        E(error::Error::ASSIGN_LOCAL_REFERENCE, ctx->firstSeen[name], name, src);
      else if (root) // global, nonlocal
        switchToUpdate(root, name, false);
    } else {
      if (auto i = in(ctx->childCaptures, name)) {
        if (*i != BindingsAttribute::CaptureType::Global && ctx->functionScope) {
          auto newScope = std::vector<int>{ctx->scope[0].id};
          seqassert(ctx->scope.front().suite, "invalid suite");
          if (!ctx->scope.front().suite->hasAttribute(Attr::Bindings))
            ctx->scope.front().suite->setAttribute(
                Attr::Bindings, std::make_unique<BindingsAttribute>());
          ctx->scope.front()
              .suite->getAttribute<BindingsAttribute>(Attr::Bindings)
              ->bindings[name] = false;
          // LOG("-> promote {} / F", name);
          auto newItem = ScopingVisitor::Context::Item(src, newScope, nullptr);
          ctx->map[name].push_back(newItem);
          // LOG("add: {} | {} := {} {}", src, name, newScope, "-");
        }
      }
      ctx->map[name].emplace_front(src, ctx->getScope(), root);
      // LOG("add: {} | {} := {} {}", src, name, ctx->getScope(),
          // root ? root->toString(-1) : "-");
    }
  } else {
    if (!in(ctx->firstSeen, name))
      ctx->firstSeen[name] = src;
    if (!in(ctx->map, name)) {
      ctx->captures[name] = BindingsAttribute::CaptureType::Read;
    }
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
    // LOG("{} : var {}: {} vs {}", getSrcInfo(), name, val->accessChecked, ctx->getScope());
    if (!val->accessChecked.empty()) {
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
        seqassert(!adding, "bad branch");
        if (!(val->binding && val->binding->hasAttribute(Attr::Bindings))) {
          // If the expression is not conditional, we can just do the check once
          val->accessChecked.push_back(ctx->getScope());
        }
        return true;
      }
    }
  }
  return false;
}

/// Get an item from the context. Perform domination analysis for accessing items
/// defined in the conditional blocks (i.e., Python scoping).
ScopingVisitor::Context::Item *
ScopingVisitor::findDominatingBinding(const std::string &name, bool allowShadow) {
  auto it = in(ctx->map, name);
  if (!it || it->empty())
    return nullptr;
  auto lastGood = it->begin();
  while (lastGood != it->end() && lastGood->ignore)
    lastGood++;
  int commonScope = int(ctx->scope.size());
  // Iterate through all bindings with the given name and find the closest binding that
  // dominates the current scope.
  for (auto i = it->begin(); i != it->end(); i++) {
    if (i->ignore)
      continue;

    bool completeDomination = i->scope.size() <= ctx->scope.size() &&
                              i->scope.back() == ctx->scope[i->scope.size() - 1].id;
    if (completeDomination) {
      commonScope = i->scope.size();
      lastGood = i;
      break;
    } else {
      seqassert(i->scope[0] == 0, "bad scoping");
      seqassert(ctx->scope[0].id == 0, "bad scoping");
      // Find the longest block prefix between the binding and the current common scope.
      commonScope = std::min(commonScope, int(i->scope.size()));
      while (commonScope > 0 && i->scope[commonScope - 1] != ctx->scope[commonScope - 1].id)
        commonScope--;
      // if (commonScope < int(ctx->scope.size()) && commonScope != p)
      //   break;
      lastGood = i;
    }
  }
  // if (commonScope != ctx->scope.size())
  //   LOG("==> {}: {} / {} vs {}", getSrcInfo(), name, ctx->getScope(), commonScope);
  seqassert(lastGood != it->end(), "corrupted scoping ({})", name);
  if (!allowShadow) { // go to the end
    lastGood = it->end();
    --lastGood;
    int p = std::min(commonScope, int(lastGood->scope.size()));
    while (p >= 0 && lastGood->scope[p - 1] != ctx->scope[p - 1].id)
      p--;
    commonScope = p;
  }

  bool gotUsedVar = false;
  if (lastGood->scope.size() != commonScope) {
    // The current scope is potentially reachable by multiple bindings that are
    // not dominated by a common binding. Create such binding in the scope that
    // dominates (covers) all of them.
    auto scope = ctx->getScope();
    auto newScope = std::vector<int>(scope.begin(), scope.begin() + commonScope);

    // Make sure to prepend a binding declaration: `var` and `var__used__ = False`
    // to the dominating scope.
    for (size_t si = commonScope; si-- > 0; si--) {
      if (!ctx->scope[si].suite)
        continue;
      if (!ctx->scope[si].suite->hasAttribute(Attr::Bindings))
        ctx->scope[si].suite->setAttribute(Attr::Bindings,
                                           std::make_unique<BindingsAttribute>());
      ctx->scope[si]
          .suite->getAttribute<BindingsAttribute>(Attr::Bindings)
          ->bindings[name] = true;
      auto newItem = ScopingVisitor::Context::Item(
          getSrcInfo(), newScope, ctx->scope[si].suite, {lastGood->scope});
      lastGood = it->insert(++lastGood, newItem);
      // LOG("-> {}: promote {} / T from {} to {} | {}",
      // getSrcInfo(),
      // name, ctx->getScope(),
      //     lastGood->scope, lastGood->accessChecked);
      gotUsedVar = true;
      break;
    }
  } else if (lastGood->binding && lastGood->binding->hasAttribute(Attr::Bindings)) {
    gotUsedVar = lastGood->binding->getAttribute<BindingsAttribute>(Attr::Bindings)
                     ->bindings[name];
  }
  // Remove all bindings after the dominant binding.
  for (auto i = it->begin(); i != it->end(); i++) {
    if (i == lastGood)
      break;
    switchToUpdate(i->binding, name, gotUsedVar);
    i->scope = lastGood->scope;
    i->ignore = true;
  }
  if (!gotUsedVar && lastGood->binding &&
      lastGood->binding->hasAttribute(Attr::Bindings))
    lastGood->binding->getAttribute<BindingsAttribute>(Attr::Bindings)->bindings[name] =
        false;
  return &(*lastGood);
}

} // namespace codon::ast
