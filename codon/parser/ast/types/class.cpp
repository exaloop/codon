// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/types/class.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

ClassType::ClassType(Cache *cache, std::string name, std::string niceName,
                     std::vector<Generic> generics, std::vector<Generic> hiddenGenerics)
    : Type(cache), name(move(name)), niceName(move(niceName)), generics(move(generics)),
      hiddenGenerics(move(hiddenGenerics)) {}
ClassType::ClassType(const ClassTypePtr &base)
    : Type(base), name(base->name), niceName(base->niceName), generics(base->generics),
      hiddenGenerics(base->hiddenGenerics) {}

int ClassType::unify(Type *typ, Unification *us) {
  if (auto tc = typ->getClass()) {
    // Check names.
    if (name != tc->name)
      return -1;
    // Check generics.
    int s1 = 3, s = 0;
    if (generics.size() != tc->generics.size())
      return -1;
    for (int i = 0; i < generics.size(); i++) {
      if ((s = generics[i].type->unify(tc->generics[i].type.get(), us)) == -1)
        return -1;
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
  c->setSrcInfo(getSrcInfo());
  return c;
}

std::vector<TypePtr> ClassType::getUnbounds() const {
  std::vector<TypePtr> u;
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
  return std::all_of(generics.begin(), generics.end(),
                     [](auto &t) { return !t.type || t.type->canRealize(); }) &&
         std::all_of(hiddenGenerics.begin(), hiddenGenerics.end(),
                     [](auto &t) { return !t.type || t.type->canRealize(); });
}

bool ClassType::isInstantiated() const {
  return std::all_of(generics.begin(), generics.end(),
                     [](auto &t) { return !t.type || t.type->isInstantiated(); }) &&
         std::all_of(hiddenGenerics.begin(), hiddenGenerics.end(),
                     [](auto &t) { return !t.type || t.type->isInstantiated(); });
}

std::string ClassType::debugString(char mode) const {
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
  if (startswith(n, TYPE_TUPLE))
    n = "Tuple";
  return fmt::format("{}{}", n, gs.empty() ? "" : fmt::format("[{}]", join(gs, ",")));
}

std::string ClassType::realizedName() const {
  if (!_rn.empty())
    return _rn;

  std::vector<std::string> gs;
  for (auto &a : generics)
    if (!a.name.empty())
      gs.push_back(a.type->realizedName());
  std::string s = join(gs, ",");
  if (canRealize())
    const_cast<ClassType *>(this)->_rn =
        fmt::format("{}{}", name, s.empty() ? "" : fmt::format("[{}]", s));
  return _rn;
}

std::string ClassType::realizedTypeName() const {
  return this->ClassType::realizedName();
}

RecordType::RecordType(Cache *cache, std::string name, std::string niceName,
                       std::vector<Generic> generics, std::vector<TypePtr> args)
    : ClassType(cache, std::move(name), std::move(niceName), std::move(generics)),
      args(std::move(args)) {}

RecordType::RecordType(const ClassTypePtr &base, std::vector<TypePtr> args)
    : ClassType(base), args(std::move(args)) {}

int RecordType::unify(Type *typ, Unification *us) {
  if (auto tr = typ->getRecord()) {
    // Handle int <-> Int[64]
    if (name == "int" && tr->name == "Int")
      return tr->unify(this, us);
    if (tr->name == "int" && name == "Int") {
      auto t64 = std::make_shared<StaticType>(cache, 64);
      return generics[0].type->unify(t64.get(), us);
    }

    int s1 = 2, s = 0;
    if (args.size() != tr->args.size())
      return -1;
    for (int i = 0; i < args.size(); i++) {
      if ((s = args[i]->unify(tr->args[i].get(), us)) != -1)
        s1 += s;
      else
        return -1;
    }
    // Handle Tuple<->@tuple: when unifying tuples, only record members matter.
    if (startswith(name, TYPE_TUPLE) || startswith(tr->name, TYPE_TUPLE)) {
      return s1 + int(name == tr->name);
    }
    return this->ClassType::unify(tr.get(), us);
  } else if (auto t = typ->getLink()) {
    return t->unify(this, us);
  } else {
    return -1;
  }
}

TypePtr RecordType::generalize(int atLevel) {
  auto c = std::static_pointer_cast<ClassType>(this->ClassType::generalize(atLevel));
  auto a = args;
  for (auto &t : a)
    t = t->generalize(atLevel);
  return std::make_shared<RecordType>(c, a);
}

TypePtr RecordType::instantiate(int atLevel, int *unboundCount,
                                std::unordered_map<int, TypePtr> *cache) {
  auto c = std::static_pointer_cast<ClassType>(
      this->ClassType::instantiate(atLevel, unboundCount, cache));
  auto a = args;
  for (auto &t : a)
    t = t->instantiate(atLevel, unboundCount, cache);
  return std::make_shared<RecordType>(c, a);
}

std::vector<TypePtr> RecordType::getUnbounds() const {
  std::vector<TypePtr> u;
  for (auto &a : args) {
    auto tu = a->getUnbounds();
    u.insert(u.begin(), tu.begin(), tu.end());
  }
  auto tu = this->ClassType::getUnbounds();
  u.insert(u.begin(), tu.begin(), tu.end());
  return u;
}

bool RecordType::canRealize() const {
  return std::all_of(args.begin(), args.end(),
                     [](auto &a) { return a->canRealize(); }) &&
         this->ClassType::canRealize();
}

bool RecordType::isInstantiated() const {
  return std::all_of(args.begin(), args.end(),
                     [](auto &a) { return a->isInstantiated(); }) &&
         this->ClassType::isInstantiated();
}

std::string RecordType::debugString(char mode) const {
  return fmt::format("{}", this->ClassType::debugString(mode));
}

std::shared_ptr<RecordType> RecordType::getHeterogenousTuple() {
  seqassert(canRealize(), "{} not realizable", toString());
  if (args.size() > 1) {
    std::string first = args[0]->realizedName();
    for (int i = 1; i < args.size(); i++)
      if (args[i]->realizedName() != first)
        return getRecord();
  }
  return nullptr;
}

} // namespace codon::ast::types
