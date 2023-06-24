// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "ctx.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

TypecheckItem::TypecheckItem(std::string canonicalName, std::string baseName,
                             std::string moduleName, types::TypePtr type,
                             std::vector<int> scope)
    : canonicalName(std::move(canonicalName)), baseName(std::move(baseName)),
      moduleName(std::move(moduleName)), type(std::move(type)),
      scope(std::move(scope)) {}

TypeContext::TypeContext(Cache *cache, std::string filename)
    : Context<TypecheckItem>(std::move(filename)), cache(cache) {
  bases.emplace_back();
  scope.blocks.emplace_back(scope.counter = 0);
  pushSrcInfo(cache->generateSrcInfo()); // Always have srcInfo() around
}

void TypeContext::add(const std::string &name, const TypeContext::Item &var) {
  auto v = find(name);
  if (v && !v->canShadow)
    E(Error::ID_INVALID_BIND, getSrcInfo(), name);
  Context<TypecheckItem>::add(name, var);
}

TypeContext::Item TypeContext::addVar(const std::string &name,
                                      const std::string &canonicalName,
                                      const types::TypePtr &type,
                                      const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  seqassert(type->getLink(), "bad var");
  auto t = std::make_shared<TypecheckItem>(canonicalName, getBaseName(), getModule(),
                                           type, scope.blocks);
  t->setSrcInfo(srcInfo);
  Context<TypecheckItem>::add(name, t);
  addAlwaysVisible(t);
  // LOG("added var/{}: {}", t->isVar() ? "v" : (t->isFunc() ? "f" : "t"),
  // canonicalName);
  return t;
}

TypeContext::Item TypeContext::addType(const std::string &name,
                                       const std::string &canonicalName,
                                       const types::TypePtr &type,
                                       const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  // seqassert(type->getClass(), "bad type");
  auto t = std::make_shared<TypecheckItem>(canonicalName, getBaseName(), getModule(),
                                           type, scope.blocks);
  t->setSrcInfo(srcInfo);
  Context<TypecheckItem>::add(name, t);
  addAlwaysVisible(t);
  // LOG("added typ/{}: {}", t->isVar() ? "v" : (t->isFunc() ? "f" : "t"),
  // canonicalName);
  return t;
}

TypeContext::Item TypeContext::addFunc(const std::string &name,
                                       const std::string &canonicalName,
                                       const types::TypePtr &type,
                                       const SrcInfo &srcInfo) {
  seqassert(!canonicalName.empty(), "empty canonical name for '{}'", name);
  seqassert(type->getFunc(), "bad func");
  auto t = std::make_shared<TypecheckItem>(canonicalName, getBaseName(), getModule(),
                                           type, scope.blocks);
  t->setSrcInfo(srcInfo);
  Context<TypecheckItem>::add(name, t);
  addAlwaysVisible(t);
  // LOG("added fun/{}: {}", t->isVar() ? "v" : (t->isFunc() ? "f" : "t"),
  // canonicalName);
  return t;
}

TypeContext::Item TypeContext::addAlwaysVisible(const TypeContext::Item &item) {
  if (!cache->typeCtx->Context<TypecheckItem>::find(item->canonicalName)) {
    cache->typeCtx->add(item->canonicalName, item);

    // Realizations etc.
    if (!in(cache->reverseIdentifierLookup, item->canonicalName))
      cache->reverseIdentifierLookup[item->canonicalName] = item->canonicalName;
  }
  return item;
}

TypeContext::Item TypeContext::find(const std::string &name) const {
  auto t = Context<TypecheckItem>::find(name);
  if (t)
    return t;

  // Item is not found in the current module. Time to look in the standard library!
  // Note: the standard library items cannot be dominated.
  auto stdlib = cache->imports[STDLIB_IMPORT].ctx;
  if (stdlib.get() != this)
    t = stdlib->Context<TypecheckItem>::find(name);

  // Maybe we are looking for a canonical identifier?
  if (!t && cache->typeCtx.get() != this)
    t = cache->typeCtx->Context<TypecheckItem>::find(name);

  return t;
}

TypeContext::Item TypeContext::forceFind(const std::string &name) const {
  auto f = find(name);
  seqassert(f, "cannot find '{}'", name);
  return f;
}

/// Getters and setters

std::string TypeContext::getBaseName() const { return bases.back().name; }

std::string TypeContext::getModule() const {
  std::string base = moduleName.status == ImportFile::STDLIB ? "std." : "";
  base += moduleName.module;
  if (auto sz = startswith(base, "__main__"))
    base = base.substr(sz);
  return base;
}

void TypeContext::dump() { dump(0); }

