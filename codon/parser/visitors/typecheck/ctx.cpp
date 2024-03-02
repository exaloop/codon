// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "ctx.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/simplify/ctx.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

TypeContext::TypeContext(Cache *cache)
    : Context<TypecheckItem>(""), cache(cache), typecheckLevel(0), age(0),
      blockLevel(0), returnEarly(false), changedNodes(0) {
  realizationBases.push_back({"", nullptr, nullptr});
  pushSrcInfo(cache->generateSrcInfo()); // Always have srcInfo() around
}

std::shared_ptr<TypecheckItem> TypeContext::add(TypecheckItem::Kind kind,
                                                const std::string &name,
                                                const types::TypePtr &type) {
  auto t = std::make_shared<TypecheckItem>(kind, type);
  add(name, t);
  return t;
}

std::shared_ptr<TypecheckItem> TypeContext::find(const std::string &name) const {
  if (auto t = Context<TypecheckItem>::find(name))
    return t;
  if (in(cache->globals, name))
    return std::make_shared<TypecheckItem>(TypecheckItem::Var, getUnbound());
  return nullptr;
}

std::shared_ptr<TypecheckItem> TypeContext::forceFind(const std::string &name) const {
  auto t = find(name);
  seqassert(t, "cannot find '{}'", name);
  return t;
}

types::TypePtr TypeContext::getType(const std::string &name) const {
  return forceFind(name)->type;
}

TypeContext::RealizationBase *TypeContext::getRealizationBase() {
  return &(realizationBases.back());
}

size_t TypeContext::getRealizationDepth() const { return realizationBases.size(); }

std::string TypeContext::getRealizationStackName() const {
  if (realizationBases.empty())
    return "";
  std::vector<std::string> s;
  for (auto &b : realizationBases)
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
      if (l->defaultType) {
        getRealizationBase()->pendingDefaults.insert(i.second);
      }
    }
  }
  if (t->getUnion() && !t->getUnion()->isSealed()) {
    t->setSrcInfo(srcInfo);
    getRealizationBase()->pendingDefaults.insert(t);
  }
  if (auto r = t->getRecord())
    if (r->repeats && r->repeats->canRealize())
      r->flatten();
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

std::shared_ptr<types::RecordType>
TypeContext::instantiateTuple(const SrcInfo &srcInfo,
                              const std::vector<types::TypePtr> &generics) {
  auto key = generateTuple(generics.size());
  auto root = forceFind(key)->type->getRecord();
  return instantiateGeneric(srcInfo, root, generics)->getRecord();
}

std::string TypeContext::generateTuple(size_t n) {
  auto key = format("_{}:{}", TYPE_TUPLE, n);
  if (!in(cache->classes, key)) {
    cache->classes[key].fields.clear();
    cache->classes[key].ast =
        std::static_pointer_cast<ClassStmt>(clone(cache->classes[TYPE_TUPLE].ast));
    auto root = std::make_shared<types::RecordType>(cache, TYPE_TUPLE, TYPE_TUPLE);
    for (size_t i = 0; i < n; i++) { // generate unique ID
      auto g = getUnbound()->getLink();
      g->kind = types::LinkType::Generic;
      g->genericName = format("T{}", i + 1);
      auto gn = cache->imports[MAIN_IMPORT].ctx->generateCanonicalName(g->genericName);
      root->generics.emplace_back(gn, g->genericName, g, g->id);
      root->args.emplace_back(g);
      cache->classes[key].ast->args.emplace_back(
          g->genericName, std::make_shared<IdExpr>("type"), nullptr, Param::Generic);
      cache->classes[key].fields.push_back(
          Cache::Class::ClassField{format("item{}", i + 1), g, ""});
    }
    std::vector<ExprPtr> eTypeArgs;
    for (size_t i = 0; i < n; i++)
      eTypeArgs.push_back(std::make_shared<IdExpr>(format("T{}", i + 1)));
    auto eType = std::make_shared<InstantiateExpr>(std::make_shared<IdExpr>(TYPE_TUPLE),
                                                   eTypeArgs);
    eType->type = root;
    cache->classes[key].mro = {eType};
    addToplevel(key, std::make_shared<TypecheckItem>(TypecheckItem::Type, root));
  }
  return key;
}

std::shared_ptr<types::RecordType> TypeContext::instantiateTuple(size_t n) {
  std::vector<types::TypePtr> t(n);
  for (size_t i = 0; i < n; i++) {
    auto g = getUnbound()->getLink();
    g->genericName = format("T{}", i + 1);
    t[i] = g;
  }
  return instantiateTuple(getSrcInfo(), t);
}

