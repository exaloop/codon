// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

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
  if (typ->getClass()) {
    if (typ->is(StdlibTypes::NoneType))
      return 1;
    if (args.empty())
      return typ->is(StdlibTypes::Function) || typ->getPartial() ||
                     typ->is(StdlibTypes::TypeWrap)
                 ? 1
                 : -1;
    TypecheckVisitor tv(cache->typeCtx);
    std::vector<CallArg> callArgs;
    for (const auto &argument : args[0]->getClass()->generics) {
      auto value = tv.N<NoneExpr>();
      value->setType(argument.type);
      callArgs.emplace_back(value);
    }
    return tv.matchCallable(typ, callArgs, args[1].get(), us) >= 0 ? 1 : -1;
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
