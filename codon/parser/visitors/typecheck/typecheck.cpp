// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "typecheck.h"

#include <memory>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/ctx.h"
#include "codon/parser/visitors/typecheck/ctx.h"
#include <fmt/format.h>

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

StmtPtr TypecheckVisitor::apply(Cache *cache, const StmtPtr &stmts) {
  if (!cache->typeCtx)
    cache->typeCtx = std::make_shared<TypeContext>(cache);
  TypecheckVisitor v(cache->typeCtx);
  auto so = clone(stmts);
  auto s = v.inferTypes(so, true);
  if (!s) {
    LOG_REALIZE("[error] {}", so->toString(2));
    v.error("cannot typecheck the program");
  }
  if (s->getSuite())
    v.prepareVTables();
  return s;
}

/**************************************************************************************/

TypecheckVisitor::TypecheckVisitor(std::shared_ptr<TypeContext> ctx,
                                   const std::shared_ptr<std::vector<StmtPtr>> &stmts)
    : ctx(std::move(ctx)) {
  prependStmts = stmts ? stmts : std::make_shared<std::vector<StmtPtr>>();
}

/**************************************************************************************/

/// Transform an expression node.
ExprPtr TypecheckVisitor::transform(ExprPtr &expr) {
  if (!expr)
    return nullptr;

  auto typ = expr->type;
  if (!expr->done) {
    bool isIntStatic = expr->staticValue.type == StaticValue::INT;
    TypecheckVisitor v(ctx, prependStmts);
    v.setSrcInfo(expr->getSrcInfo());
    ctx->pushSrcInfo(expr->getSrcInfo());
    expr->accept(v);
    ctx->popSrcInfo();
    if (v.resultExpr) {
      v.resultExpr->attributes |= expr->attributes;
      v.resultExpr->origExpr = expr;
      expr = v.resultExpr;
    }
    seqassert(expr->type, "type not set for {}", expr);
    if (!(isIntStatic && expr->type->is("bool")))
      unify(typ, expr->type);
    if (expr->done) {
      ctx->changedNodes++;
    }
  }
  realize(typ);
  LOG_TYPECHECK("[expr] {}: {}{}", getSrcInfo(), expr, expr->isDone() ? "[done]" : "");
  return expr;
}

/// Transform a type expression node.
/// Special case: replace `None` with `NoneType`
/// @throw @c ParserException if a node is not a type (use @c transform instead).
ExprPtr TypecheckVisitor::transformType(ExprPtr &expr) {
  if (expr && expr->getNone()) {
    expr = N<IdExpr>(expr->getSrcInfo(), "NoneType");
    expr->markType();
  }
  transform(expr);
  if (expr) {
    if (!expr->isType() && expr->isStatic()) {
      expr->setType(Type::makeStatic(ctx->cache, expr));
    } else if (!expr->isType()) {
      E(Error::EXPECTED_TYPE, expr, "type");
    } else {
      expr->setType(ctx->instantiate(expr->getType()));
    }
  }
  return expr;
}

void TypecheckVisitor::defaultVisit(Expr *e) {
  seqassert(false, "unexpected AST node {}", e->toString());
}

/// Transform a statement node.
StmtPtr TypecheckVisitor::transform(StmtPtr &stmt) {
  if (!stmt || stmt->done)
    return stmt;

  TypecheckVisitor v(ctx);
  v.setSrcInfo(stmt->getSrcInfo());
  auto oldAge = ctx->age;
  stmt->age = ctx->age = std::max(stmt->age, oldAge);
  ctx->pushSrcInfo(stmt->getSrcInfo());
  stmt->accept(v);
  ctx->popSrcInfo();
  ctx->age = oldAge;
  if (v.resultStmt)
    stmt = v.resultStmt;
  if (!v.prependStmts->empty()) {
    if (stmt)
      v.prependStmts->push_back(stmt);
    bool done = true;
    for (auto &s : *(v.prependStmts))
      done &= s->done;
    stmt = N<SuiteStmt>(*v.prependStmts);
    stmt->done = done;
  }
  if (stmt->done)
    ctx->changedNodes++;
  return stmt;
}

