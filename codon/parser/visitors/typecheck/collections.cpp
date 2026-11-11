// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Transform tuples.
/// @example
///   `(a1, ..., aN)` -> `Tuple.__new__(a1, ..., aN)`
void TypecheckVisitor::visit(TupleExpr *expr) {
  resultExpr = transform(
      N<CallExpr>(N<DotExpr>(N<IdExpr>(StdlibTypes::Tuple), "__new__"), expr->items));
}

/// Transform a list `[a1, ..., aN]` to the corresponding statement expression.
/// See @c transformComprehension
void TypecheckVisitor::visit(ListExpr *expr) {
  expr->setType(instantiateUnbound());
  if ((resultExpr = transformComprehension(StdlibTypes::List, "append", expr->items,
                                           expr->getExpectedType()))) {
    resultExpr->setAttribute(Attr::ExprList);
  }
}

/// Transform a set `{a1, ..., aN}` to the corresponding statement expression.
/// See @c transformComprehension
void TypecheckVisitor::visit(SetExpr *expr) {
  expr->setType(instantiateUnbound());
  if ((resultExpr = transformComprehension(StdlibTypes::Set, "add", expr->items,
                                           expr->getExpectedType()))) {
    resultExpr->setAttribute(Attr::ExprSet);
  }
}

/// Transform a dictionary `{k1: v1, ..., kN: vN}` to a corresponding statement
/// expression. See @c transformComprehension
void TypecheckVisitor::visit(DictExpr *expr) {
  expr->setType(instantiateUnbound());
  if (auto *expected = expr->getExpectedType();
      expected && expected->is(getMangledClass("", "FixedDict"))) {
    resultExpr = transformFixedDict(expr->items);
    return;
  }
  if (!expr->getExpectedType() && std::ranges::any_of(expr->items, [](auto *item) {
        return cast<KeywordStarExpr>(item) != nullptr;
      })) {
    bool fixedSpread = false;
    bool fixedKeys = true;
    for (auto *item : expr->items) {
      if (auto *spread = cast<KeywordStarExpr>(item)) {
        auto *value = transform(clone(spread->getExpr()));
        bool fixed = value->getType()->is(getMangledClass("", "FixedDict"));
        fixedSpread |= fixed;
        fixedKeys &= fixed;
      } else if (auto *pair = cast<TupleExpr>(item)) {
        auto *key = transform(clone(pair->items[0]));
        auto *literal = cast<StringExpr>(key);
        fixedKeys &= (literal && literal->isSimple()) || key->getType()->getStrStatic();
      } else {
        fixedKeys = false;
      }
    }
    if (fixedSpread && fixedKeys) {
      resultExpr = transformFixedDict(expr->items);
      return;
    }
  }
  if ((resultExpr = transformComprehension(StdlibTypes::Dict, "__setitem__",
                                           expr->items, expr->getExpectedType()))) {
    resultExpr->setAttribute(Attr::ExprDict);
  }
}

