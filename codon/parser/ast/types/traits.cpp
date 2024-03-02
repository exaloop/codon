// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
  if (auto tr = typ->getRecord()) {
    if (tr->name == "NoneType")
      return 1;
    if (tr->name != "Function" && !tr->getPartial())
      return -1;
    if (args.empty())
      return 1;

    std::vector<char> known;
    auto trFun = tr;
    if (auto pt = tr->getPartial()) {
      int ic = 0;
      std::unordered_map<int, TypePtr> c;
      trFun = pt->func->instantiate(0, &ic, &c)->getRecord();
      known = pt->known;
    } else {
      known = std::vector<char>(tr->generics[0].type->getRecord()->args.size(), 0);
    }

    auto &inArgs = args[0]->getRecord()->args;
    auto &trInArgs = trFun->generics[0].type->getRecord()->args;
    auto trAst = trFun->getFunc() ? trFun->getFunc()->ast : nullptr;
    size_t star = trInArgs.size(), kwStar = trInArgs.size();
    size_t total = 0;
    if (trAst) {
      star = trAst->getStarArgs();
      kwStar = trAst->getKwStarArgs();
      if (kwStar < trAst->args.size() && star >= trInArgs.size())
        star -= 1;
      size_t preStar = 0;
      for (size_t fi = 0; fi < trAst->args.size(); fi++) {
        if (fi != kwStar && !known[fi] && trAst->args[fi].status == Param::Normal) {
          total++;
          if (fi < star)
            preStar++;
        }
      }
      if (preStar < total) {
        if (inArgs.size() < preStar)
          return -1;
      } else if (inArgs.size() != total) {
        return -1;
      }
    } else {
      total = star = trInArgs.size();
      if (inArgs.size() != total)
        return -1;
    }
    size_t i = 0;
    for (size_t fi = 0; i < inArgs.size() && fi < star; fi++) {
      if (!known[fi] && trAst->args[fi].status == Param::Normal) {
        if (inArgs[i++]->unify(trInArgs[fi].get(), us) == -1)
          return -1;
      }
    }
    // NOTE: *args / **kwargs types will be typecheck when the function is called
    if (auto pf = trFun->getFunc()) {
      // Make sure to set types of *args/**kwargs so that the function that
      // is being unified with Callable[] can be realized

      if (star < trInArgs.size() - (kwStar < trInArgs.size())) {
        std::vector<TypePtr> starArgTypes;
        if (auto tp = tr->getPartial()) {
          auto ts = tp->args[tp->args.size() - 2]->getRecord();
          seqassert(ts, "bad partial *args/**kwargs");
          starArgTypes = ts->args;
        }
        starArgTypes.insert(starArgTypes.end(), inArgs.begin() + i, inArgs.end());

        auto tv = TypecheckVisitor(cache->typeCtx);
        auto t = cache->typeCtx->instantiateTuple(starArgTypes)->getClass();
        if (t->unify(trInArgs[star].get(), us) == -1)
          return -1;
      }
      if (kwStar < trInArgs.size()) {
        auto tv = TypecheckVisitor(cache->typeCtx);
        std::vector<std::string> names;
        std::vector<TypePtr> starArgTypes;
        if (auto tp = tr->getPartial()) {
          auto ts = tp->args.back()->getRecord();
          seqassert(ts, "bad partial *args/**kwargs");
          auto ff = tv.getClassFields(ts.get());
          for (size_t i = 0; i < ts->args.size(); i++) {
            names.emplace_back(ff[i].name);
            starArgTypes.emplace_back(ts->args[i]);
          }
        }
        auto name = tv.generateTuple(starArgTypes.size(), TYPE_KWTUPLE, names);
        auto t = cache->typeCtx->forceFind(name)->type;
        t = cache->typeCtx->instantiateGeneric(t, starArgTypes)->getClass();
        if (t->unify(trInArgs[kwStar].get(), us) == -1)
          return -1;
      }

      if (us && pf->canRealize()) {
        // Realize if possible to allow deduction of return type
        auto rf = TypecheckVisitor(cache->typeCtx).realize(pf);
        pf->unify(rf.get(), us);
      }
      if (args[1]->unify(pf->getRetType().get(), us) == -1)
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

TypePtr CallableTrait::generalize(int atLevel) {
  auto g = args;
  for (auto &t : g)
    t = t ? t->generalize(atLevel) : nullptr;
  auto c = std::make_shared<CallableTrait>(cache, g);
  c->setSrcInfo(getSrcInfo());
  return c;
}

TypePtr CallableTrait::instantiate(int atLevel, int *unboundCount,
                                   std::unordered_map<int, TypePtr> *cache) {
  auto g = args;
  for (auto &t : g)
    t = t ? t->instantiate(atLevel, unboundCount, cache) : nullptr;
  auto c = std::make_shared<CallableTrait>(this->cache, g);
  c->setSrcInfo(getSrcInfo());
  return c;
}

std::string CallableTrait::debugString(char mode) const {
  auto s = args[0]->debugString(mode);
  return fmt::format("Callable[{},{}]", startswith(s, "Tuple") ? s.substr(5) : s,
                     args[1]->debugString(mode));
}

TypeTrait::TypeTrait(TypePtr typ) : Trait(typ), type(std::move(typ)) {}

int TypeTrait::unify(Type *typ, Unification *us) { return typ->unify(type.get(), us); }

TypePtr TypeTrait::generalize(int atLevel) {
  auto c = std::make_shared<TypeTrait>(type->generalize(atLevel));
  c->setSrcInfo(getSrcInfo());
  return c;
}

TypePtr TypeTrait::instantiate(int atLevel, int *unboundCount,
                               std::unordered_map<int, TypePtr> *cache) {
  auto c = std::make_shared<TypeTrait>(type->instantiate(atLevel, unboundCount, cache));
  c->setSrcInfo(getSrcInfo());
  return c;
}

std::string TypeTrait::debugString(char mode) const {
  return fmt::format("Trait[{}]", type->debugString(mode));
}

} // namespace codon::ast::types
