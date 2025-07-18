// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast/types/link.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast::types {

LinkType::LinkType(Cache *cache, Kind kind, int id, int level, TypePtr type,
                   LiteralKind staticKind, std::shared_ptr<Trait> trait,
                   TypePtr defaultType, std::string genericName, bool passThrough)
    : Type(cache), kind(kind), id(id), level(level), type(std::move(type)),
      staticKind(staticKind), trait(std::move(trait)),
      genericName(std::move(genericName)), defaultType(std::move(defaultType)),
      passThrough(passThrough) {
  seqassert((this->type && kind == Link) || (!this->type && kind == Generic) ||
                (!this->type && kind == Unbound),
            "inconsistent link state");
}

LinkType::LinkType(TypePtr type)
    : Type(type), kind(Link), id(0), level(0), type(std::move(type)),
      staticKind(Runtime), trait(nullptr), defaultType(nullptr), passThrough(false) {
  seqassert(this->type, "link to nullptr");
}

int LinkType::unify(Type *typ, Unification *undo) {
  if (kind == Link) {
    // Case 1: Just follow the link
    return type->unify(typ, undo);
  } else {
    // Case 3: Unbound unification
    if (getStaticKind() != typ->getStaticKind()) {
      if (!getStaticKind()) {
        // other one is; move this to non-static equivalent
        if (undo) {
          undo->statics.push_back(shared_from_this());
          staticKind = typ->getStaticKind();
        }
      } else {
        return -1;
      }
    }
    if (auto t = typ->getLink()) {
      if (t->kind == Link)
        return t->type->unify(this, undo);
      if (kind != t->kind)
        return -1;
      // Identical unbound types get a score of 1
      if (id == t->id)
        return 1;
      // Generics must have matching IDs unless we are doing non-destructive unification
      if (kind == Generic)
        return undo ? -1 : 1;
      // Always merge a newer type into the older type (e.g. keep the types with
      // lower IDs around).
      if (id < t->id)
        return t->unify(this, undo);
    } else if (kind == Generic) {
      return -1;
    }
    // Generics must be handled by now; only unbounds can be unified!
    seqassertn(kind == Unbound, "not an unbound");

    // Ensure that we do not have recursive unification! (e.g. unify ?1 with list[?1])
    if (occurs(typ, undo))
      return -1;
    // Handle traits
    if (trait && trait->unify(typ, undo) == -1)
      return -1;
    // ⚠️ Unification: destructive part.
    seqassert(!type, "type has been already unified or is in inconsistent state");
    if (undo) {
      LOG_TYPECHECK("[unify] {} := {}", id, typ->debugString(2));
      // Link current type to typ and ensure that this modification is recorded in undo.
      undo->linked.push_back(shared_from_this());
      this->kind = Link;
      seqassert(!typ->getLink() || typ->getLink()->kind != Unbound ||
                    typ->getLink()->id <= id,
                "type unification is not consistent");
      this->type = typ->follow()->shared_from_this();

      if (auto t = type->getLink(); t && trait && t->kind == Unbound && !t->trait) {
        undo->traits.push_back(t->shared_from_this());
        t->trait = trait;
      }
    }
    return 0;
  }
}

TypePtr LinkType::generalize(int atLevel) const {
  // We need to preserve the pointers as something else might be pointing
  // to this unbound, hence the const_casts because shared_from_ptr
  // needs it.

  if (kind == Generic) {
    return const_cast<LinkType *>(this)->shared_from_this();
  } else if (kind == Unbound) {
    if (level >= atLevel) {
      return std::make_shared<LinkType>(
          cache, Generic, id, 0, nullptr, staticKind,
          trait ? std::static_pointer_cast<Trait>(trait->generalize(atLevel)) : nullptr,
          defaultType ? defaultType->generalize(atLevel) : nullptr, genericName,
          passThrough);
    } else {
      return const_cast<LinkType *>(this)->shared_from_this();
    }
  } else {
    seqassert(type, "link is null");
    return type->generalize(atLevel);
  }
}

