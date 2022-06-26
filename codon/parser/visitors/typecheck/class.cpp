#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

void TypecheckVisitor::visit(ClassStmt *stmt) {
  auto &attr = stmt->attributes;
  bool extension = attr.has(Attr::Extend);
  if (ctx->findInVisited(stmt->name).second && !extension)
    return;

  ClassTypePtr typ = nullptr;
  if (!extension) {
    if (stmt->isRecord())
      typ = std::make_shared<RecordType>(
          stmt->name, ctx->cache->reverseIdentifierLookup[stmt->name]);
    else
      typ = std::make_shared<ClassType>(
          stmt->name, ctx->cache->reverseIdentifierLookup[stmt->name]);
    if (stmt->isRecord() && startswith(stmt->name, TYPE_PARTIAL)) {
      seqassert(in(ctx->cache->partials, stmt->name),
                "invalid partial initialization: {}", stmt->name);
      typ = std::make_shared<PartialType>(typ->getRecord(),
                                          ctx->cache->partials[stmt->name].first,
                                          ctx->cache->partials[stmt->name].second);
    }
    typ->setSrcInfo(stmt->getSrcInfo());
    ctx->add(TypecheckItem::Type, stmt->name, typ);
    ctx->bases[0].visitedAsts[stmt->name] = {TypecheckItem::Type, typ};

    // Parse class fields.
    for (const auto &a : stmt->args)
      if (a.status != Param::Normal) { // TODO
        char staticType = 0;
        auto idx = a.type->getIndex();
        if (idx && idx->expr->isId("Static"))
          staticType = idx->index->isId("str") ? 1 : 2;
        auto t = ctx->addUnbound(N<IdExpr>(a.name).get(), ctx->typecheckLevel, true,
                                 staticType);
        auto typId = ctx->cache->unboundCount - 1;
        t->getLink()->genericName = ctx->cache->reverseIdentifierLookup[a.name];
        if (a.defaultValue) {
          auto dt = clone(a.defaultValue);
          dt = transformType(dt);
          if (a.status == Param::Generic)
            t->defaultType = dt->type;
          else
            unify(dt->type, t);
        }
        ctx->add(TypecheckItem::Type, a.name, t);
        ClassType::Generic g{a.name, ctx->cache->reverseIdentifierLookup[a.name],
                             t->generalize(ctx->typecheckLevel), typId};
        if (a.status == Param::Generic) {
          typ->generics.push_back(g);
        } else {
          typ->hiddenGenerics.push_back(g);
        }
        LOG_REALIZE("[generic/{}] {} -> {}", int(a.status), a.name, t->toString());
      }
    {
      ctx->typecheckLevel++;
      for (auto ai = 0, aj = 0; ai < stmt->args.size(); ai++)
        if (stmt->args[ai].status == Param::Normal) {
          auto si = stmt->args[ai].type->getSrcInfo();
          ctx->cache->classes[stmt->name].fields[aj].type =
              transformType(stmt->args[ai].type)
                  ->getType()
                  ->generalize(ctx->typecheckLevel - 1);
          ctx->cache->classes[stmt->name].fields[aj].type->setSrcInfo(si);
          if (stmt->isRecord())
            typ->getRecord()->args.push_back(
                ctx->cache->classes[stmt->name].fields[aj].type);
          aj++;
        }
      ctx->typecheckLevel--;
    }
    // Remove lingering generics.
    for (const auto &g : stmt->args)
      if (g.status != Param::Normal) {
        auto val = ctx->find(g.name);
        seqassert(val, "cannot find generic {}", g.name);
        auto t = val->type;
        if (g.status == Param::Generic) {
          seqassert(t && t->getLink() && t->getLink()->kind != types::LinkType::Link,
                    "generic has been unified");
          if (t->getLink()->kind == LinkType::Unbound)
            t->getLink()->kind = LinkType::Generic;
        }
        ctx->remove(g.name);
      }

    LOG_REALIZE("[class] {} -> {}", stmt->name, typ->debugString(1));
    for (auto &m : ctx->cache->classes[stmt->name].fields)
      LOG_REALIZE("       - member: {}: {}", m.name, m.type->debugString(1));
  }
  stmt->done = true;
}

}