void TypecheckVisitor::defaultVisit(Stmt *s) {
  seqassert(false, "unexpected AST node {}", s->toString());
}

/**************************************************************************************/

/// Typecheck statement expressions.
void TypecheckVisitor::visit(StmtExpr *expr) {
  auto done = true;
  for (auto &s : expr->stmts) {
    transform(s);
    done &= s->isDone();
  }
  transform(expr->expr);
  unify(expr->type, expr->expr->type);
  if (done && expr->expr->isDone())
    expr->setDone();
}

/// Typecheck a list of statements.
void TypecheckVisitor::visit(SuiteStmt *stmt) {
  std::vector<StmtPtr> stmts; // for filtering out nullptr statements
  auto done = true;
  for (auto &s : stmt->stmts) {
    if (ctx->returnEarly) {
      // If returnEarly is set (e.g., in the function) ignore the rest
      break;
    }
    if (transform(s)) {
      stmts.push_back(s);
      done &= stmts.back()->isDone();
    }
  }
  stmt->stmts = stmts;
  if (done)
    stmt->setDone();
}

/// Typecheck expression statements.
void TypecheckVisitor::visit(ExprStmt *stmt) {
  transform(stmt->expr);
  if (stmt->expr->isDone())
    stmt->setDone();
}

void TypecheckVisitor::visit(CommentStmt *stmt) { stmt->setDone(); }

/**************************************************************************************/

/// Select the best method indicated of an object that matches the given argument
/// types. See @c findMatchingMethods for details.
types::FuncTypePtr
TypecheckVisitor::findBestMethod(const ClassTypePtr &typ, const std::string &member,
                                 const std::vector<types::TypePtr> &args) {
  std::vector<CallExpr::Arg> callArgs;
  for (auto &a : args) {
    callArgs.push_back({"", std::make_shared<NoneExpr>()}); // dummy expression
    callArgs.back().value->setType(a);
  }
  auto methods = ctx->findMethod(typ.get(), member, false);
  auto m = findMatchingMethods(typ, methods, callArgs);
  return m.empty() ? nullptr : m[0];
}

/// Select the best method indicated of an object that matches the given argument
/// types. See @c findMatchingMethods for details.
types::FuncTypePtr TypecheckVisitor::findBestMethod(const ClassTypePtr &typ,
                                                    const std::string &member,
                                                    const std::vector<ExprPtr> &args) {
  std::vector<CallExpr::Arg> callArgs;
  for (auto &a : args)
    callArgs.push_back({"", a});
  auto methods = ctx->findMethod(typ.get(), member, false);
  auto m = findMatchingMethods(typ, methods, callArgs);
  return m.empty() ? nullptr : m[0];
}

/// Select the best method indicated of an object that matches the given argument
/// types. See @c findMatchingMethods for details.
types::FuncTypePtr TypecheckVisitor::findBestMethod(
    const ClassTypePtr &typ, const std::string &member,
    const std::vector<std::pair<std::string, types::TypePtr>> &args) {
  std::vector<CallExpr::Arg> callArgs;
  for (auto &[n, a] : args) {
    callArgs.push_back({n, std::make_shared<NoneExpr>()}); // dummy expression
    callArgs.back().value->setType(a);
  }
  auto methods = ctx->findMethod(typ.get(), member, false);
  auto m = findMatchingMethods(typ, methods, callArgs);
  return m.empty() ? nullptr : m[0];
}

// Search expression tree for a identifier
class IdSearchVisitor : public CallbackASTVisitor<bool, bool> {
  std::string what;
  bool result;

public:
  IdSearchVisitor(std::string what) : what(std::move(what)), result(false) {}
  bool transform(const std::shared_ptr<Expr> &expr) override {
    if (result)
      return result;
    IdSearchVisitor v(what);
    if (expr)
      expr->accept(v);
    return v.result;
  }
  bool transform(const std::shared_ptr<Stmt> &stmt) override {
    if (result)
      return result;
    IdSearchVisitor v(what);
    if (stmt)
      stmt->accept(v);
    return v.result;
  }
  void visit(IdExpr *expr) override {
    if (expr->value == what)
      result = true;
  }
};