bool TypeContext::isCanonicalName(const std::string &name) const {
  return name.rfind('.') != std::string::npos;
}

std::string TypeContext::generateCanonicalName(const std::string &name,
                                               bool includeBase, bool noSuffix) const {
  std::string newName = name;
  bool alreadyGenerated = name.find('.') != std::string::npos;
  if (includeBase && !alreadyGenerated) {
    std::string base = getBaseName();
    if (base.empty())
      base = getModule();
    if (base == "std.internal.core") {
      noSuffix = true;
      base = "";
    }
    newName = (base.empty() ? "" : (base + ".")) + newName;
  }
  auto num = cache->identifierCount[newName]++;
  if (!noSuffix && !alreadyGenerated)
    newName = format("{}.{}", newName, num);
  if (name != newName)
    cache->identifierCount[newName]++;
  cache->reverseIdentifierLookup[newName] = name;
  return newName;
}

void TypeContext::enterConditionalBlock() { scope.blocks.push_back(++scope.counter); }

ExprPtr NameVisitor::transform(const std::shared_ptr<Expr> &expr) {
  NameVisitor v(tv);
  if (expr)
    expr->accept(v);
  return v.resultExpr ? v.resultExpr : expr;
}
ExprPtr NameVisitor::transform(std::shared_ptr<Expr> &expr) {
  NameVisitor v(tv);
  if (expr)
    expr->accept(v);
  if (v.resultExpr)
    expr = v.resultExpr;
  return expr;
}
StmtPtr NameVisitor::transform(const std::shared_ptr<Stmt> &stmt) {
  NameVisitor v(tv);
  if (stmt)
    stmt->accept(v);
  return v.resultStmt ? v.resultStmt : stmt;
}
StmtPtr NameVisitor::transform(std::shared_ptr<Stmt> &stmt) {
  NameVisitor v(tv);
  if (stmt)
    stmt->accept(v);
  if (v.resultExpr)
    stmt = v.resultStmt;
  return stmt;
}
void NameVisitor::visit(IdExpr *expr) {
  while (auto s = in(tv->getCtx()->getBase()->replacements, expr->value)) {
    expr->value = s->first;
    tv->unify(expr->type, tv->getCtx()->forceFind(s->first)->type);
  }
}
void NameVisitor::visit(AssignStmt *stmt) {
  seqassert(stmt->lhs->getId(), "invalid AssignStmt {}", stmt->lhs);
  std::string lhs = stmt->lhs->getId()->value;
  if (auto changed = in(tv->getCtx()->getBase()->replacements, lhs)) {
    while (auto s = in(tv->getCtx()->getBase()->replacements, lhs))
      lhs = changed->first, changed = s;
    if (stmt->rhs && changed->second) {
      // Mark the dominating binding as used: `var.__used__ = True`
      auto u =
          N<AssignStmt>(N<IdExpr>(fmt::format("{}.__used__", lhs)), N<BoolExpr>(true));
      u->setUpdate();
      stmt->setUpdate();
      resultStmt = N<SuiteStmt>(u, stmt->shared_from_this());
    } else if (changed->second && !stmt->rhs) {
      // This assignment was a declaration only.
      // Just mark the dominating binding as used: `var.__used__ = True`
      stmt->lhs = N<IdExpr>(fmt::format("{}.__used__", lhs));
      stmt->rhs = N<BoolExpr>(true);
      stmt->setUpdate();
    }
    seqassert(stmt->rhs, "bad domination statement: '{}'", stmt->toString());
  }
}
void NameVisitor::visit(TryStmt *stmt) {
  for (auto &c : stmt->catches) {
    if (!c.var.empty()) {
      // Handle dominated except bindings
      auto changed = in(tv->getCtx()->getBase()->replacements, c.var);
      while (auto s = in(tv->getCtx()->getBase()->replacements, c.var))
        c.var = s->first, changed = s;
      if (changed && changed->second) {
        auto update =
            N<AssignStmt>(N<IdExpr>(format("{}.__used__", c.var)), N<BoolExpr>(true));
        update->setUpdate();
        c.suite = N<SuiteStmt>(update, c.suite);
      }
      if (changed)
        c.exc->setAttr(ExprAttr::Dominated);
    }
  }
}
void NameVisitor::visit(ForStmt *stmt) {
  auto var = stmt->var->getId();
  seqassert(var, "corrupt for variable: {}", stmt->var);
  auto changed = in(tv->getCtx()->getBase()->replacements, var->value);
  while (auto s = in(tv->getCtx()->getBase()->replacements, var->value))
    var->value = s->first, changed = s;
  if (changed && changed->second) {
    auto u =
        N<AssignStmt>(N<IdExpr>(format("{}.__used__", var->value)), N<BoolExpr>(true));
    u->setUpdate();
    stmt->suite = N<SuiteStmt>(u, stmt->suite);
  }
  if (changed)
    var->setAttr(ExprAttr::Dominated);
}

