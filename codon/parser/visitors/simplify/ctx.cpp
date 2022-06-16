#include "ctx.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

SimplifyContext::SimplifyContext(std::string filename, Cache *cache)
    : Context<SimplifyItem>(move(filename)), cache(cache),
      isStdlibLoading(false), moduleName{ImportFile::PACKAGE, "", ""},
      isConditionalExpr(false), allowTypeOf(true) {
  bases.emplace_back(Base(""));
  scope.push_back(scopeCnt = 0);
}

SimplifyContext::Base::Base(std::string name, std::shared_ptr<Expr> ast, Attr *attributes)
    : name(move(name)), ast(move(ast)), attributes(attributes), deducedMembers(nullptr),
      selfName(), captures(nullptr) {}

void SimplifyContext::add(const std::string &name, const SimplifyContext::Item &var) {
  auto v = find(name);
  if (v && v->noShadow)
    error("cannot shadow global or nonlocal statement");
  Context<SimplifyItem>::add(name, var);
}

SimplifyContext::Item SimplifyContext::addVar(const std::string &name,
                                              const std::string &canonicalName,
                                              const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  auto t = std::make_shared<SimplifyItem>(SimplifyItem::Var, getBaseName(),
                                          canonicalName, getModule(), scope);
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
                                          canonicalName, getModule(), scope);
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
                                          canonicalName, getModule(), scope);
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
  // N.B. Standard library items cannot be dominated!
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

  std::string canonicalName;

  seqassert(!it->second.empty(), "corrupted SimplifyContext ({})", name);
  auto lastGood = it->second.begin();
  bool isOutside = (*lastGood)->getBaseName() != getBaseName();
  int prefix = int(scope.size());
  for (auto i = it->second.begin(); i != it->second.end(); i++) {
    int p = std::min(prefix, int((*i)->scope.size()));
    while (p >= 0 && (*i)->scope[p - 1] != scope[p - 1])
      p--;
    if (p < 0)
      break;
    if (!isOutside && (*i)->getBaseName() != getBaseName())
      break;
    prefix = p;
    lastGood = i;
    if ((*i)->scope.size() <= scope.size() &&
        (*i)->scope.back() == scope[(*i)->scope.size() - 1]) // complete domination
      break;
  }
  seqassert(lastGood != it->second.end(), "corrupted scoping ({})", name);
  if (lastGood != it->second.begin() && !(*lastGood)->isVar()) {
    error("reassigning types and functions not allowed");
  }

  bool hasUsed = false;
  if ((*lastGood)->scope.size() == prefix) {
    // Current access is unambiguously covered by a binding
    canonicalName = (*lastGood)->canonicalName;
  } else {
    // LOG("-> access {} @ {} ; bound at {} ; prefix {} ; {}", name, combine2(scope),
    //     combine2((*lastGood)->scope), scope[prefix - 1], getSrcInfo());
    // Current access is potentially covered by multiple bindings that are
    // not spanned by a parent binding; create such binding
    canonicalName = generateCanonicalName(name);
    // LOG(" ! generating new {}->{} @ {} @ {}", name, canonicalName, scope[prefix - 1],
    //     getSrcInfo());
    auto item = std::make_shared<SimplifyItem>(
        (*lastGood)->kind, (*lastGood)->baseName, canonicalName,
        (*lastGood)->moduleName,
        std::vector<int>(scope.begin(), scope.begin() + prefix),
        (*lastGood)->importPath);
    item->accessChecked = false;
    lastGood = it->second.insert(++lastGood, item);
    scopeStmts[scope[prefix - 1]].push_back(std::make_unique<AssignStmt>(
        std::make_unique<IdExpr>(canonicalName), nullptr, nullptr));
    scopeStmts[scope[prefix - 1]].push_back(std::make_unique<AssignStmt>(
        std::make_unique<IdExpr>(fmt::format("{}.__used__", canonicalName)),
        std::make_unique<BoolExpr>(false), nullptr));
    if (prefix == 1) {
      cache->globals[canonicalName] = nullptr;
      cache->globals[fmt::format("{}.__used__", canonicalName)] = nullptr;
    }
    hasUsed = true;
  }
  // Remove all bindings in the middle (i.e. unify them with the most dominant binding)
  for (auto i = it->second.begin(); i != it->second.end(); i++) {
    if (i == lastGood)
      break;
    cache->replacements[(*i)->canonicalName] = {canonicalName, hasUsed};
    cache->replacements[format("{}.__used__", (*i)->canonicalName)] = {
        format("{}.__used__", canonicalName), false};
    seqassert((*i)->canonicalName != canonicalName, "invalid replacement at {}: {}",
              getSrcInfo(), canonicalName);
    // LOG("   modify {} -> {} @ i", (*i)->canonicalName, canonicalName,
    //     (*i)->getSrcInfo());
  }
  it->second.erase(it->second.begin(), lastGood);
  return it->second.front();
}

std::string SimplifyContext::getBaseName() const { return bases.back().name; }

std::string SimplifyContext::getModule() const {
  std::string base = moduleName.status == ImportFile::STDLIB ? "std." : "";
  base += moduleName.module;
  if (startswith(base, "__main__"))
    base = base.substr(8);
  return base;
}

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

void SimplifyContext::dump(int pad) {
  auto ordered =
      std::map<std::string, decltype(map)::mapped_type>(map.begin(), map.end());
  LOG("module: {}", getModule());
  LOG("base: {}", getBaseName());
  LOG("scope: {}", combine2(scope, ","));
  for (auto &i : ordered) {
    std::string s;
    auto t = i.second.front();
    LOG("{}{:.<40} {} {:40} {:30} {}", std::string(pad * 2, ' '), i.first,
        (t->isFunc() ? "F" : (t->isType() ? "T" : (t->isImport() ? "I" : "V"))),
        t->canonicalName, t->getBaseName(), combine2(t->scope, ","));
  }
}

void SimplifyContext::addBlock() { Context<SimplifyItem>::addBlock(); }
void SimplifyContext::popBlock() { Context<SimplifyItem>::popBlock(); }

} // namespace codon::ast
