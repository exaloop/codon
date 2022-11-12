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
    : Trait(cache), args(move(args)) {}

int CallableTrait::unify(Type *typ, Unification *us) {
  if (auto tr = typ->getRecord()) {
    if (tr->name == "NoneType")
      return 1;
    if (args.empty())
      return (tr->name == "Function" || tr->getPartial()) ? 1 : -1;
    auto &inargs = args[0]->getRecord()->args;
    if (tr->name == "Function") {
      // Handle Callable[...]<->Function[...].
      if (args.size() != tr->generics.size())
        return -1;
      if (inargs.size() != tr->generics[0].type->getRecord()->args.size())
        return -1;
      for (int i = 0; i < inargs.size(); i++) {
        if (inargs[i]->unify(tr->generics[0].type->getRecord()->args[i].get(), us) ==
            -1)
          return -1;
      }
      return 1;
    } else if (auto pt = tr->getPartial()) {
      // Handle Callable[...]<->Partial[...].
      std::vector<int> zeros;
      for (int pi = 9; pi < tr->name.size() && tr->name[pi] != '.'; pi++)
        if (tr->name[pi] == '0')
          zeros.emplace_back(pi - 9);
      if (zeros.size() != inargs.size())
        return -1;

      int ic = 0;
      std::unordered_map<int, TypePtr> c;
      auto pf = pt->func->instantiate(0, &ic, &c)->getFunc();
      // For partial functions, we just check can we unify without actually performing
      // unification
      for (int pi = 0, gi = 0; pi < pt->known.size(); pi++)
        if (!pt->known[pi] && pf->ast->args[pi].status == Param::Normal)
          if (inargs[gi++]->unify(pf->getArgTypes()[pi].get(), us) == -1)
            return -1;
      if (us && pf->canRealize()) {
        // Realize if possible to allow deduction of return type [and possible
        // unification!]
        auto rf = TypecheckVisitor(cache->typeCtx).realize(pf);
        pf->unify(rf.get(), us);
      }
      if (args[1]->unify(pf->getRetType().get(), us) == -1)
        return -1;
      return 1;
    }
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
  std::vector<std::string> gs;
  for (auto &a : args)
    gs.push_back(a->debugString(mode));
  return fmt::format("Callable[{}]", join(gs, ","));
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