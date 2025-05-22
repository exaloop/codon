// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "typecheck.h"

#include <fmt/format.h>
#include <memory>
#include <utility>
#include <vector>

#include "codon/cir/pyextension.h"
#include "codon/cir/util/irtools.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/ctx.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;
using namespace matcher;

/// Simplify an AST node. Load standard library if needed.
/// @param cache     Pointer to the shared cache ( @c Cache )
/// @param file      Filename to be used for error reporting
/// @param barebones Use the bare-bones standard library for faster testing
/// @param defines   User-defined static values (typically passed as `codon run -DX=Y`).
///                  Each value is passed as a string.
Stmt *TypecheckVisitor::apply(
    Cache *cache, Stmt *node, const std::string &file,
    const std::unordered_map<std::string, std::string> &defines,
    const std::unordered_map<std::string, std::string> &earlyDefines, bool barebones) {
  auto preamble = cache->N<SuiteStmt>();
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
  SuiteStmt *suite = tv.N<SuiteStmt>();
  auto &stmts = suite->items;
  stmts.push_back(tv.N<ClassStmt>(".toplevel", std::vector<Param>{}, nullptr,
                                  std::vector<Expr *>{tv.N<IdExpr>("__internal__")}));
  // Load compile-time defines (e.g., codon run -DFOO=1 ...)
  for (auto &d : defines) {
    stmts.push_back(tv.N<AssignStmt>(
        tv.N<IdExpr>(d.first), tv.N<IntExpr>(d.second),
        tv.N<IndexExpr>(tv.N<IdExpr>("Literal"), tv.N<IdExpr>("int"))));
  }
  // Set up __name__
  stmts.push_back(
      tv.N<AssignStmt>(tv.N<IdExpr>("__name__"), tv.N<StringExpr>(MODULE_MAIN)));
  stmts.push_back(node);

  if (auto err = ScopingVisitor::apply(cache, suite))
    throw exc::ParserException(std::move(err));
  auto n = tv.inferTypes(suite, true);
  if (!n) {
    auto errors = tv.findTypecheckErrors(suite);
    throw exc::ParserException(errors);
  }

  suite = tv.N<SuiteStmt>();
  suite->items.push_back(preamble);

  // Add dominated assignment declarations
  suite->items.insert(suite->items.end(), ctx->scope.back().stmts.begin(),
                      ctx->scope.back().stmts.end());
  suite->items.push_back(n);

  if (cast<SuiteStmt>(n))
    tv.prepareVTables();

  if (!ctx->cache->errors.empty())
    throw exc::ParserException(ctx->cache->errors);

  return suite;
}

void TypecheckVisitor::loadStdLibrary(
    Cache *cache, SuiteStmt *preamble,
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
  cache->classes[VAR_CLASS_TOPLEVEL] = Cache::Class();
  auto coreOrErr =
      parseCode(stdlib->cache, stdlibPath->path, "from internal.core import *");
  if (!coreOrErr)
    throw exc::ParserException(coreOrErr.takeError());
  auto *core = *coreOrErr;
  if (auto err = ScopingVisitor::apply(stdlib->cache, core))
    throw exc::ParserException(std::move(err));
  auto tv = TypecheckVisitor(stdlib, preamble);
  core = tv.inferTypes(core, true);
  preamble->addStmt(core);

  // 2. Load early compile-time defines (for standard library)
  for (auto &d : earlyDefines) {
    auto tv = TypecheckVisitor(stdlib, preamble);
    auto s =
        tv.N<AssignStmt>(tv.N<IdExpr>(d.first), tv.N<IntExpr>(d.second),
                         tv.N<IndexExpr>(tv.N<IdExpr>("Literal"), tv.N<IdExpr>("int")));
    auto def = tv.transform(s);
    preamble->addStmt(def);
  }

  // 3. Load stdlib
  auto stdOrErr = parseFile(stdlib->cache, stdlibPath->path);
  if (!stdOrErr)
    throw exc::ParserException(stdOrErr.takeError());
  auto std = *stdOrErr;
  if (auto err = ScopingVisitor::apply(stdlib->cache, std))
    throw exc::ParserException(std::move(err));
  tv = TypecheckVisitor(stdlib, preamble);
  std = tv.inferTypes(std, true);
  preamble->addStmt(std);
  stdlib->isStdlibLoading = false;
}

/// Simplify an AST node. Assumes that the standard library is loaded.
Stmt *TypecheckVisitor::apply(const std::shared_ptr<TypeContext> &ctx, Stmt *node,
                              const std::string &file) {
  auto oldFilename = ctx->getFilename();
  ctx->setFilename(file);
  auto preamble = ctx->cache->N<ast::SuiteStmt>();
  auto tv = TypecheckVisitor(ctx, preamble);
  auto n = tv.inferTypes(node, true);
  ctx->setFilename(oldFilename);
  if (!n) {
    auto errors = tv.findTypecheckErrors(node);
    throw exc::ParserException(errors);
  }
  if (!ctx->cache->errors.empty())
    throw exc::ParserException(ctx->cache->errors);

  auto suite = ctx->cache->N<SuiteStmt>(preamble);
  suite->addStmt(n);
  return suite;
}

/**************************************************************************************/

TypecheckVisitor::TypecheckVisitor(std::shared_ptr<TypeContext> ctx, SuiteStmt *pre,
                                   const std::shared_ptr<std::vector<Stmt *>> &stmts)
    : resultExpr(nullptr), resultStmt(nullptr), ctx(std::move(ctx)) {
  preamble = pre ? pre : this->ctx->cache->N<SuiteStmt>();
  prependStmts = stmts ? stmts : std::make_shared<std::vector<Stmt *>>();
}

/**************************************************************************************/

Expr *TypecheckVisitor::transform(Expr *expr) { return transform(expr, true); }

/// Transform an expression node.
Expr *TypecheckVisitor::transform(Expr *expr, bool allowTypes) {
  if (!expr)
    return nullptr;

  // auto k = typeid(*expr).name();
  // Cache::CTimer t(ctx->cache, k);

  if (!expr->getType())
    expr->setType(instantiateUnbound());

  if (!expr->isDone()) {
    TypecheckVisitor v(ctx, preamble, prependStmts);
    v.setSrcInfo(expr->getSrcInfo());
    ctx->pushNode(expr);
    expr->accept(v);
    ctx->popNode();
    if (v.resultExpr) {
      for (auto it = expr->attributes_begin(); it != expr->attributes_end(); ++it) {
        const auto *attr = expr->getAttribute(*it);
        if (!v.resultExpr->hasAttribute(*it))
          v.resultExpr->setAttribute(*it, attr->clone());
      }
      v.resultExpr->setOrigExpr(expr);
      expr = v.resultExpr;
      if (!expr->getType())
        expr->setType(instantiateUnbound());
    }
    if (!allowTypes && expr && isTypeExpr(expr))
      E(Error::UNEXPECTED_TYPE, expr, "type");
    if (expr->isDone())
      ctx->changedNodes++;
  }
  if (expr) {
    if (!expr->hasAttribute(Attr::ExprDoNotRealize)) {
      if (auto p = realize(expr->getType())) {
        unify(expr->getType(), p);
      }
    }
    LOG_TYPECHECK("[expr] {}: {}{}", getSrcInfo(), *(expr),
                  expr->isDone() ? "[done]" : "");
  }
  return expr;
}

