// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "typecheck.h"

#include <memory>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/typecheck/ctx.h"
#include <fmt/format.h>

using fmt::format;
using namespace codon::error;

namespace codon::ast {

void ScopingVisitor::apply(Cache *cache, StmtPtr &s) {
  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = cache;
  c->functionScope = false;
  ScopingVisitor v;
  v.ctx = c;
  v.transformBlock(s);
  // LOG("-> {}", s->toString(2));
}

ExprPtr ScopingVisitor::transform(const std::shared_ptr<Expr> &expr) {
  ScopingVisitor v(*this);
  if (expr)
    expr->accept(v);
  return v.resultExpr ? v.resultExpr : expr;
}

ExprPtr ScopingVisitor::transform(std::shared_ptr<Expr> &expr) {
  ScopingVisitor v(*this);
  if (expr)
    expr->accept(v);
  if (v.resultExpr)
    expr = v.resultExpr;
  return expr;
}

StmtPtr ScopingVisitor::transform(const std::shared_ptr<Stmt> &stmt) {
  ScopingVisitor v(*this);
  if (stmt)
    stmt->accept(v);
  return v.resultStmt ? v.resultStmt : stmt;
}

StmtPtr ScopingVisitor::transform(std::shared_ptr<Stmt> &stmt) {
  ScopingVisitor v(*this);
  if (stmt)
    stmt->accept(v);
  if (v.resultStmt)
    stmt = v.resultStmt;
  return stmt;
}

void ScopingVisitor::visitName(const std::string &name, bool adding, Stmt *root,
                               const SrcInfo &src) {
  if (adding && ctx->inClass)
    return;
  if (adding) {
    if (in(ctx->captures, name))
      E(error::Error::ASSIGN_LOCAL_REFERENCE, ctx->firstSeen[name], name, src);
    if (in(ctx->childCaptures, name) && ctx->functionScope) {
      auto newScope = std::vector<int>{ctx->scope[0].id};
      auto b = N<AssignStmt>(N<IdExpr>(name), nullptr, nullptr);
      auto newItem = ScopingVisitor::Context::Item{newScope, b.get()};
      ctx->scope.front().stmts.emplace_back(b);
      ctx->map[name].push_back(newItem);
    }
    ctx->map[name].push_front({ctx->getScope(), root});
    if (!root)
      ctx->temps.back().insert(name);
  } else {
    if (!in(ctx->firstSeen, name))
      ctx->firstSeen[name] = src;
    if (!in(ctx->map, name))
      ctx->captures.insert(name);
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
            N<DotExpr>("__internal__", "undef"),
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

void ScopingVisitor::transformAdding(ExprPtr &e, Stmt *root) {
  if (e->getIndex() || e->getDot()) {
    transform(e);
  } else if (e->getList() || e->getTuple() || e->getId()) {
    auto adding = true;
    std::swap(adding, ctx->adding);
    std::swap(root, ctx->root);
    transform(e);
    std::swap(root, ctx->root);
    std::swap(adding, ctx->adding);
  } else {
    seqassert(false, "bad assignment: {}", e);
  }
}

void ScopingVisitor::visit(IdExpr *expr) {
  visitName(expr->value, ctx->adding, ctx->root, expr->getSrcInfo());
}

/// Get an item from the context. Perform domination analysis for accessing items
/// defined in the conditional blocks (i.e., Python scoping).
ScopingVisitor::Context::Item *
ScopingVisitor::findDominatingBinding(const std::string &name, bool allowShadow) {
  auto it = in(ctx->map, name);
  if (!it)
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
    prefix = p;
    lastGood = i;

    // The binding completely dominates the current scope. Break.
    if (i->scope.size() <= ctx->scope.size() &&
        i->scope.back() == ctx->scope[i->scope.size() - 1].id)
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
    auto newItem = ScopingVisitor::Context::Item{newScope, b.get(), {lastGood->scope}};
    ctx->scope[prefix - 1].stmts.emplace_back(b);
    lastGood = it->insert(++lastGood, newItem);
    gotUsedVar = true;
  }
  // Remove all bindings after the dominant binding.
  for (auto i = it->begin(); i != it->end(); i++) {
    if (i == lastGood)
      break;
    // if (!(*i)->canDominate())
    //   continue;

    // These bindings (and their canonical identifiers) will be replaced by the
    // dominating binding during the type checking pass.
    auto used = format("{}.__used__", name);
    if (auto s = i->binding->getSuite()) {
      seqassert(!s->stmts.empty() && s->stmts[0]->getAssign(), "bad suite");
      auto a = s->stmts[0]->getAssign();
      a->setUpdate();
      if (gotUsedVar && s->stmts.size() < 2) {
        s->stmts.push_back(N<AssignStmt>(N<IdExpr>(used), N<BoolExpr>(true), nullptr,
                                         AssignStmt::UpdateMode::Update));
      }
    } else if (auto f = dynamic_cast<ForStmt *>(i->binding)) {
      f->var->setAttr(ExprAttr::Dominated);
      if (gotUsedVar) {
        bool skip = false;
        if (auto s = f->suite->firstInBlock())
          skip = s->getAssign() && s->getAssign()->lhs->isId(used);
        if (!skip) {
          f->suite =
              N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(used), N<BoolExpr>(true), nullptr,
                                         AssignStmt::UpdateMode::Update),
                           f->suite);
        }
      }
    } else if (i->binding) {
      // class; function; func-arg; comprehension-arg; catch-name; import-name[anything
      // really]
      // todo)) generators?!
      E(error::Error::ID_INVALID_BIND, i->binding, name);
    }
  }
  it->erase(it->begin(), lastGood);
  return &(*lastGood);
}

