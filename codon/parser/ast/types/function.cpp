// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/types/class.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

FuncType::FuncType(const std::shared_ptr<ClassType> &baseType, FunctionStmt *ast,
                   std::vector<Generic> funcGenerics, TypePtr funcParent)
    : ClassType(baseType), ast(ast), funcGenerics(std::move(funcGenerics)),
      funcParent(std::move(funcParent)) {}

int FuncType::unify(Type *typ, Unification *us) {
  if (this == typ)
    return 0;
  int s1 = 2, s = 0;
  if (auto t = typ->getFunc()) {
    // Check if names and parents match.
    if (ast->name != t->ast->name || (bool(funcParent) ^ bool(t->funcParent)))
      return -1;
    if (funcParent && (s = funcParent->unify(t->funcParent.get(), us)) == -1) {
      return -1;
    }
    s1 += s;
    // Check if function generics match.
    seqassert(funcGenerics.size() == t->funcGenerics.size(),
              "generic size mismatch for {}", ast->name);
    for (int i = 0; i < funcGenerics.size(); i++) {
      if ((s = funcGenerics[i].type->unify(t->funcGenerics[i].type.get(), us)) == -1)
        return -1;
      s1 += s;
    }
  }
  s = this->ClassType::unify(typ, us);
  return s == -1 ? s : s1 + s;
}

TypePtr FuncType::generalize(int atLevel) {
  auto g = funcGenerics;
  for (auto &t : g)
    t.type = t.type ? t.type->generalize(atLevel) : nullptr;
  auto p = funcParent ? funcParent->generalize(atLevel) : nullptr;
  return std::make_shared<FuncType>(
      std::static_pointer_cast<ClassType>(this->ClassType::generalize(atLevel)), ast, g,
      p);
}

TypePtr FuncType::instantiate(int atLevel, int *unboundCount,
                              std::unordered_map<int, TypePtr> *cache) {
  auto g = funcGenerics;
  for (auto &t : g)
    if (t.type) {
      t.type = t.type->instantiate(atLevel, unboundCount, cache);
      if (cache && cache->find(t.id) == cache->end())
        (*cache)[t.id] = t.type;
    }
  auto p = funcParent ? funcParent->instantiate(atLevel, unboundCount, cache) : nullptr;
  return std::make_shared<FuncType>(
      std::static_pointer_cast<ClassType>(
          this->ClassType::instantiate(atLevel, unboundCount, cache)),
      ast, g, p);
}

bool FuncType::hasUnbounds(bool includeGenerics) const {
  for (auto &t : funcGenerics)
    if (t.type && t.type->hasUnbounds(includeGenerics))
      return true;
  if (funcParent && funcParent->hasUnbounds(includeGenerics))
    return true;
  // Important: return type unbounds are not important, so skip them.
  for (auto &a : getArgTypes())
    if (a && a->hasUnbounds(includeGenerics))
      return true;
  return getRetType()->hasUnbounds(includeGenerics);
}

std::vector<TypePtr> FuncType::getUnbounds() const {
  std::vector<TypePtr> u;
  for (auto &t : funcGenerics)
    if (t.type) {
      auto tu = t.type->getUnbounds();
      u.insert(u.begin(), tu.begin(), tu.end());
    }
  if (funcParent) {
    auto tu = funcParent->getUnbounds();
    u.insert(u.begin(), tu.begin(), tu.end());
  }
  // Important: return type unbounds are not important, so skip them.
  for (auto &a : getArgTypes()) {
    auto tu = a->getUnbounds();
    u.insert(u.begin(), tu.begin(), tu.end());
  }
  return u;
}

bool FuncType::canRealize() const {
  // Important: return type does not have to be realized.
  bool skipSelf = ast->hasAttr(Attr::RealizeWithoutSelf);

  auto args = getArgTypes();
  for (int ai = skipSelf; ai < args.size(); ai++)
    if (!args[ai]->getFunc() && !args[ai]->canRealize())
      return false;
  bool generics = std::all_of(funcGenerics.begin(), funcGenerics.end(),
                              [](auto &a) { return !a.type || a.type->canRealize(); });
  if (!skipSelf)
    generics &= (!funcParent || funcParent->canRealize());
  return generics;
}

bool FuncType::isInstantiated() const {
  TypePtr removed = nullptr;
  auto retType = getRetType();
  if (retType->getFunc() && retType->getFunc()->funcParent.get() == this) {
    removed = retType->getFunc()->funcParent;
    retType->getFunc()->funcParent = nullptr;
  }
  auto res = std::all_of(funcGenerics.begin(), funcGenerics.end(),
                         [](auto &a) { return !a.type || a.type->isInstantiated(); }) &&
             (!funcParent || funcParent->isInstantiated()) &&
             this->ClassType::isInstantiated();
  if (removed)
    retType->getFunc()->funcParent = removed;
  return res;
}

std::string FuncType::debugString(char mode) const {
  std::vector<std::string> gs;
  for (auto &a : funcGenerics)
    if (!a.name.empty())
      gs.push_back(a.type->debugString(mode));
  std::string s = join(gs, ",");
  std::vector<std::string> as;
  // Important: return type does not have to be realized.
  if (mode == 2)
    as.push_back(getRetType()->debugString(mode));
  for (auto &a : getArgTypes())
    as.push_back(a->debugString(mode));
  std::string a = join(as, ",");
  s = s.empty() ? a : join(std::vector<std::string>{a, s}, ",");

  auto fnname = ast->name;
  if (mode == 0) {
    fnname = cache->rev(ast->name);
  }
  if (mode == 2 && funcParent)
    s += fmt::format(";{}", funcParent->debugString(mode));
  return fmt::format("{}{}", fnname, s.empty() ? "" : fmt::format("[{}]", s));
}

std::string FuncType::realizedName() const {
  std::vector<std::string> gs;
  for (auto &a : funcGenerics)
    if (!a.name.empty())
      gs.push_back(a.type->realizedName());
  std::string s = join(gs, ",");
  std::vector<std::string> as;
  // Important: return type does not have to be realized.
  for (auto &a : getArgTypes())
    as.push_back(a->getFunc() ? a->getFunc()->realizedName() : a->realizedName());
  std::string a = join(as, ",");
  s = s.empty() ? a : join(std::vector<std::string>{a, s}, ",");
  return fmt::format("{}{}{}", funcParent ? funcParent->realizedName() + ":" : "",
                     ast->name, s.empty() ? "" : fmt::format("[{}]", s));
}

std::vector<TypePtr> FuncType::getArgTypes() const {
  auto tup = generics[0].type->getClass();
  seqassert(tup->is(TYPE_TUPLE), "bad function def");
  std::vector<TypePtr> t;
  t.reserve(tup->generics.size());
  for (auto &g : tup->generics)
    t.push_back(g.type);
  return t;
}

TypePtr FuncType::getRetType() const { return generics[1].type; }

} // namespace codon::ast::types
