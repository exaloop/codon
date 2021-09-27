/*
 * typecheck_ctx.cpp --- Context for type-checking stage.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/visitors/format/format.h"
#include "parser/visitors/typecheck/typecheck_ctx.h"

using fmt::format;
using std::dynamic_pointer_cast;
using std::stack;

namespace seq {
namespace ast {

TypeContext::TypeContext(shared_ptr<Cache> cache)
    : Context<TypecheckItem>(""), cache(move(cache)), typecheckLevel(0),
      allowActivation(true), age(0), realizationDepth(0) {
  stack.push_front(vector<string>());
  bases.push_back({"", nullptr, nullptr});
}

shared_ptr<TypecheckItem> TypeContext::add(TypecheckItem::Kind kind, const string &name,
                                           types::TypePtr type) {
  auto t = make_shared<TypecheckItem>(kind, type);
  add(name, t);
  return t;
}

shared_ptr<TypecheckItem> TypeContext::find(const string &name) const {
  if (auto t = Context<TypecheckItem>::find(name))
    return t;
  auto tt = findInVisited(name);
  if (tt.second)
    return make_shared<TypecheckItem>(tt.first, tt.second);
  return nullptr;
}

types::TypePtr TypeContext::findInternal(const string &name) const {
  auto t = find(name);
  seqassert(t, "cannot find '{}'", name);
  return t->type;
}

pair<TypecheckItem::Kind, types::TypePtr>
TypeContext::findInVisited(const string &name) const {
  for (int bi = int(bases.size()) - 1; bi >= 0; bi--) {
    auto t = bases[bi].visitedAsts.find(name);
    if (t == bases[bi].visitedAsts.end())
      continue;
    return t->second;
  }
  return {TypecheckItem::Var, nullptr};
}

int TypeContext::findBase(const string &b) {
  for (int i = int(bases.size()) - 1; i >= 0; i--)
    if (b == bases[i].name)
      return i;
  seqassert(false, "cannot find base '{}'", b);
  return -1;
}

string TypeContext::getBase() const {
  if (bases.empty())
    return "";
  vector<string> s;
  for (auto &b : bases)
    if (b.type)
      s.push_back(b.type->realizedName());
  return join(s, ":");
}

shared_ptr<types::LinkType> TypeContext::addUnbound(const Expr *expr, int level,
                                                    bool setActive, char staticType) {
  auto t = make_shared<types::LinkType>(types::LinkType::Unbound, cache->unboundCount++,
                                        level, nullptr, staticType);
  // if (t->id == 7815)
  // LOG("debug");
  t->setSrcInfo(expr->getSrcInfo());
  LOG_TYPECHECK("[ub] new {}: {} ({})", t->debugString(true), expr->toString(),
                setActive);
  if (setActive && allowActivation)
    activeUnbounds[t] = cache->getContent(expr->getSrcInfo());
  return t;
}

types::TypePtr TypeContext::instantiate(const Expr *expr, types::TypePtr type,
                                        types::ClassType *generics, bool activate) {
  assert(type);
  unordered_map<int, types::TypePtr> genericCache;
  if (generics)
    for (auto &g : generics->generics)
      if (g.type &&
          !(g.type->getLink() && g.type->getLink()->kind == types::LinkType::Generic)) {
        genericCache[g.id] = g.type;
      }
  auto t = type->instantiate(getLevel(), &(cache->unboundCount), &genericCache);
  for (auto &i : genericCache) {
    if (auto l = i.second->getLink()) {
      if (l->kind != types::LinkType::Unbound)
        continue;
      i.second->setSrcInfo(expr->getSrcInfo());
      if (activeUnbounds.find(i.second) == activeUnbounds.end()) {
        LOG_TYPECHECK("[ub] #{} -> {} (during inst of {}): {} ({})", i.first,
                      i.second->debugString(true), type->debugString(true),
                      expr->toString(), activate);
        if (activate && allowActivation)
          activeUnbounds[i.second] =
              format("{} of {} in {}", l->genericName.empty() ? "?" : l->genericName,
                     type->toString(), cache->getContent(expr->getSrcInfo()));
      }
    }
  }
  LOG_TYPECHECK("[inst] {} -> {}", expr->toString(), t->debugString(true));
  return t;
}

types::TypePtr TypeContext::instantiateGeneric(const Expr *expr, types::TypePtr root,
                                               const vector<types::TypePtr> &generics) {
  auto c = root->getClass();
  assert(c);
  auto g = make_shared<types::ClassType>("", ""); // dummy generic type
  if (generics.size() != c->generics.size())
    error(expr->getSrcInfo(), "generics do not match");
  for (int i = 0; i < c->generics.size(); i++) {
    assert(c->generics[i].type);
    g->generics.push_back(
        types::ClassType::Generic("", "", generics[i], c->generics[i].id));
  }
  return instantiate(expr, root, g.get());
}

vector<types::FuncTypePtr> TypeContext::findMethod(const string &typeName,
                                                   const string &method) const {
  auto m = cache->classes.find(typeName);
  if (m != cache->classes.end()) {
    auto t = m->second.methods.find(method);
    if (t != m->second.methods.end()) {
      unordered_map<string, int> signatureLoci;
      vector<types::FuncTypePtr> vv;
      for (auto &mt : t->second) {
        // LOG("{}::{} @ {} vs. {}", typeName, method, age, mt.age);
        if (mt.age <= age) {
          auto sig = cache->functions[mt.name].ast->signature();
          auto it = signatureLoci.find(sig);
          if (it != signatureLoci.end())
            vv[it->second] = mt.type;
          else {
            signatureLoci[sig] = vv.size();
            vv.emplace_back(mt.type);
          }
        }
      }
      return vv;
    }
  }
  return {};
}

types::TypePtr TypeContext::findMember(const string &typeName,
                                       const string &member) const {
  if (member == "__elemsize__")
    return findInternal("int");
  if (member == "__atomic__")
    return findInternal("bool");
  auto m = cache->classes.find(typeName);
  if (m != cache->classes.end()) {
    for (auto &mm : m->second.fields)
      if (mm.name == member)
        return mm.type;
  }
  return nullptr;
}

types::FuncTypePtr
TypeContext::findBestMethod(const Expr *expr, const string &member,
                            const vector<pair<string, types::TypePtr>> &args,
                            bool checkSingle) {
  auto typ = expr->getType()->getClass();
  seqassert(typ, "not a class");
  auto methods = findMethod(typ->name, member);
  if (methods.empty())
    return nullptr;
  if (methods.size() == 1 && !checkSingle) // methods is not overloaded
    return methods[0];

  // Calculate the unification score for each available methods and pick the one with
  // highest score.
  vector<pair<int, int>> scores;
  for (int mi = 0; mi < methods.size(); mi++) {
    auto method = instantiate(expr, methods[mi], typ.get(), false)->getFunc();
    vector<types::TypePtr> reordered;
    vector<CallExpr::Arg> callArgs;
    for (auto &a : args) {
      callArgs.push_back({a.first, make_shared<NoneExpr>()}); // dummy expression
      callArgs.back().value->setType(a.second);
    }
    auto score = reorderNamedArgs(
        method.get(), callArgs,
        [&](int s, int k, const vector<vector<int>> &slots, bool _) {
          for (int si = 0; si < slots.size(); si++) {
            // Ignore *args, *kwargs and default arguments
            reordered.emplace_back(si == s || si == k || slots[si].size() != 1
                                       ? nullptr
                                       : args[slots[si][0]].second);
          }
          return 0;
        },
        [](const string &) { return -1; });
    if (score == -1)
      continue;
    // Scoring system for each argument:
    //   Generics, traits and default arguments get a score of zero (lowest priority).
    //   Optional unwrap gets the score of 1.
    //   Optional wrap gets the score of 2.
    //   Successful unification gets the score of 3 (highest priority).
    for (int ai = 0, mi = 1, gi = 0; ai < reordered.size(); ai++) {
      auto argType = reordered[ai];
      if (!argType)
        continue;
      auto expectedType = method->ast->args[ai].generic ? method->generics[gi++].type
                                                        : method->args[mi++];
      auto expectedClass = expectedType->getClass();
      // Ignore traits, *args/**kwargs and default arguments.
      if (expectedClass && expectedClass->name == "Generator")
        continue;
      // LOG("<~> {} {}", argType->toString(), expectedType->toString());
      auto argClass = argType->getClass();

      types::Type::Unification undo;
      int u = argType->unify(expectedType.get(), &undo);
      undo.undo();
      if (u >= 0) {
        score += u + 3;
        continue;
      }
      if (!method->ast->args[ai].generic) {
        // Unification failed: maybe we need to wrap an argument?
        if (expectedClass && expectedClass->name == TYPE_OPTIONAL && argClass &&
            argClass->name != expectedClass->name) {
          u = argType->unify(expectedClass->generics[0].type.get(), &undo);
          undo.undo();
          if (u >= 0) {
            score += u + 2;
            continue;
          }
        }
        // ... or unwrap it (less ideal)?
        if (argClass && argClass->name == TYPE_OPTIONAL && expectedClass &&
            argClass->name != expectedClass->name) {
          u = argClass->generics[0].type->unify(expectedType.get(), &undo);
          undo.undo();
          if (u >= 0) {
            score += u;
            continue;
          }
        }
      }
      // This method cannot be selected, ignore it.
      score = -1;
      break;
    }
    // LOG("{} {} / {}", typ->toString(), method->toString(), score);
    if (score >= 0)
      scores.emplace_back(std::make_pair(score, mi));
  }
  if (scores.empty())
    return nullptr;
  // Get the best score.
  sort(scores.begin(), scores.end(), std::greater<>());
  // LOG("Method: {}", methods[scores[0].second]->toString());
  // string x;
  // for (auto &a : args)
  //   x += format("{}{},", a.first.empty() ? "" : a.first + ": ",
  //   a.second->toString());
  // LOG("        {} :: {} ( {} )", typ->toString(), member, x);
  return methods[scores[0].second];
}

int TypeContext::reorderNamedArgs(types::FuncType *func,
                                  const vector<CallExpr::Arg> &args,
                                  ReorderDoneFn onDone, ReorderErrorFn onError,
                                  const vector<char> &known) {
  // See https://docs.python.org/3.6/reference/expressions.html#calls for details.
  // Final score:
  //  - +1 for each matched argument
  //  -  0 for *args/**kwargs/default arguments
  //  - -1 for failed match
  int score = 0;

  // 0. Find *args and **kwargs
  // True if there is a trailing ellipsis (full partial: fn(all_args, ...))
  bool partial = !args.empty() && args.back().value->getEllipsis() &&
                 !args.back().value->getEllipsis()->isPipeArg &&
                 args.back().name.empty();

  int starArgIndex = -1, kwstarArgIndex = -1;
  for (int i = 0; i < func->ast->args.size(); i++) {
    if ((known.empty() || !known[i]) && startswith(func->ast->args[i].name, "**"))
      kwstarArgIndex = i, score -= 2;
    else if ((known.empty() || !known[i]) && startswith(func->ast->args[i].name, "*"))
      starArgIndex = i, score -= 2;
  }
  seqassert(known.empty() || starArgIndex == -1 || !known[starArgIndex],
            "partial *args");
  seqassert(known.empty() || kwstarArgIndex == -1 || !known[kwstarArgIndex],
            "partial **kwargs");

  // 1. Assign positional arguments to slots
  // Each slot contains a list of arg's indices
  vector<vector<int>> slots(func->ast->args.size());
  seqassert(known.empty() || func->ast->args.size() == known.size(),
            "bad 'known' string");
  vector<int> extra;
  std::map<string, int> namedArgs, extraNamedArgs; // keep the map--- we need it sorted!
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

  for (auto ai : vector<int>{std::max(starArgIndex, kwstarArgIndex),
                             std::min(starArgIndex, kwstarArgIndex)})
    if (ai != -1 && !slots[ai].empty()) {
      extra.insert(extra.begin(), ai);
      slots[ai].clear();
    }

  // 2. Assign named arguments to slots
  if (!namedArgs.empty()) {
    std::map<string, int> slotNames;
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
        return onError(format("argument '{}' already assigned", n.first));
    }
  }

  // 3. Fill in *args, if present
  if (!extra.empty() && starArgIndex == -1)
    return onError(format("too many arguments for {} (expected maximum {}, got {})",
                          func->toString(), func->ast->args.size(),
                          args.size() - partial));
  if (starArgIndex != -1)
    slots[starArgIndex] = extra;

  // 4. Fill in **kwargs, if present
  if (!extraNamedArgs.empty() && kwstarArgIndex == -1)
    return onError(format("unknown argument '{}'", extraNamedArgs.begin()->first));
  if (kwstarArgIndex != -1)
    for (auto &e : extraNamedArgs)
      slots[kwstarArgIndex].push_back(e.second);

  // 5. Fill in the default arguments
  for (auto i = 0; i < func->ast->args.size(); i++)
    if (slots[i].empty() && i != starArgIndex && i != kwstarArgIndex) {
      if (!func->ast->args[i].generic &&
          (func->ast->args[i].deflt || (!known.empty() && known[i])))
        score -= 2;
      else if (!partial && !func->ast->args[i].generic)
        return onError(format("missing argument '{}'",
                              cache->reverseIdentifierLookup[func->ast->args[i].name]));
    }
  return score + onDone(starArgIndex, kwstarArgIndex, slots, partial);
}

void TypeContext::dump(int pad) {
  auto ordered = std::map<string, decltype(map)::mapped_type>(map.begin(), map.end());
  LOG("base: {}", getBase());
  for (auto &i : ordered) {
    string s;
    auto t = i.second.front().second;
    LOG("{}{:.<25} {}", string(pad * 2, ' '), i.first, t->type->toString());
  }
}

} // namespace ast
} // namespace seq
