// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
    linked[i]->kind = LinkType::Unbound;
    linked[i]->type = nullptr;
  }
  for (size_t i = leveled.size(); i-- > 0;) {
    seqassertn(leveled[i].first->kind == LinkType::Unbound, "not unbound [{}]",
               leveled[i].first->getSrcInfo());
    leveled[i].first->level = leveled[i].second;
  }
  for (auto &t : traits)
    t->trait = nullptr;
}

Type::Type(const std::shared_ptr<Type> &typ) : cache(typ->cache) {
  setSrcInfo(typ->getSrcInfo());
}

Type::Type(Cache *cache, const SrcInfo &info) : cache(cache) { setSrcInfo(info); }

TypePtr Type::follow() { return shared_from_this(); }

bool Type::hasUnbounds(bool) const { return false; }

std::vector<std::shared_ptr<Type>> Type::getUnbounds() const { return {}; }

std::string Type::toString() const { return debugString(1); }

std::string Type::prettyString() const { return debugString(0); }

bool Type::is(const std::string &s) { return getClass() && getClass()->name == s; }

char Type::isStaticType() {
  auto t = follow();
  if (t->getBoolStatic())
    return 3;
  if (t->getStrStatic())
    return 2;
  if (t->getIntStatic())
    return 1;
  if (auto l = t->getLink())
    return l->isStatic;
  return 0;
}

Type *Type::operator<<(const TypePtr &t) {
  seqassert(t, "rhs is nullptr");
  types::Type::Unification undo;
  if (unify(t.get(), &undo) >= 0) {
    return this;
  } else {
    undo.undo();
    return nullptr;
  }
}


} // namespace codon::ast::types
