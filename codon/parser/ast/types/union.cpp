// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

#include <ranges>

namespace codon::ast::types {

UnionType::UnionType(Cache *cache) : ClassType(cache, StdlibTypes::Union) {
  isTuple = true;
}

UnionType::UnionType(Cache *cache, const std::vector<ClassType::Generic> &generics)
    : ClassType(cache, StdlibTypes::Union, generics) {
  isTuple = true;
}

int UnionType::unify(Type *typ, Unification *us) {
  if (auto tr = typ->getUnion()) {
    // Do not hard-unify if we have unbounds
    if (!canRealize() || !tr->canRealize())
      return 0;

    auto u1 = getRealizationTypes();
    auto u2 = tr->getRealizationTypes();
    if (u1.size() != u2.size())
      return -1;
    int s1 = 2, s = 0;
    for (size_t i = 0; i < u1.size(); i++) {
      if ((s = u1[i]->unify(u2[i], us)) == -1)
        return -1;
      s1 += s;
    }
    return s1;
  } else if (auto tl = typ->getLink()) {
    return tl->unify(this, us);
  }
  return -1;
}

TypePtr UnionType::generalize(int atLevel) const {
  auto r = ClassType::generalize(atLevel);
  auto t = std::make_shared<UnionType>(cache, r->getClass()->generics);
  t->setSrcInfo(getSrcInfo());
  return t;
}

TypePtr UnionType::instantiate(int atLevel, int *unboundCount,
                               std::unordered_map<int, TypePtr> *cache) const {
  auto r = ClassType::instantiate(atLevel, unboundCount, cache);
  auto t = std::make_shared<UnionType>(this->cache, r->getClass()->generics);
  t->setSrcInfo(getSrcInfo());
  return t;
}

std::string UnionType::debugString(char mode) const {
  if (mode == 2)
    return this->ClassType::debugString(mode);
  if (!generics[0].type->getClass())
    return this->ClassType::debugString(mode);

  std::set<std::string> gss;
  for (auto &a : generics[0].type->getClass()->generics)
    gss.insert(a.debugString(mode));
  std::string s = join(gss, "|");
  return (name + (s.empty() ? "" : ("[" + s + "]")));
}

std::string UnionType::realizedName() const {
  seqassert(canRealize(), "cannot realize {}", debugString(2));
  return ClassType::realizedName();
}

std::vector<Type *> UnionType::getRealizationTypes() const {
  seqassert(canRealize(), "cannot realize {}", debugString(2));
  std::map<std::string, Type *> unionTypes;
  for (auto &u : generics[0].type->getClass()->generics)
    unionTypes[u.type->realizedName()] = u.type.get();
  std::vector<Type *> r;
  r.reserve(unionTypes.size());
  for (auto &t : unionTypes | std::views::values)
    r.emplace_back(t);
  return r;
}

} // namespace codon::ast::types