/// TODO)) dominate assignexprs in comprehensions?!
void ScopingVisitor::visit(GeneratorExpr *expr) {
  ctx->temps.emplace_back();
  for (auto &l : expr->loops) {
    transform(l.gen);
    transformAdding(l.vars, nullptr);
    for (auto &c : l.conds)
      transform(c);
  }
  enterConditionalBlock();
  transform(expr->expr);
  leaveConditionalBlock();
  for (auto &n : ctx->temps.back()) {
    while (ctx->map[n].begin()->binding)
      ctx->map[n].pop_front();
    ctx->map[n].pop_front();
  }
  ctx->temps.pop_back();
}

void ScopingVisitor::visit(DictGeneratorExpr *expr) {
  ctx->temps.emplace_back();
  for (auto &l : expr->loops) {
    transform(l.gen);
    transformAdding(l.vars, nullptr);
    for (auto &c : l.conds)
      transform(c);
  }
  enterConditionalBlock();
  transform(expr->key);
  transform(expr->expr);
  leaveConditionalBlock();
  for (auto &n : ctx->temps.back()) {
    while (ctx->map[n].begin()->binding)
      ctx->map[n].pop_front();
    ctx->map[n].pop_front();
  }
  ctx->temps.pop_back();
}

void ScopingVisitor::visit(IfExpr *expr) {
  transform(expr->cond);
  auto isConditional = true;
  std::swap(isConditional, ctx->isConditional);
  transform(expr->ifexpr);
  transform(expr->elsexpr);
  std::swap(isConditional, ctx->isConditional);
}

void ScopingVisitor::visit(BinaryExpr *expr) {
  transform(expr->lexpr);
  auto isConditional = expr->op == "&&" || expr->op == "||";
  std::swap(isConditional, ctx->isConditional);
  transform(expr->rexpr);
  std::swap(isConditional, ctx->isConditional);
}

void ScopingVisitor::visit(AssignExpr *expr) {
  seqassert(expr->var->getId(), "only simple assignment expression are supported");

  auto s = N<StmtExpr>(N<AssignStmt>(clone(expr->var), expr->expr), expr->var);
  // todo)) if (ctx->isConditionalExpr) {
  enterConditionalBlock();
  transform(s);
  leaveConditionalBlock();
  resultExpr = s;
}

// todo)) Globals/nonlocals cannot be shadowed in children scopes (as in Python)
// val->canShadow = false;

void ScopingVisitor::visit(AssignStmt *stmt) {
  if (stmt->lhs->getTuple() || stmt->lhs->getList()) {
    std::vector<StmtPtr> s;
    unpackAssignments(stmt->lhs, stmt->rhs, s);
    resultStmt = transform(N<SuiteStmt>(s));
  } else {
    transform(stmt->rhs);
    transform(stmt->type);
    auto a = N<AssignStmt>(stmt->lhs, stmt->rhs, stmt->type);
    a->setSrcInfo(stmt->getSrcInfo());
    resultStmt = N<SuiteStmt>(a);
    transformAdding(stmt->lhs, resultStmt.get());
  }
}