TypePtr LinkType::instantiate(int atLevel, int *unboundCount,
                              std::unordered_map<int, TypePtr> *cache) const {
  if (kind == Generic) {
    if (TypePtr *res = nullptr; cache && ((res = in(*cache, id))))
      return *res;
    auto t = std::make_shared<LinkType>(
        this->cache, Unbound, unboundCount ? (*unboundCount)++ : id, atLevel, nullptr,
        staticKind,
        trait ? std::static_pointer_cast<Trait>(
                    trait->instantiate(atLevel, unboundCount, cache))
              : nullptr,
        defaultType ? defaultType->instantiate(atLevel, unboundCount, cache) : nullptr,
        genericName, passThrough);
    if (cache)
      (*cache)[id] = t;
    return t;
  } else if (kind == Unbound) {
    // We need to preserve the pointers as something else might be pointing
    // to this unbound, hence the const_casts because shared_from_ptr
    // needs it.
    return const_cast<LinkType *>(this)->shared_from_this();
  } else {
    seqassert(type, "link is null");
    return type->instantiate(atLevel, unboundCount, cache);
  }
}

Type *LinkType::follow() {
  if (kind == Link)
    return type->follow();
  else
    return this;
}

std::vector<Type *> LinkType::getUnbounds(bool includeGenerics) const {
  if (kind == Unbound)
    return {(Type *)this};
  else if (kind == Link)
    return type->getUnbounds(includeGenerics);
  else if (includeGenerics)
    return {(Type *)this};
  return {};
}

bool LinkType::hasUnbounds(bool includeGenerics) const {
  if (kind == Unbound)
    return true;
  if (includeGenerics && kind == Generic)
    return true;
  if (kind == Link)
    return type->hasUnbounds(includeGenerics);
  return false;
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
      return (genericName.empty() ? "" : genericName + ":") +
             (kind == Unbound ? "?" : "#") + fmt::format("{}", id) +
             (trait ? ":" + trait->debugString(mode) : "") +
             (staticKind ? fmt::format(":S{}", static_cast<int>(staticKind)) : "");
    } else if (trait) {
      return trait->debugString(mode);
    }
    return (genericName.empty() ? (mode ? "?" : "<unknown type>") : genericName);
  }
  return type->debugString(mode);
}

std::string LinkType::realizedName() const {
  if (kind == Unbound)
    return ("#" + genericName);
  if (kind == Generic)
    return ("#" + genericName);
  seqassert(kind == Link, "unexpected generic link");
  return type->realizedName();
}

LinkType *LinkType::getLink() { return this; }

FuncType *LinkType::getFunc() { return kind == Link ? type->getFunc() : nullptr; }

ClassType *LinkType::getPartial() {
  return kind == Link ? type->getPartial() : nullptr;
}

ClassType *LinkType::getClass() { return kind == Link ? type->getClass() : nullptr; }

StaticType *LinkType::getStatic() { return kind == Link ? type->getStatic() : nullptr; }

IntStaticType *LinkType::getIntStatic() {
  return kind == Link ? type->getIntStatic() : nullptr;
}

StrStaticType *LinkType::getStrStatic() {
  return kind == Link ? type->getStrStatic() : nullptr;
}

BoolStaticType *LinkType::getBoolStatic() {
  return kind == Link ? type->getBoolStatic() : nullptr;
}

UnionType *LinkType::getUnion() { return kind == Link ? type->getUnion() : nullptr; }

LinkType *LinkType::getUnbound() {
  if (kind == Unbound)
    return this;
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
        undo->leveled.emplace_back(tl->shared_from_this(), tl->level);
        tl->level = level;
      }
      return false;
    } else if (tl->kind == Link) {
      return occurs(tl->type.get(), undo);
    } else {
      return false;
    }
  } else if (typ->getStatic()) {
    return false;
  }
  if (const auto tc = typ->getClass()) {
    return std::ranges::any_of(
        tc->generics.begin(), tc->generics.end(),
        [&](const auto &g) { return g.type && occurs(g.type.get(), undo); });
  } else {
    return false;
  }
}

} // namespace codon::ast::types
