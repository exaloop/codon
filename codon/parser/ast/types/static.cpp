// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/ast/types/static.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

StaticType::StaticType(Cache *cache, int64_t i)
    : Type(cache), kind(StaticType::Int), value((void *)(new int64_t(i))) {}

StaticType::StaticType(Cache *cache, const std::string &s)
    : Type(cache), kind(StaticType::String), value((void *)(new std::string(s))) {}

StaticType::~StaticType() {
  if (kind == Int)
    delete (int64_t *)value;
  if (kind == String)
    delete (std::string *)value;
}

int StaticType::unify(Type *typ, Unification *us) {
  if (auto t = typ->getStatic()) {
    if (kind != t->kind)
      return -1;
    if (isString())
      return getString() == t->getString() ? 1 : -1;
    if (isInt())
      return getInt() == t->getInt() ? 1 : -1;
  } else if (auto tl = typ->getLink()) {
    return tl->unify(this, us);
  }
  return -1;
}

TypePtr StaticType::generalize(int atLevel) { return shared_from_this(); }

TypePtr StaticType::instantiate(int atLevel, int *unboundCount,
                                std::unordered_map<int, TypePtr> *cache) {
  return shared_from_this();
}

std::vector<TypePtr> StaticType::getUnbounds() const { return {}; }

bool StaticType::canRealize() const { return true; }

std::string StaticType::debugString(char mode) const {
  if (isString())
    return fmt::format("Static['{}']", getString());
  if (isInt())
    return fmt::format("Static[{}]", getInt());
  seqassert(false, "cannot happen");
  return "";
}

std::string StaticType::realizedName() const {
  seqassert(canRealize(), "cannot realize {}", toString());
  return debugString(0);
}

bool StaticType::isString() const { return kind == StaticType::String; }

std::string StaticType::getString() const {
  seqassert(isString(), "not a string");
  return *(std::string *)value;
}

bool StaticType::isInt() const { return kind == StaticType::Int; }

int64_t StaticType::getInt() const {
  seqassert(isInt(), "not an int");
  return *(int64_t *)value;
}

std::string StaticType::getTypeName() const { return StaticType::getTypeName(kind); }

std::string StaticType::getTypeName(StaticType::Kind kind) {
  return kind == StaticType::String ? "str" : "int";
}

std::string StaticType::getTypeName(char kind) {
  return getTypeName(StaticType::Kind(kind));
}

std::shared_ptr<Expr> StaticType::getStaticExpr() const {
  if (isString())
    return std::make_shared<StringExpr>(getString());
  if (isInt())
    return std::make_shared<IntExpr>(getInt());
  seqassert(false, "cannot happen");
  return nullptr;
}

} // namespace codon::ast::types
