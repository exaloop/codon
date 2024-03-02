// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/types/link.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

LinkType::LinkType(Cache *cache, Kind kind, int id, int level, TypePtr type,
                   char isStatic, std::shared_ptr<Trait> trait, TypePtr defaultType,
                   std::string genericName)
    : Type(cache), kind(kind), id(id), level(level), type(std::move(type)),
      isStatic(isStatic), trait(std::move(trait)), genericName(std::move(genericName)),
      defaultType(std::move(defaultType)) {
  seqassert((this->type && kind == Link) || (!this->type && kind == Generic) ||
                (!this->type && kind == Unbound),
            "inconsistent link state");
}

LinkType::LinkType(TypePtr type)
    : Type(type), kind(Link), id(0), level(0), type(std::move(type)), isStatic(0),
      trait(nullptr), defaultType(nullptr) {
  seqassert(this->type, "link to nullptr");
}

int LinkType::unify(Type *typ, Unification *undo) {
  if (kind == Link) {
    // Case 1: Just follow the link
    return type->unify(typ, undo);
  } else if (kind == Generic) {
    // Case 2: Generic types cannot be unified.
    return -1;
  } else {
    // Case 3: Unbound unification
    if (isStaticType() != typ->isStaticType())
      return -1;
    if (auto ts = typ->getStatic()) {
      if (ts->expr->getId())
        return unify(ts->generics[0].type.get(), undo);
    }
    if (auto t = typ->getLink()) {
      if (t->kind == Link)
        return t->type->unify(this, undo);
      else if (t->kind == Generic)
        return -1;
      else {
        if (id == t->id) {
          // Identical unbound types get a score of 1
          return 1;
        } else if (id < t->id) {
          // Always merge a newer type into the older type (e.g. keep the types with
          // lower IDs around).
          return t->unify(this, undo);
        }
      }
    }
    // Ensure that we do not have recursive unification! (e.g. unify ?1 with list[?1])
    if (occurs(typ, undo))
      return -1;

    if (trait && trait->unify(typ, undo) == -1)
      return -1;

    // ⚠️ Unification: destructive part.
    seqassert(!type, "type has been already unified or is in inconsistent state");
    if (undo) {
      LOG_TYPECHECK("[unify] {} := {}", id, typ->debugString(2));
      // Link current type to typ and ensure that this modification is recorded in undo.
      undo->linked.push_back(this);
      kind = Link;
      seqassert(!typ->getLink() || typ->getLink()->kind != Unbound ||
                    typ->getLink()->id <= id,
                "type unification is not consistent");
      type = typ->follow();
      if (auto t = type->getLink())
        if (trait && t->kind == Unbound && !t->trait) {
          undo->traits.push_back(t.get());
          t->trait = trait;
        }
    }
    return 0;
  }
}

TypePtr LinkType::generalize(int atLevel) {
  if (kind == Generic) {
    return shared_from_this();
  } else if (kind == Unbound) {
    if (level >= atLevel)
      return std::make_shared<LinkType>(
          cache, Generic, id, 0, nullptr, isStatic,
          trait ? std::static_pointer_cast<Trait>(trait->generalize(atLevel)) : nullptr,
          defaultType ? defaultType->generalize(atLevel) : nullptr, genericName);
    else
      return shared_from_this();
  } else {
    seqassert(type, "link is null");
    return type->generalize(atLevel);
  }
}

TypePtr LinkType::instantiate(int atLevel, int *unboundCount,
                              std::unordered_map<int, TypePtr> *cache) {
  if (kind == Generic) {
    if (cache && cache->find(id) != cache->end())
      return (*cache)[id];
    auto t = std::make_shared<LinkType>(
        this->cache, Unbound, unboundCount ? (*unboundCount)++ : id, atLevel, nullptr,
        isStatic,
        trait ? std::static_pointer_cast<Trait>(
                    trait->instantiate(atLevel, unboundCount, cache))
              : nullptr,
        defaultType ? defaultType->instantiate(atLevel, unboundCount, cache) : nullptr,
        genericName);
    if (cache)
      (*cache)[id] = t;
    return t;
  } else if (kind == Unbound) {
    return shared_from_this();
  } else {
    seqassert(type, "link is null");
    return type->instantiate(atLevel, unboundCount, cache);
  }
}

