// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "ctx.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "codon/cir/attribute.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

TypecheckItem::TypecheckItem(std::string canonicalName, std::string baseName,
                             std::string moduleName, types::TypePtr type,
                             std::vector<int> scope)
    : canonicalName(std::move(canonicalName)), baseName(std::move(baseName)),
      moduleName(std::move(moduleName)), type(std::move(type)),
      scope(std::move(scope)) {}

TypeContext::TypeContext(Cache *cache, std::string filename)
    : Context<TypecheckItem>(std::move(filename)), cache(cache) {
  bases.emplace_back();
  scope.emplace_back(0);
  pushSrcInfo(cache->generateSrcInfo()); // Always have srcInfo() around
}

void TypeContext::add(const std::string &name, const TypeContext::Item &var) {
  seqassert(!var->scope.empty(), "bad scope for '{}'", name);
  // if (var->type->is("byte"))
  // LOG("--");
  // LOG("[ctx] {} @ {}: + {}: {} ({:D})", getModule(), getSrcInfo(), name,
  // var->canonicalName, var->type);
  // if (name=="V.0"&&var->type->is("int"))
  //   LOG("-");
  // LOG("{}: {:c}", name, var->type);
  Context<TypecheckItem>::add(name, var);
}

void TypeContext::removeFromMap(const std::string &name) {
  Context<TypecheckItem>::removeFromMap(name);
}

TypeContext::Item TypeContext::addVar(const std::string &name,
                                      const std::string &canonicalName,
                                      const types::TypePtr &type,
                                      const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  // seqassert(type->getLink(), "bad var");
  auto t = std::make_shared<TypecheckItem>(canonicalName, getBaseName(), getModule(),
                                           type, getScope());
  t->setSrcInfo(srcInfo);
  add(name, t);
  addAlwaysVisible(t);
  return t;
}

TypeContext::Item TypeContext::addType(const std::string &name,
                                       const std::string &canonicalName,
                                       const types::TypePtr &type,
                                       const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  // seqassert(type->getClass(), "bad type");
  auto t = std::make_shared<TypecheckItem>(canonicalName, getBaseName(), getModule(),
                                           type, getScope());
  t->setSrcInfo(srcInfo);
  add(name, t);
  addAlwaysVisible(t);
  return t;
}

TypeContext::Item TypeContext::addFunc(const std::string &name,
                                       const std::string &canonicalName,
                                       const types::TypePtr &type,
                                       const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  seqassert(type->getFunc(), "bad func");
  auto t = std::make_shared<TypecheckItem>(canonicalName, getBaseName(), getModule(),
                                           type, getScope());
  t->setSrcInfo(srcInfo);
  add(name, t);
  addAlwaysVisible(t);
  return t;
}

TypeContext::Item TypeContext::addAlwaysVisible(const TypeContext::Item &item,
                                                bool pop) {
  add(item->canonicalName, item);
  if (pop)
    stack.front().pop_back(); // do not remove it later!
  if (!cache->typeCtx->Context<TypecheckItem>::find(item->canonicalName)) {
    cache->typeCtx->add(item->canonicalName, item);
    if (pop)
      cache->typeCtx->stack.front().pop_back(); // do not remove it later!

    // Realizations etc.
    if (!in(cache->reverseIdentifierLookup, item->canonicalName))
      cache->reverseIdentifierLookup[item->canonicalName] = item->canonicalName;
  }
  return item;
}

TypeContext::Item TypeContext::find(const std::string &name) const {
  auto t = Context<TypecheckItem>::find(name);
  if (t)
    return t;

  // Item is not found in the current module. Time to look in the standard library!
  // Note: the standard library items cannot be dominated.
  auto stdlib = cache->imports[STDLIB_IMPORT].ctx;
  if (stdlib.get() != this)
    t = stdlib->Context<TypecheckItem>::find(name);

  // Maybe we are looking for a canonical identifier?
  if (!t && cache->typeCtx.get() != this)
    t = cache->typeCtx->Context<TypecheckItem>::find(name);

  return t;
}

TypeContext::Item TypeContext::forceFind(const std::string &name) const {
  auto f = find(name);
  seqassert(f, "cannot find '{}'", name);
  return f;
}

/// Getters and setters

std::string TypeContext::getBaseName() const { return bases.back().name; }

std::string TypeContext::getModule() const {
  std::string base = moduleName.status == ImportFile::STDLIB ? "std." : "";
  base += moduleName.module;
  if (auto sz = startswith(base, "__main__"))
    base = base.substr(sz);
  return base;
}