Expr *TypecheckVisitor::transformFixedDict(const std::vector<Expr *> &items) {
  std::vector<std::string> names;
  std::vector<Expr *> values;
  std::vector<Stmt *> statements;
  std::unordered_map<std::string, size_t> positions;
  auto append = [&](const std::string &name, Expr *value) {
    auto [position, inserted] = positions.emplace(name, names.size());
    if (inserted) {
      names.push_back(name);
      values.push_back(nullptr);
    }
    values[position->second] = value;
  };
  for (auto *item : items) {
    if (auto *spread = cast<KeywordStarExpr>(item)) {
      auto *value = transform(clone(spread->getExpr()));
      auto *type = value->getClassType();
      if (!type)
        return nullptr;
      if (!type->is(getMangledClass("", "FixedDict")))
        E(Error::CUSTOM, spread, "fixed_dict can only expand fixed dictionaries");
      auto *shape = extractClassGeneric(type, 0);
      if (!shape->canRealize())
        return nullptr;
      const auto &keys = ctx->cache->generatedTupleNames.at(getIntLiteral(shape));
      auto temporary = getTemporaryVar("fixed.spread");
      statements.push_back(N<AssignStmt>(N<IdExpr>(temporary), value));
      for (size_t index = 0; index < keys.size(); ++index)
        append(keys[index], N<IndexExpr>(N<DotExpr>(N<IdExpr>(temporary), "_values"),
                                         N<IntExpr>(index)));
      continue;
    }
    auto *pair = cast<TupleExpr>(item);
    if (!pair)
      pair = cast<TupleExpr>(item->getOrigExpr());
    if (!pair || pair->items.size() != 2)
      E(Error::CUSTOM, item, "fixed_dict requires literal string-keyed entries");
    auto *key = transform(clone(pair->items[0]));
    if (!key->getType())
      return nullptr;
    auto *literal = cast<StringExpr>(key);
    auto *constant = key->getType()->getStrStatic();
    if ((!literal || !literal->isSimple()) && !constant)
      E(Error::CUSTOM, key, "fixed dictionary keys must be compile-time strings");
    auto name = constant ? constant->value : literal->getValue();
    auto temporary = getTemporaryVar("fixed.entry");
    statements.push_back(N<AssignStmt>(N<IdExpr>(temporary), clone(item)));
    append(name, N<IndexExpr>(N<IdExpr>(temporary), N<IntExpr>(1)));
  }
  return transform(N<StmtExpr>(
      statements, N<CallExpr>(N<IdExpr>(getMangledClass("", "FixedDict")),
                              N<TupleExpr>(values), N<IntExpr>(generateKwId(names)))));
}

/// Transform a tuple generator expression.
/// @example
///   `tuple(expr for i in tuple_generator)` -> `Tuple.N.__new__(expr...)`
void TypecheckVisitor::visit(GeneratorExpr *expr) {
  // List comprehension optimization:
  // Use `iter.__len__()` when creating list if there is a single for loop
  // without any if conditions in the comprehension
  bool canOptimize =
      expr->kind == GeneratorExpr::ListGenerator && expr->loopCount() == 1;
  if (canOptimize) {
    auto iter = transform(clone(cast<ForStmt>(expr->getFinalSuite())->getIter()));
    auto ce = cast<CallExpr>(iter);
    if (IdExpr *id = nullptr; ce && ((id = cast<IdExpr>(ce->getExpr())))) {
      // Turn off this optimization for static items
      canOptimize &= !startswith(id->getValue(), "std.internal.static");
    }
  }

  Expr *var = N<IdExpr>(getTemporaryVar("gen"));
  if (expr->kind == GeneratorExpr::ListGenerator) {
    // List comprehensions
    expr->setFinalExpr(
        N<CallExpr>(N<DotExpr>(clone(var), "append"), expr->getFinalExpr()));
    auto suite = expr->getFinalSuite();
    auto noOptStmt = N<SuiteStmt>(
        N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>(StdlibTypes::List))), suite);

    if (canOptimize) {
      auto optimizeVar = getTemporaryVar("i");
      auto origIter = cast<ForStmt>(expr->getFinalSuite())->getIter();

      auto optStmt = clone(noOptStmt);
      cast<ForStmt>((*cast<SuiteStmt>(optStmt))[1])->iter = N<IdExpr>(optimizeVar);
      optStmt = N<SuiteStmt>(
          N<AssignStmt>(N<IdExpr>(optimizeVar), clone(origIter)),
          N<AssignStmt>(
              clone(var),
              N<CallExpr>(N<IdExpr>(StdlibTypes::List),
                          N<CallExpr>(N<DotExpr>(N<IdExpr>(optimizeVar), "__len__")))),
          (*cast<SuiteStmt>(optStmt))[1]);
      resultExpr = N<IfExpr>(
          N<CallExpr>(N<IdExpr>("hasattr"), clone(origIter), N<StringExpr>("__len__")),
          N<StmtExpr>(optStmt, clone(var)), N<StmtExpr>(noOptStmt, var));
    } else {
      resultExpr = N<StmtExpr>(noOptStmt, var);
    }
    resultExpr = transform(resultExpr);
  } else if (expr->kind == GeneratorExpr::SetGenerator) {
    // Set comprehensions
    auto head = N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>(StdlibTypes::Set)));
    expr->setFinalExpr(
        N<CallExpr>(N<DotExpr>(clone(var), "add"), expr->getFinalExpr()));
    auto suite = expr->getFinalSuite();
    resultExpr = transform(N<StmtExpr>(N<SuiteStmt>(head, suite), var));
  } else if (expr->kind == GeneratorExpr::DictGenerator) {
    // Dictionary comprehensions
    auto head = N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>(StdlibTypes::Dict)));
    expr->setFinalExpr(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                   N<StarExpr>(expr->getFinalExpr())));
    auto suite = expr->getFinalSuite();
    resultExpr = transform(N<StmtExpr>(N<SuiteStmt>(head, suite), var));
  } else if (expr->kind == GeneratorExpr::TupleGenerator) {
    seqassert(expr->loopCount() == 1, "invalid tuple generator");
    auto gen = transform(cast<ForStmt>(expr->getFinalSuite())->getIter());
    if (!gen->getType()->canRealize())
      return; // Wait until the iterator can be realized

    auto block = N<SuiteStmt>();
    // `tuple = tuple_generator`
    auto tupleVar = getTemporaryVar("tuple");
    block->addStmt(N<AssignStmt>(N<IdExpr>(tupleVar), gen));

    auto forStmt = clone(cast<ForStmt>(expr->getFinalSuite()));
    auto finalExpr = expr->getFinalExpr();
    auto [ok, delay, preamble, staticItems] = transformStaticLoopCall(
        cast<ForStmt>(expr->getFinalSuite())->getVar(), &forStmt->suite, gen,
        [&](Stmt *wrap) { return N<StmtExpr>(clone(wrap), clone(finalExpr)); }, true);
    if (!ok)
      E(Error::CALL_BAD_ITER, gen, gen->getType()->prettyString());
    if (delay)
      return;

    std::vector<Expr *> tupleItems;
    for (auto &i : staticItems)
      tupleItems.push_back(cast<Expr>(i));
    if (preamble)
      block->addStmt(preamble);
    resultExpr = transform(N<StmtExpr>(block, N<TupleExpr>(tupleItems)));
  } else {
    expr->loops =
        transform(expr->getFinalSuite()); // assume: internal data will be changed
    if (!expr->getFinalExpr()) {
      // Case such as (0 for _ in static.range(2))
      // TODO: make this better.
      E(Error::CUSTOM, expr,
        "generator cannot be compiled. If using static tuple generator, use tuple(...) "
        "instead.");
    }
    unify(expr->getType(), instantiateType(getStdLibType("Generator"),
                                           {expr->getFinalExpr()->getType()}));
    if (realize(expr->getType()))
      expr->setDone();
  }
}

