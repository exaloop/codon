// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "typecheck.h"

#include <memory>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/typecheck/ctx.h"
#include <fmt/format.h>

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Simplify an AST node. Load standard library if needed.
/// @param cache     Pointer to the shared cache ( @c Cache )
/// @param file      Filename to be used for error reporting
/// @param barebones Use the bare-bones standard library for faster testing
/// @param defines   User-defined static values (typically passed as `codon run -DX=Y`).
///                  Each value is passed as a string.
StmtPtr TypecheckVisitor::apply(
    Cache *cache, const StmtPtr &node, const std::string &file,
    const std::unordered_map<std::string, std::string> &defines,
    const std::unordered_map<std::string, std::string> &earlyDefines, bool barebones) {
  auto preamble = std::make_shared<std::vector<StmtPtr>>();
  seqassertn(cache->module, "cache's module is not set");

  // Load standard library if it has not been loaded
  if (!in(cache->imports, STDLIB_IMPORT))
    loadStdLibrary(cache, preamble, earlyDefines, barebones);

  // Set up the context and the cache
  auto ctx = std::make_shared<TypeContext>(cache, file);
  cache->imports[file] = cache->imports[MAIN_IMPORT] = {MAIN_IMPORT, file, ctx};
  ctx->setFilename(file);
  ctx->moduleName = {ImportFile::PACKAGE, file, MODULE_MAIN};

  // Prepare the code
  auto tv = TypecheckVisitor(ctx, preamble);
  StmtPtr suite = tv.N<SuiteStmt>();
  auto &stmts = suite->getSuite()->stmts;
  stmts.push_back(tv.N<ClassStmt>(".toplevel", std::vector<Param>{}, nullptr,
                                  std::vector<ExprPtr>{tv.N<IdExpr>(Attr::Internal)}));
  // Load compile-time defines (e.g., codon run -DFOO=1 ...)
  for (auto &d : defines) {
    stmts.push_back(
        tv.N<AssignStmt>(tv.N<IdExpr>(d.first), tv.N<IntExpr>(d.second),
                         tv.N<IndexExpr>(tv.N<IdExpr>("Static"), tv.N<IdExpr>("int"))));
  }
  // Set up __name__
  stmts.push_back(
      tv.N<AssignStmt>(tv.N<IdExpr>("__name__"), tv.N<StringExpr>(MODULE_MAIN)));
  stmts.push_back(node);

  ScopingVisitor::apply(cache, suite);
  auto n = tv.inferTypes(suite, true);
  if (!n) {
    LOG("[error=>] {}", suite->toString(2));
    tv.error("cannot typecheck the program");
  }

  suite = tv.N<SuiteStmt>();
  suite->getSuite()->stmts.push_back(tv.N<SuiteStmt>(*preamble));

  // Add dominated assignment declarations
  suite->getSuite()->stmts.insert(suite->getSuite()->stmts.end(),
                                  ctx->scope.back().stmts.begin(),
                                  ctx->scope.back().stmts.end());
  suite->getSuite()->stmts.push_back(n);

  if (n->getSuite())
    tv.prepareVTables();

  if (!ctx->cache->errors.empty())
    throw exc::ParserException();

  return suite;
}

