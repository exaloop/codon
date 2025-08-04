// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/types/class.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

std::string ClassType::Generic::debugString(char mode) const {
  if (!staticKind && type->getStatic() && mode != 2)
    return type->getStatic()->getNonStaticType()->debugString(mode);
  return type->debugString(mode);
}

std::string ClassType::Generic::realizedName() const {
  if (!staticKind && type->getStatic())
    return type->getStatic()->getNonStaticType()->realizedName();
  return type->realizedName();
}

ClassType::Generic ClassType::Generic::generalize(int atLevel) const {
  TypePtr t = nullptr;
  if (!staticKind && type && type->getStatic())
    t = type->getStatic()->getNonStaticType()->generalize(atLevel);
  else if (type)
    t = type->generalize(atLevel);
  return {name, t, id, staticKind};
}

ClassType::Generic
ClassType::Generic::instantiate(int atLevel, int *unboundCount,
                                std::unordered_map<int, TypePtr> *cache) const {
  TypePtr t = nullptr;
  if (!staticKind && type && type->getStatic())
    t = type->getStatic()->getNonStaticType()->instantiate(atLevel, unboundCount,
                                                           cache);
  else if (type)
    t = type->instantiate(atLevel, unboundCount, cache);
  return {name, t, id, staticKind};
}

ClassType::ClassType(Cache *cache, std::string name, std::vector<Generic> generics,
                     std::vector<Generic> hiddenGenerics)
    : Type(cache), name(std::move(name)), generics(std::move(generics)),
      hiddenGenerics(std::move(hiddenGenerics)) {}
ClassType::ClassType(const ClassType *base)
    : Type(*base), name(base->name), generics(base->generics),
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

    int s1 = 3, s = 0;
    if (name == "__NTuple__" && tc->name == name) {
      auto n1 = generics[0].getType()->getIntStatic();
      auto n2 = tc->generics[0].getType()->getIntStatic();
      if (n1 && n2) {
        auto t1 = generics[1].getType()->getClass();
        auto t2 = tc->generics[1].getType()->getClass();
        seqassert(t1 && t2, "bad ntuples");
        if (n1->value * t1->generics.size() != n2->value * t2->generics.size())
          return -1;
        for (size_t i = 0; i < t1->generics.size() * n1->value; i++) {
          if ((s = t1->generics[i % t1->generics.size()].getType()->unify(
                   t2->generics[i % t2->generics.size()].getType(), us)) == -1)
            return -1;
          s1 += s;
        }
        return s1;
      }
    } else if (tc->name == "__NTuple__") {
      return tc->unify(this, us);
    } else if (name == "__NTuple__" && tc->name == TYPE_TUPLE) {
      auto n1 = generics[0].getType()->getIntStatic();
      if (!n1) {
        auto n = tc->generics.size();
        auto tn = std::make_shared<IntStaticType>(cache, n);
        // If we are unifying NT[N, T] and T[X, X, ...], we assume that N is number of
        // X's
        if (generics[0].type->unify(tn.get(), us) == -1)
          return -1;

        auto tv = TypecheckVisitor(cache->typeCtx);
        TypePtr tt;
        if (n) {
          tt = tv.instantiateType(tv.generateTuple(1), {tc->generics[0].getType()});
          for (size_t i = 1; i < tc->generics.size(); i++) {
            if ((s = tt->getClass()->generics[0].getType()->unify(
                     tc->generics[i].getType(), us)) == -1)
              return -1;
            s1 += s;
          }
        } else {
          tt = tv.instantiateType(tv.generateTuple(1));
          // tt = tv.instantiateType(tv.generateTuple(0));
        }
        if (generics[1].type->unify(tt.get(), us) == -1)
          return -1;
      } else {
        auto t1 = generics[1].getType()->getClass();
        seqassert(t1, "bad ntuples");
        if (n1->value * t1->generics.size() != tc->generics.size())
          return -1;
        for (size_t i = 0; i < t1->generics.size() * n1->value; i++) {
          if ((s = t1->generics[i % t1->generics.size()].getType()->unify(
                   tc->generics[i].getType(), us)) == -1)
            return -1;
          s1 += s;
        }
      }
      return s1;
    }

    // Check names.
    if (name != tc->name) {
      return -1;
    }
    // Check generics.
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