/// Transform a collection of type `type` to a statement expression:
///   `[a1, ..., aN]` -> `cont = [type](); (cont.[fn](a1); ...); cont`
/// Union parameters in expectedType fix storage; other parameters use common-type
/// inference. Dictionary keys and values are inferred independently.
/// Any star-expression within the collection will be expanded:
///   `[a, *b]` -> `cont.[fn](a); for i in b: cont.[fn](i)`.
/// @example
///   `[a, *b, c]`  -> ```cont = List(3)
///                       cont.append(a)
///                       for i in b: cont.append(i)
///                       cont.append(c)```
///   `{a, *b, c}`  -> ```cont = Set()
///                       cont.add(a)
///                       for i in b: cont.add(i)
///                       cont.add(c)```
///   `{a: 1, **d}` -> ```cont = Dict()
///                       cont.__setitem__((a, 1))
///                       for i in b.items(): cont.__setitem__((i[0], i[i]))```
Expr *TypecheckVisitor::transformComprehension(const std::string &type,
                                               const std::string &fn,
                                               std::vector<Expr *> &items,
                                               types::Type *expectedType) {
  // Deduce the lowest common type of the collection--- in other words, the lowest
  // common ancestor of all types in the collection. For example, `type([1, 1.2]) ==
  // type([1.2, 1]) == float` because float is an "ancestor" of int.
  // TODO: use wrapExpr...
  auto lctyp = [&](ClassType *collectionCls, ClassType *ti) -> TypePtr {
    if (!collectionCls)
      return ti->shared_from_this();
    // NoneType introduces nullability without becoming the element type.
    if (collectionCls->is(StdlibTypes::NoneType) != ti->is(StdlibTypes::NoneType)) {
      auto member = collectionCls->is(StdlibTypes::NoneType) ? ti : collectionCls;
      return member->is(StdlibTypes::Optional)
                 ? member->shared_from_this()
                 : instantiateType(getStdLibType(StdlibTypes::Optional),
                                   std::vector<Type *>{member});
    }
    if (collectionCls->is("Int") && ti->is("float")) {
      // Rule: int derives from float
      return ti->shared_from_this();
    } else if (collectionCls->name != StdlibTypes::Optional &&
               ti->name == StdlibTypes::Optional) {
      // Rule: T derives from Optional[T]
      return instantiateType(getStdLibType(StdlibTypes::Optional),
                             std::vector<types::Type *>{collectionCls});
    } else if (collectionCls->name == StdlibTypes::Optional &&
               ti->name != StdlibTypes::Optional) {
      return instantiateType(getStdLibType(StdlibTypes::Optional),
                             std::vector<types::Type *>{ti});
    } else if (!collectionCls->is("pyobj") && ti->is("pyobj")) {
      // Rule: anything derives from pyobj
      return ti->shared_from_this();
    } else if (collectionCls->name != ti->name) {
      // Rule: subclass derives from its base class
      const auto &mros = getClass(collectionCls)->mro;
      const auto &tMros = getClass(ti)->mro;
      for (size_t i = 0; i < mros.size(); i++) {
        auto t = instantiateType(mros[i].get(), collectionCls);
        for (size_t j = 0; j < tMros.size(); j++) {
          auto tj = instantiateType(tMros[j].get(), ti);
          if (t->unify(tj.get(), nullptr) >= 0) {
            return t->shared_from_this();
          }
        }
      }
    }
    return nullptr;
  };

  bool isDict = type == StdlibTypes::Dict;
  auto root = getStdLibType(type);
  if (isDict) // Instantiate Dict's class template, not the alias.
    root = getStdLibType(root->name);
  std::vector<TypePtr> elementTypes;
  std::vector<bool> declaredUnions;
  for (int di = 0; di < (isDict ? 2 : 1); ++di) {
    // Dict is an alias: compare the resolved class, e.g. OrderedDict.
    auto member = expectedType && expectedType->is(root->name)
                      ? extractClassGeneric(expectedType, di)
                      : nullptr;
    declaredUnions.push_back(member && member->getUnion());
    elementTypes.push_back(declaredUnions.back() ? member->shared_from_this()
                                                 : instantiateUnbound());
  }
  bool done = true;
  for (auto &i : items) {
    ClassType *typ = nullptr;
    std::vector<ClassType *> entryTypes;
    bool fixedView = false;
    if (!isDict && cast<StarExpr>(i)) {
      auto star = cast<StarExpr>(i);
      star->expr = transform(N<CallExpr>(N<DotExpr>(star->getExpr(), "__iter__")));
      if (star->getExpr()->getType()->is("Generator"))
        typ = extractClassGeneric(star->getExpr()->getType())->getClass();
    } else if (isDict && cast<KeywordStarExpr>(i)) {
      auto star = cast<KeywordStarExpr>(i);
      star->expr = transform(N<CallExpr>(N<DotExpr>(star->getExpr(), "items")));
      if (star->getExpr()->getType()->is("Generator"))
        typ = extractClassGeneric(star->getExpr()->getType())->getClass();
      else if (star->getExpr()->getType()->is(
                   getMangledClass("std.internal.types.tuple", "_FixedDictView"))) {
        auto *entries = extractClassGeneric(star->getExpr()->getType())->getClass();
        if (!entries)
          return nullptr;
        fixedView = true;
        for (const auto &entry : entries->generics)
          entryTypes.push_back(entry.getType()->getClass());
      }
    } else {
      i = transform(i);
      typ = i->getClassType();
    }
    if (typ)
      entryTypes.push_back(typ);
    if (entryTypes.empty() && !fixedView) {
      done = false;
      continue;
    }
    for (auto *entryType : entryTypes) {
      if (!entryType)
        return nullptr;
      if (isDict)
        entryType = unify(entryType, instantiateType(generateTuple(2)))->getClass();
      for (size_t di = 0; di < elementTypes.size(); ++di) {
        // A declared union fixes storage; only inferred slots participate in lctyp.
        // Keep dict keys/values independent (union_collection_common_type).
        if (declaredUnions[di])
          continue;
        auto member = isDict ? extractClassGeneric(entryType, di) : entryType;
        if (!elementTypes[di]->getClass())
          unify(elementTypes[di].get(), member);
        else if (auto t = lctyp(elementTypes[di]->getClass(), member->getClass()))
          elementTypes[di] = t;
      }
    }
  }
  if (!done)
    return nullptr;
  auto ta = instantiateType(root, ctx->cache->castVectorPtr(elementTypes));
  if (isDict && !(expectedType && expectedType->is(root->name)) && !items.empty()) {
    bool fixedKeys = true;
    bool incompatible = false;
    for (auto *item : items) {
      auto *pair = cast<TupleExpr>(item->getOrigExpr());
      if (!pair)
        pair = cast<TupleExpr>(item);
      auto *entryType = item->getClassType();
      if (!pair || pair->items.size() != 2 || !entryType || !entryType->canRealize()) {
        fixedKeys = false;
        break;
      }
      auto *key = transform(clone(pair->items[0]));
      auto *literal = cast<StringExpr>(key);
      if ((!literal || !literal->isSimple()) && !key->getType()->getStrStatic()) {
        fixedKeys = false;
        break;
      }
      auto *keyType = extractClassGeneric(entryType, 0);
      auto *valueType = extractClassGeneric(entryType, 1);
      if (elementTypes[0]->unify(keyType, nullptr) < 0 ||
          elementTypes[1]->unify(valueType, nullptr) < 0)
        incompatible |= !findBestMethod(ta->getClass(), "__setitem__",
                                        {ta.get(), keyType, valueType});
    }
    if (fixedKeys && incompatible)
      return transformFixedDict(items);
  }
  std::vector<Stmt *> stmts;
  Expr *var = N<IdExpr>(getTemporaryVar("cont"));

  std::vector<Expr *> constructorArgs{};
  if (type == StdlibTypes::List && !items.empty()) {
    // Optimization: pre-allocate the list with the exact number of elements
    constructorArgs.push_back(N<IntExpr>(items.size()));
  }
  auto t = N<IdExpr>(isDict ? root->name : type);
  t->setType(instantiateTypeVar(ta.get()));
  stmts.push_back(N<AssignStmt>(clone(var), N<CallExpr>(t, constructorArgs)));
  for (const auto &it : items) {
    if (!isDict && cast<StarExpr>(it)) {
      // Unpack star-expression by iterating over it
      // `*star` -> `for i in star: cont.[fn](i)`
      auto star = cast<StarExpr>(it);
      Expr *forVar = N<IdExpr>(getTemporaryVar("i"));
      star->getExpr()->setAttribute(Attr::ExprStarSequenceItem);
      stmts.push_back(N<ForStmt>(
          clone(forVar), star->getExpr(),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), clone(forVar)))));
    } else if (isDict && cast<KeywordStarExpr>(it)) {
      // Expand kwstar-expression by iterating over it: see the example above
      auto star = cast<KeywordStarExpr>(it);
      Expr *forVar = N<IdExpr>(getTemporaryVar("it"));
      star->getExpr()->setAttribute(Attr::ExprStarSequenceItem);
      stmts.push_back(N<ForStmt>(
          clone(forVar), star->getExpr(),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(0)),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(1))))));
    } else {
      it->setAttribute(Attr::ExprSequenceItem);
      if (isDict) {
        Expr *head = it, *lead = nullptr;
        if (hasSideEffect(head)) {
          auto var = getTemporaryVar("star");
          lead = N<AssignExpr>(N<IdExpr>(var), head);
          head = N<IdExpr>(var);
        } else {
          lead = clone(head);
        }
        lead->setAttribute(Attr::ExprSequenceItem);
        head->setAttribute(Attr::ExprSequenceItem);
        stmts.push_back(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn),
                                                N<IndexExpr>(lead, N<IntExpr>(0)),
                                                N<IndexExpr>(head, N<IntExpr>(1)))));
      } else {
        it->setAttribute(Attr::ExprSequenceItem);
        stmts.push_back(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), it)));
      }
    }
  }
  return transform(N<StmtExpr>(stmts, var));
}

} // namespace codon::ast
