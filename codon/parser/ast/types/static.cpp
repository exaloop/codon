// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/ast/types/static.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

StaticType::StaticType(Cache *cache, const std::shared_ptr<Expr> &e)
    : Type(cache), expr(e->clone()) {
  if (!expr->isStatic() || !expr->staticValue.evaluated) {
    std::unordered_set<std::string> seen;
    parseExpr(expr, seen);
  }
}

StaticType::StaticType(Cache *cache, std::vector<ClassType::Generic> generics,
                       const std::shared_ptr<Expr> &e)
    : Type(cache), generics(std::move(generics)), expr(e->clone()) {}

StaticType::StaticType(Cache *cache, int64_t i)
    : Type(cache), expr(std::make_shared<IntExpr>(i)) {}

StaticType::StaticType(Cache *cache, const std::string &s)
    : Type(cache), expr(std::make_shared<StringExpr>(s)) {}

int StaticType::unify(Type *typ, Unification *us) {
  if (auto t = typ->getStatic()) {
    if (canRealize())
      expr->staticValue = evaluate();
    if (t->canRealize())
      t->expr->staticValue = t->evaluate();
    // Check if both types are already evaluated.
    if (expr->staticValue.type != t->expr->staticValue.type)
      return -1;
    if (expr->staticValue.evaluated && t->expr->staticValue.evaluated)
      return expr->staticValue == t->expr->staticValue ? 2 : -1;
    else if (expr->staticValue.evaluated && !t->expr->staticValue.evaluated)
      return typ->unify(this, us);

    // Right now, *this is not evaluated
    // Let us see can we unify it with other _if_ it is a simple IdExpr?
    if (expr->getId() && t->expr->staticValue.evaluated) {
      return generics[0].type->unify(typ, us);
    }

    // At this point, *this is a complex expression (e.g. A+1).
    seqassert(!generics.empty(), "unevaluated simple expression");
    if (generics.size() != t->generics.size())
      return -1;

    int s1 = 2, s = 0;
    if (!(expr->getId() && t->expr->getId()) && expr->toString() != t->expr->toString())
      return -1;
    for (int i = 0; i < generics.size(); i++) {
      if ((s = generics[i].type->unify(t->generics[i].type.get(), us)) == -1)
        return -1;
      s1 += s;
    }
    return s1;
  } else if (auto tl = typ->getLink()) {
    return tl->unify(this, us);
  }
  return -1;
}

TypePtr StaticType::generalize(int atLevel) {
  auto e = generics;
  for (auto &t : e)
    t.type = t.type ? t.type->generalize(atLevel) : nullptr;
  auto c = std::make_shared<StaticType>(cache, e, expr);
  c->setSrcInfo(getSrcInfo());
  return c;
}

TypePtr StaticType::instantiate(int atLevel, int *unboundCount,
                                std::unordered_map<int, TypePtr> *cache) {
  auto e = generics;
  for (auto &t : e)
    t.type = t.type ? t.type->instantiate(atLevel, unboundCount, cache) : nullptr;
  auto c = std::make_shared<StaticType>(this->cache, e, expr);
  c->setSrcInfo(getSrcInfo());
  return c;
}

std::vector<TypePtr> StaticType::getUnbounds() const {
  std::vector<TypePtr> u;
  for (auto &t : generics)
    if (t.type) {
      auto tu = t.type->getUnbounds();
      u.insert(u.begin(), tu.begin(), tu.end());
    }
  return u;
}

bool StaticType::canRealize() const {
  if (!expr->staticValue.evaluated)
    for (auto &t : generics)
      if (t.type && !t.type->canRealize())
        return false;
  return true;
}

bool StaticType::isInstantiated() const { return expr->staticValue.evaluated; }

std::string StaticType::debugString(char mode) const {
  if (expr->staticValue.evaluated)
    return expr->staticValue.toString();
  if (mode == 2) {
    std::vector<std::string> s;
    for (auto &g : generics)
      s.push_back(g.type->debugString(mode));
    return fmt::format("Static[{};{}]", join(s, ","), expr->toString());
  } else {
    return fmt::format("Static[{}]", FormatVisitor::apply(expr));
  }
}

std::string StaticType::realizedName() const {
  seqassert(canRealize(), "cannot realize {}", toString());
  std::vector<std::string> deps;
  for (auto &e : generics)
    deps.push_back(e.type->realizedName());
  if (!expr->staticValue.evaluated) // If not already evaluated, evaluate!
    const_cast<StaticType *>(this)->expr->staticValue = evaluate();
  seqassert(expr->staticValue.evaluated, "static value not evaluated");
  return expr->staticValue.toString();
}

StaticValue StaticType::evaluate() const {
  if (expr->staticValue.evaluated)
    return expr->staticValue;
  cache->typeCtx->addBlock();
  for (auto &g : generics)
    cache->typeCtx->add(TypecheckItem::Type, g.name, g.type);
  auto oldChangedNodes = cache->typeCtx->changedNodes;
  auto en = TypecheckVisitor(cache->typeCtx).transform(expr->clone());
  cache->typeCtx->changedNodes = oldChangedNodes;
  seqassert(en->isStatic() && en->staticValue.evaluated, "{} cannot be evaluated", en);
  cache->typeCtx->popBlock();
  return en->staticValue;
}

void StaticType::parseExpr(const ExprPtr &e, std::unordered_set<std::string> &seen) {
  e->type = nullptr;
  if (auto ei = e->getId()) {
    if (!in(seen, ei->value)) {
      auto val = cache->typeCtx->find(ei->value);
      seqassert(val && val->type->isStaticType(), "invalid static expression");
      auto genTyp = val->type->follow();
      auto id = genTyp->getLink() ? genTyp->getLink()->id
                : genTyp->getStatic()->generics.empty()
                    ? 0
                    : genTyp->getStatic()->generics[0].id;
      generics.emplace_back(ClassType::Generic(
          ei->value, cache->typeCtx->cache->reverseIdentifierLookup[ei->value], genTyp,
          id));
      seen.insert(ei->value);
    }
  } else if (auto eu = e->getUnary()) {
    parseExpr(eu->expr, seen);
  } else if (auto eb = e->getBinary()) {
    parseExpr(eb->lexpr, seen);
    parseExpr(eb->rexpr, seen);
  } else if (auto ef = e->getIf()) {
    parseExpr(ef->cond, seen);
    parseExpr(ef->ifexpr, seen);
    parseExpr(ef->elsexpr, seen);
  }
}

} // namespace codon::ast::types