TypePtr LinkType::follow() {
  if (kind == Link)
    return type->follow();
  else
    return shared_from_this();
}

std::vector<TypePtr> LinkType::getUnbounds() const {
  if (kind == Unbound)
    return {std::const_pointer_cast<Type>(shared_from_this())};
  else if (kind == Link)
    return type->getUnbounds();
  return {};
}

bool LinkType::canRealize() const {
  if (kind != Link)
    return false;
  else
    return type->canRealize();
}

bool LinkType::isInstantiated() const { return kind == Link && type->isInstantiated(); }

std::string LinkType::debugString(char mode) const {
  if (kind == Unbound || kind == Generic) {
    if (mode == 2) {
      return fmt::format("{}{}{}{}", genericName.empty() ? "" : genericName + ":",
                         kind == Unbound ? '?' : '#', id,
                         trait ? ":" + trait->debugString(mode) : "");
    }
    if (trait)
      return trait->debugString(mode);
    return (genericName.empty() ? (mode ? "?" : "<unknown type>") : genericName);
  }
  return type->debugString(mode);
}

std::string LinkType::realizedName() const {
  if (kind == Unbound || kind == Generic)
    return "?";
  seqassert(kind == Link, "unexpected generic link");
  return type->realizedName();
}

std::shared_ptr<LinkType> LinkType::getLink() {
  return std::static_pointer_cast<LinkType>(shared_from_this());
}

std::shared_ptr<FuncType> LinkType::getFunc() {
  return kind == Link ? type->getFunc() : nullptr;
}

std::shared_ptr<PartialType> LinkType::getPartial() {
  return kind == Link ? type->getPartial() : nullptr;
}

std::shared_ptr<ClassType> LinkType::getClass() {
  return kind == Link ? type->getClass() : nullptr;
}

std::shared_ptr<RecordType> LinkType::getRecord() {
  return kind == Link ? type->getRecord() : nullptr;
}

std::shared_ptr<StaticType> LinkType::getStatic() {
  return kind == Link ? type->getStatic() : nullptr;
}

std::shared_ptr<UnionType> LinkType::getUnion() {
  return kind == Link ? type->getUnion() : nullptr;
}

std::shared_ptr<LinkType> LinkType::getUnbound() {
  if (kind == Unbound)
    return std::static_pointer_cast<LinkType>(shared_from_this());
  if (kind == Link)
    return type->getUnbound();
  return nullptr;
}

bool LinkType::occurs(Type *typ, Type::Unification *undo) {
  if (auto tl = typ->getLink()) {
    if (tl->kind == Unbound) {
      if (tl->id == id)
        return true;
      if (tl->trait && occurs(tl->trait.get(), undo))
        return true;
      if (undo && tl->level > level) {
        undo->leveled.emplace_back(make_pair(tl.get(), tl->level));
        tl->level = level;
      }
      return false;
    } else if (tl->kind == Link) {
      return occurs(tl->type.get(), undo);
    } else {
      return false;
    }
  } else if (auto ts = typ->getStatic()) {
    for (auto &g : ts->generics)
      if (g.type && occurs(g.type.get(), undo))
        return true;
    return false;
  }
  if (auto tc = typ->getClass()) {
    for (auto &g : tc->generics)
      if (g.type && occurs(g.type.get(), undo))
        return true;
    if (auto tr = typ->getRecord())
      for (auto &t : tr->args)
        if (occurs(t.get(), undo))
          return true;
    return false;
  } else {
    return false;
  }
}

} // namespace codon::ast::types
