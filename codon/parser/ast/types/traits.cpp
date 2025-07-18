// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

Trait::Trait(const std::shared_ptr<Type> &type) : Type(type) {}

Trait::Trait(Cache *cache) : Type(cache) {}

bool Trait::canRealize() const { return false; }

bool Trait::isInstantiated() const { return false; }

std::string Trait::realizedName() const { return ""; }

CallableTrait::CallableTrait(Cache *cache, std::vector<TypePtr> args)
    : Trait(cache), args(std::move(args)) {}

int CallableTrait::unify(Type *typ, Unification *us) {
  /// TODO: one day merge with the CallExpr's logic...
  if (auto tr = typ->getClass()) {
    TypePtr ft = nullptr;
    if (typ->is("TypeWrap")) {
      TypecheckVisitor tv(cache->typeCtx);
      ft = tv.instantiateType(
          tv.findMethod(typ->getClass(), "__call_no_self__").front(), typ->getClass());
      tr = ft->getClass();
    }

    if (tr->name == "NoneType")
      return 1;
    if (tr->name != "Function" && !tr->getPartial())
      return -1;
    if (!tr->isRecord())
      return -1;
    if (args.empty())
      return 1;

    std::string known;
    TypePtr func = nullptr; // trFun can point to it
    auto trFun = tr;
    if (auto pt = tr->getPartial()) {
      int ic = 0;
      std::unordered_map<int, TypePtr> c;
      func = pt->getPartialFunc()->instantiate(0, &ic, &c);
      trFun = func->getClass();
      known = pt->getPartialMask();

      auto knownArgTypes = pt->generics[1].type->getClass();
      for (size_t i = 0, j = 0, k = 0; i < known.size(); i++)
        if ((*func->getFunc()->ast)[i].isGeneric()) {
          j++;
        } else if (known[i] == ClassType::PartialFlag::Included) {
          if ((*func->getFunc())[i - j]->unify(knownArgTypes->generics[k].type.get(),
                                               us) == -1)
            return -1;
          k++;
        }
    } else {
      known = std::string(tr->generics[0].type->getClass()->generics.size(),
                          ClassType::PartialFlag::Missing);
    }

    auto inArgs = args[0]->getClass();
    auto trInArgs = trFun->generics[0].type->getClass();
    auto trAst = trFun->getFunc() ? trFun->getFunc()->ast : nullptr;
    size_t star = 0, kwStar = trInArgs->generics.size();
    size_t total = 0;
    if (trAst) {
      star = trAst->getStarArgs();
      kwStar = trAst->getKwStarArgs();
      for (size_t fi = 0; fi < trAst->size(); fi++) {
        if (fi < star && !(*trAst)[fi].isValue())
          star--;
        if (fi < kwStar && !(*trAst)[fi].isValue())
          kwStar--;
      }
      if (kwStar < trAst->size() && star >= trInArgs->generics.size())
        star -= 1;
      size_t preStar = 0;
      for (size_t fi = 0; fi < trAst->size(); fi++) {
        if (fi != kwStar && known[fi] != ClassType::PartialFlag::Included &&
            (*trAst)[fi].isValue() && !startswith((*trAst)[fi].getName(), "$")) {
          total++;
          if (fi < star)
            preStar++;
        }
      }
      if (preStar < total) {
        if (inArgs->generics.size() < preStar)
          return -1;
      } else if (inArgs->generics.size() != total) {
        return -1;
      }
    } else {
      total = star = trInArgs->generics.size();
      if (inArgs->generics.size() != total)
        return -1;
    }
    size_t i = 0;
    for (size_t fi = 0; i < inArgs->generics.size() && fi < star; fi++) {
      if (known[fi] != ClassType::PartialFlag::Included && trAst &&
          (*trAst)[fi].isValue() && !startswith((*trAst)[fi].getName(), "$")) {
        if (inArgs->generics[i++].type->unify(trInArgs->generics[fi].type.get(), us) ==
            -1)
          return -1;
      }
    }
    auto tv = TypecheckVisitor(cache->typeCtx);
    if (auto pf = trFun->getFunc()) {
      // Make sure to set types of *args/**kwargs so that the function that
      // is being unified with CallableTrait[] can be realized
      if (star < trInArgs->generics.size() - (kwStar < trInArgs->generics.size())) {
        std::vector<Type *> starArgTypes;
        if (auto tp = tr->getPartial()) {
          auto ts = tp->generics[1].type->getClass();
          seqassert(ts && !ts->generics.empty() &&
                        ts->generics[ts->generics.size() - 1].type->getClass(),
                    "bad partial *args/**kwargs");
          for (auto &tt :
               ts->generics[ts->generics.size() - 1].type->getClass()->generics)
            starArgTypes.push_back(tt.getType());
        }
        for (; i < inArgs->generics.size(); i++)
          starArgTypes.push_back(inArgs->generics[i].getType());
        if ((*pf->ast)[star].getType()) {
          // if we have *args: type, use those types
          auto starTyp =
              tv.extractType(tv.transform(clone((*pf->ast)[star].getType())));
          for (auto &t : starArgTypes)
            t = starTyp;
        }

        auto tn =
            tv.instantiateType(tv.generateTuple(starArgTypes.size()), starArgTypes);
        if (tn->unify(trInArgs->generics[star].type.get(), us) == -1)
          return -1;
      }
      if (kwStar < trInArgs->generics.size()) {
        auto tt = tv.generateTuple(0);
        size_t id = 0;
        if (auto tp = tr->getPartial()) {
          auto ts = tp->generics[2].type->getClass();
          seqassert(ts && ts->is("NamedTuple"), "bad partial *args/**kwargs");
          id = ts->generics[0].type->getIntStatic()->value;
          tt = ts->generics[1].getType()->getClass();
        }
        auto tid = std::make_shared<IntStaticType>(cache, id);
        auto kt = tv.instantiateType(tv.getStdLibType("NamedTuple"), {tid.get(), tt});
        if (kt->unify(trInArgs->generics[kwStar].type.get(), us) == -1)
          return -1;
      }

      if (us && pf->canRealize()) {
        // Realize if possible to allow deduction of return type
        auto rf = tv.realize(pf);
        pf->unify(rf, us);
      }
      if (args[1]->unify(pf->getRetType(), us) == -1)
        return -1;
    }
    return 1;
  } else if (auto tl = typ->getLink()) {
    if (tl->kind == LinkType::Link)
      return unify(tl->type.get(), us);
    if (tl->kind == LinkType::Unbound) {
      if (tl->trait) {
        auto tt = dynamic_cast<CallableTrait *>(tl->trait.get());
        if (!tt || tt->args.size() != args.size())
          return -1;
        for (int i = 0; i < args.size(); i++)
          if (args[i]->unify(tt->args[i].get(), us) == -1)
            return -1;
      }
      return 1;
    }
  }
  return -1;
}

