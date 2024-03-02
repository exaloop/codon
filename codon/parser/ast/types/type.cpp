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

std::vector<std::shared_ptr<Type>> Type::getUnbounds() const { return {}; }

std::string Type::toString() const { return debugString(1); }

std::string Type::prettyString() const { return debugString(0); }

bool Type::is(const std::string &s) { return getClass() && getClass()->name == s; }

char Type::isStaticType() {
  auto t = follow();
  if (auto s = t->getStatic())
    return char(s->expr->staticValue.type);
  if (auto l = t->getLink())
    return l->isStatic;
  return false;
}

TypePtr Type::makeType(Cache *cache, const std::string &name,
                       const std::string &niceName, bool isRecord) {
  if (name == "Union")
    return std::make_shared<UnionType>(cache);
  if (isRecord)
    return std::make_shared<RecordType>(cache, name, niceName);
  return std::make_shared<ClassType>(cache, name, niceName);
}

std::shared_ptr<StaticType> Type::makeStatic(Cache *cache, const ExprPtr &expr) {
  return std::make_shared<StaticType>(cache, expr);
}

} // namespace codon::ast::types
