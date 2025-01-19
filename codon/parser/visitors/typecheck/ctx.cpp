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
  auto e = cache->N<NoneExpr>();
  e->setSrcInfo(cache->generateSrcInfo());
  pushNode(e); // Always have srcInfo() around
}

void TypeContext::add(const std::string &name, const TypeContext::Item &var) {
  seqassert(!var->scope.empty(), "bad scope for '{}'", name);
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

TypeContext::Item TypeContext::find(const std::string &name, int64_t time) const {
  auto it = map.find(name);
  if (it != map.end()) {
    for (auto &i : it->second) {
      if (i->getBaseName() != getBaseName() || !time || i->getTime() <= time)
        return i;
    }
  }

  // Item is not found in the current module. Time to look in the standard library!
  // Note: the standard library items cannot be dominated.
  TypeContext::Item t = nullptr;
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

} // namespace codon::ast
