// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "ctx.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

SimplifyContext::SimplifyContext(std::string filename, Cache *cache)
    : Context<SimplifyItem>(std::move(filename)), cache(cache), isStdlibLoading(false),
      moduleName{ImportFile::PACKAGE, "", ""}, isConditionalExpr(false),
      allowTypeOf(true) {
  bases.emplace_back(Base(""));
  scope.blocks.push_back(scope.counter = 0);
}

SimplifyContext::Base::Base(std::string name, Attr *attributes)
    : name(std::move(name)), attributes(attributes), deducedMembers(nullptr),
      selfName(), captures(nullptr), pyCaptures(nullptr) {}

void SimplifyContext::add(const std::string &name, const SimplifyContext::Item &var) {
  auto v = find(name);
  if (v && v->noShadow)
    E(Error::ID_INVALID_BIND, getSrcInfo(), name);
  Context<SimplifyItem>::add(name, var);
}

SimplifyContext::Item SimplifyContext::addVar(const std::string &name,
                                              const std::string &canonicalName,
                                              const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  auto t = std::make_shared<SimplifyItem>(SimplifyItem::Var, getBaseName(),
                                          canonicalName, getModule(), scope.blocks);
  t->setSrcInfo(srcInfo);
  Context<SimplifyItem>::add(name, t);
  Context<SimplifyItem>::add(canonicalName, t);
  return t;
}

SimplifyContext::Item SimplifyContext::addType(const std::string &name,
                                               const std::string &canonicalName,
                                               const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  auto t = std::make_shared<SimplifyItem>(SimplifyItem::Type, getBaseName(),
                                          canonicalName, getModule(), scope.blocks);
  t->setSrcInfo(srcInfo);
  Context<SimplifyItem>::add(name, t);
  Context<SimplifyItem>::add(canonicalName, t);
  return t;
}

SimplifyContext::Item SimplifyContext::addFunc(const std::string &name,
                                               const std::string &canonicalName,
                                               const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  auto t = std::make_shared<SimplifyItem>(SimplifyItem::Func, getBaseName(),
                                          canonicalName, getModule(), scope.blocks);
  t->setSrcInfo(srcInfo);
  Context<SimplifyItem>::add(name, t);
  Context<SimplifyItem>::add(canonicalName, t);
  return t;
}

SimplifyContext::Item
SimplifyContext::addAlwaysVisible(const SimplifyContext::Item &item) {
  auto i = std::make_shared<SimplifyItem>(item->kind, item->baseName,
                                          item->canonicalName, item->moduleName,
                                          std::vector<int>{0}, item->importPath);
  auto stdlib = cache->imports[STDLIB_IMPORT].ctx;
  if (!stdlib->find(i->canonicalName)) {
    stdlib->add(i->canonicalName, i);
  }
  return i;
}

SimplifyContext::Item SimplifyContext::find(const std::string &name) const {
  auto t = Context<SimplifyItem>::find(name);
  if (t)
    return t;

  // Item is not found in the current module. Time to look in the standard library!
  // Note: the standard library items cannot be dominated.
  auto stdlib = cache->imports[STDLIB_IMPORT].ctx;
  if (stdlib.get() != this)
    t = stdlib->find(name);
  return t;
}

SimplifyContext::Item SimplifyContext::forceFind(const std::string &name) const {
  auto f = find(name);
  seqassert(f, "cannot find '{}'", name);
  return f;
}

SimplifyContext::Item SimplifyContext::findDominatingBinding(const std::string &name) {
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
    bool completeDomination =
        (*i)->scope.size() <= scope.blocks.size() &&
        (*i)->scope.back() == scope.blocks[(*i)->scope.size() - 1];
    if (!completeDomination && prefix < int(scope.blocks.size()) && prefix != p) {
      break;
    }
    prefix = p;
    lastGood = i;
    // The binding completely dominates the current scope. Break.
    if (completeDomination)
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
    auto item = std::make_shared<SimplifyItem>(
        (*lastGood)->kind, (*lastGood)->baseName, canonicalName,
        (*lastGood)->moduleName,
        std::vector<int>(scope.blocks.begin(), scope.blocks.begin() + prefix),
        (*lastGood)->importPath);
    item->accessChecked = {(*lastGood)->scope};
    lastGood = it->second.insert(++lastGood, item);
    stack.front().push_back(name);
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

std::string SimplifyContext::getBaseName() const { return bases.back().name; }

std::string SimplifyContext::getModule() const {
  std::string base = moduleName.status == ImportFile::STDLIB ? "std." : "";
  base += moduleName.module;
  if (auto sz = startswith(base, "__main__"))
    base = base.substr(sz);
  return base;
}

void SimplifyContext::dump() { dump(0); }

std::string SimplifyContext::generateCanonicalName(const std::string &name,
                                                   bool includeBase,
                                                   bool zeroId) const {
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

void SimplifyContext::enterConditionalBlock() {
  scope.blocks.push_back(++scope.counter);
}

void SimplifyContext::leaveConditionalBlock(std::vector<StmtPtr> *stmts) {
  if (stmts && in(scope.stmts, scope.blocks.back()))
    stmts->insert(stmts->begin(), scope.stmts[scope.blocks.back()].begin(),
                  scope.stmts[scope.blocks.back()].end());
  scope.blocks.pop_back();
}

bool SimplifyContext::isGlobal() const { return bases.size() == 1; }

bool SimplifyContext::isConditional() const { return scope.blocks.size() > 1; }

SimplifyContext::Base *SimplifyContext::getBase() {
  return bases.empty() ? nullptr : &(bases.back());
}

bool SimplifyContext::inFunction() const {
  return !isGlobal() && !bases.back().isType();
}

bool SimplifyContext::inClass() const { return !isGlobal() && bases.back().isType(); }

bool SimplifyContext::isOuter(const Item &val) const {
  return getBaseName() != val->getBaseName() || getModule() != val->getModule();
}

SimplifyContext::Base *SimplifyContext::getClassBase() {
  if (bases.size() >= 2 && bases[bases.size() - 2].isType())
    return &(bases[bases.size() - 2]);
  return nullptr;
}

void SimplifyContext::dump(int pad) {
  auto ordered =
      std::map<std::string, decltype(map)::mapped_type>(map.begin(), map.end());
  LOG("location: {}", getSrcInfo());
  LOG("module:   {}", getModule());
  LOG("base:     {}", getBaseName());
  LOG("scope:    {}", fmt::join(scope.blocks, ","));
  for (auto &s : stack.front())
    LOG("-> {}", s);
  for (auto &i : ordered) {
    std::string s;
    bool f = true;
    for (auto &t : i.second) {
      LOG("{}{} {} {:40} {:30} {}", std::string(pad * 2, ' '),
          !f ? std::string(40, ' ') : format("{:.<40}", i.first),
          (t->isFunc() ? "F" : (t->isType() ? "T" : (t->isImport() ? "I" : "V"))),
          t->canonicalName, t->getBaseName(), combine2(t->scope, ","));
      f = false;
    }
  }
}

} // namespace codon::ast