/// Unpack an assignment expression `lhs = rhs` into a list of simple assignment
/// expressions (e.g., `a = b`, `a.x = b`, or `a[x] = b`).
/// Handle Python unpacking rules.
/// @example
///   `(a, b) = c`     -> `a = c[0]; b = c[1]`
///   `a, b = c`       -> `a = c[0]; b = c[1]`
///   `[a, *x, b] = c` -> `a = c[0]; x = c[1:-1]; b = c[-1]`.
/// Non-trivial right-hand expressions are first stored in a temporary variable.
/// @example
///   `a, b = c, d + foo()` -> `assign = (c, d + foo); a = assign[0]; b = assign[1]`.
/// Each assignment is unpacked recursively to allow cases like `a, (b, c) = d`.
void ScopingVisitor::unpackAssignments(const ExprPtr &lhs, ExprPtr rhs,
                                       std::vector<StmtPtr> &stmts) {
  std::vector<ExprPtr> leftSide;
  if (auto et = lhs->getTuple()) {
    // Case: (a, b) = ...
    for (auto &i : et->items)
      leftSide.push_back(i);
  } else if (auto el = lhs->getList()) {
    // Case: [a, b] = ...
    for (auto &i : el->items)
      leftSide.push_back(i);
  } else {
    // Case: simple assignment (a = b, a.x = b, or a[x] = b)
    stmts.push_back(N<AssignStmt>(clone(lhs), clone(rhs)));
    return;
  }

  // Prepare the right-side expression
  auto srcPos = rhs->getSrcInfo();
  if (!rhs->getId()) {
    // Store any non-trivial right-side expression into a variable
    auto var = ctx->cache->getTemporaryVar("assign");
    ExprPtr newRhs = N<IdExpr>(srcPos, var);
    stmts.push_back(N<AssignStmt>(newRhs, clone(rhs)));
    rhs = newRhs;
  }

  // Process assignments until the fist StarExpr (if any)
  size_t st = 0;
  for (; st < leftSide.size(); st++) {
    if (leftSide[st]->getStar())
      break;
    // Transformation: `leftSide_st = rhs[st]` where `st` is static integer
    auto rightSide = N<IndexExpr>(srcPos, clone(rhs), N<IntExpr>(srcPos, st));
    // Recursively process the assignment because of cases like `(a, (b, c)) = d)`
    unpackAssignments(leftSide[st], rightSide, stmts);
  }
  // Process StarExpr (if any) and the assignments that follow it
  if (st < leftSide.size() && leftSide[st]->getStar()) {
    // StarExpr becomes SliceExpr (e.g., `b` in `(a, *b, c) = d` becomes `d[1:-2]`)
    auto rightSide = N<IndexExpr>(
        srcPos, clone(rhs),
        N<SliceExpr>(srcPos, N<IntExpr>(srcPos, st),
                     // this slice is either [st:] or [st:-lhs_len + st + 1]
                     leftSide.size() == st + 1
                         ? nullptr
                         : N<IntExpr>(srcPos, -leftSide.size() + st + 1),
                     nullptr));
    unpackAssignments(leftSide[st]->getStar()->what, rightSide, stmts);
    st += 1;
    // Process remaining assignments. They will use negative indices (-1, -2 etc.)
    // because we do not know how big is StarExpr
    for (; st < leftSide.size(); st++) {
      if (leftSide[st]->getStar())
        E(Error::ASSIGN_MULTI_STAR, leftSide[st]);
      rightSide = N<IndexExpr>(srcPos, clone(rhs),
                               N<IntExpr>(srcPos, -int(leftSide.size() - st)));
      unpackAssignments(leftSide[st], rightSide, stmts);
    }
  }
}

void ScopingVisitor::visit(IfStmt *stmt) {
  transform(stmt->cond);

  enterConditionalBlock();
  transform(stmt->ifSuite);
  leaveConditionalBlock(stmt->ifSuite);

  enterConditionalBlock();
  transform(stmt->elseSuite);
  leaveConditionalBlock(stmt->elseSuite);
}

void ScopingVisitor::visit(MatchStmt *stmt) {
  transform(stmt->what);
  for (auto &m : stmt->cases) {
    transform(m.pattern);
    transform(m.guard);

    enterConditionalBlock();
    transform(m.suite);
    leaveConditionalBlock(m.suite);
  }
}

void ScopingVisitor::visit(WhileStmt *stmt) {
  transform(stmt->cond);

  enterConditionalBlock();
  ctx->scope.back().seenVars = std::make_shared<std::unordered_set<std::string>>();
  transform(stmt->suite);
  auto seen = *(ctx->scope.back().seenVars);
  leaveConditionalBlock(stmt->suite);
  for (auto &var : seen)
    findDominatingBinding(var);

  enterConditionalBlock();
  transform(stmt->elseSuite);
  leaveConditionalBlock(stmt->elseSuite);
}

