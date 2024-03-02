// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

UnionType::UnionType(Cache *cache) : RecordType(cache, "Union", "Union") {
  for (size_t i = 0; i < 256; i++)
    pendingTypes.emplace_back(
        std::make_shared<LinkType>(cache, LinkType::Generic, i, 0, nullptr));
}

UnionType::UnionType(Cache *cache, const std::vector<ClassType::Generic> &generics,
                     const std::vector<TypePtr> &pendingTypes)
    : RecordType(cache, "Union", "Union", generics), pendingTypes(pendingTypes) {}

int UnionType::unify(Type *typ, Unification *us) {
  if (typ->getUnion()) {
    auto tr = typ->getUnion();
    if (!isSealed() && !tr->isSealed()) {
      for (size_t i = 0; i < pendingTypes.size(); i++)
        if (pendingTypes[i]->unify(tr->pendingTypes[i].get(), us) == -1)
          return -1;
      return RecordType::unify(typ, us);
    } else if (!isSealed()) {
      return tr->unify(this, us);
    } else if (!tr->isSealed()) {
      if (tr->pendingTypes[0]->getLink() &&
          tr->pendingTypes[0]->getLink()->kind == LinkType::Unbound)
        return RecordType::unify(tr.get(), us);
      return -1;
    }
    // Do not hard-unify if we have unbounds
    if (!canRealize() || !tr->canRealize())
      return 0;

    auto u1 = getRealizationTypes();
    auto u2 = tr->getRealizationTypes();
    if (u1.size() != u2.size())
      return -1;
    int s1 = 2, s = 0;
    for (size_t i = 0; i < u1.size(); i++) {
      if ((s = u1[i]->unify(u2[i].get(), us)) == -1)
        return -1;
      s1 += s;
    }
    return s1;
  } else if (auto tl = typ->getLink()) {
    return tl->unify(this, us);
  }
  return -1;
}

TypePtr UnionType::generalize(int atLevel) {
  auto r = RecordType::generalize(atLevel);
  auto p = pendingTypes;
  for (auto &t : p)
    t = t->generalize(atLevel);
  auto t = std::make_shared<UnionType>(cache, r->getClass()->generics, p);
  t->setSrcInfo(getSrcInfo());
  return t;
}

TypePtr UnionType::instantiate(int atLevel, int *unboundCount,
                               std::unordered_map<int, TypePtr> *cache) {
  auto r = RecordType::instantiate(atLevel, unboundCount, cache);
  auto p = pendingTypes;
  for (auto &t : p)
    t = t->instantiate(atLevel, unboundCount, cache);
  auto t = std::make_shared<UnionType>(this->cache, r->getClass()->generics, p);
  t->setSrcInfo(getSrcInfo());
  return t;
}

std::string UnionType::debugString(char mode) const {
  if (mode == 2)
    return this->RecordType::debugString(mode);
  if (!generics[0].type->getRecord())
    return this->RecordType::debugString(mode);

  std::set<std::string> gss;
  for (auto &a : generics[0].type->getRecord()->args)
    gss.insert(a->debugString(mode));
  std::string s;
  for (auto &i : gss)
    s += "," + i;
  return fmt::format("{}{}", name, s.empty() ? "" : fmt::format("[{}]", s.substr(1)));
}

bool UnionType::canRealize() const { return isSealed() && RecordType::canRealize(); }

std::string UnionType::realizedName() const {
  seqassert(canRealize(), "cannot realize {}", toString());
  std::set<std::string> gss;
  for (auto &a : generics[0].type->getRecord()->args)
    gss.insert(a->realizedName());
  std::string s;
  for (auto &i : gss)
    s += "," + i;
  return fmt::format("{}{}", name, s.empty() ? "" : fmt::format("[{}]", s.substr(1)));
}

std::string UnionType::realizedTypeName() const { return realizedName(); }

void UnionType::addType(TypePtr typ) {
  seqassert(!isSealed(), "union already sealed");
  if (this == typ.get())
    return;
  if (auto tu = typ->getUnion()) {
    if (tu->isSealed()) {
      for (auto &t : tu->generics[0].type->getRecord()->args)
        addType(t);
    } else {
      for (auto &t : tu->pendingTypes) {
        if (t->getLink() && t->getLink()->kind == LinkType::Unbound)
          break;
        else
          addType(t);
      }
    }
  } else {
    // Find first pending generic to which we can attach this!
    Unification us;
    for (auto &t : pendingTypes)
      if (auto l = t->getLink()) {
        if (l->kind == LinkType::Unbound) {
          t->unify(typ.get(), &us);
          return;
        }
      }
    E(error::Error::UNION_TOO_BIG, this);
  }
}

bool UnionType::isSealed() const { return generics[0].type->getRecord() != nullptr; }

void UnionType::seal() {
  seqassert(!isSealed(), "union already sealed");
  auto tv = TypecheckVisitor(cache->typeCtx);

  size_t i;
  for (i = 0; i < pendingTypes.size(); i++)
    if (pendingTypes[i]->getLink() &&
        pendingTypes[i]->getLink()->kind == LinkType::Unbound)
      break;
  std::vector<TypePtr> typeSet(pendingTypes.begin(), pendingTypes.begin() + i);
  auto t = cache->typeCtx->instantiateTuple(typeSet);
  Unification us;
  generics[0].type->unify(t.get(), &us);
}

std::vector<types::TypePtr> UnionType::getRealizationTypes() {
  seqassert(canRealize(), "cannot realize {}", debugString(1));
  std::map<std::string, types::TypePtr> unionTypes;
  for (auto &u : generics[0].type->getRecord()->args)
    unionTypes[u->realizedName()] = u;
  std::vector<types::TypePtr> r;
  r.reserve(unionTypes.size());
  for (auto &[_, t] : unionTypes)
    r.emplace_back(t);
  return r;
}

} // namespace codon::ast::types