std::string TypeContext::getModulePath() const { return moduleName.path; }

void TypeContext::dump() { dump(0); }

bool TypeContext::isCanonicalName(const std::string &name) const {
  return name.rfind('.') != std::string::npos;
}

std::string TypeContext::generateCanonicalName(const std::string &name,
                                               bool includeBase, bool noSuffix) const {
  std::string newName = name;
  bool alreadyGenerated = name.find('.') != std::string::npos;
  if (alreadyGenerated)
    return name;
  includeBase &= !(!name.empty() && name[0] == '%');
  if (includeBase && !alreadyGenerated) {
    std::string base = getBaseName();
    if (base.empty())
      base = getModule();
    if (base == "std.internal.core") {
      noSuffix = true;
      base = "";
    }
    newName = (base.empty() ? "" : (base + ".")) + newName;
  }
  auto num = cache->identifierCount[newName]++;
  if (!noSuffix && !alreadyGenerated)
    newName = format("{}.{}", newName, num);
  if (name != newName)
    cache->identifierCount[newName]++;
  cache->reverseIdentifierLookup[newName] = name;
  return newName;
}

bool TypeContext::isGlobal() const { return bases.size() == 1; }

bool TypeContext::isConditional() const { return scope.size() > 1; }

TypeContext::Base *TypeContext::getBase() {
  return bases.empty() ? nullptr : &(bases.back());
}

bool TypeContext::inFunction() const { return !isGlobal() && !bases.back().isType(); }

bool TypeContext::inClass() const { return !isGlobal() && bases.back().isType(); }

bool TypeContext::isOuter(const Item &val) const {
  return getBaseName() != val->getBaseName() || getModule() != val->getModule();
}

TypeContext::Base *TypeContext::getClassBase() {
  if (bases.size() >= 2 && bases[bases.size() - 2].isType())
    return &(bases[bases.size() - 2]);
  return nullptr;
}

size_t TypeContext::getRealizationDepth() const { return bases.size(); }

std::string TypeContext::getRealizationStackName() const {
  if (bases.empty())
    return "";
  std::vector<std::string> s;
  for (auto &b : bases)
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
    // special case: __SELF__
    if (type->getFunc() && !type->getFunc()->funcGenerics.empty() &&
        type->getFunc()->funcGenerics[0].niceName == "__SELF__") {
      genericCache[type->getFunc()->funcGenerics[0].id] = generics;
    }
  }
  auto t = type->instantiate(typecheckLevel, &(cache->unboundCount), &genericCache);
  for (auto &i : genericCache) {
    if (auto l = i.second->getLink()) {
      i.second->setSrcInfo(srcInfo);
      if (l->defaultType) {
        getBase()->pendingDefaults.insert(i.second);
      }
    }
  }
  if (auto ft = t->getFunc()) {
    if (auto b = ft->ast->getAttribute<BindingsAttribute>(Attr::Bindings)) {
      auto module =
          ft->ast->getAttribute<ir::StringValueAttribute>(Attr::Module)->value;
      const auto &imp = cache->imports[module];
      std::unordered_map<std::string, std::string> key;
      for (const auto &[c, _] : b->captures) {
        auto h = imp.ctx->find(c);
        // seqassert(h, "bad function {}: cannot locate {}", ft->name, c);
        key[c] = h ? h->canonicalName : "";
      }
      auto &cm = cache->functions[ft->ast->name].captureMappings;
      size_t idx = 0;
      for (; idx < cm.size(); idx++)
        if (cm[idx] == key)
          break;
      if (idx == cm.size())
        cm.push_back(key);
      // if (idx)
      //   LOG("--> {}: realize {}: {} / {}", getSrcInfo(), ft->debugString(2), idx,
      //   key);
      ft->index = idx;
    }
  }
  if (t->getUnion() && !t->getUnion()->isSealed()) {
    t->setSrcInfo(srcInfo);
    getBase()->pendingDefaults.insert(t);
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
    auto t = generics[i];
    seqassert(c->generics[i].type, "generic is null");
    if (!c->generics[i].isStatic && t->getStatic())
      t = t->getStatic()->getNonStaticType();
    g->generics.emplace_back("", "", t, c->generics[i].id, c->generics[i].isStatic);
  }
  return instantiate(srcInfo, root, g);
}