/// Check if a function can be called with the given arguments.
/// See @c reorderNamedArgs for details.
int TypecheckVisitor::canCall(const types::FuncTypePtr &fn,
                              const std::vector<CallExpr::Arg> &args,
                              std::shared_ptr<types::PartialType> part) {
  auto getPartialArg = [&](size_t pi) -> types::TypePtr {
    if (pi < part->args.size())
      return part->args[pi];
    else
      return nullptr;
  };

  std::vector<std::pair<types::TypePtr, size_t>> reordered;
  auto niGenerics = fn->ast->getNonInferrableGenerics();
  auto score = ctx->reorderNamedArgs(
      fn.get(), args,
      [&](int s, int k, const std::vector<std::vector<int>> &slots, bool _) {
        for (int si = 0, gi = 0, pi = 0; si < slots.size(); si++) {
          if (fn->ast->args[si].status == Param::Generic) {
            if (slots[si].empty()) {
              // is this "real" type?
              if (in(niGenerics, fn->ast->args[si].name) &&
                  !fn->ast->args[si].defaultValue) {
                return -1;
              }
              reordered.emplace_back(nullptr, 0);
            } else {
              seqassert(gi < fn->funcGenerics.size(), "bad fn");
              if (!fn->funcGenerics[gi].type->isStaticType() &&
                  !args[slots[si][0]].value->isType())
                return -1;
              reordered.emplace_back(args[slots[si][0]].value->type, slots[si][0]);
            }
            gi++;
          } else if (si == s || si == k || slots[si].size() != 1) {
            // Partials
            if (slots[si].empty() && part && part->known[si]) {
              reordered.emplace_back(getPartialArg(pi++), 0);
            } else {
              // Ignore *args, *kwargs and default arguments
              reordered.emplace_back(nullptr, 0);
            }
          } else {
            reordered.emplace_back(args[slots[si][0]].value->type, slots[si][0]);
          }
        }
        return 0;
      },
      [](error::Error, const SrcInfo &, const std::string &) { return -1; },
      part ? part->known : std::vector<char>{});
  int ai = 0, mai = 0, gi = 0, real_gi = 0;
  for (; score != -1 && ai < reordered.size(); ai++) {
    auto expectTyp = fn->ast->args[ai].status == Param::Normal
                         ? fn->getArgTypes()[mai++]
                         : fn->funcGenerics[gi++].type;
    auto [argType, argTypeIdx] = reordered[ai];
    if (!argType)
      continue;
    real_gi += fn->ast->args[ai].status != Param::Normal;
    if (fn->ast->args[ai].status != Param::Normal) {
      // Check if this is a good generic!
      if (expectTyp && expectTyp->isStaticType()) {
        if (!args[argTypeIdx].value->isStatic()) {
          score = -1;
          break;
        } else {
          argType = Type::makeStatic(ctx->cache, args[argTypeIdx].value);
        }
      } else {
        /// TODO: check if these are real types or if traits are satisfied
        continue;
      }
    }
    try {
      ExprPtr dummy = std::make_shared<IdExpr>("");
      dummy->type = argType;
      dummy->setDone();
      wrapExpr(dummy, expectTyp, fn);
      types::Type::Unification undo;
      if (dummy->type->unify(expectTyp.get(), &undo) >= 0) {
        undo.undo();
      } else {
        score = -1;
      }
    } catch (const exc::ParserException &) {
      // Ignore failed wraps
      score = -1;
    }
  }
  if (score >= 0)
    score += (real_gi == fn->funcGenerics.size());
  return score;
}

/// Select the best method among the provided methods given the list of arguments.
/// See @c reorderNamedArgs for details.
std::vector<types::FuncTypePtr>
TypecheckVisitor::findMatchingMethods(const types::ClassTypePtr &typ,
                                      const std::vector<types::FuncTypePtr> &methods,
                                      const std::vector<CallExpr::Arg> &args) {
  // Pick the last method that accepts the given arguments.
  std::vector<types::FuncTypePtr> results;
  for (const auto &mi : methods) {
    if (!mi)
      continue; // avoid overloads that have not been seen yet
    auto method = ctx->instantiate(mi, typ)->getFunc();
    int score = canCall(method, args);
    if (score != -1) {
      results.push_back(mi);
    }
  }
  return results;
}

