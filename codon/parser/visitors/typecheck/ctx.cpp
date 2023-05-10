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

TypeContext::TypeContext(Cache *cache, std::string filename)
    : Context<TypecheckItem>(std::move(filename)), cache(cache), isStdlibLoading(false),
      moduleName{ImportFile::PACKAGE, "", ""}, isConditionalExpr(false),
      allowTypeOf(true), typecheckLevel(0), age(0), blockLevel(0), returnEarly(false),
      changedNodes(0) {
  bases.emplace_back("");
  scope.blocks.push_back(scope.counter = 0);
  realizationBases.push_back({"", nullptr, nullptr});
  pushSrcInfo(cache->generateSrcInfo()); // Always have srcInfo() around
}

TypeContext::Base::Base(std::string name, Attr *attributes)
    : name(std::move(name)), attributes(attributes), deducedMembers(nullptr),
      selfName(), captures(nullptr), pyCaptures(nullptr) {}

void TypeContext::add(const std::string &name, const TypeContext::Item &var) {
  auto v = find(name);
  if (v && v->noShadow)
    E(Error::ID_INVALID_BIND, getSrcInfo(), name);
  Context<TypecheckItem>::add(name, var);
}

TypeContext::Item TypeContext::addVar(const std::string &name,
                                      const std::string &canonicalName,
                                      const SrcInfo &srcInfo,
                                      const types::TypePtr &type) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  auto t = std::make_shared<TypecheckItem>(TypecheckItem::Var, getBaseName(),
                                           canonicalName, getModule(), scope.blocks);
  t->setSrcInfo(srcInfo);
  t->type = type;
  Context<TypecheckItem>::add(name, t);
  Context<TypecheckItem>::add(canonicalName, t);
  return t;
}

TypeContext::Item TypeContext::addType(const std::string &name,
                                       const std::string &canonicalName,
                                       const SrcInfo &srcInfo,
                                       const types::TypePtr &type) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  auto t = std::make_shared<TypecheckItem>(TypecheckItem::Type, getBaseName(),
                                           canonicalName, getModule(), scope.blocks);
  t->setSrcInfo(srcInfo);
  t->type = type;
  Context<TypecheckItem>::add(name, t);
  Context<TypecheckItem>::add(canonicalName, t);
  return t;
}

TypeContext::Item TypeContext::addFunc(const std::string &name,
                                       const std::string &canonicalName,
                                       const SrcInfo &srcInfo,
                                       const types::TypePtr &type) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  auto t = std::make_shared<TypecheckItem>(TypecheckItem::Func, getBaseName(),
                                           canonicalName, getModule(), scope.blocks);
  t->setSrcInfo(srcInfo);
  t->type = type;
  Context<TypecheckItem>::add(name, t);
  Context<TypecheckItem>::add(canonicalName, t);
  return t;
}

TypeContext::Item TypeContext::addAlwaysVisible(const TypeContext::Item &item) {
  auto i = std::make_shared<TypecheckItem>(item->kind, item->baseName,
                                           item->canonicalName, item->moduleName,
                                           std::vector<int>{0}, item->importPath);
  auto stdlib = cache->imports[STDLIB_IMPORT].ctx;
  if (!stdlib->find(i->canonicalName)) {
    stdlib->add(i->canonicalName, i);
  }
  return i;
}

TypeContext::Item TypeContext::find(const std::string &name) const {
  auto t = Context<TypecheckItem>::find(name);
  if (t)
    return t;

  // Item is not found in the current module. Time to look in the standard library!
  // Note: the standard library items cannot be dominated.
  auto stdlib = cache->imports[STDLIB_IMPORT].ctx;
  if (stdlib.get() != this)
    t = stdlib->find(name);

  // if (in(cache->globals, name))
  //   return std::make_shared<TypecheckItem>(TypecheckItem::Var, getUnbound());
  // return nullptr;

  return t;
}

TypeContext::Item TypeContext::forceFind(const std::string &name) const {
  auto f = find(name);
  seqassert(f, "cannot find '{}'", name);
  return f;
}