std::vector<types::FuncTypePtr> TypeContext::findMethod(types::ClassType *type,
                                                        const std::string &method,
                                                        bool hideShadowed) {
  std::vector<types::FuncTypePtr> vv;
  std::unordered_set<std::string> signatureLoci;

  auto populate = [&](const auto &cls) {
    auto t = in(cls.methods, method);
    if (!t)
      return;

    auto mt = cache->overloads[*t];
    for (int mti = int(mt.size()) - 1; mti >= 0; mti--) {
      auto &method = mt[mti];
      if (endswith(method, ":dispatch") || !cache->functions[method].type)
        continue;
      if (hideShadowed) {
        auto sig = cache->functions[method].ast->signature();
        if (!in(signatureLoci, sig)) {
          signatureLoci.insert(sig);
          vv.emplace_back(cache->functions[method].type);
        }
      } else {
        vv.emplace_back(cache->functions[method].type);
      }
    }
  };
  if (auto cls = cache->getClass(type)) {
    for (const auto &pc : cls->mro) {
      auto mc = cache->getClass(pc);
      seqassert(mc, "class '{}' not found", pc->name);
      populate(*mc);
    }
  }
  return vv;
}

Cache::Class::ClassField *TypeContext::findMember(const types::ClassTypePtr &type,
                                                  const std::string &member) const {
  if (auto cls = cache->getClass(type)) {
    for (const auto &pc : cls->mro) {
      auto mc = cache->getClass(pc);
      seqassert(mc, "class '{}' not found", pc->name);
      for (auto &mm : mc->fields) {
        if (pc->is(TYPE_TUPLE) && (&mm - &(mc->fields[0])) >= type->generics.size())
          break;
        if (mm.name == member)
          return &mm;
      }
    }
  }
  return nullptr;
}

int TypeContext::reorderNamedArgs(types::FuncType *func,
                                  const std::vector<CallArg> &args,
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
  bool partial = !args.empty() && cast<EllipsisExpr>(args.back().value) &&
                 cast<EllipsisExpr>(args.back().value)->getMode() != EllipsisExpr::PIPE &&
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
  auto s = onDone(starArgIndex, kwstarArgIndex, slots, partial);
  return s != -1 ? score + s : -1;
}

void TypeContext::dump(int pad) {
  auto ordered =
      std::map<std::string, decltype(map)::mapped_type>(map.begin(), map.end());
  LOG("current module: {} ({})", moduleName.module, moduleName.path);
  LOG("current base:   {} / {}", getRealizationStackName(), getBase()->name);
  for (auto &i : ordered) {
    std::string s;
    auto t = i.second.front();
    LOG("{}{:.<25}", std::string(size_t(pad) * 2, ' '), i.first);
    LOG("   ... kind:      {}", t->isType() * 100 + t->isFunc() * 10 + t->isVar());
    LOG("   ... canonical: {}", t->canonicalName);
    LOG("   ... base:      {}", t->baseName);
    LOG("   ... module:    {}", t->moduleName);
    LOG("   ... type:      {}", t->type ? t->type->debugString(2) : "<null>");
    LOG("   ... scope:     {}", t->scope);
    LOG("   ... gnrc/sttc: {} / {}", t->generic, int(t->isStatic()));
  }
}

std::string TypeContext::debugInfo() {
  return fmt::format("[{}:i{}@{}]", getBase()->name, getBase()->iteration,
                     getSrcInfo());
}

std::shared_ptr<std::pair<std::vector<types::TypePtr>, std::vector<types::TypePtr>>>
TypeContext::getFunctionArgs(const types::TypePtr &t) {
  if (!t->getFunc())
    return nullptr;
  auto fn = t->getFunc();
  auto ret = std::make_shared<
      std::pair<std::vector<types::TypePtr>, std::vector<types::TypePtr>>>();
  for (auto &t : fn->funcGenerics)
    ret->first.push_back(t.type);
  for (auto &t : fn->generics[0].type->getClass()->generics)
    ret->second.push_back(t.type);
  return ret;
}

types::FuncTypePtr TypeContext::extractFunction(const types::TypePtr &t) {
  if (auto f = t->getFunc())
    return f;
  if (auto p = t->getPartial())
    return p->getPartialFunc();
  return nullptr;
}

types::TypePtr TypeContext::getType(const std::string &s) {
  auto val = forceFind(s);
  return s == "type" ? val->type : getType(val->type);
}

types::TypePtr TypeContext::getType(types::TypePtr t) {
  while (t && t->is("type"))
    t = t->getClass()->generics[0].type;
  return t;
}

} // namespace codon::ast
