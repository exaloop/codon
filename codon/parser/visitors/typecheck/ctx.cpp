// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "ctx.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/format/format.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

TypeContext::TypeContext(Cache *cache)
    : Context<TypecheckItem>(""), cache(cache), typecheckLevel(0), age(0),
      blockLevel(0), returnEarly(false), changedNodes(0) {
  realizationBases.push_back({"", nullptr, nullptr});
  pushSrcInfo(cache->generateSrcInfo()); // Always have srcInfo() around
}

std::shared_ptr<TypecheckItem> TypeContext::add(TypecheckItem::Kind kind,
                                                const std::string &name,
                                                const types::TypePtr &type) {
  auto t = std::make_shared<TypecheckItem>(kind, type);
  add(name, t);
  return t;
}

std::shared_ptr<TypecheckItem> TypeContext::find(const std::string &name) const {
  if (auto t = Context<TypecheckItem>::find(name))
    return t;
  if (in(cache->globals, name))
    return std::make_shared<TypecheckItem>(TypecheckItem::Var, getUnbound());
  return nullptr;
}

std::shared_ptr<TypecheckItem> TypeContext::forceFind(const std::string &name) const {
  auto t = find(name);
  seqassert(t, "cannot find '{}'", name);
  return t;
}

types::TypePtr TypeContext::getType(const std::string &name) const {
  return forceFind(name)->type;
}

TypeContext::RealizationBase *TypeContext::getRealizationBase() {
  return &(realizationBases.back());
}

size_t TypeContext::getRealizationDepth() const { return realizationBases.size(); }

std::string TypeContext::getRealizationStackName() const {
  if (realizationBases.empty())
    return "";
  std::vector<std::string> s;
  for (auto &b : realizationBases)
    if (b.type)
      s.push_back(b.type->realizedName());
  return join(s, ":");
}

std::shared_ptr<types::LinkType> TypeContext::getUnbound(const SrcInfo &srcInfo,
                                                         int level) const {
  auto typ = std::make_shared<types::LinkType>(cache, types::LinkType::Unbound,
                                               cache->unboundCount++, level, nullptr);
  typ->setSrcInfo(srcInfo);
  return typ;
}

std::shared_ptr<types::LinkType> TypeContext::getUnbound(const SrcInfo &srcInfo) const {
  return getUnbound(srcInfo, typecheckLevel);
}

std::shared_ptr<types::LinkType> TypeContext::getUnbound() const {
  return getUnbound(getSrcInfo(), typecheckLevel);
}

types::TypePtr TypeContext::instantiate(const SrcInfo &srcInfo,
                                        const types::TypePtr &type,
                                        const types::ClassTypePtr &generics) {
  seqassert(type, "type is null");
  std::unordered_map<int, types::TypePtr> genericCache;
  if (generics) {
    for (auto &g : generics->generics)
      if (g.type &&
          !(g.type->getLink() && g.type->getLink()->kind == types::LinkType::Generic)) {
        genericCache[g.id] = g.type;
      }
  }
  auto t = type->instantiate(typecheckLevel, &(cache->unboundCount), &genericCache);
  for (auto &i : genericCache) {
    if (auto l = i.second->getLink()) {
      i.second->setSrcInfo(srcInfo);
      if (l->defaultType)
        pendingDefaults.insert(i.second);
    }
  }
  if (t->getUnion() && !t->getUnion()->isSealed()) {
    t->setSrcInfo(srcInfo);
    pendingDefaults.insert(t);
  }
  return t;
}

types::TypePtr
TypeContext::instantiateGeneric(const SrcInfo &srcInfo, const types::TypePtr &root,
                                const std::vector<types::TypePtr> &generics) {
  auto c = root->getClass();
  seqassert(c, "root class is null");
  // dummy generic type
  auto g = std::make_shared<types::ClassType>(cache, "", "");
  if (generics.size() != c->generics.size()) {
    E(Error::GENERICS_MISMATCH, srcInfo, cache->rev(c->name), c->generics.size(),
      generics.size());
  }
  for (int i = 0; i < c->generics.size(); i++) {
    seqassert(c->generics[i].type, "generic is null");
    g->generics.emplace_back("", "", generics[i], c->generics[i].id);
  }
  return instantiate(srcInfo, root, g);
}

std::vector<types::FuncTypePtr> TypeContext::findMethod(const std::string &typeName,
                                                        const std::string &method,
                                                        bool hideShadowed) const {
  std::vector<types::FuncTypePtr> vv;
  std::unordered_set<std::string> signatureLoci;

  auto populate = [&](const auto &cls) {
    auto t = in(cls.methods, method);
    if (!t)
      return;
    auto mt = cache->overloads[*t];
    for (int mti = int(mt.size()) - 1; mti >= 0; mti--) {
      auto &method = mt[mti];
      if (endswith(method.name, ":dispatch") || !cache->functions[method.name].type)
        continue;
      if (method.age <= age) {
        if (hideShadowed) {
          auto sig = cache->functions[method.name].ast->signature();
          if (!in(signatureLoci, sig)) {
            signatureLoci.insert(sig);
            vv.emplace_back(cache->functions[method.name].type);
          }
        } else {
          vv.emplace_back(cache->functions[method.name].type);
        }
      }
    }
  };
  if (auto cls = in(cache->classes, typeName)) {
    for (auto &pt : cls->mro) {
      if (auto pc = pt->type->getClass()) {
        auto mc = in(cache->classes, pc->name);
        seqassert(mc, "class '{}' not found", pc->name);
        populate(*mc);
      }
    }
  }
  return vv;
}