void TypeContext::leaveConditionalBlock(std::vector<StmtPtr> *stmts,
                                        TypecheckVisitor *tv) {
  if (stmts && in(scope.stmts, scope.blocks.back())) {
    stmts->insert(stmts->begin(), scope.stmts[scope.blocks.back()].begin(),
                  scope.stmts[scope.blocks.back()].end());
    NameVisitor nv(tv);
    for (auto &s : *stmts)
      nv.transform(s);
  }
  scope.blocks.pop_back();
}

bool TypeContext::isGlobal() const { return bases.size() == 1; }

bool TypeContext::isConditional() const { return scope.blocks.size() > 1; }

TypeContext::Base *TypeContext::getBase() {
  return bases.empty() ? nullptr : &(bases.back());
}

bool TypeContext::inFunction() const { return !isGlobal() && !bases.back().isType(); }

bool TypeContext::inClass() const { return !isGlobal() && bases.back().isType(); }

bool TypeContext::isOuter(const Item &val) const {
  return getBaseName() != val->getBaseName() || getModule() != val->getModule();
}

TypeContext::Base *TypeContext::getClassBase() {
  if (bases.size() >= 2 && bases[bases.size() - 2].isType())
    return &(bases[bases.size() - 2]);
  return nullptr;
}

size_t TypeContext::getRealizationDepth() const { return bases.size(); }

std::string TypeContext::getRealizationStackName() const {
  if (bases.empty())
    return "";
  std::vector<std::string> s;
  for (auto &b : bases)
    if (b.type)
      s.push_back(b.type->realizedName());
  return join(s, ":");
}

std::shared_ptr<types::LinkType> TypeContext::getUnbound(const SrcInfo &srcInfo,
                                                         int level) const {
  auto typ = std::make_shared<types::LinkType>(cache, types::LinkType::Unbound,
                                               cache->unboundCount++, level, nullptr);
  typ->setSrcInfo(srcInfo);
  return typ;
}

std::shared_ptr<types::LinkType> TypeContext::getUnbound(const SrcInfo &srcInfo) const {
  return getUnbound(srcInfo, typecheckLevel);
}

std::shared_ptr<types::LinkType> TypeContext::getUnbound() const {
  return getUnbound(getSrcInfo(), typecheckLevel);
}

types::TypePtr TypeContext::instantiate(const SrcInfo &srcInfo,
                                        const types::TypePtr &type,
                                        const types::ClassTypePtr &generics) {
  seqassert(type, "type is null");
  std::unordered_map<int, types::TypePtr> genericCache;
  if (generics) {
    for (auto &g : generics->generics)
      if (g.type &&
          !(g.type->getLink() && g.type->getLink()->kind == types::LinkType::Generic)) {
        genericCache[g.id] = g.type;
      }
  }
  auto t = type->instantiate(typecheckLevel, &(cache->unboundCount), &genericCache);
  for (auto &i : genericCache) {
    if (auto l = i.second->getLink()) {
      i.second->setSrcInfo(srcInfo);
      if (l->defaultType)
        pendingDefaults.insert(i.second);
    }
  }
  if (t->getUnion() && !t->getUnion()->isSealed()) {
    t->setSrcInfo(srcInfo);
    pendingDefaults.insert(t);
  }
  return t;
}

types::TypePtr
TypeContext::instantiateGeneric(const SrcInfo &srcInfo, const types::TypePtr &root,
                                const std::vector<types::TypePtr> &generics) {
  auto c = root->getClass();
  seqassert(c, "root class is null");
  // dummy generic type
  auto g = std::make_shared<types::ClassType>(cache, "", "");
  if (generics.size() != c->generics.size()) {
    E(Error::GENERICS_MISMATCH, srcInfo, cache->rev(c->name), c->generics.size(),
      generics.size());
  }
  for (int i = 0; i < c->generics.size(); i++) {
    seqassert(c->generics[i].type, "generic is null");
    g->generics.emplace_back("", "", generics[i], c->generics[i].id);
  }
  return instantiate(srcInfo, root, g);
}