/// Wrap an expression to coerce it to the expected type if the type of the expression
/// does not match it. Also unify types.
/// @example
///   expected `Generator`                -> `expr.__iter__()`
///   expected `float`, got `int`         -> `float(expr)`
///   expected `Optional[T]`, got `T`     -> `Optional(expr)`
///   expected `T`, got `Optional[T]`     -> `unwrap(expr)`
///   expected `Function`, got a function -> partialize function
///   expected `T`, got `Union[T...]`     -> `__internal__.get_union(expr, T)`
///   expected `Union[T...]`, got `T`     -> `__internal__.new_union(expr, Union[T...])`
///   expected base class, got derived    -> downcast to base class
/// @param allowUnwrap allow optional unwrapping.
bool TypecheckVisitor::wrapExpr(ExprPtr &expr, const TypePtr &expectedType,
                                const FuncTypePtr &callee, bool allowUnwrap) {
  auto expectedClass = expectedType->getClass();
  auto exprClass = expr->getType()->getClass();
  auto doArgWrap =
      !callee || !callee->ast->hasAttr("std.internal.attributes.no_argument_wrap");
  if (!doArgWrap)
    return true;
  auto doTypeWrap =
      !callee || !callee->ast->hasAttr("std.internal.attributes.no_type_wrap");
  if (callee && expr->isType()) {
    auto c = expr->type->getClass();
    if (!c)
      return false;
    if (doTypeWrap) {
      if (c->getRecord())
        expr = transform(N<CallExpr>(expr, N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
      else
        expr = transform(N<CallExpr>(
            N<IdExpr>("__internal__.class_ctr:0"),
            std::vector<CallExpr::Arg>{{"T", expr},
                                       {"", N<EllipsisExpr>(EllipsisExpr::PARTIAL)}}));
    }
  }

  std::unordered_set<std::string> hints = {"Generator", "float", TYPE_OPTIONAL,
                                           "pyobj"};
  if (!exprClass && expectedClass && in(hints, expectedClass->name)) {
    return false; // argument type not yet known.
  } else if (expectedClass && expectedClass->name == "Generator" &&
             exprClass->name != expectedClass->name && !expr->getEllipsis()) {
    // Note: do not do this in pipelines (TODO: why?)
    expr = transform(N<CallExpr>(N<DotExpr>(expr, "__iter__")));
  } else if (expectedClass && expectedClass->name == "float" &&
             exprClass->name == "int") {
    expr = transform(N<CallExpr>(N<IdExpr>("float"), expr));
  } else if (expectedClass && expectedClass->name == TYPE_OPTIONAL &&
             exprClass->name != expectedClass->name) {
    expr = transform(N<CallExpr>(N<IdExpr>(TYPE_OPTIONAL), expr));
  } else if (allowUnwrap && expectedClass && exprClass &&
             exprClass->name == TYPE_OPTIONAL &&
             exprClass->name != expectedClass->name) { // unwrap optional
    expr = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr));
  } else if (expectedClass && expectedClass->name == "pyobj" &&
             exprClass->name != expectedClass->name) { // wrap to pyobj
    expr = transform(
        N<CallExpr>(N<IdExpr>("pyobj"), N<CallExpr>(N<DotExpr>(expr, "__to_py__"))));
  } else if (allowUnwrap && expectedClass && exprClass && exprClass->name == "pyobj" &&
             exprClass->name != expectedClass->name) { // unwrap pyobj
    auto texpr = N<IdExpr>(expectedClass->name);
    texpr->setType(expectedType);
    expr =
        transform(N<CallExpr>(N<DotExpr>(texpr, "__from_py__"), N<DotExpr>(expr, "p")));
  } else if (callee && exprClass && expr->type->getFunc() &&
             !(expectedClass && expectedClass->name == "Function")) {
    // Wrap raw Seq functions into Partial(...) call for easy realization.
    expr = partializeFunction(expr->type->getFunc());
  } else if (allowUnwrap && exprClass && expr->type->getUnion() && expectedClass &&
             !expectedClass->getUnion()) {
    // Extract union types via __internal__.get_union
    if (auto t = realize(expectedClass)) {
      auto e = realize(expr->type);
      if (!e)
        return false;
      bool ok = false;
      for (auto &ut : e->getUnion()->getRealizationTypes()) {
        if (ut->unify(t.get(), nullptr) >= 0) {
          ok = true;
          break;
        }
      }
      if (ok) {
        expr = transform(N<CallExpr>(N<IdExpr>("__internal__.get_union:0"), expr,
                                     N<IdExpr>(t->realizedName())));
      }
    } else {
      return false;
    }
  } else if (exprClass && expectedClass && expectedClass->getUnion()) {
    // Make union types via __internal__.new_union
    if (!expectedClass->getUnion()->isSealed()) {
      expectedClass->getUnion()->addType(exprClass);
    }
    if (auto t = realize(expectedClass)) {
      if (expectedClass->unify(exprClass.get(), nullptr) == -1)
        expr = transform(N<CallExpr>(N<IdExpr>("__internal__.new_union:0"), expr,
                                     NT<IdExpr>(t->realizedName())));
    } else {
      return false;
    }
  } else if (exprClass && expectedClass && exprClass->name != expectedClass->name) {
    // Cast derived classes to base classes
    auto &mros = ctx->cache->classes[exprClass->name].mro;
    for (size_t i = 1; i < mros.size(); i++) {
      auto t = ctx->instantiate(mros[i]->type, exprClass);
      if (t->unify(expectedClass.get(), nullptr) >= 0) {
        if (!expr->isId("")) {
          expr = castToSuperClass(expr, expectedClass, true);
        } else { // Just checking can this be done
          expr->type = expectedClass;
        }
        break;
      }
    }
  }
  return true;
}