void ScopingVisitor::visit(ForStmt *stmt) {
  transform(stmt->iter);
  transform(stmt->decorator);
  for (auto &a : stmt->ompArgs)
    transform(a.value);

  if (!stmt->var->getId()) {
    auto var = N<IdExpr>(ctx->cache->getTemporaryVar("for"));
    stmt->suite =
        N<SuiteStmt>(N<AssignStmt>(clone(stmt->var), clone(var)), stmt->suite);
    stmt->var = var;
  }
  transformAdding(stmt->var, stmt);

  enterConditionalBlock();
  ctx->scope.back().seenVars = std::make_shared<std::unordered_set<std::string>>();
  transform(stmt->suite);
  auto seen = *(ctx->scope.back().seenVars);
  leaveConditionalBlock(stmt->suite);
  for (auto &var : seen)
    if (var != stmt->var->getId()->value)
      findDominatingBinding(var);

  enterConditionalBlock();
  transform(stmt->elseSuite);
  leaveConditionalBlock(stmt->elseSuite);
}

void ScopingVisitor::visit(ImportStmt *stmt) {
  if (ctx->functionScope && stmt->what && stmt->what->isId("*"))
    E(error::Error::IMPORT_STAR, stmt);

  //   transform(stmt->from);
  if (stmt->as.empty())
    transformAdding(stmt->what, stmt);
  else
    visitName(stmt->as, true, stmt, stmt->getSrcInfo());
  for (auto &a : stmt->args) {
    transform(a.type);
    transform(a.defaultValue);
  }
  transform(stmt->ret);
}

void ScopingVisitor::visit(TryStmt *stmt) {
  enterConditionalBlock();
  transform(stmt->suite);
  leaveConditionalBlock(stmt->suite);

  for (auto &a : stmt->catches) {
    ctx->temps.emplace_back();
    transform(a.exc);
    if (!a.var.empty())
      visitName(a.var, true, stmt, a.exc->getSrcInfo());

    enterConditionalBlock();
    transform(a.suite);
    leaveConditionalBlock(a.suite);

    for (auto &n : ctx->temps.back()) {
      while (ctx->map[n].begin()->binding)
        ctx->map[n].pop_front();
      ctx->map[n].pop_front();
    }
    ctx->temps.pop_back();
  }
  transform(stmt->finally);
}

void ScopingVisitor::visit(FunctionStmt *stmt) {
  visitName(stmt->name, true, stmt, stmt->getSrcInfo());

  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = ctx->cache;
  c->functionScope = true;
  ScopingVisitor v;
  c->scope.emplace_back(0);
  v.ctx = c;
  v.visitName(stmt->name, true, stmt, stmt->getSrcInfo());
  for (auto &a : stmt->args)
    v.visitName(a.name, true, stmt, a.getSrcInfo());
  c->scope.pop_back();
  v.transformBlock(stmt->suite);

  stmt->attributes.captures = c->captures;
  for (const auto &n : c->captures) {
    ctx->childCaptures.insert(n);
  }

  // LOG("=> {} :: cap {}", stmt->name, c->captures);
  // LOG("{}", stmt->toString(2));
}

void ScopingVisitor::transformBlock(StmtPtr &s) {
  ctx->scope.emplace_back(0);
  transform(s);
  if (!ctx->scope.back().stmts.empty()) {
    ctx->scope.back().stmts.push_back(s);
    s = N<SuiteStmt>(ctx->scope.back().stmts);
    ctx->scope.back().stmts.clear();
  }
  for (auto &n : ctx->childCaptures) {
    if (!findDominatingBinding(n, false))
      ctx->captures.insert(n); // propagate!
  }
  if (!ctx->scope.back().stmts.empty()) {
    ctx->scope.back().stmts.push_back(s);
    s = N<SuiteStmt>(ctx->scope.back().stmts);
  }
  ctx->scope.pop_back();
}

void ScopingVisitor::visit(ClassStmt *stmt) {
  if (stmt->hasAttr(Attr::Extend))
    visitName(stmt->name);
  else
    visitName(stmt->name, true, stmt, stmt->getSrcInfo());
  for (auto &a : stmt->args) {
    transform(a.type);
    transform(a.defaultValue);
  }
  auto inClass = true;
  std::swap(inClass, ctx->inClass);
  transform(stmt->suite);
  std::swap(inClass, ctx->inClass);
  //   for (auto &d : stmt->decorators)
  //     transform(d);
  for (auto &d : stmt->baseClasses)
    transform(d);
  for (auto &d : stmt->staticBaseClasses)
    transform(d);
}

void ScopingVisitor::enterConditionalBlock() {
  ctx->scope.emplace_back(ctx->cache->blockCount++);
}

void ScopingVisitor::leaveConditionalBlock() {
  ctx->scope.pop_back();
  seqassert(!ctx->scope.empty(), "empty scope");
}

void ScopingVisitor::leaveConditionalBlock(StmtPtr &stmts) {
  if (!ctx->scope.back().stmts.empty()) {
    ctx->scope.back().stmts.push_back(stmts);
    stmts = N<SuiteStmt>(ctx->scope.back().stmts);
  }
  leaveConditionalBlock();
}

} // namespace codon::ast