std::vector<types::FuncTypePtr> TypeContext::findMethod(const std::string &typeName,
                                                        const std::string &method,
                                                        bool hideShadowed) const {
  std::vector<types::FuncTypePtr> vv;
  std::unordered_set<std::string> signatureLoci;

  auto populate = [&](const auto &cls) {
    auto t = in(cls.methods, method);
    if (!t)
      return;
    auto mt = cache->overloads[*t];
    for (int mti = int(mt.size()) - 1; mti >= 0; mti--) {
      auto &method = mt[mti];
      if (endswith(method, ":dispatch") || !cache->functions[method].type)
        continue;
      // if (method.age <= age) {
      if (hideShadowed) {
        auto sig = cache->functions[method].ast->signature();
        if (!in(signatureLoci, sig)) {
          signatureLoci.insert(sig);
          vv.emplace_back(cache->functions[method].type);
        }
      } else {
        vv.emplace_back(cache->functions[method].type);
      }
      // }
    }
  };
  if (auto cls = in(cache->classes, typeName)) {
    for (auto &pt : cls->mro) {
      if (auto pc = pt->type->getClass()) {
        auto mc = in(cache->classes, pc->name);
        seqassert(mc, "class '{}' not found", pc->name);
        populate(*mc);
      }
    }
  }
  return vv;
}

types::TypePtr TypeContext::findMember(const std::string &typeName,
                                       const std::string &member) const {
  if (auto cls = in(cache->classes, typeName)) {
    for (auto &pt : cls->mro) {
      if (auto pc = pt->type->getClass()) {
        auto mc = in(cache->classes, pc->name);
        seqassert(mc, "class '{}' not found", pc->name);
        for (auto &mm : mc->fields) {
          if (mm.name == member)
            return mm.type;
        }
      }
    }
  }
  return nullptr;
}

int TypeContext::reorderNamedArgs(types::FuncType *func,
                                  const std::vector<CallExpr::Arg> &args,
                                  const ReorderDoneFn &onDone,
                                  const ReorderErrorFn &onError,
                                  const std::vector<char> &known) {
  // See https://docs.python.org/3.6/reference/expressions.html#calls for details.
  // Final score:
  //  - +1 for each matched argument
  //  -  0 for *args/**kwargs/default arguments
  //  - -1 for failed match
  int score = 0;

  // 0. Find *args and **kwargs
  // True if there is a trailing ellipsis (full partial: fn(all_args, ...))
  bool partial = !args.empty() && args.back().value->getEllipsis() &&
                 args.back().value->getEllipsis()->mode != EllipsisExpr::PIPE &&
                 args.back().name.empty();

  int starArgIndex = -1, kwstarArgIndex = -1;
  for (int i = 0; i < func->ast->args.size(); i++) {
    if (startswith(func->ast->args[i].name, "**"))
      kwstarArgIndex = i, score -= 2;
    else if (startswith(func->ast->args[i].name, "*"))
      starArgIndex = i, score -= 2;
  }

  // 1. Assign positional arguments to slots
  // Each slot contains a list of arg's indices
  std::vector<std::vector<int>> slots(func->ast->args.size());
  seqassert(known.empty() || func->ast->args.size() == known.size(),
            "bad 'known' string");
  std::vector<int> extra;
  std::map<std::string, int> namedArgs,
      extraNamedArgs; // keep the map--- we need it sorted!
  for (int ai = 0, si = 0; ai < args.size() - partial; ai++) {
    if (args[ai].name.empty()) {
      while (!known.empty() && si < slots.size() && known[si])
        si++;
      if (si < slots.size() && (starArgIndex == -1 || si < starArgIndex))
        slots[si++] = {ai};
      else
        extra.emplace_back(ai);
    } else {
      namedArgs[args[ai].name] = ai;
    }
  }
  score += 2 * int(slots.size() - func->funcGenerics.size());

  for (auto ai : std::vector<int>{std::max(starArgIndex, kwstarArgIndex),
                                  std::min(starArgIndex, kwstarArgIndex)})
    if (ai != -1 && !slots[ai].empty()) {
      extra.insert(extra.begin(), ai);
      slots[ai].clear();
    }

  // 2. Assign named arguments to slots
  if (!namedArgs.empty()) {
    std::map<std::string, int> slotNames;
    for (int i = 0; i < func->ast->args.size(); i++)
      if (known.empty() || !known[i]) {
        slotNames[cache->reverseIdentifierLookup[func->ast->args[i].name]] = i;
      }
    for (auto &n : namedArgs) {
      if (!in(slotNames, n.first))
        extraNamedArgs[n.first] = n.second;
      else if (slots[slotNames[n.first]].empty())
        slots[slotNames[n.first]].push_back(n.second);
      else
        return onError(Error::CALL_REPEATED_NAME, args[n.second].value->getSrcInfo(),
                       Emsg(Error::CALL_REPEATED_NAME, n.first));
    }
  }

  // 3. Fill in *args, if present
  if (!extra.empty() && starArgIndex == -1)
    return onError(Error::CALL_ARGS_MANY, getSrcInfo(),
                   Emsg(Error::CALL_ARGS_MANY, cache->rev(func->ast->name),
                        func->ast->args.size(), args.size() - partial));

  if (starArgIndex != -1)
    slots[starArgIndex] = extra;

  // 4. Fill in **kwargs, if present
  if (!extraNamedArgs.empty() && kwstarArgIndex == -1)
    return onError(Error::CALL_ARGS_INVALID,
                   args[extraNamedArgs.begin()->second].value->getSrcInfo(),
                   Emsg(Error::CALL_ARGS_INVALID, extraNamedArgs.begin()->first,
                        cache->rev(func->ast->name)));
  if (kwstarArgIndex != -1)
    for (auto &e : extraNamedArgs)
      slots[kwstarArgIndex].push_back(e.second);

  // 5. Fill in the default arguments
  for (auto i = 0; i < func->ast->args.size(); i++)
    if (slots[i].empty() && i != starArgIndex && i != kwstarArgIndex) {
      if (func->ast->args[i].status == Param::Normal &&
          (func->ast->args[i].defaultValue || (!known.empty() && known[i])))
        score -= 2;
      else if (!partial && func->ast->args[i].status == Param::Normal)
        return onError(Error::CALL_ARGS_MISSING, getSrcInfo(),
                       Emsg(Error::CALL_ARGS_MISSING, cache->rev(func->ast->name),
                            cache->reverseIdentifierLookup[func->ast->args[i].name]));
    }
  return score + onDone(starArgIndex, kwstarArgIndex, slots, partial);
}