TypePtr ClassType::generalize(int atLevel) const {
  std::vector<Generic> g, hg;
  for (auto &t : generics)
    g.push_back(t.generalize(atLevel));
  for (auto &t : hiddenGenerics)
    hg.push_back(t.generalize(atLevel));
  auto c = std::make_shared<ClassType>(cache, name, g, hg);
  c->isTuple = isTuple;
  c->setSrcInfo(getSrcInfo());
  return c;
}

TypePtr ClassType::instantiate(int atLevel, int *unboundCount,
                               std::unordered_map<int, TypePtr> *cache) const {
  std::vector<Generic> g, hg;
  for (auto &t : generics)
    g.push_back(t.instantiate(atLevel, unboundCount, cache));
  for (auto &t : hiddenGenerics)
    hg.push_back(t.instantiate(atLevel, unboundCount, cache));
  auto c = std::make_shared<ClassType>(this->cache, name, g, hg);
  c->isTuple = isTuple;
  c->setSrcInfo(getSrcInfo());
  return c;
}

bool ClassType::hasUnbounds(bool includeGenerics) const {
  if (name == "unrealized_type")
    return false;
  auto pred = [includeGenerics](const auto &t) {
    return t.type && t.type->hasUnbounds(includeGenerics);
  };
  return std::ranges::any_of(generics.begin(), generics.end(), pred) ||
         std::ranges::any_of(hiddenGenerics.begin(), hiddenGenerics.end(), pred);
}

std::vector<Type *> ClassType::getUnbounds(bool includeGenerics) const {
  std::vector<Type *> u;
  if (name == "unrealized_type")
    return u;
  for (auto &t : generics)
    if (t.type) {
      auto tu = t.type->getUnbounds(includeGenerics);
      u.insert(u.begin(), tu.begin(), tu.end());
    }
  for (auto &t : hiddenGenerics)
    if (t.type) {
      auto tu = t.type->getUnbounds(includeGenerics);
      u.insert(u.begin(), tu.begin(), tu.end());
    }
  return u;
}

bool ClassType::canRealize() const {
  if (name == "type") {
    if (!hasUnbounds(false))
      return true; // always true!
  }
  if (name == "unrealized_type")
    return generics[0].type->getClass() != nullptr;
  auto pred = [](auto &t) { return !t.type || t.type->canRealize(); };
  return std::ranges::all_of(generics.begin(), generics.end(), pred) &&
         std::ranges::all_of(hiddenGenerics.begin(), hiddenGenerics.end(), pred);
}

bool ClassType::isInstantiated() const {
  if (name == "unrealized_type")
    return generics[0].type->getClass() != nullptr;
  auto pred = [](auto &t) { return !t.type || t.type->isInstantiated(); };
  return std::ranges::all_of(generics.begin(), generics.end(), pred) &&
         std::ranges::all_of(hiddenGenerics.begin(), hiddenGenerics.end(), pred);
}