TypePtr CallableTrait::generalize(int atLevel) const {
  auto g = args;
  for (auto &t : g)
    t = t ? t->generalize(atLevel) : nullptr;
  auto c = std::make_shared<CallableTrait>(cache, g);
  c->setSrcInfo(getSrcInfo());
  return c;
}

TypePtr CallableTrait::instantiate(int atLevel, int *unboundCount,
                                   std::unordered_map<int, TypePtr> *cache) const {
  auto g = args;
  for (auto &t : g)
    t = t ? t->instantiate(atLevel, unboundCount, cache) : nullptr;
  auto c = std::make_shared<CallableTrait>(this->cache, g);
  c->setSrcInfo(getSrcInfo());
  return c;
}

std::string CallableTrait::debugString(char mode) const {
  auto s = args[0]->debugString(mode);
  return fmt::format("CallableTrait[{},{}]", startswith(s, "Tuple") ? s.substr(5) : s,
                     args[1]->debugString(mode));
}

TypeTrait::TypeTrait(TypePtr typ) : Trait(typ), type(std::move(typ)) {}

int TypeTrait::unify(Type *typ, Unification *us) {
  if (typ->getClass()) {
    // does not make sense otherwise and results in infinite cycles
    return typ->unify(type.get(), us);
  }
  if (typ->getUnbound())
    return 0;
  return -1;
}

TypePtr TypeTrait::generalize(int atLevel) const {
  auto c = std::make_shared<TypeTrait>(type->generalize(atLevel));
  c->setSrcInfo(getSrcInfo());
  return c;
}

TypePtr TypeTrait::instantiate(int atLevel, int *unboundCount,
                               std::unordered_map<int, TypePtr> *cache) const {
  auto c = std::make_shared<TypeTrait>(type->instantiate(atLevel, unboundCount, cache));
  c->setSrcInfo(getSrcInfo());
  return c;
}

std::string TypeTrait::debugString(char mode) const {
  return fmt::format("Trait[{}]", type->getClass() ? type->getClass()->name : "-");
}

} // namespace codon::ast::types