void TypecheckVisitor::loadStdLibrary(
    Cache *cache, const std::shared_ptr<std::vector<StmtPtr>> &preamble,
    const std::unordered_map<std::string, std::string> &earlyDefines, bool barebones) {
  // Load the internal.__init__
  auto stdlib = std::make_shared<TypeContext>(cache, STDLIB_IMPORT);
  auto stdlibPath =
      getImportFile(cache->argv0, STDLIB_INTERNAL_MODULE, "", true, cache->module0);
  const std::string initFile = "__init__.codon";
  if (!stdlibPath || !endswith(stdlibPath->path, initFile))
    E(Error::COMPILER_NO_STDLIB);

  /// Use __init_test__ for faster testing (e.g., #%% name,barebones)
  /// TODO: get rid of it one day...
  if (barebones) {
    stdlibPath->path =
        stdlibPath->path.substr(0, stdlibPath->path.size() - initFile.size()) +
        "__init_test__.codon";
  }
  stdlib->setFilename(stdlibPath->path);
  cache->imports[stdlibPath->path] =
      cache->imports[STDLIB_IMPORT] = {STDLIB_IMPORT, stdlibPath->path, stdlib};

  // Load the standard library
  stdlib->isStdlibLoading = true;
  stdlib->moduleName = {ImportFile::STDLIB, stdlibPath->path, "__init__"};
  stdlib->setFilename(stdlibPath->path);

  // 1. Core definitions
  auto core = parseCode(stdlib->cache, stdlibPath->path, "from internal.core import *");
  ScopingVisitor::apply(stdlib->cache, core);
  auto tv = TypecheckVisitor(stdlib, preamble);
  core = tv.inferTypes(core, true);
  preamble->push_back(core);

  // 2. Load early compile-time defines (for standard library)
  for (auto &d : earlyDefines) {
    auto tv = TypecheckVisitor(stdlib, preamble);
    auto s =
        tv.N<AssignStmt>(tv.N<IdExpr>(d.first), tv.N<IntExpr>(d.second),
                         tv.N<IndexExpr>(tv.N<IdExpr>("Static"), tv.N<IdExpr>("int")));
    auto def = tv.transform(s);
    preamble->push_back(def);
  }

  // 3. Load stdlib
  auto std = parseFile(stdlib->cache, stdlibPath->path);
  ScopingVisitor::apply(stdlib->cache, std);
  tv = TypecheckVisitor(stdlib, preamble);
  std = tv.inferTypes(std, true);
  preamble->push_back(std);
  stdlib->isStdlibLoading = false;
}

/// Simplify an AST node. Assumes that the standard library is loaded.
StmtPtr TypecheckVisitor::apply(const std::shared_ptr<TypeContext> &ctx,
                                const StmtPtr &node, const std::string &file) {
  auto oldFilename = ctx->getFilename();
  ctx->setFilename(file);
  auto preamble = std::make_shared<std::vector<StmtPtr>>();
  auto tv = TypecheckVisitor(ctx, preamble);
  auto n = tv.inferTypes(node, true);
  ctx->setFilename(oldFilename);
  if (!n) {
    tv.error("cannot typecheck the program");
  }
  if (!ctx->cache->errors.empty()) {
    throw exc::ParserException();
  }

  auto suite = std::make_shared<SuiteStmt>(*preamble);
  suite->stmts.push_back(n);
  return suite;
}

/**************************************************************************************/

TypecheckVisitor::TypecheckVisitor(std::shared_ptr<TypeContext> ctx,
                                   const std::shared_ptr<std::vector<StmtPtr>> &pre,
                                   const std::shared_ptr<std::vector<StmtPtr>> &stmts)
    : ctx(std::move(ctx)) {
  preamble = pre ? pre : std::make_shared<std::vector<StmtPtr>>();
  prependStmts = stmts ? stmts : std::make_shared<std::vector<StmtPtr>>();
}

/**************************************************************************************/

ExprPtr TypecheckVisitor::transform(ExprPtr &expr) { return transform(expr, true); }

/// Transform an expression node.
ExprPtr TypecheckVisitor::transform(ExprPtr &expr, bool allowTypes) {
  if (!expr)
    return nullptr;

  if (!expr->type)
    unify(expr->type, ctx->getUnbound());

  auto typ = expr->type;
  if (!expr->done) {
    TypecheckVisitor v(ctx, preamble, prependStmts);
    v.setSrcInfo(expr->getSrcInfo());
    ctx->pushSrcInfo(expr->getSrcInfo());
    expr->accept(v);
    ctx->popSrcInfo();
    if (v.resultExpr) {
      v.resultExpr->attributes |= expr->attributes;
      v.resultExpr->origExpr = expr;
      expr = v.resultExpr;
    }
    if (!allowTypes && expr && expr->type->is("type"))
      E(Error::UNEXPECTED_TYPE, expr, "type");
    if (!expr->type)
      unify(expr->type, ctx->getUnbound());
    // if (auto s = typ->isStaticType()) { // realize replaced T with int/str
    //   if (!(s == StaticValue::INT && expr->getInt()) &&
    //       !(s == StaticValue::STRING && expr->getString())) {
    //     unify(typ, expr->type);
    //   }
    // } else {
    unify(typ, expr->type);
    // if (!allowStatics) {
    //   if (auto u = expr->type->getStatic())
    //     expr->type = ctx->getType(StaticType::getTypeName(u));
    // }
    if (expr->done)
      ctx->changedNodes++;
  }
  realize(typ);
  LOG_TYPECHECK("[expr] {}: {}{}", getSrcInfo(), expr, expr->isDone() ? "[done]" : "");
  return expr;
}

