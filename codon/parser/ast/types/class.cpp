// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/types/class.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

ClassType::ClassType(Cache *cache, std::string name, std::string niceName,
                     std::vector<Generic> generics, std::vector<Generic> hiddenGenerics)
    : Type(cache), name(std::move(name)), niceName(std::move(niceName)),
      generics(std::move(generics)), hiddenGenerics(std::move(hiddenGenerics)) {}
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
                       std::vector<Generic> generics, std::vector<TypePtr> args,
                       bool noTuple, const std::shared_ptr<StaticType> &repeats)
    : ClassType(cache, std::move(name), std::move(niceName), std::move(generics)),
      args(std::move(args)), noTuple(false), repeats(repeats) {}

RecordType::RecordType(const ClassTypePtr &base, std::vector<TypePtr> args,
                       bool noTuple, const std::shared_ptr<StaticType> &repeats)
    : ClassType(base), args(std::move(args)), noTuple(noTuple), repeats(repeats) {}

int RecordType::unify(Type *typ, Unification *us) {
  if (auto tr = typ->getRecord()) {
    // Handle int <-> Int[64]
    if (name == "int" && tr->name == "Int")
      return tr->unify(this, us);
    if (tr->name == "int" && name == "Int") {
      auto t64 = std::make_shared<StaticType>(cache, 64);
      return generics[0].type->unify(t64.get(), us);
    }

    // TODO: we now support very limited unification strategy where repetitions must
    // match. We should expand this later on...
    if (repeats || tr->repeats) {
      if (!repeats && tr->repeats) {
        auto n = std::make_shared<StaticType>(cache, args.size());
        if (tr->repeats->unify(n.get(), us) == -1)
          return -1;
      } else if (!tr->repeats) {
        auto n = std::make_shared<StaticType>(cache, tr->args.size());
        if (repeats->unify(n.get(), us) == -1)
          return -1;
      } else {
        if (repeats->unify(tr->repeats.get(), us) == -1)
          return -1;
      }
    }
    if (getRepeats() != -1)
      flatten();
    if (tr->getRepeats() != -1)
      tr->flatten();

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
    if (name == TYPE_TUPLE || tr->name == TYPE_TUPLE) {
      if (!args.empty() || (!noTuple && !tr->noTuple)) // prevent POD<->() unification
        return s1 + int(name == tr->name);
      else
        return -1;
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
  auto r = repeats ? repeats->generalize(atLevel)->getStatic() : nullptr;
  return std::make_shared<RecordType>(c, a, noTuple, r);
}

TypePtr RecordType::instantiate(int atLevel, int *unboundCount,
                                std::unordered_map<int, TypePtr> *cache) {
  auto c = std::static_pointer_cast<ClassType>(
      this->ClassType::instantiate(atLevel, unboundCount, cache));
  auto a = args;
  for (auto &t : a)
    t = t->instantiate(atLevel, unboundCount, cache);
  auto r = repeats ? repeats->instantiate(atLevel, unboundCount, cache)->getStatic()
                   : nullptr;
  return std::make_shared<RecordType>(c, a, noTuple, r);
}

std::vector<TypePtr> RecordType::getUnbounds() const {
  std::vector<TypePtr> u;
  if (repeats) {
    auto tu = repeats->getUnbounds();
    u.insert(u.begin(), tu.begin(), tu.end());
  }
  for (auto &a : args) {
    auto tu = a->getUnbounds();
    u.insert(u.begin(), tu.begin(), tu.end());
  }
  auto tu = this->ClassType::getUnbounds();
  u.insert(u.begin(), tu.begin(), tu.end());
  return u;
}

bool RecordType::canRealize() const {
  return getRepeats() >= 0 &&
         std::all_of(args.begin(), args.end(),
                     [](auto &a) { return a->canRealize(); }) &&
         this->ClassType::canRealize();
}

bool RecordType::isInstantiated() const {
  return (!repeats || repeats->isInstantiated()) &&
         std::all_of(args.begin(), args.end(),
                     [](auto &a) { return a->isInstantiated(); }) &&
         this->ClassType::isInstantiated();
}

std::string RecordType::realizedName() const {
  if (!_rn.empty())
    return _rn;
  if (name == TYPE_TUPLE) {
    std::vector<std::string> gs;
    auto n = getRepeats();
    if (n == -1)
      gs.push_back(repeats->realizedName());
    for (int i = 0; i < std::max(n, int64_t(0)); i++)
      for (auto &a : args)
        gs.push_back(a->realizedName());
    std::string s = join(gs, ",");
    if (canRealize())
      const_cast<RecordType *>(this)->_rn =
          fmt::format("{}{}", name, s.empty() ? "" : fmt::format("[{}]", s));
    return _rn;
  }
  return ClassType::realizedName();
}

std::string RecordType::debugString(char mode) const {
  if (name == TYPE_TUPLE) {
    std::vector<std::string> gs;
    auto n = getRepeats();
    if (n == -1)
      gs.push_back(repeats->debugString(mode));
    for (int i = 0; i < std::max(n, int64_t(0)); i++)
      for (auto &a : args)
        gs.push_back(a->debugString(mode));
    return fmt::format("{}{}", name,
                       gs.empty() ? "" : fmt::format("[{}]", join(gs, ",")));
  } else {
    return fmt::format("{}{}", repeats ? repeats->debugString(mode) + "," : "",
                       this->ClassType::debugString(mode));
  }
}

std::string RecordType::realizedTypeName() const { return realizedName(); }

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

/// Returns -1 if the type cannot be realized yet
int64_t RecordType::getRepeats() const {
  if (!repeats)
    return 1;
  if (repeats->canRealize())
    return std::max(repeats->evaluate().getInt(), int64_t(0));
  return -1;
}

void RecordType::flatten() {
  auto n = getRepeats();
  seqassert(n >= 0, "bad call to flatten");

  auto a = args;
  args.clear();
  for (int64_t i = 0; i < n; i++)
    args.insert(args.end(), a.begin(), a.end());

  repeats = nullptr;
}

} // namespace codon::ast::types