std::string ClassType::debugString(char mode) const {
  if (name == "NamedTuple") {
    if (auto ids = generics[0].type->getIntStatic()) {
      auto id = ids->value;
      seqassert(id >= 0 && id < cache->generatedTupleNames.size(), "bad id: {}", id);
      const auto &names = cache->generatedTupleNames[id];
      auto ts = generics[1].getType()->getClass();
      if (names.empty())
        return name;
      std::vector<std::string> as;
      for (size_t i = 0; i < names.size(); i++)
        as.push_back(names[i] + "=" + ts->generics[i].debugString(mode));
      return name + "[" + join(as, ",") + "]";
    } else {
      return name + "[" + generics[0].type->debugString(mode) + "]";
    }
  } else if (name == "Partial" && generics[3].type->getClass() && mode != 2) {
    // Name: function[full_args](instantiated_args...)
    std::vector<std::string> as;
    auto known = getPartialMask();
    auto func = getPartialFunc();

    std::vector<std::string> args;
    if (auto ta = generics[1].type->getClass())
      for (const auto &i : ta->generics)
        args.push_back(i.debugString(mode));
    size_t ai = 0, gi = 0;
    for (size_t i = 0; i < known.size(); i++) {
      if ((*func->ast)[i].isValue()) {
        as.emplace_back(ai < args.size() ? (known[i] == ClassType::PartialFlag::Included
                                                ? args[ai]
                                                : ("..." + (mode == 0 ? "" : args[ai])))
                                         : "...");
        if (known[i] == ClassType::PartialFlag::Included)
          ai++;
      } else {
        auto s = func->funcGenerics[gi].debugString(mode);
        as.emplace_back((known[i] == ClassType::PartialFlag::Included
                             ? s
                             : ("..." + (mode == 0 ? "" : s))));
        gi++;
      }
    }
    if (!args.empty()) {
      if (args.back() != "Tuple") // unused *args (by default always 0 in mask)
        as.push_back(args.back());
    }
    auto ks = generics[2].type->debugString(mode);
    if (ks.size() > 10) {                 // if **kwargs is used
      ks = ks.substr(11, ks.size() - 12); // chop off NamedTuple[...]
      as.push_back(ks);
    }

    auto fnname = func->ast->getName();
    if (mode == 0) {
      fnname = cache->rev(func->ast->getName());
    } else {
      fnname = func->ast->getName();
    }
    return fnname + "(" + join(as, ",") + ")";
  }
  std::vector<std::string> gs;
  for (auto &a : generics)
    if (!a.name.empty())
      gs.push_back(a.debugString(mode));
  if ((mode == 2) && !hiddenGenerics.empty()) {
    for (auto &a : hiddenGenerics)
      if (!a.name.empty())
        gs.push_back("-" + a.debugString(mode));
  }
  // Special formatting for Functions and Tuples
  auto n = mode == 0 ? cache->rev(name) : name;
  return n + (gs.empty() ? "" : ("[" + join(gs, ",") + "]"));
}

std::string ClassType::realizedName() const {
  if (!_rn.empty())
    return _rn;

  std::string s;
  if (name == "Partial") {
    s = debugString(1);
  } else {
    std::vector<std::string> gs;
    if (name == "Union" && generics[0].type->getClass()) {
      std::set<std::string> gss;
      for (auto &a : generics[0].type->getClass()->generics)
        gss.insert(a.realizedName());
      gs = {join(gss, " | ")};
    } else {
      for (auto &a : generics)
        if (!a.name.empty())
          gs.push_back(a.realizedName());
    }
    s = join(gs, ",");
    s = name + (s.empty() ? "" : ("[" + s + "]"));
  }

  if (canRealize())
    const_cast<ClassType *>(this)->_rn = s;

  return s;
}

FuncType *ClassType::getPartialFunc() const {
  seqassert(name == "Partial", "not a partial");
  auto n = generics[3].type->getClass()->generics[0].type;
  seqassert(n->getFunc(), "not a partial func");
  return n->getFunc();
}

std::string ClassType::getPartialMask() const {
  seqassert(name == "Partial", "not a partial");
  auto n = generics[0].type->getStrStatic()->value;
  return n;
}

bool ClassType::isPartialEmpty() const {
  auto a = generics[1].type->getClass();
  auto ka = generics[2].type->getClass();
  return a->generics.size() == 1 && a->generics[0].type->getClass()->generics.empty() &&
         ka->generics[1].type->getClass()->generics.empty();
}

} // namespace codon::ast::types
