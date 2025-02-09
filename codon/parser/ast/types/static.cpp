// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

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

StaticType::StaticType(Cache *cache, const std::string &typeName)
    : ClassType(cache, typeName, typeName) {}

TypePtr StaticType::generalize(int atLevel) { return shared_from_this(); }

TypePtr StaticType::instantiate(int atLevel, int *unboundCount,
                                std::unordered_map<int, TypePtr> *cache) {
  return shared_from_this();
}

bool StaticType::canRealize() const { return true; }

bool StaticType::isInstantiated() const { return true; }

std::string StaticType::realizedName() const { return debugString(0); }

Type *StaticType::getNonStaticType() const { return cache->findClass(name); }

/*****************************************************************/

IntStaticType::IntStaticType(Cache *cache, int64_t i)
    : StaticType(cache, "int"), value(i) {}

int IntStaticType::unify(Type *typ, Unification *us) {
  if (auto t = typ->getIntStatic()) {
    return value == t->value ? 1 : -1;
  } else if (auto c = typ->getClass()) {
    return ClassType::unify(c, us);
  } else if (auto tl = typ->getLink()) {
    return tl->unify(this, us);
  } else {
    return -1;
  }
}

std::string IntStaticType::debugString(char mode) const {
  return mode == 0 ? fmt::format("{}", value) : fmt::format("Static[{}]", value);
}

Expr *IntStaticType::getStaticExpr() const { return cache->N<IntExpr>(value); }

/*****************************************************************/

StrStaticType::StrStaticType(Cache *cache, std::string s)
    : StaticType(cache, "str"), value(std::move(s)) {}

int StrStaticType::unify(Type *typ, Unification *us) {
  if (auto t = typ->getStrStatic()) {
    return value == t->value ? 1 : -1;
  } else if (auto c = typ->getClass()) {
    return ClassType::unify(c, us);
  } else if (auto tl = typ->getLink()) {
    return tl->unify(this, us);
  } else {
    return -1;
  }
}

std::string StrStaticType::debugString(char mode) const {
  return mode == 0 ? fmt::format("'{}'", escape(value))
                   : fmt::format("Static['{}']", escape(value));
}

Expr *StrStaticType::getStaticExpr() const { return cache->N<StringExpr>(value); }

/*****************************************************************/

BoolStaticType::BoolStaticType(Cache *cache, bool b)
    : StaticType(cache, "bool"), value(b) {}

int BoolStaticType::unify(Type *typ, Unification *us) {
  if (auto t = typ->getBoolStatic()) {
    return value == t->value ? 1 : -1;
  } else if (auto c = typ->getClass()) {
    return ClassType::unify(c, us);
  } else if (auto tl = typ->getLink()) {
    return tl->unify(this, us);
  } else {
    return -1;
  }
}

std::string BoolStaticType::debugString(char mode) const {
  return fmt::format("Static[{}]", value ? "True" : "False");
}

Expr *BoolStaticType::getStaticExpr() const { return cache->N<BoolExpr>(value); }

} // namespace codon::ast::types