types::TypePtr TypeContext::findMember(const std::string &typeName,
                                       const std::string &member) const {
  if (auto cls = in(cache->classes, typeName)) {
    for (auto &pt : cls->mro) {
      if (auto pc = pt->type->getClass()) {
        auto mc = in(cache->classes, pc->name);
        seqassert(mc, "class '{}' not found", pc->name);
        for (auto &mm : mc->fields) {
          if (mm.name == member)
            return mm.type;
        }
      }
    }
  }
  return nullptr;
}

int TypeContext::reorderNamedArgs(types::FuncType *func,
                                  const std::vector<CallExpr::Arg> &args,
                                  const ReorderDoneFn &onDone,
                                  const ReorderErrorFn &onError,
                                  const std::vector<char> &known) {
  // See https://docs.python.org/3.6/reference/expressions.html#calls for details.
  // Final score:
  //  - +1 for each matched argument
  //  -  0 for *args/**kwargs/default arguments
  //  - -1 for failed match
  int score = 0;

  // 0. Find *args and **kwargs
  // True if there is a trailing ellipsis (full partial: fn(all_args, ...))
  bool partial = !args.empty() && args.back().value->getEllipsis() &&
                 !args.back().value->getEllipsis()->isPipeArg &&
                 args.back().name.empty();

  int starArgIndex = -1, kwstarArgIndex = -1;
  for (int i = 0; i < func->ast->args.size(); i++) {
    if (startswith(func->ast->args[i].name, "**"))
      kwstarArgIndex = i, score -= 2;
    else if (startswith(func->ast->args[i].name, "*"))
      starArgIndex = i, score -= 2;
  }

  // 1. Assign positional arguments to slots
  // Each slot contains a list of arg's indices
  std::vector<std::vector<int>> slots(func->ast->args.size());
  seqassert(known.empty() || func->ast->args.size() == known.size(),
            "bad 'known' string");
  std::vector<int> extra;
  std::map<std::string, int> namedArgs,
      extraNamedArgs; // keep the map--- we need it sorted!
  for (int ai = 0, si = 0; ai < args.size() - partial; ai++) {
    if (args[ai].name.empty()) {
      while (!known.empty() && si < slots.size() && known[si])
        si++;
      if (si < slots.size() && (starArgIndex == -1 || si < starArgIndex))
        slots[si++] = {ai};
      else
        extra.emplace_back(ai);
    } else {
      namedArgs[args[ai].name] = ai;
    }
  }
  score += 2 * int(slots.size() - func->funcGenerics.size());

  for (auto ai : std::vector<int>{std::max(starArgIndex, kwstarArgIndex),
                                  std::min(starArgIndex, kwstarArgIndex)})
    if (ai != -1 && !slots[ai].empty()) {
      extra.insert(extra.begin(), ai);
      slots[ai].clear();
    }

  // 2. Assign named arguments to slots
  if (!namedArgs.empty()) {
    std::map<std::string, int> slotNames;
    for (int i = 0; i < func->ast->args.size(); i++)
      if (known.empty() || !known[i]) {
        slotNames[cache->reverseIdentifierLookup[func->ast->args[i].name]] = i;
      }
    for (auto &n : namedArgs) {
      if (!in(slotNames, n.first))
        extraNamedArgs[n.first] = n.second;
      else if (slots[slotNames[n.first]].empty())
        slots[slotNames[n.first]].push_back(n.second);
      else
        return onError(Error::CALL_REPEATED_NAME, args[n.second].value->getSrcInfo(),
                       Emsg(Error::CALL_REPEATED_NAME, n.first));
    }
  }

  // 3. Fill in *args, if present
  if (!extra.empty() && starArgIndex == -1)
    return onError(Error::CALL_ARGS_MANY, getSrcInfo(),
                   Emsg(Error::CALL_ARGS_MANY, cache->rev(func->ast->name),
                        func->ast->args.size(), args.size() - partial));

  if (starArgIndex != -1)
    slots[starArgIndex] = extra;

  // 4. Fill in **kwargs, if present
  if (!extraNamedArgs.empty() && kwstarArgIndex == -1)
    return onError(Error::CALL_ARGS_INVALID,
                   args[extraNamedArgs.begin()->second].value->getSrcInfo(),
                   Emsg(Error::CALL_ARGS_INVALID, extraNamedArgs.begin()->first,
                        cache->rev(func->ast->name)));
  if (kwstarArgIndex != -1)
    for (auto &e : extraNamedArgs)
      slots[kwstarArgIndex].push_back(e.second);

  // 5. Fill in the default arguments
  for (auto i = 0; i < func->ast->args.size(); i++)
    if (slots[i].empty() && i != starArgIndex && i != kwstarArgIndex) {
      if (func->ast->args[i].status == Param::Normal &&
          (func->ast->args[i].defaultValue || (!known.empty() && known[i])))
        score -= 2;
      else if (!partial && func->ast->args[i].status == Param::Normal)
        return onError(Error::CALL_ARGS_MISSING, getSrcInfo(),
                       Emsg(Error::CALL_ARGS_MISSING, cache->rev(func->ast->name),
                            cache->reverseIdentifierLookup[func->ast->args[i].name]));
    }
  return score + onDone(starArgIndex, kwstarArgIndex, slots, partial);
}

void TypeContext::dump(int pad) {
  auto ordered =
      std::map<std::string, decltype(map)::mapped_type>(map.begin(), map.end());
  LOG("base: {}", getRealizationStackName());
  for (auto &i : ordered) {
    std::string s;
    auto t = i.second.front();
    LOG("{}{:.<25} {}", std::string(pad * 2, ' '), i.first, t->type);
  }
}

std::string TypeContext::debugInfo() {
  return fmt::format("[{}:i{}@{}]", getRealizationBase()->name,
                     getRealizationBase()->iteration, getSrcInfo());
}

} // namespace codon::ast
