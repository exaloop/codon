// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/types/type.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

/// Undo a destructive unification.
void Type::Unification::undo() {
  for (size_t i = linked.size(); i-- > 0;) {
    linked[i]->getLink()->kind = LinkType::Unbound;
    linked[i]->getLink()->type = nullptr;
  }
  for (size_t i = leveled.size(); i-- > 0;) {
    seqassertn(leveled[i].first->getLink()->kind == LinkType::Unbound,
               "not unbound [{}]", leveled[i].first->getSrcInfo());
    leveled[i].first->getLink()->level = leveled[i].second;
  }
  for (auto &t : traits)
    t->getLink()->trait = nullptr;
  for (auto &t : statics)
    t->getLink()->staticKind = LiteralKind::Runtime;
}

Type::Type(const std::shared_ptr<Type> &typ) : cache(typ->cache) {
  setSrcInfo(typ->getSrcInfo());
}

Type::Type(Cache *cache, const SrcInfo &info) : cache(cache) { setSrcInfo(info); }

Type *Type::follow() { return this; }

bool Type::hasUnbounds(bool) const { return false; }

std::vector<Type *> Type::getUnbounds(bool) const { return {}; }

std::string Type::toString() const { return debugString(2); }

std::string Type::prettyString() const { return debugString(0); }

bool Type::is(const std::string &s) { return getClass() && getClass()->name == s; }

LiteralKind Type::getStaticKind() {
  if (auto s = getStatic())
    return s->getStaticKind();
  if (auto l = follow()->getLink())
    return l->staticKind;
  return LiteralKind::Runtime;
}

LiteralKind Type::literalFromString(const std::string &s) {
  if (s == "int")
    return LiteralKind::Int;
  if (s == "str")
    return LiteralKind::String;
  if (s == "bool")
    return LiteralKind::Bool;
  return LiteralKind::Runtime;
}

std::string Type::stringFromLiteral(LiteralKind k) {
  if (k == LiteralKind::Int)
    return "int";
  if (k == LiteralKind::String)
    return "str";
  if (k == LiteralKind::Bool)
    return "bool";
  return "";
}

Type *Type::operator<<(Type *t) {
  seqassert(t, "rhs is nullptr");
  types::Type::Unification undo;
  if (unify(t, &undo) >= 0) {
    return this;
  } else {
    undo.undo();
    return nullptr;
  }
}

} // namespace codon::ast::types