void TypeContext::dump(int pad) {
  auto ordered =
      std::map<std::string, decltype(map)::mapped_type>(map.begin(), map.end());
  LOG("current module: {} ({})", moduleName.module, moduleName.path);
  LOG("current base:   {} / {}", getRealizationStackName(), getBase()->name);
  for (auto &i : ordered) {
    std::string s;
    auto t = i.second.front();
    LOG("{}{:.<25}", std::string(size_t(pad) * 2, ' '), i.first);
    LOG("   ... kind:      {}", t->isType() * 100 + t->isFunc() * 10 + t->isVar());
    LOG("   ... canonical: {}", t->canonicalName);
    LOG("   ... base:      {}", t->baseName);
    LOG("   ... module:    {}", t->moduleName);
    LOG("   ... type:      {}", t->type ? t->type->debugString(2) : "<null>");
    LOG("   ... scope:     {}", t->scope);
    LOG("   ... access:    {}", t->accessChecked);
    LOG("   ... shdw/dom:  {} / {}", t->canShadow, t->avoidDomination);
    LOG("   ... gnrc/sttc: {} / {}", t->generic, int(t->staticType));
  }
}

std::string TypeContext::debugInfo() {
  return fmt::format("[{}:i{}@{}]", getBase()->name, getBase()->iteration,
                     getSrcInfo());
}

std::shared_ptr<std::pair<std::vector<types::TypePtr>, std::vector<types::TypePtr>>>
TypeContext::getFunctionArgs(const types::TypePtr &t) {
  if (!t->getFunc())
    return nullptr;
  auto fn = t->getFunc();
  auto ret = std::make_shared<
      std::pair<std::vector<types::TypePtr>, std::vector<types::TypePtr>>>();
  for (auto &t : fn->funcGenerics)
    ret->first.push_back(t.type);
  for (auto &t : fn->generics[0].type->getRecord()->args)
    ret->second.push_back(t);
  return ret;
}

std::shared_ptr<std::string> TypeContext::getStaticString(const types::TypePtr &t) {
  if (auto s = t->getStatic()) {
    auto r = s->evaluate();
    if (r.type == StaticValue::STRING)
      return std::make_shared<std::string>(r.getString());
  }
  return nullptr;
}

std::shared_ptr<int64_t> TypeContext::getStaticInt(const types::TypePtr &t) {
  if (auto s = t->getStatic()) {
    auto r = s->evaluate();
    if (r.type == StaticValue::INT)
      return std::make_shared<int64_t>(r.getInt());
  }
  return nullptr;
}

types::FuncTypePtr TypeContext::extractFunction(const types::TypePtr &t) {
  if (auto f = t->getFunc())
    return f;
  if (auto p = t->getPartial())
    return p->func;
  return nullptr;
}

} // namespace codon::ast
