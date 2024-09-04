// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/types/class.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

ClassType::ClassType(Cache *cache, std::string name, std::string niceName,
                     std::vector<Generic> generics, std::vector<Generic> hiddenGenerics)
    : Type(cache), name(std::move(name)), niceName(std::move(niceName)),
      generics(std::move(generics)), hiddenGenerics(std::move(hiddenGenerics)) {}
ClassType::ClassType(ClassType *base)
    : Type(*base), name(base->name), niceName(base->niceName), generics(base->generics),
      hiddenGenerics(base->hiddenGenerics), isTuple(base->isTuple) {}

int ClassType::unify(Type *typ, Unification *us) {
  if (auto tc = typ->getClass()) {
    if (name == "int" && tc->name == "Int")
      return tc->unify(this, us);
    if (tc->name == "int" && name == "Int") {
      auto t64 = std::make_shared<IntStaticType>(cache, 64);
      return generics[0].type->unify(t64.get(), us);
    }
    if (name == "unrealized_type" && tc->name == name) {
      // instantiate + unify!
      std::unordered_map<int, types::TypePtr> genericCache;
      auto l = generics[0].type->instantiate(0, &(cache->unboundCount), &genericCache);
      genericCache.clear();
      auto r =
          tc->generics[0].type->instantiate(0, &(cache->unboundCount), &genericCache);
      return l->unify(r.get(), us);
    }
    // Check names.
    if (name != tc->name)
      return -1;
    // Check generics.
    int s1 = 3, s = 0;
    if (generics.size() != tc->generics.size())
      return -1;
    for (int i = 0; i < generics.size(); i++) {
      if ((s = generics[i].type->unify(tc->generics[i].type.get(), us)) == -1) {
        return -1;
      }
      s1 += s;
    }
    for (int i = 0; i < hiddenGenerics.size(); i++) {
      if ((s = hiddenGenerics[i].type->unify(tc->hiddenGenerics[i].type.get(), us)) ==
          -1) {
        return -1;
      }
      s1 += s;
    }
    return s1;
  } else if (auto tl = typ->getLink()) {
    return tl->unify(this, us);
  } else {
    return -1;
  }
}

TypePtr ClassType::generalize(int atLevel) {
  auto g = generics, hg = hiddenGenerics;
  for (auto &t : g)
    t.type = t.type ? t.type->generalize(atLevel) : nullptr;
  for (auto &t : hg)
    t.type = t.type ? t.type->generalize(atLevel) : nullptr;
  auto c = std::make_shared<ClassType>(cache, name, niceName, g, hg);
  c->isTuple = isTuple;
  c->setSrcInfo(getSrcInfo());
  return c;
}

TypePtr ClassType::instantiate(int atLevel, int *unboundCount,
                               std::unordered_map<int, TypePtr> *cache) {
  auto g = generics, hg = hiddenGenerics;
  for (auto &t : g)
    t.type = t.type ? t.type->instantiate(atLevel, unboundCount, cache) : nullptr;
  for (auto &t : hg)
    t.type = t.type ? t.type->instantiate(atLevel, unboundCount, cache) : nullptr;
  auto c = std::make_shared<ClassType>(this->cache, name, niceName, g, hg);
  c->isTuple = isTuple;
  c->setSrcInfo(getSrcInfo());
  return c;
}

bool ClassType::hasUnbounds(bool includeGenerics) const {
  for (auto &t : generics)
    if (t.type && t.type->hasUnbounds(includeGenerics))
      return true;
  for (auto &t : hiddenGenerics)
    if (t.type && t.type->hasUnbounds(includeGenerics))
      return true;
  return false;
}

std::vector<Type *> ClassType::getUnbounds() const {
  std::vector<Type *> u;
  for (auto &t : generics)
    if (t.type) {
      auto tu = t.type->getUnbounds();
      u.insert(u.begin(), tu.begin(), tu.end());
    }
  for (auto &t : hiddenGenerics)
    if (t.type) {
      auto tu = t.type->getUnbounds();
      u.insert(u.begin(), tu.begin(), tu.end());
    }
  return u;
}

bool ClassType::canRealize() const {
  if (name == "type") {
    if (!hasUnbounds())
      return true; // always true!
  }
  if (name == "unrealized_type")
    return generics[0].type->getClass() != nullptr;
  return std::all_of(generics.begin(), generics.end(),
                     [](auto &t) { return !t.type || t.type->canRealize(); }) &&
         std::all_of(hiddenGenerics.begin(), hiddenGenerics.end(),
                     [](auto &t) { return !t.type || t.type->canRealize(); });
}