std::vector<types::FuncTypePtr> TypeContext::findMethod(types::ClassType *type,
                                                        const std::string &method,
                                                        bool hideShadowed) {
  auto typeName = type->name;
  if (type->is(TYPE_TUPLE)) {
    auto sz = type->getRecord()->getRepeats();
    if (sz != -1)
      type->getRecord()->flatten();
    sz = int64_t(type->getRecord()->args.size());
    typeName = format("_{}:{}", TYPE_TUPLE, sz);
    if (in(cache->classes[TYPE_TUPLE].methods, method) &&
        !in(cache->classes[typeName].methods, method)) {
      auto type = forceFind(typeName)->type;

      cache->classes[typeName].methods[method] =
          cache->classes[TYPE_TUPLE].methods[method];
      auto &o = cache->overloads[cache->classes[typeName].methods[method]];
      auto f = cache->functions[o[0].name];
      f.realizations.clear();

      seqassert(f.type, "tuple fn type not yet set");
      f.ast->attributes.parentClass = typeName;
      f.ast = std::static_pointer_cast<FunctionStmt>(clone(f.ast));
      f.ast->name = format("{}{}", f.ast->name.substr(0, f.ast->name.size() - 1), sz);
      f.ast->attributes.set(Attr::Method);

      auto eType = clone(cache->classes[typeName].mro[0]);
      eType->type = nullptr;
      for (auto &a : f.ast->args)
        if (a.type && a.type->isId(TYPE_TUPLE)) {
          a.type = eType;
        }
      if (f.ast->ret && f.ast->ret->isId(TYPE_TUPLE))
        f.ast->ret = eType;
      // TODO: resurrect Tuple[N].__new__(defaults...)
      if (method == "__new__") {
        for (size_t i = 0; i < sz; i++) {
          auto n = format("item{}", i + 1);
          f.ast->args.emplace_back(
              cache->imports[MAIN_IMPORT].ctx->generateCanonicalName(n),
              std::make_shared<IdExpr>(format("T{}", i + 1))
              // std::make_shared<CallExpr>(
              // std::make_shared<IdExpr>(format("T{}", i + 1)))
          );
        }
      }
      cache->reverseIdentifierLookup[f.ast->name] = method;
      cache->functions[f.ast->name] = f;
      cache->functions[f.ast->name].type =
          TypecheckVisitor(cache->typeCtx).makeFunctionType(f.ast.get());
      addToplevel(f.ast->name,
                  std::make_shared<TypecheckItem>(TypecheckItem::Func,
                                                  cache->functions[f.ast->name].type));
      o.push_back(Cache::Overload{f.ast->name, 0});
    }
  }

  std::vector<types::FuncTypePtr> vv;
  std::unordered_set<std::string> signatureLoci;

  auto populate = [&](const auto &cls) {
    auto t = in(cls.methods, method);
    if (!t)
      return;
    auto mt = cache->overloads[*t];
    for (int mti = int(mt.size()) - 1; mti >= 0; mti--) {
      auto &method = mt[mti];
      if (endswith(method.name, ":dispatch") || !cache->functions[method.name].type)
        continue;
      if (method.age <= age) {
        if (hideShadowed) {
          auto sig = cache->functions[method.name].ast->signature();
          if (!in(signatureLoci, sig)) {
            signatureLoci.insert(sig);
            vv.emplace_back(cache->functions[method.name].type);
          }
        } else {
          vv.emplace_back(cache->functions[method.name].type);
        }
      }
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

types::TypePtr TypeContext::findMember(const types::ClassTypePtr &type,
                                       const std::string &member) const {
  if (type->is(TYPE_TUPLE)) {
    if (!startswith(member, "item") || member.size() < 5)
      return nullptr;
    int id = 0;
    for (int i = 4; i < member.size(); i++) {
      if (member[i] >= '0' + (i == 4) && member[i] <= '9')
        id = id * 10 + member[i] - '0';
      else
        return nullptr;
    }
    auto sz = type->getRecord()->getRepeats();
    if (sz != -1)
      type->getRecord()->flatten();
    if (id < 1 || id > type->getRecord()->args.size())
      return nullptr;
    return type->getRecord()->args[id - 1];
  }
  if (auto cls = in(cache->classes, type->name)) {
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
  auto s = onDone(starArgIndex, kwstarArgIndex, slots, partial);
  return s != -1 ? score + s : -1;
}

void TypeContext::dump(int pad) {
  auto ordered =
      std::map<std::string, decltype(map)::mapped_type>(map.begin(), map.end());
  LOG("base: {}", getRealizationStackName());
  for (auto &i : ordered) {
    std::string s;
    auto t = i.second.front();
    LOG("{}{:.<25} {}", std::string(pad * 2, ' '), i.first, t->type);
  }
}

std::string TypeContext::debugInfo() {
  return fmt::format("[{}:i{}@{}]", getRealizationBase()->name,
                     getRealizationBase()->iteration, getSrcInfo());
}

std::shared_ptr<std::pair<std::vector<types::TypePtr>, std::vector<types::TypePtr>>>
TypeContext::getFunctionArgs(types::TypePtr t) {
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

std::shared_ptr<std::string> TypeContext::getStaticString(types::TypePtr t) {
  if (auto s = t->getStatic()) {
    auto r = s->evaluate();
    if (r.type == StaticValue::STRING)
      return std::make_shared<std::string>(r.getString());
  }
  return nullptr;
}

std::shared_ptr<int64_t> TypeContext::getStaticInt(types::TypePtr t) {
  if (auto s = t->getStatic()) {
    auto r = s->evaluate();
    if (r.type == StaticValue::INT)
      return std::make_shared<int64_t>(r.getInt());
  }
  return nullptr;
}

types::FuncTypePtr TypeContext::extractFunction(types::TypePtr t) {
  if (auto f = t->getFunc())
    return f;
  if (auto p = t->getPartial())
    return p->func;
  return nullptr;
}

} // namespace codon::ast