TypeContext::Item TypeContext::findDominatingBinding(const std::string &name) {
  auto it = map.find(name);
  if (it == map.end())
    return find(name);
  seqassert(!it->second.empty(), "corrupted SimplifyContext ({})", name);

  // The item is found. Let's see is it accessible now.

  std::string canonicalName;
  auto lastGood = it->second.begin();
  bool isOutside = (*lastGood)->getBaseName() != getBaseName();
  int prefix = int(scope.blocks.size());
  // Iterate through all bindings with the given name and find the closest binding that
  // dominates the current scope.
  for (auto i = it->second.begin(); i != it->second.end(); i++) {
    // Find the longest block prefix between the binding and the current scope.
    int p = std::min(prefix, int((*i)->scope.size()));
    while (p >= 0 && (*i)->scope[p - 1] != scope.blocks[p - 1])
      p--;
    // We reached the toplevel. Break.
    if (p < 0)
      break;
    // We went outside the function scope. Break.
    if (!isOutside && (*i)->getBaseName() != getBaseName())
      break;
    prefix = p;
    lastGood = i;
    // The binding completely dominates the current scope. Break.
    if ((*i)->scope.size() <= scope.blocks.size() &&
        (*i)->scope.back() == scope.blocks[(*i)->scope.size() - 1])
      break;
  }
  seqassert(lastGood != it->second.end(), "corrupted scoping ({})", name);
  if (lastGood != it->second.begin() && !(*lastGood)->isVar())
    E(Error::CLASS_INVALID_BIND, getSrcInfo(), name);

  bool hasUsed = false;
  if ((*lastGood)->scope.size() == prefix) {
    // The current scope is dominated by a binding. Use that binding.
    canonicalName = (*lastGood)->canonicalName;
  } else {
    // The current scope is potentially reachable by multiple bindings that are
    // not dominated by a common binding. Create such binding in the scope that
    // dominates (covers) all of them.
    canonicalName = generateCanonicalName(name);
    auto item = std::make_shared<TypecheckItem>(
        (*lastGood)->kind, (*lastGood)->baseName, canonicalName,
        (*lastGood)->moduleName,
        std::vector<int>(scope.blocks.begin(), scope.blocks.begin() + prefix),
        (*lastGood)->importPath);
    item->accessChecked = {(*lastGood)->scope};
    lastGood = it->second.insert(++lastGood, item);
    // Make sure to prepend a binding declaration: `var` and `var__used__ = False`
    // to the dominating scope.
    scope.stmts[scope.blocks[prefix - 1]].push_back(std::make_unique<AssignStmt>(
        std::make_unique<IdExpr>(canonicalName), nullptr, nullptr));
    scope.stmts[scope.blocks[prefix - 1]].push_back(std::make_unique<AssignStmt>(
        std::make_unique<IdExpr>(fmt::format("{}.__used__", canonicalName)),
        std::make_unique<BoolExpr>(false), nullptr));
    // Reached the toplevel? Register the binding as global.
    if (prefix == 1) {
      cache->addGlobal(canonicalName);
      cache->addGlobal(fmt::format("{}.__used__", canonicalName));
    }
    hasUsed = true;
  }
  // Remove all bindings after the dominant binding.
  for (auto i = it->second.begin(); i != it->second.end(); i++) {
    if (i == lastGood)
      break;
    if (!(*i)->canDominate())
      continue;
    // These bindings (and their canonical identifiers) will be replaced by the
    // dominating binding during the type checking pass.
    cache->replacements[(*i)->canonicalName] = {canonicalName, hasUsed};
    cache->replacements[format("{}.__used__", (*i)->canonicalName)] = {
        format("{}.__used__", canonicalName), false};
    seqassert((*i)->canonicalName != canonicalName, "invalid replacement at {}: {}",
              getSrcInfo(), canonicalName);
    auto it = std::find(stack.front().begin(), stack.front().end(), name);
    if (it != stack.front().end())
      stack.front().erase(it);
  }
  it->second.erase(it->second.begin(), lastGood);
  return it->second.front();
}

std::string TypeContext::getBaseName() const { return bases.back().name; }

std::string TypeContext::getModule() const {
  std::string base = moduleName.status == ImportFile::STDLIB ? "std." : "";
  base += moduleName.module;
  if (auto sz = startswith(base, "__main__"))
    base = base.substr(sz);
  return base;
}

void TypeContext::dump() { dump(0); }

std::string TypeContext::generateCanonicalName(const std::string &name,
                                               bool includeBase, bool zeroId) const {
  std::string newName = name;
  bool alreadyGenerated = name.find('.') != std::string::npos;
  if (includeBase && !alreadyGenerated) {
    std::string base = getBaseName();
    if (base.empty())
      base = getModule();
    if (base == "std.internal.core")
      base = "";
    newName = (base.empty() ? "" : (base + ".")) + newName;
  }
  auto num = cache->identifierCount[newName]++;
  if (num)
    newName = format("{}.{}", newName, num);
  if (name != newName && !zeroId)
    cache->identifierCount[newName]++;
  cache->reverseIdentifierLookup[newName] = name;
  return newName;
}

void TypeContext::enterConditionalBlock() { scope.blocks.push_back(++scope.counter); }

void TypeContext::leaveConditionalBlock(std::vector<StmtPtr> *stmts) {
  if (stmts && in(scope.stmts, scope.blocks.back()))
    stmts->insert(stmts->begin(), scope.stmts[scope.blocks.back()].begin(),
                  scope.stmts[scope.blocks.back()].end());
  scope.blocks.pop_back();
}

bool TypeContext::isGlobal() const { return bases.size() == 1; }

bool TypeContext::isConditional() const { return scope.blocks.size() > 1; }

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
                 args.back().value->getEllipsis()->mode != EllipsisExpr::PIPE &&
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

std::shared_ptr<std::pair<std::vector<types::TypePtr>, std::vector<types::TypePtr>>>
TypeContext::getFunctionArgs(types::TypePtr t) {
  if (!t->getFunc())
    return nullptr;
  auto fn = t->getFunc();
  auto ret = std::make_shared<
      std::pair<std::vector<types::TypePtr>, std::vector<types::TypePtr>>>();
  for (auto &t : fn->funcGenerics)
    ret->first.push_back(t.type);
  for (auto &t : fn->generics[0].type->getRecord()->args)
    ret->second.push_back(t);
  return ret;
}

std::shared_ptr<std::string> TypeContext::getStaticString(types::TypePtr t) {
  if (auto s = t->getStatic()) {
    auto r = s->evaluate();
    if (r.type == StaticValue::STRING)
      return std::make_shared<std::string>(r.getString());
  }
  return nullptr;
}

std::shared_ptr<int64_t> TypeContext::getStaticInt(types::TypePtr t) {
  if (auto s = t->getStatic()) {
    auto r = s->evaluate();
    if (r.type == StaticValue::INT)
      return std::make_shared<int64_t>(r.getInt());
  }
  return nullptr;
}

types::FuncTypePtr TypeContext::extractFunction(types::TypePtr t) {
  if (auto f = t->getFunc())
    return f;
  if (auto p = t->getPartial())
    return p->func;
  return nullptr;
}

} // namespace codon::ast