bool ClassType::isInstantiated() const {
  if (name == "unrealized_type")
    return generics[0].type->getClass() != nullptr;
  return std::all_of(generics.begin(), generics.end(),
                     [](auto &t) { return !t.type || t.type->isInstantiated(); }) &&
         std::all_of(hiddenGenerics.begin(), hiddenGenerics.end(),
                     [](auto &t) { return !t.type || t.type->isInstantiated(); });
}

ClassType *ClassType::getHeterogenousTuple() {
  seqassert(canRealize(), "{} not realizable", toString());
  seqassert(name == TYPE_TUPLE, "{} not a tuple", toString());
  if (generics.size() > 1) {
    std::string first = generics[0].type->realizedName();
    for (int i = 1; i < generics.size(); i++)
      if (generics[i].type->realizedName() != first)
        return getClass();
  }
  return nullptr;
}

std::string ClassType::debugString(char mode) const {
  if (name == "Partial" && generics[3].type->getClass()) {
    std::vector<std::string> as;
    auto known = getPartialMask();
    auto func = getPartialFunc();
    for (int i = 0, gi = 0; i < known.size(); i++) {
      if ((*func->ast)[i].isValue())
        as.emplace_back(
            known[i]
                ? generics[1].type->getClass()->generics[gi++].type->debugString(mode)
                : "...");
    }
    auto fnname = func->ast->getName();
    if (mode == 0) {
      fnname = cache->rev(func->ast->getName());
    } else if (mode == 2) {
      fnname = func->debugString(mode);
    }
    return fmt::format("{}[{}{}]", fnname, join(as, ","),
                       mode == 2
                           ? fmt::format(";{};{}", generics[1].type->debugString(mode),
                                         generics[2].type->debugString(mode))
                           : "");
  }
  std::vector<std::string> gs;
  for (auto &a : generics)
    if (!a.name.empty())
      gs.push_back(a.type->debugString(mode));
  if ((mode == 2) && !hiddenGenerics.empty()) {
    for (auto &a : hiddenGenerics)
      if (!a.name.empty())
        gs.push_back("-" + a.type->debugString(mode));
  }
  // Special formatting for Functions and Tuples
  auto n = mode == 0 ? niceName : name;
  return fmt::format("{}{}", n, gs.empty() ? "" : fmt::format("[{}]", join(gs, ",")));
}

std::string ClassType::realizedName() const {
  if (!_rn.empty())
    return _rn;

  std::string s;
  std::vector<std::string> gs;
  if (name == "Partial") {
    gs.push_back(generics[3].type->realizedName());
    for (size_t i = 0; i < generics.size() - 1; i++)
      gs.push_back(generics[i].type->realizedName());
  } else if (name == "Union" && generics[0].type->getClass()) {
    std::set<std::string> gss;
    for (auto &a : generics[0].type->getClass()->generics)
      gss.insert(a.type->realizedName());
    gs = {join(gss, " | ")};
  } else {
    for (auto &a : generics)
      if (!a.name.empty()) {
        if (!a.isStatic && a.type->getStatic()) {
          gs.push_back(a.type->getStatic()->name);
        } else {
          gs.push_back(a.type->realizedName());
        }
      }
  }
  s = join(gs, ",");
  s = fmt::format("{}{}", name, s.empty() ? "" : fmt::format("[{}]", s));
  return s;
}

FuncType *ClassType::getPartialFunc() const {
  seqassert(name == "Partial", "not a partial");
  auto n = generics[3].type->getClass()->generics[0].type;
  seqassert(n->getFunc(), "not a partial func");
  return n->getFunc();
}

std::vector<char> ClassType::getPartialMask() const {
  seqassert(name == "Partial", "not a partial");
  auto n = generics[0].type->getStrStatic()->value;
  std::vector<char> r(n.size(), 0);
  for (size_t i = 0; i < n.size(); i++)
    if (n[i] == '1')
      r[i] = 1;
  return r;
}

bool ClassType::isPartialEmpty() const {
  auto a = generics[1].type->getClass();
  auto ka = generics[2].type->getClass();
  return a->generics.size() == 1 && a->generics[0].type->getClass()->generics.empty() &&
         ka->generics[1].type->getClass()->generics.empty();
}

int ClassType::getRepeats() const {
  if (name == TYPE_TUPLE)
    return hiddenGenerics.front().getType()->getIntStatic()->value;
  return 1;
}

} // namespace codon::ast::types