/// Transform a type expression node.
/// Special case: replace `None` with `NoneType`
/// @throw @c ParserException if a node is not a type (use @c transform instead).
Expr *TypecheckVisitor::transformType(Expr *expr) {
  if (cast<NoneExpr>(expr)) {
    auto ne = N<IdExpr>("NoneType");
    ne->setSrcInfo(expr->getSrcInfo());
    expr = ne;
  }
  expr = transform(expr);
  if (expr) {
    if (expr->getType()->isStaticType()) {
      ;
    } else if (isTypeExpr(expr)) {
      expr->setType(instantiateType(expr->getType()));
    } else if (expr->getType()->getUnbound() &&
               !expr->getType()->getUnbound()->genericName.empty()) {
      // generic!
      expr->setType(instantiateType(expr->getType()));
    } else if (expr->getType()->getUnbound() && expr->getType()->getUnbound()->trait) {
      // generic (is type)!
      expr->setType(instantiateType(expr->getType()));
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
Stmt *TypecheckVisitor::transform(Stmt *stmt) {
  if (!stmt || stmt->isDone())
    return stmt;

  // auto _t = std::chrono::high_resolution_clock::now();
  TypecheckVisitor v(ctx, preamble);
  v.setSrcInfo(stmt->getSrcInfo());
  // if (!stmt->toString(-1).empty())
  //   LOG_TYPECHECK("> [{}] [{}:{}] {}", getSrcInfo(), ctx->getBaseName(),
  //                 ctx->getBase()->iteration, stmt->toString(-1));
  ctx->pushNode(stmt);

  int64_t time = 0;
  if (auto a = stmt->getAttribute<ir::IntValueAttribute>(Attr::ExprTime))
    time = a->value;
  auto oldTime = ctx->time;
  ctx->time = time;
  stmt->accept(v);
  ctx->time = oldTime;
  ctx->popNode();
  if (v.resultStmt)
    stmt = v.resultStmt;
  if (!v.prependStmts->empty()) {
    if (stmt)
      v.prependStmts->push_back(stmt);
    bool done = true;
    for (auto &s : *(v.prependStmts))
      done &= s->isDone();
    stmt = N<SuiteStmt>(*v.prependStmts);
    if (done)
      stmt->setDone();
  }
  if (stmt->isDone())
    ctx->changedNodes++;
  // if (!stmt->toString(-1).empty())
  //   LOG_TYPECHECK("< [{}] [{}:{}] {}", getSrcInfo(), ctx->getBaseName(),
  //                 ctx->getBase()->iteration, stmt->toString(-1));
  // if (stmt && !cast<SuiteStmt>(stmt))
  //   LOG("{} | {}: {} | {}",
  //       int64_t(std::chrono::duration_cast<std::chrono::microseconds>(
  //                   std::chrono::high_resolution_clock::now() - _t)
  //                   .count()),
  //       stmt->getSrcInfo(), ctx->getBaseName(), split(stmt->toString(0), '\n')[0]);
  return stmt;
}

void TypecheckVisitor::defaultVisit(Stmt *s) {
  seqassert(false, "unexpected AST node {}", s->toString());
}

/**************************************************************************************/

/// Typecheck statement expressions.
void TypecheckVisitor::visit(StmtExpr *expr) {
  auto done = true;
  for (auto &s : *expr) {
    s = transform(s);
    done &= s->isDone();
  }
  expr->expr = transform(expr->getExpr());
  unify(expr->getType(), expr->getExpr()->getType());
  if (done && expr->getExpr()->isDone())
    expr->setDone();
}

/// Typecheck a list of statements.
void TypecheckVisitor::visit(SuiteStmt *stmt) {
  std::vector<Stmt *> stmts; // for filtering out nullptr statements
  auto done = true;

  std::vector<Stmt *> prepend;
  if (auto b = stmt->getAttribute<BindingsAttribute>(Attr::Bindings)) {
    for (auto &[n, bd] : b->bindings) {
      prepend.push_back(N<AssignStmt>(N<IdExpr>(n), nullptr));
      if (bd.count > 0)
        prepend.push_back(N<AssignStmt>(
            N<IdExpr>(fmt::format("{}{}", n, VAR_USED_SUFFIX)), N<BoolExpr>(false)));
    }
    stmt->eraseAttribute(Attr::Bindings);
  }
  if (!prepend.empty())
    stmt->items.insert(stmt->items.begin(), prepend.begin(), prepend.end());
  for (auto *s : *stmt) {
    if (ctx->returnEarly) {
      // If returnEarly is set (e.g., in the function) ignore the rest
      break;
    }
    if ((s = transform(s))) {
      if (!cast<SuiteStmt>(s)) {
        done &= s->isDone();
        stmts.push_back(s);
      } else {
        for (auto *ss : *cast<SuiteStmt>(s)) {
          if (ss) {
            done &= ss->isDone();
            stmts.push_back(ss);
          }
        }
      }
    }
  }
  stmt->items = stmts;
  if (done)
    stmt->setDone();
}

/// Typecheck expression statements.
void TypecheckVisitor::visit(ExprStmt *stmt) {
  stmt->expr = transform(stmt->getExpr());
  if (stmt->getExpr()->isDone())
    stmt->setDone();
}

void TypecheckVisitor::visit(AwaitStmt *stmt) {
  E(Error::CUSTOM, stmt, "await not yet supported");
}

void TypecheckVisitor::visit(CustomStmt *stmt) {
  if (stmt->getSuite()) {
    auto fn = in(ctx->cache->customBlockStmts, stmt->getKeyword());
    seqassert(fn, "unknown keyword {}", stmt->getKeyword());
    resultStmt = (*fn).second(this, stmt);
  } else {
    auto fn = in(ctx->cache->customExprStmts, stmt->getKeyword());
    seqassert(fn, "unknown keyword {}", stmt->getKeyword());
    resultStmt = (*fn)(this, stmt);
  }
}

void TypecheckVisitor::visit(CommentStmt *stmt) { stmt->setDone(); }

void TypecheckVisitor::visit(DirectiveStmt *stmt) {
  if (stmt->getKey() == "auto_python") {
    ctx->autoPython = stmt->getValue() == "1";
    compilationWarning(
        fmt::format("directive '{}' = {}", stmt->getKey(), ctx->autoPython),
        stmt->getSrcInfo().file, stmt->getSrcInfo().line, stmt->getSrcInfo().col);
  } else {
    compilationWarning(fmt::format("unknown directive '{}'", stmt->getKey()),
                       stmt->getSrcInfo().file, stmt->getSrcInfo().line,
                       stmt->getSrcInfo().col);
  }
  stmt->setDone();
}

/**************************************************************************************/

/// Select the best method indicated of an object that matches the given argument
/// types. See @c findMatchingMethods for details.
types::FuncType *
TypecheckVisitor::findBestMethod(ClassType *typ, const std::string &member,
                                 const std::vector<types::Type *> &args) {
  std::vector<CallArg> callArgs;
  for (auto &a : args) {
    callArgs.emplace_back("", N<NoneExpr>()); // dummy expression
    callArgs.back().value->setType(a->shared_from_this());
  }
  auto methods = findMethod(typ, member, false);
  auto m = findMatchingMethods(typ, methods, callArgs);
  return m.empty() ? nullptr : m[0];
}

/// Select the best method indicated of an object that matches the given argument
/// types. See @c findMatchingMethods for details.
types::FuncType *TypecheckVisitor::findBestMethod(ClassType *typ,
                                                  const std::string &member,
                                                  const std::vector<Expr *> &args) {
  std::vector<CallArg> callArgs;
  for (auto &a : args)
    callArgs.emplace_back("", a);
  auto methods = findMethod(typ, member, false);
  auto m = findMatchingMethods(typ, methods, callArgs);
  return m.empty() ? nullptr : m[0];
}

/// Select the best method indicated of an object that matches the given argument
/// types. See @c findMatchingMethods for details.
types::FuncType *TypecheckVisitor::findBestMethod(
    ClassType *typ, const std::string &member,
    const std::vector<std::pair<std::string, types::Type *>> &args) {
  std::vector<CallArg> callArgs;
  for (auto &[n, a] : args) {
    callArgs.emplace_back(n, N<NoneExpr>()); // dummy expression
    callArgs.back().value->setType(a->shared_from_this());
  }
  auto methods = findMethod(typ, member, false);
  auto m = findMatchingMethods(typ, methods, callArgs);
  return m.empty() ? nullptr : m[0];
}

/// Check if a function can be called with the given arguments.
/// See @c reorderNamedArgs for details.
int TypecheckVisitor::canCall(types::FuncType *fn, const std::vector<CallArg> &args,
                              types::ClassType *part) {
  std::vector<types::Type *> partialArgs;
  if (part && part->getPartial()) {
    auto known = part->getPartialMask();
    auto knownArgTypes = extractClassGeneric(part, 1)->getClass();
    for (size_t i = 0, j = 0, k = 0; i < known.size(); i++)
      if (known[i] == ClassType::PartialFlag::Included) {
        partialArgs.push_back(extractClassGeneric(knownArgTypes, k));
        k++;
      } else if (known[i] == ClassType::PartialFlag::Default) {
        k++;
      }
  }

  std::vector<std::pair<types::Type *, size_t>> reordered;
  auto niGenerics = fn->ast->getNonInferrableGenerics();
  auto score = reorderNamedArgs(
      fn, args,
      [&](int s, int k, const std::vector<std::vector<int>> &slots, bool _) {
        for (int si = 0, gi = 0, pi = 0; si < slots.size(); si++) {
          if ((*fn->ast)[si].isGeneric()) {
            if (slots[si].empty()) {
              // is this "real" type?
              if (in(niGenerics, (*fn->ast)[si].getName()) &&
                  !(*fn->ast)[si].getDefault())
                return -1;
              reordered.emplace_back(nullptr, 0);
            } else {
              seqassert(gi < fn->funcGenerics.size(), "bad fn");
              if (!extractFuncGeneric(fn, gi)->isStaticType() &&
                  !isTypeExpr(args[slots[si][0]]))
                return -1;
              reordered.emplace_back(args[slots[si][0]].getExpr()->getType(),
                                     slots[si][0]);
            }
            gi++;
          } else if (si == s || si == k || slots[si].size() != 1) {
            // Partials
            if (slots[si].empty() && part && part->getPartial() &&
                part->getPartialMask()[si] == ClassType::PartialFlag::Included) {
              reordered.emplace_back(partialArgs[pi++], 0);
            } else {
              // Ignore *args, *kwargs and default arguments
              reordered.emplace_back(nullptr, 0);
            }
          } else {
            reordered.emplace_back(args[slots[si][0]].getExpr()->getType(),
                                   slots[si][0]);
          }
        }
        return 0;
      },
      [](error::Error, const SrcInfo &, const std::string &) { return -1; },
      part && part->getPartial() ? part->getPartialMask() : "");
  int ai = 0, mai = 0, gi = 0, real_gi = 0;
  for (; score != -1 && ai < reordered.size(); ai++) {
    auto expectTyp = (*fn->ast)[ai].isValue() ? extractFuncArgType(fn, mai++)
                                              : extractFuncGeneric(fn, gi++);
    auto [argType, argTypeIdx] = reordered[ai];
    if (!argType)
      continue;
    real_gi += !(*fn->ast)[ai].isValue();
    if (!(*fn->ast)[ai].isValue()) {
      // Check if this is a good generic!
      if (expectTyp && expectTyp->isStaticType()) {
        if (!args[argTypeIdx].getExpr()->getType()->isStaticType()) {
          score = -1;
          break;
        } else {
          argType = args[argTypeIdx].getExpr()->getType();
        }
      } else {
        /// TODO: check if these are real types or if traits are satisfied
        continue;
      }
    }

    auto [_, newArgTyp, __] = canWrapExpr(argType, expectTyp, fn);
    if (!newArgTyp)
      newArgTyp = argType->shared_from_this();
    if (newArgTyp->unify(expectTyp, nullptr) < 0)
      score = -1;
  }
  if (score >= 0)
    score += (real_gi == fn->funcGenerics.size());
  return score;
}

/// Select the best method among the provided methods given the list of arguments.
/// See @c reorderNamedArgs for details.
std::vector<types::FuncType *> TypecheckVisitor::findMatchingMethods(
    types::ClassType *typ, const std::vector<types::FuncType *> &methods,
    const std::vector<CallArg> &args, types::ClassType *part) {
  // Pick the last method that accepts the given arguments.
  std::vector<types::FuncType *> results;
  for (const auto &mi : methods) {
    if (!mi)
      continue; // avoid overloads that have not been seen yet
    auto method = instantiateType(mi, typ);
    int score = canCall(method->getFunc(), args, part);
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
///   expected `Union[T...]`, got `T`     -> `__internal__.new_union(expr,
///   Union[T...])` expected base class, got derived    -> downcast to base class
/// @param allowUnwrap allow optional unwrapping.
bool TypecheckVisitor::wrapExpr(Expr **expr, Type *expectedType, FuncType *callee,
                                bool allowUnwrap) {
  auto [canWrap, newArgTyp, fn] = canWrapExpr((*expr)->getType(), expectedType, callee,
                                              allowUnwrap, cast<EllipsisExpr>(*expr));
  // TODO: get rid of this line one day!
  if ((*expr)->getType()->isStaticType() &&
      (!expectedType || !expectedType->isStaticType()))
    (*expr)->setType(getUnderlyingStaticType((*expr)->getType())->shared_from_this());
  if (canWrap && fn) {
    *expr = transform(fn(*expr));
  }
  return canWrap;
}

std::tuple<bool, TypePtr, std::function<Expr *(Expr *)>>
TypecheckVisitor::canWrapExpr(Type *exprType, Type *expectedType, FuncType *callee,
                              bool allowUnwrap, bool isEllipsis) {
  auto expectedClass = expectedType->getClass();
  auto exprClass = exprType->getClass();
  auto doArgWrap = !callee || !callee->ast->hasFunctionAttribute(
                                  "std.internal.attributes.no_argument_wrap.0:0");
  if (!doArgWrap)
    return {true, expectedType ? expectedType->shared_from_this() : nullptr, nullptr};

  TypePtr type = nullptr;
  std::function<Expr *(Expr *)> fn = nullptr;

  if (callee && exprType->is(TYPE_TYPE)) {
    auto c = extractClassType(exprType);
    if (!c)
      return {false, nullptr, nullptr};
    if (!(expectedType && (expectedType->is(TYPE_TYPE)))) {
      type = instantiateType(getStdLibType("TypeWrap"), std::vector<types::Type *>{c});
      fn = [&](Expr *expr) -> Expr * {
        return N<CallExpr>(N<IdExpr>("TypeWrap"), expr);
      };
    }
    return {true, type, fn};
  }

  std::unordered_set<std::string> hints = {"Generator", "float", TYPE_OPTIONAL,
                                           "pyobj"};
  if (!expectedType || !expectedType->isStaticType()) {
    if (auto c = exprType->isStaticType()) {
      exprType = getUnderlyingStaticType(exprType);
      exprClass = exprType->getClass();
      type = exprType->shared_from_this();
    }
  }

  if (!exprClass && expectedClass && in(hints, expectedClass->name)) {
    return {false, nullptr, nullptr}; // argument type not yet known.
  }

  else if (expectedClass && !expectedClass->is("Capsule") && exprClass &&
           exprClass->is("Capsule")) {
    type = extractClassGeneric(exprClass)->shared_from_this();
    fn = [&](Expr *expr) -> Expr * {
      return N<CallExpr>(N<IdExpr>("__internal__.capsule_get"), expr);
    };
  } else if (expectedClass && expectedClass->is("Capsule") && exprClass &&
             !exprClass->is("Capsule")) {
    type = instantiateType(getStdLibType("Capsule"), std::vector<Type *>{exprClass});
    fn = [&](Expr *expr) -> Expr * {
      return N<CallExpr>(N<IdExpr>("__internal__.capsule_make"), expr);
    };
  }

  else if (expectedClass && expectedClass->is("Generator") &&
           !exprClass->is(expectedClass->name) && !isEllipsis) {
    if (findMethod(exprClass, "__iter__").empty())
      return {false, nullptr, nullptr};
    // Note: do not do this in pipelines (TODO: why?)
    type = instantiateType(expectedClass);
    fn = [&](Expr *expr) -> Expr * {
      return N<CallExpr>(N<DotExpr>(expr, "__iter__"));
    };
  }

  else if (expectedClass && expectedClass->is("float") && exprClass->is("int")) {
    type = instantiateType(expectedClass);
    fn = [&](Expr *expr) -> Expr * { return N<CallExpr>(N<IdExpr>("float"), expr); };
  }

  else if (expectedClass && expectedClass->is("bool") && exprClass &&
           !exprClass->is("bool")) {
    type = instantiateType(expectedClass);
    fn = [&](Expr *expr) -> Expr * {
      return N<CallExpr>(N<DotExpr>(expr, "__bool__"));
    };
  }

  else if (expectedClass && expectedClass->is(TYPE_OPTIONAL) && exprClass &&
           !exprClass->is(expectedClass->name)) {
    type =
        instantiateType(getStdLibType(TYPE_OPTIONAL), std::vector<Type *>{exprClass});
    fn = [&](Expr *expr) -> Expr * {
      return N<CallExpr>(N<IdExpr>(TYPE_OPTIONAL), expr);
    };
  } else if (allowUnwrap && expectedClass && exprClass &&
             exprClass->is(TYPE_OPTIONAL) &&
             !exprClass->is(expectedClass->name)) { // unwrap optional
    type = instantiateType(extractClassGeneric(exprClass));
    fn = [&](Expr *expr) -> Expr * { return N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr); };
  }

  else if (expectedClass && expectedClass->is("pyobj") &&
           !exprClass->is(expectedClass->name)) { // wrap to pyobj
    if (findMethod(exprClass, "__to_py__").empty())
      return {false, nullptr, nullptr};
    type = instantiateType(expectedClass);
    fn = [&](Expr *expr) -> Expr * {
      return N<CallExpr>(N<IdExpr>("pyobj"),
                         N<CallExpr>(N<DotExpr>(expr, "__to_py__")));
    };
  }

  else if (allowUnwrap && expectedClass && exprClass && exprClass->is("pyobj") &&
           !exprClass->is(expectedClass->name)) { // unwrap pyobj
    if (findMethod(expectedClass, "__from_py__").empty())
      return {false, nullptr, nullptr};
    type = instantiateType(expectedClass);
    auto texpr = N<IdExpr>(expectedClass->name);
    texpr->setType(expectedType->shared_from_this());
    fn = [this, texpr](Expr *expr) -> Expr * {
      return N<CallExpr>(N<DotExpr>(texpr, "__from_py__"), N<DotExpr>(expr, "p"));
    };
  }

  else if (expectedClass && expectedClass->is(TYPE_CALLABLE) && exprClass &&
           (exprClass->getPartial() || exprClass->getFunc() ||
            exprClass->is(TYPE_FUNCTION))) {
    // Get list of arguments
    std::vector<Type *> argTypes;
    Type *retType;
    std::shared_ptr<FuncType> fnType = nullptr;
    if (!exprClass->getPartial()) {
      auto targs = extractClassGeneric(exprClass)->getClass();
      for (size_t i = 0; i < targs->size(); i++)
        argTypes.push_back((*targs)[i]);
      retType = extractClassGeneric(exprClass, 1);
    } else {
      fnType = instantiateType(exprClass->getPartial()->getPartialFunc());
      std::vector<types::Type *> argumentTypes;
      auto known = exprClass->getPartial()->getPartialMask();
      for (size_t i = 0; i < known.size(); i++) {
        if (known[i] != ClassType::PartialFlag::Included)
          argTypes.push_back((*fnType)[i]);
      }
      retType = fnType->getRetType();
    }
    auto expectedArgs = extractClassGeneric(expectedClass)->getClass();
    if (argTypes.size() != expectedArgs->size())
      return {false, nullptr, nullptr};
    for (size_t i = 0; i < argTypes.size(); i++) {
      if (argTypes[i]->unify((*expectedArgs)[i], nullptr) < 0)
        return {false, nullptr, nullptr};
    }
    if (retType->unify(extractClassGeneric(expectedClass, 1), nullptr) < 0)
      return {false, nullptr, nullptr};

    type = expectedType->shared_from_this();
    fn = [this, type](Expr *expr) -> Expr * {
      auto exprClass = expr->getType()->getClass();
      auto expectedClass = type->getClass();

      std::vector<Type *> argTypes;
      Type *retType;
      std::shared_ptr<FuncType> fnType = nullptr;
      if (!exprClass->getPartial()) {
        auto targs = extractClassGeneric(exprClass)->getClass();
        for (size_t i = 0; i < targs->size(); i++)
          argTypes.push_back((*targs)[i]);
        retType = extractClassGeneric(exprClass, 1);
      } else {
        fnType = instantiateType(exprClass->getPartial()->getPartialFunc());
        std::vector<types::Type *> argumentTypes;
        auto known = exprClass->getPartial()->getPartialMask();
        for (size_t i = 0; i < known.size(); i++) {
          if (known[i] != ClassType::PartialFlag::Included)
            argTypes.push_back((*fnType)[i]);
        }
        retType = fnType->getRetType();
      }
      auto expectedArgs = extractClassGeneric(expectedClass)->getClass();
      for (size_t i = 0; i < argTypes.size(); i++)
        unify(argTypes[i], (*expectedArgs)[i]);
      auto tr = unify(retType, extractClassGeneric(expectedClass, 1));

      std::string fname;
      Expr *retFn = nullptr, *dataArg = nullptr, *dataType = nullptr;
      if (exprClass->getPartial()) {
        auto rf = realize(exprClass);
        fname = rf->realizedName();
        seqassert(rf, "not realizable");
        retFn = N<IndexExpr>(
            N<CallExpr>(N<IndexExpr>(N<IdExpr>("Ptr"), N<IdExpr>(rf->realizedName())),
                        N<IdExpr>("data")),
            N<IntExpr>(0));
        dataType = N<IdExpr>("cobj");
      } else if (exprClass->getFunc()) {
        auto rf = realize(exprClass);
        seqassert(rf, "not realizable");
        fname = rf->realizedName();
        retFn = N<IdExpr>(rf->getFunc()->realizedName());
        dataArg = N<CallExpr>(N<IdExpr>("cobj"));
        dataType = N<IdExpr>("cobj");
      } else {
        seqassert(exprClass->is("Function"), "bad type: {}", exprClass->debugString(2));
        auto rf = realize(exprClass);
        seqassert(rf, "not realizable");
        fname = rf->realizedName();
        retFn = N<CallExpr>(N<IdExpr>(rf->realizedName()), N<IdExpr>("data"));
        dataType = N<IdExpr>("cobj");
      }
      fname = fmt::format(".proxy.{}", fname);

      if (!ctx->find(fname)) {
        // Create wrapper if needed
        auto f = N<FunctionStmt>(
            fname, nullptr,
            std::vector<Param>{
                Param{"data", dataType},
                Param{"args", N<IdExpr>(expectedArgs->realizedName())}}, // Tuple[...]
            N<SuiteStmt>(
                N<ReturnStmt>(N<CallExpr>(retFn, N<StarExpr>(N<IdExpr>("args"))))));
        f = cast<FunctionStmt>(transform(f));
      }
      auto e = N<CallExpr>(N<IdExpr>(TYPE_CALLABLE), N<IdExpr>(fname),
                           dataArg ? dataArg : expr);
      return e;
    };
  } else if (callee && exprClass && exprType->getFunc() &&
             !(expectedClass && expectedClass->is("Function"))) {
    // Wrap raw Seq functions into Partial(...) call for easy realization.
    // Special case: Seq functions are embedded (via lambda!)
    if (expectedClass)
      type = instantiateType(expectedClass);
    auto fnName = exprType->getFunc()->ast->getName();
    fn = [&, fnName](Expr *expr) -> Expr * {
      auto p = N<CallExpr>(N<IdExpr>(fnName), N<EllipsisExpr>(EllipsisExpr::PARTIAL));
      if (auto se = cast<StmtExpr>(expr))
        return N<StmtExpr>(se->items, p);
      return p;
    };
  } else if (expectedClass && expectedClass->is("Function") && exprClass &&
             exprClass->getPartial() && exprClass->getPartial()->isPartialEmpty()) {
    type = instantiateType(expectedClass);
    auto fnName = exprClass->getPartial()->getPartialFunc()->ast->name;
    auto t = instantiateType(ctx->forceFind(fnName)->getType());
    if (type->unify(t.get(), nullptr) >= 0)
      fn = [&](Expr *expr) -> Expr * { return N<IdExpr>(fnName); };
    else
      type = nullptr;
  }

  else if (allowUnwrap && exprClass && exprType->getUnion() && expectedClass &&
           !expectedClass->getUnion()) {
    // Extract union types via __internal__.get_union
    if (auto t = realize(expectedClass)) {
      auto e = realize(exprType);
      if (!e)
        return {false, nullptr, nullptr};
      bool ok = false;
      for (auto &ut : e->getUnion()->getRealizationTypes()) {
        if (ut->unify(t, nullptr) >= 0) {
          ok = true;
          break;
        }
      }
      if (ok) {
        type = t->shared_from_this();
        fn = [this, type](Expr *expr) -> Expr * {
          return N<CallExpr>(N<IdExpr>("__internal__.get_union:0"), expr,
                             N<IdExpr>(type->realizedName()));
        };
      }
    } else {
      return {false, nullptr, nullptr};
    }
  }

  else if (exprClass && expectedClass && expectedClass->getUnion()) {
    // Make union types via __internal__.new_union
    if (!expectedClass->getUnion()->isSealed()) {
      if (!expectedClass->getUnion()->addType(exprClass))
        E(error::Error::UNION_TOO_BIG, expectedClass->getSrcInfo(),
          expectedClass->getUnion()->pendingTypes.size());
    }
    if (auto t = realize(expectedClass)) {
      if (expectedClass->unify(exprClass, nullptr) == -1) {
        type = t->shared_from_this();
        fn = [this, type](Expr *expr) -> Expr * {
          return N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "new_union"), expr,
                             N<IdExpr>(type->realizedName()));
        };
      }
    } else {
      return {false, nullptr, nullptr};
    }
  }

  else if (exprClass && exprClass->is(TYPE_TYPE) && expectedClass &&
           (expectedClass->is("TypeWrap"))) {
    type = instantiateType(getStdLibType("TypeWrap"),
                           std::vector<types::Type *>{exprClass});
    fn = [this](Expr *expr) -> Expr * {
      return N<CallExpr>(N<IdExpr>("TypeWrap"), expr);
    };
  }

  else if (exprClass && expectedClass && !exprClass->is(expectedClass->name)) {
    // Cast derived classes to base classes
    const auto &mros = ctx->cache->getClass(exprClass)->mro;
    for (size_t i = 1; i < mros.size(); i++) {
      auto t = instantiateType(mros[i].get(), exprClass);
      if (t->unify(expectedClass, nullptr) >= 0) {
        type = expectedClass->shared_from_this();
        fn = [this, type](Expr *expr) -> Expr * {
          return castToSuperClass(expr, type->getClass(), true);
        };
        break;
      }
    }
  }

  return {true, type, fn};
}

/// Cast derived class to a base class.
Expr *TypecheckVisitor::castToSuperClass(Expr *expr, ClassType *superTyp,
                                         bool isVirtual) {
  ClassType *typ = expr->getClassType();
  for (auto &field : getClassFields(typ)) {
    for (auto &parentField : getClassFields(superTyp))
      if (field.name == parentField.name) {
        auto t = instantiateType(field.getType(), typ);
        unify(t.get(), instantiateType(parentField.getType(), superTyp));
      }
  }
  realize(superTyp);
  auto typExpr = N<IdExpr>(superTyp->realizedName());
  return transform(
      N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_super"), expr, typExpr));
}

/// Unpack a Tuple or KwTuple expression into (name, type) vector.
/// Name is empty when handling Tuple; otherwise it matches names of KwTuple.
std::shared_ptr<std::vector<std::pair<std::string, types::Type *>>>
TypecheckVisitor::unpackTupleTypes(Expr *expr) {
  auto ret = std::make_shared<std::vector<std::pair<std::string, types::Type *>>>();
  if (auto tup = cast<TupleExpr>(expr->getOrigExpr())) {
    for (auto &a : *tup) {
      a = transform(a);
      if (!a->getClassType())
        return nullptr;
      ret->emplace_back("", a->getType());
    }
  } else if (auto kw = cast<CallExpr>(expr->getOrigExpr())) {
    auto val = extractClassType(expr->getType());
    if (!val || !val->is("NamedTuple") || !extractClassGeneric(val, 1)->getClass() ||
        !extractClassGeneric(val)->canRealize())
      return nullptr;
    auto id = getIntLiteral(val);
    seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}", id);
    auto names = ctx->cache->generatedTupleNames[id];
    auto types = extractClassGeneric(val, 1)->getClass();
    seqassert(startswith(types->name, "Tuple"), "bad NamedTuple argument");
    for (size_t i = 0; i < types->generics.size(); i++) {
      if (!extractClassGeneric(types, i))
        return nullptr;
      ret->emplace_back(names[i], extractClassGeneric(types, i));
    }
  } else {
    return nullptr;
  }
  return ret;
}

std::vector<std::pair<std::string, Expr *>>
TypecheckVisitor::extractNamedTuple(Expr *expr) {
  std::vector<std::pair<std::string, Expr *>> ret;

  seqassert(expr->getType()->is("NamedTuple") &&
                extractClassGeneric(expr->getClassType())->canRealize(),
            "bad named tuple: {}", *expr);
  auto id = getIntLiteral(expr->getClassType());
  seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}", id);
  auto names = ctx->cache->generatedTupleNames[id];
  for (size_t i = 0; i < names.size(); i++) {
    ret.emplace_back(names[i], N<IndexExpr>(N<DotExpr>(expr, "args"), N<IntExpr>(i)));
  }
  return ret;
}

std::vector<Cache::Class::ClassField>
TypecheckVisitor::getClassFields(types::ClassType *t) const {
  auto f = getClass(t->name)->fields;
  if (t->is(TYPE_TUPLE))
    f = std::vector<Cache::Class::ClassField>(f.begin(),
                                              f.begin() + t->generics.size());
  return f;
}

std::vector<types::TypePtr>
TypecheckVisitor::getClassFieldTypes(types::ClassType *cls) {
  return withClassGenerics(cls, [&]() {
    std::vector<types::TypePtr> result;
    for (auto &field : getClassFields(cls)) {
      auto ftyp = instantiateType(field.getType(), cls);
      if (!ftyp->canRealize() && field.typeExpr) {
        auto t = extractType(transform(clean_clone(field.typeExpr)));
        unify(ftyp.get(), t);
      }
      result.push_back(ftyp);
    }
    return result;
  });
}

types::Type *TypecheckVisitor::extractType(types::Type *t) {
  while (t && t->is(TYPE_TYPE))
    t = extractClassGeneric(t);
  return t;
}

types::Type *TypecheckVisitor::extractType(Expr *e) {
  if (cast<IdExpr>(e) && cast<IdExpr>(e)->getValue() == TYPE_TYPE)
    return e->getType();
  if (auto i = cast<InstantiateExpr>(e))
    if (cast<IdExpr>(i->getExpr()) &&
        cast<IdExpr>(i->getExpr())->getValue() == TYPE_TYPE)
      return e->getType();
  return extractType(e->getType());
}

types::Type *TypecheckVisitor::extractType(const std::string &s) {
  auto c = ctx->forceFind(s);
  return s == TYPE_TYPE ? c->getType() : extractType(c->getType());
}

types::ClassType *TypecheckVisitor::extractClassType(Expr *e) {
  auto t = extractType(e);
  return t->getClass();
}

types::ClassType *TypecheckVisitor::extractClassType(types::Type *t) {
  return extractType(t)->getClass();
}

types::ClassType *TypecheckVisitor::extractClassType(const std::string &s) {
  return extractType(s)->getClass();
}

bool TypecheckVisitor::isUnbound(types::Type *t) const {
  return t->getUnbound() != nullptr;
}

bool TypecheckVisitor::isUnbound(Expr *e) const { return isUnbound(e->getType()); }

bool TypecheckVisitor::hasOverloads(const std::string &root) {
  auto i = in(ctx->cache->overloads, root);
  return i && i->size() > 1;
}

std::vector<std::string> TypecheckVisitor::getOverloads(const std::string &root) {
  auto i = in(ctx->cache->overloads, root);
  seqassert(i, "bad root");
  return *i;
}

std::string TypecheckVisitor::getUnmangledName(const std::string &s) const {
  return ctx->cache->rev(s);
}

Cache::Class *TypecheckVisitor::getClass(const std::string &t) const {
  auto i = in(ctx->cache->classes, t);
  return i;
}

Cache::Class *TypecheckVisitor::getClass(types::Type *t) const {
  if (t) {
    if (auto c = t->getClass())
      return getClass(c->name);
  }
  seqassert(false, "bad class");
  return nullptr;
}

Cache::Function *TypecheckVisitor::getFunction(const std::string &n) const {
  auto i = in(ctx->cache->functions, n);
  return i;
}

Cache::Function *TypecheckVisitor::getFunction(types::Type *t) const {
  seqassert(t->getFunc(), "bad function");
  return getFunction(t->getFunc()->getFuncName());
}

Cache::Class::ClassRealization *
TypecheckVisitor::getClassRealization(types::Type *t) const {
  seqassert(t->canRealize(), "bad class");
  auto i = in(getClass(t)->realizations, t->getClass()->realizedName());
  seqassert(i, "bad class realization");
  return i->get();
}

std::string TypecheckVisitor::getRootName(types::FuncType *t) {
  auto i = in(ctx->cache->functions, t->getFuncName());
  seqassert(i && !i->rootName.empty(), "bad function");
  return i->rootName;
}

bool TypecheckVisitor::isTypeExpr(Expr *e) {
  return e && e->getType() && e->getType()->is(TYPE_TYPE);
}

Cache::Module *TypecheckVisitor::getImport(const std::string &s) {
  auto i = in(ctx->cache->imports, s);
  seqassert(i, "bad import");
  return i;
}

std::string TypecheckVisitor::getArgv() const { return ctx->cache->argv0; }

std::string TypecheckVisitor::getRootModulePath() const { return ctx->cache->module0; }

std::vector<std::string> TypecheckVisitor::getPluginImportPaths() const {
  return ctx->cache->pluginImportPaths;
}

bool TypecheckVisitor::isDispatch(const std::string &s) {
  return endswith(s, FN_DISPATCH_SUFFIX);
}

bool TypecheckVisitor::isDispatch(FunctionStmt *ast) {
  return ast && isDispatch(ast->name);
}

bool TypecheckVisitor::isDispatch(types::Type *f) {
  return f->getFunc() && isDispatch(f->getFunc()->ast);
}

void TypecheckVisitor::addClassGenerics(types::ClassType *typ, bool func,
                                        bool onlyMangled, bool instantiate) {
  auto addGen = [&](const types::ClassType::Generic &g) {
    auto t = g.type;
    if (instantiate) {
      if (auto l = t->getLink())
        if (l->kind == types::LinkType::Generic) {
          auto lx = std::make_shared<types::LinkType>(*l);
          lx->kind = types::LinkType::Unbound;
          t = lx;
        }
    }
    seqassert(!g.isStatic || t->isStaticType(), "{} not a static: {}", g.name,
              *(g.type));
    if (!g.isStatic && !t->is(TYPE_TYPE))
      t = instantiateTypeVar(t.get());
    auto n = onlyMangled ? g.name : getUnmangledName(g.name);
    auto v = ctx->addType(n, g.name, t);
    v->generic = true;
  };

  if (func && typ->getFunc()) {
    auto tf = typ->getFunc();
    // LOG("// adding {}", tf->debugString(2));
    for (auto parent = tf->funcParent; parent;) {
      if (auto f = parent->getFunc()) {
        // Add parent function generics
        for (auto &g : f->funcGenerics)
          addGen(g);
        parent = f->funcParent;
      } else {
        // Add parent class generics
        seqassert(parent->getClass(), "not a class: {}", *parent);
        for (auto &g : parent->getClass()->generics)
          addGen(g);
        for (auto &g : parent->getClass()->hiddenGenerics)
          addGen(g);
        break;
      }
    }
    for (auto &g : tf->funcGenerics)
      addGen(g);
  } else {
    for (auto &g : typ->hiddenGenerics)
      addGen(g);
    for (auto &g : typ->generics)
      addGen(g);
  }
}

types::TypePtr TypecheckVisitor::instantiateTypeVar(types::Type *t) {
  return instantiateType(ctx->forceFind(TYPE_TYPE)->getType(), {t});
}

void TypecheckVisitor::registerGlobal(const std::string &name, bool initialized) {
  if (!in(ctx->cache->globals, name)) {
    ctx->cache->globals[name] = {initialized, nullptr};
  }
}

types::ClassType *TypecheckVisitor::getStdLibType(const std::string &type) {
  auto t = getImport(STDLIB_IMPORT)->ctx->forceFind(type)->getType();
  if (type == TYPE_TYPE)
    return t->getClass();
  return extractClassType(t);
}

types::Type *TypecheckVisitor::extractClassGeneric(types::Type *t, int idx) const {
  seqassert(t->getClass() && idx < t->getClass()->generics.size(), "bad class");
  return t->getClass()->generics[idx].type.get();
}

types::Type *TypecheckVisitor::extractFuncGeneric(types::Type *t, int idx) const {
  seqassert(t->getFunc() && idx < t->getFunc()->funcGenerics.size(), "bad function");
  return t->getFunc()->funcGenerics[idx].type.get();
}

types::Type *TypecheckVisitor::extractFuncArgType(types::Type *t, int idx) {
  seqassert(t->getFunc(), "bad function");
  return extractClassGeneric(extractClassGeneric(t), idx);
}

std::string TypecheckVisitor::getClassMethod(types::Type *typ,
                                             const std::string &member) {
  if (auto cls = getClass(typ)) {
    if (auto t = in(cls->methods, member))
      return *t;
  }
  seqassertn(false, "cannot find '{}' in '{}'", member, *typ);
  return "";
}

std::string TypecheckVisitor::getTemporaryVar(const std::string &s) {
  return ctx->cache->getTemporaryVar(s);
}

std::string TypecheckVisitor::getStrLiteral(types::Type *t, size_t pos) {
  seqassert(t && t->getClass(), "not a class");
  if (t->getStrStatic())
    return t->getStrStatic()->value;
  auto ct = extractClassGeneric(t, pos);
  seqassert(ct->canRealize() && ct->getStrStatic(), "not a string literal");
  return ct->getStrStatic()->value;
}

int64_t TypecheckVisitor::getIntLiteral(types::Type *t, size_t pos) {
  seqassert(t && t->getClass(), "not a class");
  if (t->getIntStatic())
    return t->getIntStatic()->value;
  auto ct = extractClassGeneric(t, pos);
  seqassert(ct->canRealize() && ct->getIntStatic(), "not a string literal");
  return ct->getIntStatic()->value;
}

bool TypecheckVisitor::getBoolLiteral(types::Type *t, size_t pos) {
  seqassert(t && t->getClass(), "not a class");
  if (t->getBoolStatic())
    return t->getBoolStatic()->value;
  auto ct = extractClassGeneric(t, pos);
  seqassert(ct->canRealize() && ct->getBoolStatic(), "not a bool literal");
  return ct->getBoolStatic()->value;
}

bool TypecheckVisitor::isImportFn(const std::string &s) {
  return startswith(s, "%_import_");
}

int64_t TypecheckVisitor::getTime() { return ctx->time; }

types::Type *TypecheckVisitor::getUnderlyingStaticType(types::Type *t) {
  if (t->getStatic()) {
    return t->getStatic()->getNonStaticType();
  } else if (auto c = t->isStaticType()) {
    if (c == 1)
      return getStdLibType("int");
    if (c == 2)
      return getStdLibType("str");
    if (c == 3)
      return getStdLibType("bool");
  }
  return t;
}

std::shared_ptr<types::LinkType>
TypecheckVisitor::instantiateUnbound(const SrcInfo &srcInfo, int level) const {
  auto typ = std::make_shared<types::LinkType>(
      ctx->cache, types::LinkType::Unbound, ctx->cache->unboundCount++, level, nullptr);
  typ->setSrcInfo(srcInfo);
  return typ;
}

std::shared_ptr<types::LinkType>
TypecheckVisitor::instantiateUnbound(const SrcInfo &srcInfo) const {
  return instantiateUnbound(srcInfo, ctx->typecheckLevel);
}

std::shared_ptr<types::LinkType> TypecheckVisitor::instantiateUnbound() const {
  return instantiateUnbound(getSrcInfo(), ctx->typecheckLevel);
}

types::TypePtr TypecheckVisitor::instantiateType(const SrcInfo &srcInfo,
                                                 types::Type *type,
                                                 types::ClassType *generics) {
  seqassert(type, "type is null");
  std::unordered_map<int, types::TypePtr> genericCache;
  if (generics) {
    for (auto &g : generics->generics)
      if (g.type &&
          !(g.type->getLink() && g.type->getLink()->kind == types::LinkType::Generic)) {
        genericCache[g.id] = g.type;
      }
    // special case: __SELF__
    if (type->getFunc() && !type->getFunc()->funcGenerics.empty() &&
        type->getFunc()->funcGenerics[0].niceName == "__SELF__") {
      genericCache[type->getFunc()->funcGenerics[0].id] = generics->shared_from_this();
    }
  }
  auto t = type->instantiate(ctx->typecheckLevel, &(ctx->cache->unboundCount),
                             &genericCache);
  for (auto &i : genericCache) {
    if (auto l = i.second->getLink()) {
      i.second->setSrcInfo(srcInfo);
      if (l->defaultType) {
        ctx->getBase()->pendingDefaults[0].insert(i.second);
      }
    }
  }
  if (auto ft = t->getFunc()) {
    if (auto b = ft->ast->getAttribute<BindingsAttribute>(Attr::Bindings)) {
      auto module =
          ft->ast->getAttribute<ir::StringValueAttribute>(Attr::Module)->value;
      auto imp = getImport(module);
      std::unordered_map<std::string, std::string> key;

      // look for generics [todo: speed-up!]
      std::unordered_set<std::string> generics;
      for (auto &g : ft->funcGenerics)
        generics.insert(getUnmangledName(g.name));
      for (auto parent = ft->funcParent; parent;) {
        if (auto f = parent->getFunc()) {
          for (auto &g : f->funcGenerics)
            generics.insert(getUnmangledName(g.name));
          parent = f->funcParent;
        } else {
          for (auto &g : parent->getClass()->generics)
            generics.insert(getUnmangledName(g.name));
          for (auto &g : parent->getClass()->hiddenGenerics)
            generics.insert(getUnmangledName(g.name));
          break;
        }
      }
      for (const auto &[c, _] : b->captures) {
        if (!in(generics, c)) { // ignore inherited captures!
          if (auto h = imp->ctx->find(c)) {
            key[c] = h->canonicalName;
          } else {
            key.clear();
            break;
          }
        }
      }
      if (!key.empty()) {
        auto &cm = getFunction(ft->getFuncName())->captureMappings;
        size_t idx = 0;
        for (; idx < cm.size(); idx++)
          if (cm[idx] == key)
            break;
        if (idx == cm.size())
          cm.push_back(key);
        ft->index = idx;
      }
    }
  }
  if (t->getUnion() && !t->getUnion()->isSealed()) {
    t->setSrcInfo(srcInfo);
    ctx->getBase()->pendingDefaults[0].insert(t);
  }
  return t;
}

types::TypePtr
TypecheckVisitor::instantiateType(const SrcInfo &srcInfo, types::Type *root,
                                  const std::vector<types::Type *> &generics) {
  auto c = root->getClass();
  seqassert(c, "root class is null");
  // dummy generic type
  auto g = std::make_shared<types::ClassType>(ctx->cache, "", "");
  if (generics.size() != c->generics.size()) {
    E(Error::GENERICS_MISMATCH, srcInfo, getUnmangledName(c->name), c->generics.size(),
      generics.size());
  }
  for (int i = 0; i < c->generics.size(); i++) {
    auto t = generics[i];
    seqassert(c->generics[i].type, "generic is null");
    if (!c->generics[i].isStatic && t->getStatic())
      t = t->getStatic()->getNonStaticType();
    g->generics.emplace_back("", "", t->shared_from_this(), c->generics[i].id,
                             c->generics[i].isStatic);
  }
  return instantiateType(srcInfo, root, g.get());
}

std::vector<types::FuncType *> TypecheckVisitor::findMethod(types::ClassType *type,
                                                            const std::string &method,
                                                            bool hideShadowed) {
  std::vector<types::FuncType *> vv;
  std::unordered_set<std::string> signatureLoci;

  auto populate = [&](const auto &cls) {
    auto t = in(cls.methods, method);
    if (!t)
      return;

    auto mt = getOverloads(*t);
    for (int mti = int(mt.size()) - 1; mti >= 0; mti--) {
      auto method = mt[mti];
      auto f = getFunction(method);
      if (isDispatch(method) || !f->getType())
        continue;
      if (hideShadowed) {
        auto sig = f->ast->getSignature();
        if (!in(signatureLoci, sig)) {
          signatureLoci.insert(sig);
          vv.emplace_back(f->getType());
        }
      } else {
        vv.emplace_back(f->getType());
      }
    }
  };
  if (type->is("Capsule")) {
    type = extractClassGeneric(type)->getClass();
  }
  if (type && type->is(TYPE_TUPLE) && method == "__new__" && !type->generics.empty()) {
    generateTuple(type->generics.size());
    auto mc = getClass(TYPE_TUPLE);
    populate(*mc);
    for (auto f : vv)
      if (f->size() == type->generics.size())
        return {f};
    return {};
  }
  if (auto cls = getClass(type)) {
    for (const auto &pc : cls->mro) {
      auto mc = getClass(pc->name == "__NTuple__" ? TYPE_TUPLE : pc->name);
      populate(*mc);
    }
  }
  return vv;
}

Cache::Class::ClassField *
TypecheckVisitor::findMember(types::ClassType *type, const std::string &member) const {
  if (type->is("Capsule")) {
    type = extractClassGeneric(type)->getClass();
  }
  if (auto cls = getClass(type)) {
    for (const auto &pc : cls->mro) {
      auto mc = getClass(pc.get());
      for (auto &mm : mc->fields) {
        if (pc->is(TYPE_TUPLE) && (&mm - &(mc->fields[0])) >= type->generics.size())
          break;
        if (mm.name == member)
          return &mm;
      }
    }
  }
  return nullptr;
}

int TypecheckVisitor::reorderNamedArgs(types::FuncType *func,
                                       const std::vector<CallArg> &args,
                                       const ReorderDoneFn &onDone,
                                       const ReorderErrorFn &onError,
                                       const std::string &known) {
  // See https://docs.python.org/3.6/reference/expressions.html#calls for details.
  // Final score:
  //  - +1 for each matched argument
  //  -  0 for *args/**kwargs/default arguments
  //  - -1 for failed match
  int score = 0;

  // 0. Find *args and **kwargs
  // True if there is a trailing ellipsis (full partial: fn(all_args, ...))
  bool partial =
      !args.empty() && cast<EllipsisExpr>(args.back().value) &&
      cast<EllipsisExpr>(args.back().value)->getMode() != EllipsisExpr::PIPE &&
      args.back().name.empty();

  int starArgIndex = -1, kwstarArgIndex = -1;
  for (int i = 0; i < func->ast->size(); i++) {
    if (startswith((*func->ast)[i].name, "**"))
      kwstarArgIndex = i, score -= 2;
    else if (startswith((*func->ast)[i].name, "*"))
      starArgIndex = i, score -= 2;
  }

  // 1. Assign positional arguments to slots
  // Each slot contains a list of arg's indices
  std::vector<std::vector<int>> slots(func->ast->size());
  seqassert(known.empty() || func->ast->size() == known.size(), "bad 'known' string");
  std::vector<int> extra;
  std::map<std::string, int> namedArgs,
      extraNamedArgs; // keep the map---we need it sorted!
  for (int ai = 0, si = 0; ai < args.size() - partial; ai++) {
    if (args[ai].name.empty()) {
      while (!known.empty() && si < slots.size() &&
             known[si] == ClassType::PartialFlag::Included)
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
    for (int i = 0; i < func->ast->size(); i++)
      if (known.empty() || known[i] != ClassType::PartialFlag::Included) {
        auto [_, n] = (*func->ast)[i].getNameWithStars();
        slotNames[getUnmangledName(n)] = i;
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
                   Emsg(Error::CALL_ARGS_MANY, getUnmangledName(func->ast->getName()),
                        func->ast->size(), args.size() - partial));

  if (starArgIndex != -1)
    slots[starArgIndex] = extra;

  // 4. Fill in **kwargs, if present
  if (!extraNamedArgs.empty() && kwstarArgIndex == -1)
    return onError(Error::CALL_ARGS_INVALID,
                   args[extraNamedArgs.begin()->second].value->getSrcInfo(),
                   Emsg(Error::CALL_ARGS_INVALID, extraNamedArgs.begin()->first,
                        getUnmangledName(func->ast->getName())));
  if (kwstarArgIndex != -1)
    for (auto &e : extraNamedArgs)
      slots[kwstarArgIndex].push_back(e.second);

  // 5. Fill in the default arguments
  for (auto i = 0; i < func->ast->size(); i++)
    if (slots[i].empty() && i != starArgIndex && i != kwstarArgIndex) {
      if (((*func->ast)[i].isValue() &&
           ((*func->ast)[i].getDefault() ||
            (!known.empty() && known[i] == ClassType::PartialFlag::Included))) ||
          startswith((*func->ast)[i].getName(), "$")) {
        score -= 2;
      } else if (!partial && (*func->ast)[i].isValue()) {
        auto [_, n] = (*func->ast)[i].getNameWithStars();
        return onError(Error::CALL_ARGS_MISSING, getSrcInfo(),
                       Emsg(Error::CALL_ARGS_MISSING,
                            getUnmangledName(func->ast->getName()),
                            getUnmangledName(n)));
      }
    }
  auto s = onDone(starArgIndex, kwstarArgIndex, slots, partial);
  return s != -1 ? score + s : -1;
}

bool TypecheckVisitor::isCanonicalName(const std::string &name) const {
  return name.rfind('.') != std::string::npos;
}

types::FuncType *TypecheckVisitor::extractFunction(types::Type *t) const {
  if (auto f = t->getFunc())
    return f;
  if (auto p = t->getPartial())
    return p->getPartialFunc();
  return nullptr;
}

class SearchVisitor : public CallbackASTVisitor<void, void> {
  std::function<bool(Expr *)> exprPredicate;
  std::function<bool(Stmt *)> stmtPredicate;

public:
  std::vector<ASTNode *> result;

public:
  SearchVisitor(const std::function<bool(Expr *)> &exprPredicate,
                const std::function<bool(Stmt *)> &stmtPredicate)
      : exprPredicate(exprPredicate), stmtPredicate(stmtPredicate) {}
  void transform(Expr *expr) override {
    if (expr && exprPredicate(expr)) {
      result.push_back(expr);
    } else {
      SearchVisitor v(exprPredicate, stmtPredicate);
      if (expr)
        expr->accept(v);
      result.insert(result.end(), v.result.begin(), v.result.end());
    }
  }
  void transform(Stmt *stmt) override {
    if (stmt && stmtPredicate(stmt)) {
      SearchVisitor v(exprPredicate, stmtPredicate);
      stmt->accept(v);
      result.insert(result.end(), v.result.begin(), v.result.end());
    }
  }
};

ParserErrors TypecheckVisitor::findTypecheckErrors(Stmt *n) {
  SearchVisitor v([](Expr *e) { return !e->isDone(); },
                  [](Stmt *s) { return !s->isDone(); });
  v.transform(n);
  std::vector<ErrorMessage> errors;
  for (auto e : v.result)
    errors.emplace_back(
        fmt::format("cannot typecheck {}", split(e->toString(0), '\n').front()),
        e->getSrcInfo());
  return ParserErrors(errors);
}

ir::PyType TypecheckVisitor::cythonizeClass(const std::string &name) {
  auto c = getClass(name);
  if (!c->module.empty())
    return {"", ""};
  if (!in(c->methods, "__to_py__") || !in(c->methods, "__from_py__"))
    return {"", ""};

  LOG_USER("[py] Cythonizing {}", name);
  ir::PyType py{getUnmangledName(name), c->ast->getDocstr()};

  auto tc = ctx->forceFind(name)->getType();
  if (!tc->canRealize())
    E(Error::CUSTOM, c->ast, "cannot realize '{}' for Python export",
      getUnmangledName(name));
  tc = realize(tc);
  seqassertn(tc, "cannot realize '{}'", name);

  // 1. Replace to_py / from_py with _PyWrap.wrap_to_py/from_py
  if (auto ofnn = in(c->methods, "__to_py__")) {
    auto fnn = getOverloads(*ofnn).front(); // default first overload!
    auto fna = getFunction(fnn)->ast;
    fna->suite = SuiteStmt::wrap(
        N<ReturnStmt>(N<CallExpr>(N<IdExpr>(format("{}.wrap_to_py:0", CYTHON_PYWRAP)),
                                  N<IdExpr>(fna->begin()->name))));
  }
  if (auto ofnn = in(c->methods, "__from_py__")) {
    auto fnn = getOverloads(*ofnn).front(); // default first overload!
    auto fna = getFunction(fnn)->ast;
    fna->suite = SuiteStmt::wrap(
        N<ReturnStmt>(N<CallExpr>(N<IdExpr>(format("{}.wrap_from_py:0", CYTHON_PYWRAP)),
                                  N<IdExpr>(fna->begin()->name), N<IdExpr>(name))));
  }
  for (auto &n : std::vector<std::string>{"__from_py__", "__to_py__"}) {
    auto fnn = getOverloads(*in(c->methods, n)).front();
    auto fn = getFunction(fnn);
    ir::Func *oldIR = nullptr;
    if (!fn->realizations.empty())
      oldIR = fn->realizations.begin()->second->ir;
    fn->realizations.clear();
    auto tf = realize(fn->type);
    seqassertn(tf, "cannot re-realize '{}'", fnn);
    if (oldIR) {
      std::vector<ir::Value *> args;
      for (auto it = oldIR->arg_begin(); it != oldIR->arg_end(); ++it) {
        args.push_back(ctx->cache->module->Nr<ir::VarValue>(*it));
      }
      cast<ir::BodiedFunc>(oldIR)->setBody(
          ir::util::series(ir::util::call(fn->realizations.begin()->second->ir, args)));
    }
  }
  for (auto &[rn, r] :
       getFunction(format("{}.py_type:0", CYTHON_PYWRAP))->realizations) {
    if (r->type->funcGenerics[0].type->unify(tc, nullptr) >= 0) {
      py.typePtrHook = r->ir;
      break;
    }
  }

  // 2. Handle methods
  auto methods = c->methods;
  for (const auto &[n, ofnn] : methods) {
    auto canonicalName = getOverloads(ofnn).back();
    auto fn = getFunction(canonicalName);
    if (getOverloads(ofnn).size() == 1 && fn->ast->hasAttribute(Attr::AutoGenerated))
      continue;
    auto fna = fn->ast;
    bool isMethod = fna->hasAttribute(Attr::Method);
    bool isProperty = fna->hasAttribute(Attr::Property);

    std::string call = format("{}.wrap_multiple", CYTHON_PYWRAP);
    bool isMagic = false;
    if (startswith(n, "__") && endswith(n, "__")) {
      auto m = n.substr(2, n.size() - 4);
      if (m == "new" && c->ast->hasAttribute(Attr::Tuple))
        m = "init";
      auto cls = getClass(CYTHON_PYWRAP);
      if (auto i = in(c->methods, "wrap_magic_" + m)) {
        call = *i;
        isMagic = true;
      }
    }
    if (isProperty)
      call = format("{}.wrap_get", CYTHON_PYWRAP);

    auto fnName = call + ":0";
    auto generics = std::vector<types::TypePtr>{tc->shared_from_this()};
    if (isProperty) {
      generics.push_back(instantiateStatic(getUnmangledName(canonicalName)));
    } else if (!isMagic) {
      generics.push_back(instantiateStatic(n));
      generics.push_back(instantiateStatic(int64_t(isMethod)));
    }
    auto f = realizeIRFunc(getFunction(fnName)->getType(), generics);
    if (!f)
      continue;

    LOG_USER("[py] {} -> {} ({}; {})", n, call, isMethod, isProperty);
    if (isProperty) {
      py.getset.push_back({getUnmangledName(canonicalName), "", f, nullptr});
    } else if (n == "__repr__") {
      py.repr = f;
    } else if (n == "__add__") {
      py.add = f;
    } else if (n == "__iadd__") {
      py.iadd = f;
    } else if (n == "__sub__") {
      py.sub = f;
    } else if (n == "__isub__") {
      py.isub = f;
    } else if (n == "__mul__") {
      py.mul = f;
    } else if (n == "__imul__") {
      py.imul = f;
    } else if (n == "__mod__") {
      py.mod = f;
    } else if (n == "__imod__") {
      py.imod = f;
    } else if (n == "__divmod__") {
      py.divmod = f;
    } else if (n == "__pow__") {
      py.pow = f;
    } else if (n == "__ipow__") {
      py.ipow = f;
    } else if (n == "__neg__") {
      py.neg = f;
    } else if (n == "__pos__") {
      py.pos = f;
    } else if (n == "__abs__") {
      py.abs = f;
    } else if (n == "__bool__") {
      py.bool_ = f;
    } else if (n == "__invert__") {
      py.invert = f;
    } else if (n == "__lshift__") {
      py.lshift = f;
    } else if (n == "__ilshift__") {
      py.ilshift = f;
    } else if (n == "__rshift__") {
      py.rshift = f;
    } else if (n == "__irshift__") {
      py.irshift = f;
    } else if (n == "__and__") {
      py.and_ = f;
    } else if (n == "__iand__") {
      py.iand = f;
    } else if (n == "__xor__") {
      py.xor_ = f;
    } else if (n == "__ixor__") {
      py.ixor = f;
    } else if (n == "__or__") {
      py.or_ = f;
    } else if (n == "__ior__") {
      py.ior = f;
    } else if (n == "__int__") {
      py.int_ = f;
    } else if (n == "__float__") {
      py.float_ = f;
    } else if (n == "__floordiv__") {
      py.floordiv = f;
    } else if (n == "__ifloordiv__") {
      py.ifloordiv = f;
    } else if (n == "__truediv__") {
      py.truediv = f;
    } else if (n == "__itruediv__") {
      py.itruediv = f;
    } else if (n == "__index__") {
      py.index = f;
    } else if (n == "__matmul__") {
      py.matmul = f;
    } else if (n == "__imatmul__") {
      py.imatmul = f;
    } else if (n == "__len__") {
      py.len = f;
    } else if (n == "__getitem__") {
      py.getitem = f;
    } else if (n == "__setitem__") {
      py.setitem = f;
    } else if (n == "__contains__") {
      py.contains = f;
    } else if (n == "__hash__") {
      py.hash = f;
    } else if (n == "__call__") {
      py.call = f;
    } else if (n == "__str__") {
      py.str = f;
    } else if (n == "__iter__") {
      py.iter = f;
    } else if (n == "__del__") {
      py.del = f;
    } else if (n == "__init__" ||
               (c->ast->hasAttribute(Attr::Tuple) && n == "__new__")) {
      py.init = f;
    } else {
      py.methods.push_back(ir::PyFunction{
          n, fna->getDocstr(), f,
          fna->hasAttribute(Attr::Method) ? ir::PyFunction::Type::METHOD
                                          : ir::PyFunction::Type::CLASS,
          // always use FASTCALL for now; works even for 0- or 1- arg methods
          2});
      py.methods.back().keywords = true;
    }
  }

  for (auto &m : py.methods) {
    if (in(std::set<std::string>{"__lt__", "__le__", "__eq__", "__ne__", "__gt__",
                                 "__ge__"},
           m.name)) {
      py.cmp = realizeIRFunc(
          ctx->forceFind(format("{}.wrap_cmp:0", CYTHON_PYWRAP))->type->getFunc(),
          {tc->shared_from_this()});
      break;
    }
  }

  if (c->realizations.size() != 1)
    E(Error::CUSTOM, c->ast, "cannot pythonize generic class '{}'", name);
  auto r = c->realizations.begin()->second;
  py.type = r->ir;
  seqassertn(!r->type->is(TYPE_TUPLE), "tuples not yet done");
  for (auto &[mn, mt] : r->fields) {
    /// TODO: handle PyMember for tuples
    // Generate getters & setters
    auto generics =
        std::vector<types::TypePtr>{tc->shared_from_this(), instantiateStatic(mn)};
    auto gf = realizeIRFunc(
        getFunction(format("{}.wrap_get:0", CYTHON_PYWRAP))->getType(), generics);
    ir::Func *sf = nullptr;
    if (!c->ast->hasAttribute(Attr::Tuple))
      sf = realizeIRFunc(getFunction(format("{}.wrap_set:0", CYTHON_PYWRAP))->getType(),
                         generics);
    py.getset.push_back({mn, "", gf, sf});
    LOG_USER("[py] {}: {} . {}", "member", name, mn);
  }
  return py;
}

ir::PyType TypecheckVisitor::cythonizeIterator(const std::string &name) {
  LOG_USER("[py] iterfn: {}", name);
  ir::PyType py{name, ""};
  auto cr = ctx->cache->classes[CYTHON_ITER].realizations[name];
  auto tc = cr->getType();
  for (auto &[rn, r] :
       getFunction(format("{}.py_type:0", CYTHON_PYWRAP))->realizations) {
    if (extractFuncGeneric(r->getType())->unify(tc, nullptr) >= 0) {
      py.typePtrHook = r->ir;
      break;
    }
  }

  const auto &methods = getClass(CYTHON_ITER)->methods;
  for (auto &n : std::vector<std::string>{"_iter", "_iternext"}) {
    auto fnn = getOverloads(getClass(CYTHON_ITER)->methods[n]).front();
    auto rtv = realize(instantiateType(getFunction(fnn)->getType(), tc->getClass()));
    auto f =
        getFunction(rtv->getFunc()->getFuncName())->realizations[rtv->realizedName()];
    if (n == "_iter")
      py.iter = f->ir;
    else
      py.iternext = f->ir;
  }
  py.type = cr->ir;
  return py;
}

ir::PyFunction TypecheckVisitor::cythonizeFunction(const std::string &name) {
  auto f = getFunction(name);
  if (f->isToplevel) {
    auto fnName = format("{}.wrap_multiple:0", CYTHON_PYWRAP);
    auto generics = std::vector<types::TypePtr>{
        ctx->forceFind(".toplevel")->type,
        instantiateStatic(getUnmangledName(f->ast->getName())),
        instantiateStatic(int64_t(0))};
    if (auto ir = realizeIRFunc(getFunction(fnName)->getType(), generics)) {
      LOG_USER("[py] {}: {}", "toplevel", name);
      ir::PyFunction fn{getUnmangledName(name), f->ast->getDocstr(), ir,
                        ir::PyFunction::Type::TOPLEVEL, int(f->ast->size())};
      fn.keywords = true;
      return fn;
    }
  }
  return {"", ""};
}

} // namespace codon::ast