/// Cast derived class to a base class.
ExprPtr TypecheckVisitor::castToSuperClass(ExprPtr expr, ClassTypePtr superTyp,
                                           bool isVirtual) {
  ClassTypePtr typ = expr->type->getClass();
  for (auto &field : getClassFields(typ.get())) {
    for (auto &parentField : getClassFields(superTyp.get()))
      if (field.name == parentField.name) {
        unify(ctx->instantiate(field.type, typ),
              ctx->instantiate(parentField.type, superTyp));
      }
  }
  realize(superTyp);
  auto typExpr = N<IdExpr>(superTyp->name);
  typExpr->setType(superTyp);
  return transform(
      N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_super"), expr, typExpr));
}

/// Unpack a Tuple or KwTuple expression into (name, type) vector.
/// Name is empty when handling Tuple; otherwise it matches names of KwTuple.
std::shared_ptr<std::vector<std::pair<std::string, types::TypePtr>>>
TypecheckVisitor::unpackTupleTypes(ExprPtr expr) {
  auto ret = std::make_shared<std::vector<std::pair<std::string, types::TypePtr>>>();
  if (auto tup = expr->origExpr->getTuple()) {
    for (auto &a : tup->items) {
      transform(a);
      if (!a->getType()->getClass())
        return nullptr;
      ret->push_back({"", a->getType()});
    }
  } else if (auto kw = expr->origExpr->getCall()) { // origExpr?
    auto kwCls = in(ctx->cache->classes, expr->getType()->getClass()->name);
    seqassert(kwCls, "cannot find {}", expr->getType()->getClass()->name);
    for (size_t i = 0; i < kw->args.size(); i++) {
      auto &a = kw->args[i].value;
      transform(a);
      if (!a->getType()->getClass())
        return nullptr;
      ret->push_back({kwCls->fields[i].name, a->getType()});
    }
  } else {
    return nullptr;
  }
  return ret;
}

std::vector<Cache::Class::ClassField> &
TypecheckVisitor::getClassFields(types::ClassType *t) {
  seqassert(t && in(ctx->cache->classes, t->name), "cannot find '{}'",
            t ? t->name : "<null>");
  if (t->is(TYPE_TUPLE) && !t->getRecord()->args.empty()) {
    auto key = ctx->generateTuple(t->getRecord()->args.size());
    return ctx->cache->classes[key].fields;
  } else {
    return ctx->cache->classes[t->name].fields;
  }
}

} // namespace codon::ast
