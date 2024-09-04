// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/types/class.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

FuncType::FuncType(ClassType *baseType, FunctionStmt *ast, size_t index,
                   std::vector<Generic> funcGenerics, TypePtr funcParent)
    : ClassType(baseType), ast(ast), index(index),
      funcGenerics(std::move(funcGenerics)), funcParent(std::move(funcParent)) {}

int FuncType::unify(Type *typ, Unification *us) {
  if (this == typ)
    return 0;
  int s1 = 2, s = 0;
  if (auto t = typ->getFunc()) {
    // Check if names and parents match.
    if (ast->getName() != t->ast->getName() || index != t->index ||
        (bool(funcParent) ^ bool(t->funcParent)))
      return -1;
    if (funcParent && (s = funcParent->unify(t->funcParent.get(), us)) == -1) {
      return -1;
    }
    s1 += s;
    // Check if function generics match.
    seqassert(funcGenerics.size() == t->funcGenerics.size(),
              "generic size mismatch for {}", ast->getName());
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
  auto r = this->ClassType::generalize(atLevel);
  auto t = std::make_shared<FuncType>(r->getClass(), ast, index, g, p);
  return t;
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
  auto r = this->ClassType::instantiate(atLevel, unboundCount, cache);
  auto t = std::make_shared<FuncType>(r->getClass(), ast, index, g, p);
  return t;
}

bool FuncType::hasUnbounds(bool includeGenerics) const {
  for (auto &t : funcGenerics)
    if (t.type && t.type->hasUnbounds(includeGenerics))
      return true;
  if (funcParent && funcParent->hasUnbounds(includeGenerics))
    return true;
  // Important: return type unbounds are not important, so skip them
  for (const auto &a : *this)
    if (a.getType()->hasUnbounds(includeGenerics))
      return true;
  return getRetType()->hasUnbounds(includeGenerics);
}

std::vector<Type *> FuncType::getUnbounds() const {
  std::vector<Type *> u;
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
  for (const auto &a : *this) {
    auto tu = a.getType()->getUnbounds();
    u.insert(u.begin(), tu.begin(), tu.end());
  }
  return u;
}

bool FuncType::canRealize() const {
  // Important: return type does not have to be realized.
  bool skipSelf = ast->hasAttribute(Attr::RealizeWithoutSelf);

  for (int ai = skipSelf; ai < size(); ai++)
    if (!(*this)[ai]->getFunc() && !(*this)[ai]->canRealize())
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
  for (const auto &a : *this)
    as.push_back(a.getType()->debugString(mode));
  std::string a = join(as, ",");
  s = s.empty() ? a : join(std::vector<std::string>{s, a}, ";");

  auto fnname = ast->getName();
  if (mode == 0) {
    fnname = cache->rev(ast->getName());
  }
  if (mode && index)
    fnname += fmt::format("/{}", index);
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
  for (const auto &a : *this)
    as.push_back(a.getType()->getFunc() ? a.getType()->getFunc()->realizedName()
                                        : a.getType()->realizedName());
  std::string a = join(as, ",");
  s = s.empty() ? a : join(std::vector<std::string>{a, s}, ",");
  return fmt::format("{}{}{}{}", funcParent ? funcParent->realizedName() + ":" : "",
                     ast->getName(), index ? fmt::format("/{}", index) : "",
                     s.empty() ? "" : fmt::format("[{}]", s));
}

Type *FuncType::getRetType() const { return generics[1].type.get(); }

std::string FuncType::getFuncName() const { return ast->getName(); }

Type *FuncType::operator[](int i) const {
  return generics[0].type->getClass()->generics[i].getType();
}

std::vector<ClassType::Generic>::iterator FuncType::begin() const {
  return generics[0].type->getClass()->generics.begin();
}

std::vector<ClassType::Generic>::iterator FuncType::end() const {
  return generics[0].type->getClass()->generics.begin() +
         generics[0].type->getClass()->generics.size();
}

size_t FuncType::size() const { return generics[0].type->getClass()->generics.size(); }

bool FuncType::empty() const { return generics[0].type->getClass()->generics.empty(); }

} // namespace codon::ast::types