/// Transform a type expression node.
/// @param allowTypeOf Set if `type()` expressions are allowed. Usually disallowed in
///                    class/function definitions.
/// Special case: replace `None` with `NoneType`
/// @throw @c ParserException if a node is not a type (use @c transform instead).
ExprPtr TypecheckVisitor::transformType(ExprPtr &expr, bool allowTypeOf) {
  auto oldTypeOf = ctx->allowTypeOf;
  ctx->allowTypeOf = allowTypeOf;
  if (expr && expr->getNone()) {
    expr = N<IdExpr>(expr->getSrcInfo(), "NoneType");
  }
  transform(expr);
  ctx->allowTypeOf = oldTypeOf;
  if (expr) {
    if (expr->type->isStaticType()) {
      ;
    } else if (expr->type->is("type")) {
      expr->setType(ctx->instantiate(expr->getType()));
    } else if (expr->type->getUnbound() &&
               !expr->type->getUnbound()->genericName.empty()) {
      // generic!
      expr->setType(ctx->instantiate(expr->getType()));
    } else {
      E(Error::EXPECTED_TYPE, expr, "type");
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

  TypecheckVisitor v(ctx, preamble);
  v.setSrcInfo(stmt->getSrcInfo());
  if (!stmt->toString(-1).empty())
    LOG_TYPECHECK("> [{}] [{}:{}] {}", getSrcInfo(), ctx->getBaseName(),
                  ctx->getBase()->iteration, stmt->toString(-1));
  ctx->pushSrcInfo(stmt->getSrcInfo());
  stmt->accept(v);
  ctx->popSrcInfo();
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
  if (!stmt->toString(-1).empty())
    LOG_TYPECHECK("< [{}] [{}:{}] {}", getSrcInfo(), ctx->getBaseName(),
                  ctx->getBase()->iteration, stmt->toString(-1));
  // LOG("[stmt] {}: {} {}", getSrcInfo(), split(stmt->toString(1), '\n').front(),
  // stmt->isDone() ? "[done]" : "");
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
      if (!s->getSuite()) {
        done &= s->isDone();
        stmts.push_back(s);
      } else {
        for (auto &ss : s->getSuite()->stmts) {
          done &= ss->isDone();
          stmts.push_back(ss);
        }
      }
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

void TypecheckVisitor::visit(CustomStmt *stmt) {
  if (stmt->suite) {
    auto fn = ctx->cache->customBlockStmts.find(stmt->keyword);
    seqassert(fn != ctx->cache->customBlockStmts.end(), "unknown keyword {}",
              stmt->keyword);
    resultStmt = fn->second.second(this, stmt);
  } else {
    auto fn = ctx->cache->customExprStmts.find(stmt->keyword);
    seqassert(fn != ctx->cache->customExprStmts.end(), "unknown keyword {}",
              stmt->keyword);
    resultStmt = fn->second(this, stmt);
  }
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
    callArgs.emplace_back("", std::make_shared<NoneExpr>()); // dummy expression
    callArgs.back().value->setType(a);
  }
  auto methods = ctx->findMethod(typ->name, member, false);
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
    callArgs.emplace_back("", a);
  auto methods = ctx->findMethod(typ->name, member, false);
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
    callArgs.emplace_back(n, std::make_shared<NoneExpr>()); // dummy expression
    callArgs.back().value->setType(a);
  }
  auto methods = ctx->findMethod(typ->name, member, false);
  auto m = findMatchingMethods(typ, methods, callArgs);
  return m.empty() ? nullptr : m[0];
}

/// Check if a function can be called with the given arguments.
/// See @c reorderNamedArgs for details.
int TypecheckVisitor::canCall(const types::FuncTypePtr &fn,
                              const std::vector<CallExpr::Arg> &args) {
  std::vector<std::pair<types::TypePtr, size_t>> reordered;
  auto niGenerics = fn->ast->getNonInferrableGenerics();
  auto score = ctx->reorderNamedArgs(
      fn.get(), args,
      [&](int s, int k, const std::vector<std::vector<int>> &slots, bool _) {
        for (int si = 0; si < slots.size(); si++) {
          if (fn->ast->args[si].status == Param::Generic) {
            if (slots[si].empty()) {
              // is this "real" type?
              if (in(niGenerics, fn->ast->args[si].name) &&
                  !fn->ast->args[si].defaultValue)
                return -1;
              reordered.emplace_back(nullptr, 0);
            } else {
              reordered.emplace_back(args[slots[si][0]].value->type, slots[si][0]);
            }
          } else if (si == s || si == k || slots[si].size() != 1) {
            // Ignore *args, *kwargs and default arguments
            reordered.emplace_back(nullptr, 0);
          } else {
            reordered.emplace_back(args[slots[si][0]].value->type, slots[si][0]);
          }
        }
        return 0;
      },
      [](error::Error, const SrcInfo &, const std::string &) { return -1; });
  for (int ai = 0, mai = 0, gi = 0; score != -1 && ai < reordered.size(); ai++) {
    auto expectTyp = fn->ast->args[ai].status == Param::Normal
                         ? fn->getArgTypes()[mai++]
                         : fn->funcGenerics[gi++].type;
    auto [argType, argTypeIdx] = reordered[ai];
    if (!argType)
      continue;
    if (fn->ast->args[ai].status != Param::Normal) {
      // Check if this is a good generic!
      if (expectTyp && expectTyp->isStaticType()) {
        if (!args[argTypeIdx].value->type->isStaticType()) {
          score = -1;
          break;
        } else {
          argType = args[argTypeIdx].value->type;
        }
      } else {
        /// TODO: check if these are real types or if traits are satisfied
        continue;
      }
    }
    // else if (auto c = argType->isStaticType()) {
    //   auto n = c == 1 ? "int" : "str";
    //   argType = ctx->getType(n);
    //   LOG("-> {} {} => {}", fn->debugString(2), argType, argType);
    // }
    ctx->addBlock();
    ExprPtr dummy = std::make_shared<IdExpr>("#");
    dummy->type = argType;
    dummy->setDone();
    ctx->addVar("#", "#", std::make_shared<types::LinkType>(dummy->type));
    try {
      wrapExpr(dummy, expectTyp, fn);
      types::Type::Unification undo;
      if (dummy->type->unify(expectTyp.get(), &undo) >= 0) {
        undo.undo();
      } else {
        // LOG("[call] {:a}: {:c} + {:c} failed", fn, dummy->type, expectTyp);
        score = -1;
      }
    } catch (const exc::ParserException &) {
      // Ignore failed wraps
      score = -1;
    }
    ctx->popBlock();
  }
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
      !callee || !callee->ast->hasAttr("std.internal.attributes.no_argument_wrap.0");
  if (!doArgWrap)
    return true;

  auto doTypeWrap =
      !callee || !callee->ast->hasAttr("std.internal.attributes.no_type_wrap.0");
  if (callee && expr->type->is("type")) {
    auto c = expr->type->getClass();
    if (!c)
      return false;
    if (doTypeWrap) {
      if (c->isRecord())
        expr = transform(N<CallExpr>(expr, N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
      else
        expr = transform(N<CallExpr>(
            N<DotExpr>(N<IdExpr>("__internal__"), "class_ctr"),
            std::vector<CallExpr::Arg>{{"T", expr},
                                       {"", N<EllipsisExpr>(EllipsisExpr::PARTIAL)}}));
    }
  }

  std::unordered_set<std::string> hints = {"Generator", "float", TYPE_OPTIONAL,
                                           "pyobj"};
  if (expr->type->getStatic() && (!expectedType || !expectedType->isStaticType())) {
    expr->type = expr->type->getStatic()->getNonStaticType();
    exprClass = expr->getType()->getClass();
    // return true;
  }
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
    // Special case: Seq functions are embedded (via lambda!)
    seqassert(expr->getId() ||
                  (expr->getStmtExpr() && expr->getStmtExpr()->expr->getId()),
              "bad partial function: {}", expr);
    auto p = partializeFunction(expr->type->getFunc());
    if (auto se = expr->getStmtExpr()) {
      expr = transform(N<StmtExpr>(se->stmts, p));
    } else {
      expr = p;
    }
  } else if (allowUnwrap && exprClass && expr->type->getUnion() && expectedClass &&
             !expectedClass->getUnion()) {
    // Extract union types via __internal__.get_union
    if (auto t = realize(expectedClass)) {
      expr = transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "get_union"),
                                   expr, N<IdExpr>(t->realizedName())));
    } else {
      return false;
    }
  } else if (exprClass && expectedClass && expectedClass->getUnion()) {
    // Make union types via __internal__.new_union
    if (!expectedClass->getUnion()->isSealed())
      expectedClass->getUnion()->addType(exprClass);
    if (auto t = realize(expectedClass)) {
      if (expectedClass->unify(exprClass.get(), nullptr) == -1)
        expr = transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "new_union"),
                                     expr, N<IdExpr>(t->realizedName())));
    } else {
      return false;
    }
  } else if (exprClass && expectedClass && exprClass->name != expectedClass->name) {
    // Cast derived classes to base classes
    const auto &mros = ctx->cache->classes[exprClass->name].mro;
    for (size_t i = 1; i < mros.size(); i++) {
      auto t = ctx->instantiate(mros[i], exprClass);
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
  for (auto &field : ctx->cache->classes[typ->name].fields) {
    for (auto &parentField : ctx->cache->classes[superTyp->name].fields)
      if (field.name == parentField.name) {
        unify(ctx->instantiate(field.type, typ),
              ctx->instantiate(parentField.type, superTyp));
      }
  }
  realize(superTyp);
  auto typExpr = N<IdExpr>(superTyp->realizedName());
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
      ret->emplace_back("", a->getType());
    }
  } else if (auto kw = expr->origExpr->getCall()) { // origExpr?
    auto val = kw->type->getClass();
    if (!val || val->name != "NamedTuple" || val->generics[1].type->getClass() ||
        !val->generics[0].type->canRealize())
      return nullptr;
    auto id = val->generics[0].type->getIntStatic()->value;
    seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}", id);
    auto names = ctx->cache->generatedTupleNames[id];
    auto types = val->generics[1].type->getClass();
    seqassert(startswith(types->name, "Tuple"), "bad NamedTuple argument");
    for (size_t i = 0; i < types->generics.size(); i++) {
      if (!types->generics[i].type)
        return nullptr;
      ret->push_back({names[i], types->generics[i].type});
    }
  } else {
    return nullptr;
  }
  return ret;
}

std::vector<std::pair<std::string, ExprPtr>>
TypecheckVisitor::extractNamedTuple(ExprPtr expr) {
  std::vector<std::pair<std::string, ExprPtr>> ret;

  seqassert(expr->type->is("NamedTuple") &&
                expr->type->getClass()->generics[0].type->canRealize(),
            "bad named tuple: {}", expr);
  auto id = expr->type->getClass()->generics[0].type->getIntStatic()->value;
  seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}", id);
  auto names = ctx->cache->generatedTupleNames[id];
  for (size_t i = 0; i < names.size(); i++) {
    ret.emplace_back(names[i], N<IndexExpr>(N<DotExpr>(expr, "args"), N<IntExpr>(i)));
  }
  return ret;
}

types::TypePtr TypecheckVisitor::getType(const ExprPtr &e) {
  auto t = e->type;
  if (e->isId("type"))
    return t;
  if (auto i = e->getInstantiate())
    if (i->typeExpr->isId("type"))
      return t;

  while (t && t->is("type"))
    t = t->getClass()->generics[0].type;
  return t;
}

} // namespace codon::ast
