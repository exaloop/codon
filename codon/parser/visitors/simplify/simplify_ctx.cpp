#include "simplify_ctx.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon {
namespace ast {

SimplifyItem::SimplifyItem(Kind k, std::string base, std::string canonicalName,
                           bool global)
    : kind(k), base(move(base)), canonicalName(move(canonicalName)), global(global) {}

SimplifyContext::SimplifyContext(std::string filename, Cache *cache)
    : Context<SimplifyItem>(move(filename)), cache(move(cache)),
      isStdlibLoading(false), moduleName{ImportFile::PACKAGE, "", ""}, canAssign(true),
      allowTypeOf(true), substitutions(nullptr) {}

SimplifyContext::Base::Base(std::string name, std::shared_ptr<Expr> ast, int attributes)
    : name(move(name)), ast(move(ast)), attributes(attributes) {}

std::shared_ptr<SimplifyItem> SimplifyContext::add(SimplifyItem::Kind kind,
                                                   const std::string &name,
                                                   const std::string &canonicalName,
                                                   bool global) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  auto t = std::make_shared<SimplifyItem>(kind, getBase(), canonicalName, global);
  Context<SimplifyItem>::add(name, t);
  Context<SimplifyItem>::add(canonicalName, t);
  return t;
}

std::shared_ptr<SimplifyItem> SimplifyContext::find(const std::string &name) const {
  auto t = Context<SimplifyItem>::find(name);
  if (t)
    return t;
  // Item is not found in the current module. Time to look in the standard library!
  auto stdlib = cache->imports[STDLIB_IMPORT].ctx;
  if (stdlib.get() != this) {
    t = stdlib->find(name);
    if (t)
      return t;
  }
  // Check if there is a global mangled function with this name (for Simplify callbacks)
  auto fn = cache->functions.find(name);
  if (fn != cache->functions.end())
    return std::make_shared<SimplifyItem>(SimplifyItem::Func, "", name, true);
  return nullptr;
}

std::string SimplifyContext::getBase() const {
  if (bases.empty())
    return "";
  return bases.back().name;
}

std::string SimplifyContext::generateCanonicalName(const std::string &name,
                                                   bool includeBase) const {
  std::string newName = name;
  if (includeBase && name.find('.') == std::string::npos) {
    std::string base = getBase();
    if (base.empty()) {
      base = moduleName.status == ImportFile::STDLIB ? "std." : "";
      base += moduleName.module;
      if (startswith(base, "__main__"))
        base = base.substr(8);
    }
    newName = (base.empty() ? "" : (base + ".")) + newName;
  }
  auto num = cache->identifierCount[newName]++;
  newName = num ? format("{}.{}", newName, num) : newName;
  if (newName != name)
    cache->identifierCount[newName]++;
  cache->reverseIdentifierLookup[newName] = name;
  return newName;
}

void SimplifyContext::dump(int pad) {
  auto ordered =
      std::map<std::string, decltype(map)::mapped_type>(map.begin(), map.end());
  LOG("base: {}", getBase());
  for (auto &i : ordered) {
    std::string s;
    auto t = i.second.front().second;
    LOG("{}{:.<25} {} {}", std::string(pad * 2, ' '), i.first, t->canonicalName,
        t->getBase());
  }
}

} // namespace ast
} // namespace codon
