// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/stmt.h"
#include "codon/parser/ast/types/function.h"
#include "codon/parser/cache.h"

namespace codon::ast::types {

FuncType::FuncType(const ClassType *baseType, FunctionStmt *ast,
                   std::vector<Generic> funcGenerics, TypePtr funcParent)
    : ClassType(baseType), ast(ast), funcGenerics(std::move(funcGenerics)),
      funcParent(std::move(funcParent)) {}

int FuncType::unify(Type *typ, Unification *us) {
  if (this == typ)
    return 0;
  int s1 = 2, s = 0;
  if (auto t = typ->getFunc()) {
    // Check if names and parents match.
    if (ast->getName() != t->ast->getName() ||
        (static_cast<bool>(funcParent) ^ static_cast<bool>(t->funcParent)))
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

TypePtr FuncType::generalize(int atLevel) const {
  std::vector<Generic> fg;
  for (auto &t : funcGenerics)
    fg.push_back(t.generalize(atLevel));
  auto p = funcParent ? funcParent->generalize(atLevel) : nullptr;

  auto r = std::static_pointer_cast<ClassType>(this->ClassType::generalize(atLevel));
  auto t = std::make_shared<FuncType>(r->getClass(), ast, fg, p);
  return t;
}

TypePtr FuncType::instantiate(int atLevel, int *unboundCount,
                              std::unordered_map<int, TypePtr> *cache) const {
  std::vector<Generic> fg;
  for (auto &t : funcGenerics) {
    fg.push_back(t.instantiate(atLevel, unboundCount, cache));
    if (cache && fg.back().type) {
      if (auto c = in(*cache, t.id))
        *c = fg.back().type;
    }
  }
  auto p = funcParent ? funcParent->instantiate(atLevel, unboundCount, cache) : nullptr;
  auto r = std::static_pointer_cast<ClassType>(
      this->ClassType::instantiate(atLevel, unboundCount, cache));
  auto t = std::make_shared<FuncType>(r->getClass(), ast, fg, p);
  return t;
}

bool FuncType::hasUnbounds(bool includeGenerics) const {
  for (auto &t : funcGenerics)
    if (t.type && t.type->hasUnbounds(includeGenerics))
      return true;
  if (funcParent && funcParent->hasUnbounds(includeGenerics))
    return true;
  for (const auto &a : *this)
    if (a.getType()->hasUnbounds(includeGenerics))
      return true;
  return getRetType()->hasUnbounds(includeGenerics);
}

std::vector<Type *> FuncType::getUnbounds(bool includeGenerics) const {
  std::vector<Type *> u;
  for (auto &t : funcGenerics)
    if (t.type) {
      auto tu = t.type->getUnbounds(includeGenerics);
      u.insert(u.begin(), tu.begin(), tu.end());
    }
  if (funcParent) {
    auto tu = funcParent->getUnbounds(includeGenerics);
    u.insert(u.begin(), tu.begin(), tu.end());
  }
  // Important: return type unbounds are not important, so skip them.
  for (const auto &a : *this) {
    auto tu = a.getType()->getUnbounds(includeGenerics);
    u.insert(u.begin(), tu.begin(), tu.end());
  }
  return u;
}

bool FuncType::canRealize() const {
  bool allowPassThrough = ast->hasAttribute(Attr::AllowPassThrough);
  // Important: return type does not have to be realized.
  for (int ai = 0; ai < size(); ai++)
    if (!(*this)[ai]->getFunc() && !(*this)[ai]->canRealize()) {
      if (!allowPassThrough)
        return false;
      for (auto &u : (*this)[ai]->getUnbounds(true))
        if (u->getLink()->kind == LinkType::Generic || !u->getLink()->passThrough)
          return false;
    }
  bool generics =
      std::ranges::all_of(funcGenerics.begin(), funcGenerics.end(),
                          [](auto &a) { return !a.type || a.type->canRealize(); });
  if (generics && funcParent && !funcParent->canRealize()) {
    if (!allowPassThrough)
      return false;
    for (auto &u : funcParent->getUnbounds(true)) {
      if (u->getLink()->kind == LinkType::Generic || !u->getLink()->passThrough)
        return false;
    }
  }
  return generics;
}

bool FuncType::isInstantiated() const {
  TypePtr removed = nullptr;
  auto retType = getRetType();
  if (retType->getFunc() && retType->getFunc()->funcParent.get() == this) {
    removed = retType->getFunc()->funcParent;
    retType->getFunc()->funcParent = nullptr;
  }
  auto res = std::ranges::all_of(
                 funcGenerics.begin(), funcGenerics.end(),
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
      gs.push_back(mode < 2 ? a.type->debugString(mode)
                            : (cache->rev(a.name) + "=" + a.type->debugString(mode)));
  std::string s = join(gs, ",");
  std::vector<std::string> as;
  // Important: return type does not have to be realized.
  if (mode == 2)
    as.push_back("RET=" + getRetType()->debugString(mode));

  if (mode < 2 || !ast) {
    for (const auto &a : *this) {
      as.push_back(a.debugString(mode));
    }
  } else {
    for (size_t i = 0, si = 0; i < ast->size(); i++) {
      if ((*ast)[i].isGeneric())
        continue;
      as.push_back(((*ast)[i].getName() + "=" + (*this)[si++]->debugString(mode)));
    }
  }
  std::string a = join(as, ",");
  s = s.empty() ? a : join(std::vector<std::string>{s, a}, ";");

  seqassert(ast, "ast must not be null");
  auto fnname = ast->getName();
  if (mode == 0) {
    fnname = cache->rev(ast->getName());
  }
  if (mode == 2 && funcParent)
    s += ";" + funcParent->debugString(mode);
  return fnname + (s.empty() ? "" : ("[" + s + "]"));
}

std::string FuncType::realizedName() const {
  std::vector<std::string> gs;
  for (auto &a : funcGenerics)
    if (!a.name.empty())
      gs.push_back(a.realizedName());
  std::string s = join(gs, ",");
  std::vector<std::string> as;
  // Important: return type does not have to be realized.
  for (const auto &a : *this)
    as.push_back(a.getType()->getFunc() ? a.getType()->getFunc()->realizedName()
                                        : a.realizedName());
  std::string a = join(as, ",");
  s = s.empty() ? a : join(std::vector<std::string>{a, s}, ",");
  return (funcParent ? funcParent->realizedName() + ":" : "") + ast->getName() +
         (s.empty() ? "" : ("[" + s + "]"));
}

Type *FuncType::getRetType() const { return generics[1].type.get(); }

std::string FuncType::getFuncName() const { return ast->getName(); }

Type *FuncType::operator[](size_t i) const {
  return generics[0].type->getClass()->generics[i].getType();
}

std::vector<ClassType::Generic>::iterator FuncType::begin() const {
  return generics[0].type->getClass()->generics.begin();
}

std::vector<ClassType::Generic>::iterator FuncType::end() const {
  return generics[0].type->getClass()->generics.end();
}

size_t FuncType::size() const { return generics[0].type->getClass()->generics.size(); }

bool FuncType::empty() const { return generics[0].type->getClass()->generics.empty(); }

} // namespace codon::ast::types
