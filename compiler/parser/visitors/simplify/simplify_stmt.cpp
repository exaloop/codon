/*
 * simplify_statement.cpp --- AST statement simplifications.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/peg/peg.h"
#include "parser/visitors/format/format.h"
#include "parser/visitors/simplify/simplify.h"

using fmt::format;
using std::dynamic_pointer_cast;

namespace seq {
namespace ast {

struct ReplacementVisitor : ReplaceASTVisitor {
  const unordered_map<string, ExprPtr> *table;
  void transform(ExprPtr &e) override {
    if (!e)
      return;
    ReplacementVisitor v;
    v.table = table;
    e->accept(v);
    if (auto i = e->getId()) {
      auto it = table->find(i->value);
      if (it != table->end())
        e = it->second->clone();
    }
  }
  void transform(StmtPtr &e) override {
    if (!e)
      return;
    ReplacementVisitor v;
    v.table = table;
    e->accept(v);
  }
};
template <typename T> T replace(const T &e, const unordered_map<string, ExprPtr> &s) {
  ReplacementVisitor v;
  v.table = &s;
  auto ep = clone(e);
  v.transform(ep);
  return ep;
}

StmtPtr SimplifyVisitor::transform(const StmtPtr &stmt) {
  if (!stmt)
    return nullptr;

  SimplifyVisitor v(ctx, preamble);
  v.setSrcInfo(stmt->getSrcInfo());
  const_cast<Stmt *>(stmt.get())->accept(v);
  if (v.resultStmt)
    v.resultStmt->age = ctx->cache->age;
  return v.resultStmt;
}

void SimplifyVisitor::defaultVisit(Stmt *s) { resultStmt = s->clone(); }

/**************************************************************************************/

void SimplifyVisitor::visit(SuiteStmt *stmt) {
  vector<StmtPtr> r;
  // Make sure to add context blocks if this suite requires it...
  if (stmt->ownBlock)
    ctx->addBlock();
  for (const auto &s : stmt->stmts)
    SuiteStmt::flatten(transform(s), r);
  // ... and to remove it later.
  if (stmt->ownBlock)
    ctx->popBlock();
  resultStmt = N<SuiteStmt>(r, stmt->ownBlock);
}

void SimplifyVisitor::visit(ContinueStmt *stmt) {
  if (ctx->loops.empty())
    error("continue outside of a loop");
  resultStmt = stmt->clone();
}

void SimplifyVisitor::visit(BreakStmt *stmt) {
  if (ctx->loops.empty())
    error("break outside of a loop");
  if (!ctx->loops.back().empty()) {
    resultStmt = N<SuiteStmt>(
        transform(N<AssignStmt>(N<IdExpr>(ctx->loops.back()), N<BoolExpr>(false))),
        stmt->clone());
  } else {
    resultStmt = stmt->clone();
  }
}

void SimplifyVisitor::visit(ExprStmt *stmt) {
  resultStmt = N<ExprStmt>(transform(stmt->expr, true));
}

void SimplifyVisitor::visit(AssignStmt *stmt) {
  vector<StmtPtr> stmts;
  if (stmt->rhs && stmt->rhs->getBinary() && stmt->rhs->getBinary()->inPlace) {
    /// Case 1: a += b
    seqassert(!stmt->type, "invalid AssignStmt {}", stmt->toString());
    stmts.push_back(transformAssignment(stmt->lhs, stmt->rhs, nullptr, false, true));
  } else if (stmt->type) {
    /// Case 2:
    stmts.push_back(transformAssignment(stmt->lhs, stmt->rhs, stmt->type, true, false));
  } else {
    unpackAssignments(stmt->lhs, stmt->rhs, stmts, stmt->shadow, false);
  }
  resultStmt = stmts.size() == 1 ? stmts[0] : N<SuiteStmt>(stmts);
}

void SimplifyVisitor::visit(DelStmt *stmt) {
  if (auto eix = stmt->expr->getIndex()) {
    resultStmt = N<ExprStmt>(transform(
        N<CallExpr>(N<DotExpr>(clone(eix->expr), "__delitem__"), clone(eix->index))));
  } else if (auto ei = stmt->expr->getId()) {
    resultStmt = transform(
        N<AssignStmt>(clone(stmt->expr),
                      N<CallExpr>(N<CallExpr>(N<IdExpr>("type"), clone(stmt->expr)))));
    ctx->remove(ei->value);
  } else {
    error("invalid del statement");
  }
}

void SimplifyVisitor::visit(PrintStmt *stmt) {
  vector<CallExpr::Arg> args;
  for (auto &i : stmt->items)
    args.emplace_back(CallExpr::Arg{"", transform(i)});
  if (stmt->isInline)
    args.emplace_back(CallExpr::Arg{"end", N<StringExpr>(" ")});
  resultStmt = N<ExprStmt>(N<CallExpr>(transform(N<IdExpr>("print")), args));
}

void SimplifyVisitor::visit(ReturnStmt *stmt) {
  if (!ctx->inFunction())
    error("expected function body");
  resultStmt = N<ReturnStmt>(transform(stmt->expr));
}

void SimplifyVisitor::visit(YieldStmt *stmt) {
  if (!ctx->inFunction())
    error("expected function body");
  resultStmt = N<YieldStmt>(transform(stmt->expr));
}

void SimplifyVisitor::visit(YieldFromStmt *stmt) {
  auto var = ctx->cache->getTemporaryVar("yield");
  resultStmt = transform(
      N<ForStmt>(N<IdExpr>(var), clone(stmt->expr), N<YieldStmt>(N<IdExpr>(var))));
}

void SimplifyVisitor::visit(AssertStmt *stmt) {
  ExprPtr msg = N<StringExpr>("");
  if (stmt->message)
    msg = N<CallExpr>(N<IdExpr>("str"), clone(stmt->message));
  if (ctx->getLevel() && ctx->bases.back().attributes & FLAG_TEST)
    resultStmt = transform(
        N<IfStmt>(N<UnaryExpr>("!", clone(stmt->expr)),
                  N<ExprStmt>(N<CallExpr>(N<DotExpr>("__internal__", "seq_assert_test"),
                                          N<StringExpr>(stmt->getSrcInfo().file),
                                          N<IntExpr>(stmt->getSrcInfo().line), msg))));
  else
    resultStmt = transform(
        N<IfStmt>(N<UnaryExpr>("!", clone(stmt->expr)),
                  N<ThrowStmt>(N<CallExpr>(N<DotExpr>("__internal__", "seq_assert"),
                                           N<StringExpr>(stmt->getSrcInfo().file),
                                           N<IntExpr>(stmt->getSrcInfo().line), msg))));
}

void SimplifyVisitor::visit(WhileStmt *stmt) {
  ExprPtr cond = N<CallExpr>(N<DotExpr>(clone(stmt->cond), "__bool__"));
  string breakVar;
  StmtPtr assign = nullptr;
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    breakVar = ctx->cache->getTemporaryVar("no_break");
    assign =
        transform(N<AssignStmt>(N<IdExpr>(breakVar), N<BoolExpr>(true), nullptr, true));
  }
  ctx->loops.push_back(breakVar); // needed for transforming break in loop..else blocks
  StmtPtr whileStmt = N<WhileStmt>(transform(cond), transform(stmt->suite));
  ctx->loops.pop_back();
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    resultStmt =
        N<SuiteStmt>(assign, whileStmt,
                     N<IfStmt>(transform(N<CallExpr>(N<DotExpr>(breakVar, "__bool__"))),
                               transform(stmt->elseSuite)));
  } else {
    resultStmt = whileStmt;
  }
}

void SimplifyVisitor::visit(ForStmt *stmt) {
  vector<CallExpr::Arg> ompArgs;
  ExprPtr decorator = clone(stmt->decorator);
  if (decorator) {
    ExprPtr callee = decorator;
    if (auto c = callee->getCall())
      callee = c->expr;
    if (!callee || !callee->isId("par"))
      error("for loop can only take parallel decorator");
    vector<CallExpr::Arg> args;
    string openmp;
    vector<CallExpr::Arg> omp;
    if (auto c = decorator->getCall())
      for (auto &a : c->args) {
        if (a.name == "openmp" ||
            (a.name.empty() && openmp.empty() && a.value->getString())) {
          omp = parseOpenMP(ctx->cache, a.value->getString()->getValue(),
                            a.value->getSrcInfo());
        } else {
          args.push_back({a.name, transform(a.value)});
        }
      }
    for (auto &a : omp)
      args.push_back({a.name, transform(a.value)});
    decorator = N<CallExpr>(transform(N<IdExpr>("for_par")), args);
  }

  string breakVar;
  auto iter = transform(stmt->iter); // needs in-advance transformation to prevent
                                     // name clashes with the iterator variable
  StmtPtr assign = nullptr, forStmt = nullptr;
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    breakVar = ctx->cache->getTemporaryVar("no_break");
    assign =
        transform(N<AssignStmt>(N<IdExpr>(breakVar), N<BoolExpr>(true), nullptr, true));
  }
  ctx->loops.push_back(breakVar); // needed for transforming break in loop..else blocks
  ctx->addBlock();
  if (auto i = stmt->var->getId()) {
    ctx->add(SimplifyItem::Var, i->value, ctx->generateCanonicalName(i->value));
    forStmt = N<ForStmt>(transform(stmt->var), clone(iter), transform(stmt->suite),
                         nullptr, decorator, ompArgs);
  } else {
    string varName = ctx->cache->getTemporaryVar("for");
    ctx->add(SimplifyItem::Var, varName, varName);
    auto var = N<IdExpr>(varName);
    vector<StmtPtr> stmts;
    stmts.push_back(N<AssignStmt>(clone(stmt->var), clone(var), nullptr, true));
    stmts.push_back(clone(stmt->suite));
    forStmt = N<ForStmt>(clone(var), clone(iter), transform(N<SuiteStmt>(stmts)),
                         nullptr, decorator, ompArgs);
  }
  ctx->popBlock();
  ctx->loops.pop_back();

  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    resultStmt =
        N<SuiteStmt>(assign, forStmt,
                     N<IfStmt>(transform(N<CallExpr>(N<DotExpr>(breakVar, "__bool__"))),
                               transform(stmt->elseSuite)));
  } else {
    resultStmt = forStmt;
  }
}

void SimplifyVisitor::visit(IfStmt *stmt) {
  seqassert(stmt->cond, "invalid if statement");
  resultStmt = N<IfStmt>(transform(stmt->cond), transform(stmt->ifSuite),
                         transform(stmt->elseSuite));
}

void SimplifyVisitor::visit(MatchStmt *stmt) {
  auto var = ctx->cache->getTemporaryVar("match");
  auto result = N<SuiteStmt>();
  result->stmts.push_back(
      N<AssignStmt>(N<IdExpr>(var), clone(stmt->what), nullptr, true));
  for (auto &c : stmt->cases) {
    ctx->addBlock();
    StmtPtr suite = N<SuiteStmt>(clone(c.suite), N<BreakStmt>());
    if (c.guard)
      suite = N<IfStmt>(clone(c.guard), suite);
    result->stmts.push_back(transformPattern(N<IdExpr>(var), clone(c.pattern), suite));
    ctx->popBlock();
  }
  result->stmts.push_back(N<BreakStmt>()); // break even if there is no case _.
  resultStmt = transform(N<WhileStmt>(N<BoolExpr>(true), result));
}

void SimplifyVisitor::visit(TryStmt *stmt) {
  vector<TryStmt::Catch> catches;
  auto suite = transform(stmt->suite);
  for (auto &ctch : stmt->catches) {
    ctx->addBlock();
    auto var = ctch.var;
    if (!ctch.var.empty()) {
      var = ctx->generateCanonicalName(ctch.var);
      ctx->add(SimplifyItem::Var, ctch.var, var);
    }
    catches.push_back({var, transformType(ctch.exc), transform(ctch.suite)});
    ctx->popBlock();
  }
  resultStmt = N<TryStmt>(suite, catches, transform(stmt->finally));
}

void SimplifyVisitor::visit(ThrowStmt *stmt) {
  resultStmt = N<ThrowStmt>(transform(stmt->expr));
}

void SimplifyVisitor::visit(WithStmt *stmt) {
  assert(stmt->items.size());
  vector<StmtPtr> content;
  for (int i = int(stmt->items.size()) - 1; i >= 0; i--) {
    string var =
        stmt->vars[i].empty() ? ctx->cache->getTemporaryVar("with") : stmt->vars[i];
    content = vector<StmtPtr>{
        N<AssignStmt>(N<IdExpr>(var), clone(stmt->items[i]), nullptr, true),
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(var, "__enter__"))),
        N<TryStmt>(!content.empty() ? N<SuiteStmt>(content, true) : clone(stmt->suite),
                   vector<TryStmt::Catch>{},
                   N<SuiteStmt>(vector<StmtPtr>{N<ExprStmt>(
                                    N<CallExpr>(N<DotExpr>(var, "__exit__")))},
                                true))};
  }
  resultStmt = transform(N<SuiteStmt>(content, true));
}

void SimplifyVisitor::visit(GlobalStmt *stmt) {
  if (ctx->bases.empty() || ctx->bases.back().isType())
    error("global outside of a function");
  auto val = ctx->find(stmt->var);
  if (!val || !val->isVar())
    error("identifier '{}' not found", stmt->var);
  if (!val->getBase().empty())
    error("not a top-level variable");
  seqassert(!val->canonicalName.empty(), "'{}' does not have a canonical name",
            stmt->var);
  ctx->cache->globals.insert(val->canonicalName);
  val->global = true;
  ctx->add(SimplifyItem::Var, stmt->var, val->canonicalName, true);
}

void SimplifyVisitor::visit(ImportStmt *stmt) {
  seqassert(!ctx->inClass(), "imports within a class");
  if (stmt->from && stmt->from->isId("C")) {
    /// Handle C imports
    if (auto i = stmt->what->getId())
      resultStmt = transformCImport(i->value, stmt->args, stmt->ret.get(), stmt->as);
    else if (auto d = stmt->what->getDot())
      resultStmt = transformCDLLImport(d->expr.get(), d->member, stmt->args,
                                       stmt->ret.get(), stmt->as);
    else
      seqassert(false, "invalid C import statement");
    return;
  } else if (stmt->from && stmt->from->isId("python") && stmt->what) {
    resultStmt =
        transformPythonImport(stmt->what.get(), stmt->args, stmt->ret.get(), stmt->as);
    return;
  }

  // Transform import a.b.c.d to "a/b/c/d".
  vector<string> dirs; // Path components
  if (stmt->from) {
    Expr *e = stmt->from.get();
    while (auto d = e->getDot()) {
      dirs.push_back(d->member);
      e = d->expr.get();
    }
    if (!e->getId() || !stmt->args.empty() || stmt->ret ||
        (stmt->what && !stmt->what->getId()))
      error("invalid import statement");
    dirs.push_back(e->getId()->value);
  }
  // Handle dots (e.g. .. in from ..m import x).
  seqassert(stmt->dots >= 0, "negative dots in ImportStmt");
  for (int i = 0; i < stmt->dots - 1; i++)
    dirs.emplace_back("..");
  string path;
  for (int i = int(dirs.size()) - 1; i >= 0; i--)
    path += dirs[i] + (i ? "/" : "");
  // Fetch the import!
  auto file = getImportFile(ctx->cache->argv0, path, ctx->getFilename(), false,
                            ctx->cache->module0);
  if (!file)
    error("cannot locate import '{}'", join(dirs, "."));

  // If the imported file has not been seen before, load it.
  if (ctx->cache->imports.find(file->path) == ctx->cache->imports.end())
    transformNewImport(*file);
  const auto &import = ctx->cache->imports[file->path];
  string importVar = import.importVar;
  string importDoneVar = importVar + "_done";

  // Import variable is empty if it has already been loaded during the standard library
  // initialization.
  if (!ctx->isStdlibLoading && !importVar.empty()) {
    vector<StmtPtr> ifSuite;
    ifSuite.emplace_back(N<ExprStmt>(N<CallExpr>(N<IdExpr>(importVar))));
    ifSuite.emplace_back(N<UpdateStmt>(N<IdExpr>(importDoneVar), N<BoolExpr>(true)));
    resultStmt = N<IfStmt>(N<CallExpr>(N<DotExpr>(importDoneVar, "__invert__")),
                           N<SuiteStmt>(ifSuite));
  }

  if (!stmt->what) {
    // Case 1: import foo
    auto name = stmt->as.empty() ? path : stmt->as;
    auto var = importVar + "_var";
    resultStmt = N<SuiteStmt>(
        resultStmt, transform(N<AssignStmt>(N<IdExpr>(var),
                                            N<CallExpr>(N<IdExpr>("Import"),
                                                        N<StringExpr>(file->module),
                                                        N<StringExpr>(file->path)),
                                            N<IdExpr>("Import"))));
    ctx->add(SimplifyItem::Var, name, var);
    ctx->find(name)->importPath = file->path;
  } else if (stmt->what->isId("*")) {
    // Case 2: from foo import *
    seqassert(stmt->as.empty(), "renamed star-import");
    // Just copy all symbols from import's context here.
    for (auto &i : *(import.ctx))
      if (!startswith(i.first, "_") && i.second.front().second->isGlobal()) {
        ctx->add(i.first, i.second.front().second);
        ctx->add(i.second.front().second->canonicalName, i.second.front().second);
      }
  } else {
    // Case 3: from foo import bar
    auto i = stmt->what->getId();
    seqassert(i, "not a valid import what expression");
    auto c = import.ctx->find(i->value);
    // Make sure that we are importing an existing global symbol
    if (!c || !c->isGlobal())
      error("symbol '{}' not found in {}", i->value, file->path);
    ctx->add(stmt->as.empty() ? i->value : stmt->as, c);
    ctx->add(c->canonicalName, c);
  }
}

void SimplifyVisitor::visit(FunctionStmt *stmt) {
  vector<ExprPtr> decorators;
  Attr attr = stmt->attributes;
  for (auto &d : stmt->decorators) {
    if (d->isId("__attribute__")) {
      if (stmt->decorators.size() != 1)
        error("__attribute__ cannot be mixed with other decorators");
      attr.isAttribute = true;
    } else if (d->isId(Attr::LLVM))
      attr.set(Attr::LLVM);
    else if (d->isId(Attr::Python))
      attr.set(Attr::Python);
    else if (d->isId(Attr::Internal))
      attr.set(Attr::Internal);
    else if (d->isId(Attr::Atomic))
      attr.set(Attr::Atomic);
    else if (d->isId(Attr::Property))
      attr.set(Attr::Property);
    else if (d->isId(Attr::ForceRealize))
      attr.set(Attr::ForceRealize);
    else {
      // Let's check if this is a attribute
      auto dt = transform(clone(d));
      if (dt && dt->getId()) {
        auto ci = ctx->find(dt->getId()->value);
        if (ci && ci->kind == SimplifyItem::Func) {
          if (ctx->cache->functions[ci->canonicalName].ast->attributes.isAttribute) {
            attr.set(ci->canonicalName);
            continue;
          }
        }
      }
      decorators.emplace_back(clone(d));
    }
  }
  if (attr.has(Attr::Python)) {
    // Handle Python code separately
    resultStmt = transformPythonDefinition(stmt->name, stmt->args, stmt->ret.get(),
                                           stmt->suite->firstInBlock());
    // TODO: error on decorators
    return;
  }

  auto canonicalName = ctx->generateCanonicalName(stmt->name, true);
  bool isClassMember = ctx->inClass();
  bool isEnclosedFunc = ctx->inFunction();

  if (attr.has(Attr::ForceRealize) && (ctx->getLevel() || isClassMember))
    error("builtins must be defined at the toplevel");

  auto oldBases = move(ctx->bases);
  ctx->bases = vector<SimplifyContext::Base>();
  if (!isClassMember)
    // Class members are added to class' method table
    ctx->add(SimplifyItem::Func, stmt->name, canonicalName, ctx->isToplevel());
  if (isClassMember)
    ctx->bases.push_back(oldBases[0]);
  ctx->bases.emplace_back(SimplifyContext::Base{canonicalName}); // Add new base...
  ctx->addBlock();                                               // ... and a block!
  // Set atomic flag if @atomic attribute is present.
  if (attr.has(Attr::Atomic))
    ctx->bases.back().attributes |= FLAG_ATOMIC;
  if (attr.has(Attr::Test))
    ctx->bases.back().attributes |= FLAG_TEST;
  // Add generic identifiers to the context
  unordered_set<string> seenArgs;
  // Parse function arguments and add them to the context.
  vector<Param> args;
  bool defaultsStarted = false, hasStarArg = false, hasKwArg = false;
  // Add generics first
  for (int ia = 0; ia < stmt->args.size(); ia++) {
    auto &a = stmt->args[ia];
    // check if this is a generic!
    if (a.type && (a.type->isId("type") || a.type->isId("TypeVar") ||
                   (a.type->getIndex() && a.type->getIndex()->expr->isId("Static"))))
      a.generic = true;
    string varName = a.name;
    int stars = trimStars(varName);
    if (stars == 2) {
      if (hasKwArg || a.deflt || ia != stmt->args.size() - 1)
        error("invalid **kwargs");
      hasKwArg = true;
    } else if (stars == 1) {
      if (hasStarArg || a.deflt)
        error("invalid *args");
      hasStarArg = true;
    }
    if (in(seenArgs, varName))
      error("'{}' declared twice", varName);
    seenArgs.insert(varName);
    if (!a.deflt && defaultsStarted && !stars && !a.generic)
      error("non-default argument '{}' after a default argument", varName);
    defaultsStarted |= bool(a.deflt);

    auto typeAst = a.type;
    if (!typeAst && isClassMember && ia == 0 && a.name == "self") {
      typeAst = ctx->bases[ctx->bases.size() - 2].ast;
      attr.set(".changedSelf");
    }

    if (attr.has(Attr::C)) {
      if (a.deflt)
        error("C functions do not accept default argument");
      if (stars != 1 && !typeAst)
        error("C functions require explicit type annotations");
      if (stars == 1)
        attr.set(Attr::CVarArg);
    }

    // First add all generics!
    auto name = ctx->generateCanonicalName(varName);
    args.emplace_back(Param{string(stars, '*') + name, typeAst, a.deflt, a.generic});
    if (a.generic) {
      if (a.type->getIndex() && a.type->getIndex()->expr->isId("Static"))
        ctx->add(SimplifyItem::Var, varName, name);
      else
        ctx->add(SimplifyItem::Type, varName, name);
    }
  }
  for (auto &a : args) {
    a.type = transformType(a.type, false);
    a.deflt = transform(a.deflt, true);
  }
  // Delay adding to context to prevent "def foo(a, b=a)"
  for (auto &a : args) {
    if (!a.generic) {
      string canName = a.name;
      trimStars(canName);
      ctx->add(SimplifyItem::Var, ctx->cache->reverseIdentifierLookup[canName],
               canName);
    }
  }
  // Parse the return type.
  if (!stmt->ret && (attr.has(Attr::LLVM) || attr.has(Attr::C)))
    error("LLVM functions must have a return type");
  auto ret = transformType(stmt->ret, false);
  // Parse function body.
  StmtPtr suite = nullptr;
  std::map<string, string> captures;
  if (!attr.has(Attr::Internal) && !attr.has(Attr::C)) {
    ctx->addBlock();
    if (attr.has(Attr::LLVM)) {
      suite = transformLLVMDefinition(stmt->suite->firstInBlock());
    } else if (attr.has(Attr::C)) {
      ;
    } else {
      if ((isEnclosedFunc || attr.has(Attr::Capture)) && !isClassMember)
        ctx->captures.emplace_back(std::map<string, string>{});
      suite = SimplifyVisitor(ctx, preamble).transform(stmt->suite);
      if ((isEnclosedFunc || attr.has(Attr::Capture)) && !isClassMember) {
        captures = ctx->captures.back();
        ctx->captures.pop_back();
      }
    }
    ctx->popBlock();
  }

  // Once the body is done, check if this function refers to a variable (or generic)
  // from outer scope (e.g. it's parent is not -1). If so, store the name of the
  // innermost base that was referred to in this function.
  auto isMethod = ctx->bases.back().attributes & FLAG_METHOD;
  ctx->bases.pop_back();
  ctx->bases = move(oldBases);
  ctx->popBlock();
  attr.module =
      format("{}{}", ctx->moduleName.status == ImportFile::STDLIB ? "std::" : "::",
             ctx->moduleName.module);

  if (isClassMember) { // If this is a method...
    // ... set the enclosing class name...
    attr.parentClass = ctx->bases.back().name;
    // ... add the method to class' method list ...
    ctx->cache->classes[ctx->bases.back().name].methods[stmt->name].push_back(
        {canonicalName, nullptr, ctx->cache->age});
    // ... and if the function references outer class variable (by definition a
    // generic), mark it as not static as it needs fully instantiated class to be
    // realized. For example, in class A[T]: def foo(): pass, A.foo() can be realized
    // even if T is unknown. However, def bar(): return T() cannot because it needs T
    // (and is thus accordingly marked with ATTR_IS_METHOD).
    if (isMethod)
      attr.set(Attr::Method);
  }

  vector<CallExpr::Arg> partialArgs;
  if (!captures.empty()) {
    Param kw;
    if (hasKwArg) {
      kw = args.back();
      args.pop_back();
    }
    for (auto &c : captures) {
      args.emplace_back(Param{c.second, nullptr, nullptr});
      partialArgs.emplace_back(CallExpr::Arg{
          c.second, N<IdExpr>(ctx->cache->reverseIdentifierLookup[c.first])});
    }
    if (hasKwArg)
      args.push_back(kw);
    partialArgs.emplace_back(CallExpr::Arg{"", N<EllipsisExpr>()});
  }
  auto f = N<FunctionStmt>(canonicalName, ret, args, suite, attr);
  preamble->functions.push_back(
      N<FunctionStmt>(canonicalName, clone(f->ret), clone_nop(f->args), suite, attr));
  // Make sure to cache this (generic) AST for later realization.
  ctx->cache->functions[canonicalName].ast = f;

  ExprPtr finalExpr;
  if (!captures.empty())
    finalExpr = N<CallExpr>(N<IdExpr>(stmt->name), partialArgs);
  if (isClassMember && decorators.size())
    error("decorators cannot be applied to class methods");
  for (int j = int(decorators.size()) - 1; j >= 0; j--) {
    if (auto c = const_cast<CallExpr *>(decorators[j]->getCall())) {
      c->args.emplace(c->args.begin(),
                      CallExpr::Arg{"", finalExpr ? finalExpr : N<IdExpr>(stmt->name)});
      finalExpr = N<CallExpr>(c->expr, c->args);
    } else {
      finalExpr =
          N<CallExpr>(decorators[j], finalExpr ? finalExpr : N<IdExpr>(stmt->name));
    }
  }
  if (finalExpr)
    resultStmt = transform(N<AssignStmt>(N<IdExpr>(stmt->name), finalExpr));
}

void SimplifyVisitor::visit(ClassStmt *stmt) {
  enum Magic { Init, Repr, Eq, Order, Hash, Pickle, Container, Python };
  Attr attr = stmt->attributes;
  vector<char> hasMagic(10, 2);
  hasMagic[Init] = hasMagic[Pickle] = 1;
  // @tuple(init=, repr=, eq=, order=, hash=, pickle=, container=, python=, add=,
  // internal=...)
  // @dataclass(...)
  // @extend
  for (auto &d : stmt->decorators) {
    if (auto c = d->getCall()) {
      if (c->expr->isId(Attr::Tuple))
        attr.set(Attr::Tuple);
      else if (!c->expr->isId("dataclass"))
        error("invalid class attribute");
      else if (attr.has(Attr::Tuple))
        error("class already marked as tuple");
      for (auto &a : c->args) {
        auto b = CAST(a.value, BoolExpr);
        if (!b)
          error("expected static boolean");
        auto val = b->value;
        if (a.name == "init")
          hasMagic[Init] = val;
        else if (a.name == "repr")
          hasMagic[Repr] = val;
        else if (a.name == "eq")
          hasMagic[Eq] = val;
        else if (a.name == "order")
          hasMagic[Order] = val;
        else if (a.name == "hash")
          hasMagic[Hash] = val;
        else if (a.name == "pickle")
          hasMagic[Pickle] = val;
        else if (a.name == "python")
          hasMagic[Python] = val;
        else if (a.name == "container")
          hasMagic[Container] = val;
        else
          error("invalid decorator argument");
      }
    } else if (d->isId(Attr::Tuple)) {
      if (attr.has(Attr::Tuple))
        error("class already marked as tuple");
      attr.set(Attr::Tuple);
    } else if (d->isId(Attr::Extend)) {
      attr.set(Attr::Extend);
      if (stmt->decorators.size() != 1)
        error("extend cannot be combined with other decorators");
      if (!ctx->bases.empty())
        error("extend is only allowed at the toplevel");
    } else if (d->isId(Attr::Internal)) {
      attr.set(Attr::Internal);
    }
  }
  for (int i = 1; i < hasMagic.size(); i++)
    if (hasMagic[i] == 2)
      hasMagic[i] = attr.has(Attr::Tuple) ? 1 : 0;

  // Extensions (@extend) cases are handled bit differently
  // (no auto method-generation, no arguments etc.)
  bool extension = attr.has(Attr::Extend);
  bool isRecord = attr.has(Attr::Tuple); // does it have @tuple attribute

  // Special name handling is needed because of nested classes.
  string name = stmt->name;
  if (!ctx->bases.empty() && ctx->bases.back().isType()) {
    const auto &a = ctx->bases.back().ast;
    string parentName =
        a->getId() ? a->getId()->value : a->getIndex()->expr->getId()->value;
    name = parentName + "." + name;
  }

  // Generate/find class' canonical name (unique ID) and AST
  string canonicalName;
  ClassStmt *originalAST = nullptr;
  auto classItem =
      make_shared<SimplifyItem>(SimplifyItem::Type, "", "", ctx->isToplevel());
  if (!extension) {
    classItem->canonicalName = canonicalName =
        ctx->generateCanonicalName(name, !attr.has(Attr::Internal));
    // Reference types are added to the context at this stage.
    // Record types (tuples) are added after parsing class arguments to prevent
    // recursive record types (that are allowed for reference types).
    if (!isRecord) {
      ctx->add(name, classItem);
      ctx->cache->imports[STDLIB_IMPORT].ctx->addToplevel(canonicalName, classItem);
    }
    originalAST = stmt;
  } else {
    // Find the canonical name of a class that is to be extended
    auto val = ctx->find(name);
    if (!val || val->kind != SimplifyItem::Type)
      error("cannot find type '{}' to extend", name);
    canonicalName = val->canonicalName;
    const auto &astIter = ctx->cache->classes.find(canonicalName);
    if (astIter == ctx->cache->classes.end())
      error("cannot extend type alias or an instantiation ({})", name);
    originalAST = astIter->second.ast.get();
    if (stmt->args.size())
      error("extensions cannot be generic or declare members");
  }

  // Add the class base.
  auto oldBases = move(ctx->bases);
  ctx->bases = vector<SimplifyContext::Base>();
  ctx->bases.emplace_back(SimplifyContext::Base(canonicalName));
  ctx->bases.back().ast = make_shared<IdExpr>(name);

  if (extension && !stmt->baseClasses.empty())
    error("extensions cannot inherit other classes");
  vector<ClassStmt *> baseASTs;
  vector<Param> args;
  vector<unordered_map<string, ExprPtr>> substitutions;
  vector<int> argSubstitutions;
  unordered_set<string> seenMembers;
  for (auto &baseClass : stmt->baseClasses) {
    string bcName;
    vector<ExprPtr> subs;
    if (auto i = baseClass->getId())
      bcName = i->value;
    else if (auto e = baseClass->getIndex()) {
      if (auto i = e->expr->getId()) {
        bcName = i->value;
        subs = e->index->getTuple() ? e->index->getTuple()->items
                                    : vector<ExprPtr>{e->index};
      }
    }
    bcName = transformType(N<IdExpr>(bcName))->getId()->value;
    if (bcName.empty() || !in(ctx->cache->classes, bcName))
      error(baseClass.get(), "invalid base class");
    baseASTs.push_back(ctx->cache->classes[bcName].ast.get());
    if (baseASTs.back()->attributes.has(Attr::Tuple) != isRecord)
      error("tuples cannot inherit reference classes (and vice versa)");
    if (baseASTs.back()->attributes.has(Attr::Internal))
      error("cannot inherit internal types");
    int si = 0;
    substitutions.push_back({});
    for (auto &a : baseASTs.back()->args)
      if (a.generic) {
        if (si >= subs.size())
          error(baseClass.get(), "wrong number of generics");
        substitutions.back()[a.name] = clone(subs[si++]);
      }
    if (si != subs.size())
      error(baseClass.get(), "wrong number of generics");
    for (auto &a : baseASTs.back()->args)
      if (!a.generic) {
        if (seenMembers.find(a.name) != seenMembers.end())
          error(a.type, "'{}' declared twice", a.name);
        seenMembers.insert(a.name);
        args.emplace_back(Param{a.name, a.type, a.deflt});
        argSubstitutions.push_back(substitutions.size() - 1);
        if (!extension)
          ctx->cache->classes[canonicalName].fields.push_back({a.name, nullptr});
      }
  }

  // Add generics, if any, to the context.
  ctx->addBlock();
  vector<ExprPtr> genAst;
  substitutions.push_back({});
  for (auto &a : (extension ? originalAST : stmt)->args) {
    seqassert(a.type, "no type provided for '{}'", a.name);
    if (a.type && (a.type->isId("type") || a.type->isId("TypeVar") ||
                   (a.type->getIndex() && a.type->getIndex()->expr->isId("Static"))))
      a.generic = true;
    if (seenMembers.find(a.name) != seenMembers.end())
      error(a.type, "'{}' declared twice", a.name);
    seenMembers.insert(a.name);
    if (a.generic) {
      auto varName = extension ? a.name : ctx->generateCanonicalName(a.name);
      auto name = extension ? ctx->cache->reverseIdentifierLookup[a.name] : a.name;
      if (a.type->getIndex() && a.type->getIndex()->expr->isId("Static"))
        ctx->add(SimplifyItem::Var, name, varName, true);
      else
        ctx->add(SimplifyItem::Type, name, varName, true);
      genAst.push_back(N<IdExpr>(varName));
      args.emplace_back(Param{varName, a.type, a.deflt, a.generic});
    } else {
      args.emplace_back(Param{a.name, a.type, a.deflt});
      if (!extension)
        ctx->cache->classes[canonicalName].fields.push_back({a.name, nullptr});
    }
    argSubstitutions.push_back(substitutions.size() - 1);
  }
  if (!genAst.empty())
    ctx->bases.back().ast =
        make_shared<IndexExpr>(N<IdExpr>(name), N<TupleExpr>(genAst));

  vector<StmtPtr> stmts{nullptr}; // Will be filled later!
  // Parse nested classes
  for (auto sp : getClassMethods(stmt->suite))
    if (sp && sp->getClass()) {
      // Add dummy base to fix nested class' name.
      ctx->bases.emplace_back(SimplifyContext::Base(canonicalName));
      ctx->bases.back().ast = make_shared<IdExpr>(name);
      auto origName = sp->getClass()->name;
      stmts.emplace_back(transform(sp));
      ctx->add(origName,
               ctx->find(stmts.back()->getSuite()->stmts[0]->getClass()->name));
      ctx->bases.pop_back();
    }

  vector<Param> memberArgs;
  for (auto &s : substitutions)
    for (auto &i : s)
      i.second = transform(i.second, true);
  for (int ai = 0; ai < args.size(); ai++) {
    auto &a = args[ai];
    if (argSubstitutions[ai] == substitutions.size() - 1) {
      a.type = transformType(a.type, false);
      a.deflt = transform(a.deflt, true);
    } else {
      a.type = replace(a.type, substitutions[argSubstitutions[ai]]);
      a.deflt = replace(a.deflt, substitutions[argSubstitutions[ai]]);
    }
    if (!a.generic)
      memberArgs.push_back(a);
  }

  // Parse class members (arguments) and methods.
  auto suite = N<SuiteStmt>();
  if (!extension) {
    // Now that we are done with arguments, add record type to the context.
    // However, we need to unroll a block/base, add it, and add the unrolled
    // block/base back.
    if (isRecord) {
      ctx->addPrevBlock(name, classItem);
      ctx->cache->imports[STDLIB_IMPORT].ctx->addToplevel(canonicalName, classItem);
    }
    // Create a cached AST.
    attr.module =
        format("{}{}", ctx->moduleName.status == ImportFile::STDLIB ? "std::" : "::",
               ctx->moduleName.module);
    ctx->cache->classes[canonicalName].ast =
        N<ClassStmt>(canonicalName, args, N<SuiteStmt>(), attr);
    vector<StmtPtr> fns;
    ExprPtr codeType = ctx->bases.back().ast->clone();
    vector<string> magics{};
    // Internal classes do not get any auto-generated members.
    if (!attr.has(Attr::Internal)) {
      // Prepare a list of magics that are to be auto-generated.
      if (isRecord)
        magics = {"len", "hash"};
      else
        magics = {"new", "raw"};
      if (hasMagic[Init])
        magics.emplace_back(isRecord ? "new" : "init");
      if (hasMagic[Eq])
        for (auto &i : {"eq", "ne"})
          magics.emplace_back(i);
      if (hasMagic[Order])
        for (auto &i : {"lt", "gt", "le", "ge"})
          magics.emplace_back(i);
      if (hasMagic[Pickle])
        for (auto &i : {"pickle", "unpickle"})
          magics.emplace_back(i);
      if (hasMagic[Repr])
        magics.emplace_back("str");
      if (hasMagic[Container])
        for (auto &i : {"iter", "getitem"})
          magics.emplace_back(i);
      if (hasMagic[Python])
        for (auto &i : {"to_py", "from_py"})
          magics.emplace_back(i);

      if (hasMagic[Container] && startswith(stmt->name, TYPE_TUPLE))
        magics.emplace_back("contains");
      if (!startswith(stmt->name, TYPE_TUPLE))
        magics.emplace_back("dict");
      if (startswith(stmt->name, TYPE_TUPLE))
        magics.emplace_back("add");
    }
    // Codegen default magic methods and add them to the final AST.
    for (auto &m : magics) {
      transform(codegenMagic(m, ctx->bases.back().ast.get(), memberArgs, isRecord));
      suite->stmts.push_back(preamble->functions.back());
    }
  }
  for (int ai = 0; ai < baseASTs.size(); ai++)
    for (auto sp : getClassMethods(baseASTs[ai]->suite))
      if (auto f = sp->getFunction()) {
        if (f->attributes.has("autogenerated"))
          continue;
        auto subs = substitutions[ai];
        auto newName = ctx->generateCanonicalName(
            ctx->cache->reverseIdentifierLookup[f->name], true);
        auto nf = std::dynamic_pointer_cast<FunctionStmt>(replace(sp, subs));
        subs[nf->name] = N<IdExpr>(newName);
        nf->name = newName;
        suite->stmts.push_back(nf);
        nf->attributes.parentClass = ctx->bases.back().name;

        // check original ast...
        if (nf->attributes.has(".changedSelf"))
          nf->args[0].type = transformType(ctx->bases.back().ast);
        preamble->functions.push_back(clone(nf));
        ctx->cache->functions[newName].ast = nf;
        ctx->cache->classes[ctx->bases.back().name]
            .methods[ctx->cache->reverseIdentifierLookup[f->name]]
            .push_back({newName, nullptr, ctx->cache->age});
      }
  for (auto sp : getClassMethods(stmt->suite))
    if (sp && !sp->getClass()) {
      transform(sp);
      suite->stmts.push_back(preamble->functions.back());
    }
  ctx->bases.pop_back();
  ctx->bases = move(oldBases);
  ctx->popBlock();

  auto c = ctx->cache->classes[canonicalName].ast.get();
  if (!extension) {
    // Update the cached AST.
    seqassert(c, "not a class AST for {}", canonicalName);
    preamble->globals.push_back(c->clone());
    c->suite = clone(suite);
    // if (stmt->baseClasses.size())
    // LOG("{} -> {}", stmt->name, c->toString(0));
  }
  stmts[0] = N<ClassStmt>(canonicalName, vector<Param>{}, N<SuiteStmt>(),
                          Attr({Attr::Extend}), vector<ExprPtr>{}, vector<ExprPtr>{});
  resultStmt = N<SuiteStmt>(stmts);
}

void SimplifyVisitor::visit(CustomStmt *stmt) {
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

/**************************************************************************************/

StmtPtr SimplifyVisitor::transformAssignment(const ExprPtr &lhs, const ExprPtr &rhs,
                                             const ExprPtr &type, bool shadow,
                                             bool mustExist) {
  if (auto ei = lhs->getIndex()) {
    seqassert(!type, "unexpected type annotation");
    return transform(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(ei->expr), "__setitem__"),
                                             clone(ei->index), rhs->clone())));
  } else if (auto ed = lhs->getDot()) {
    seqassert(!type, "unexpected type annotation");
    return N<AssignMemberStmt>(transform(ed->expr), ed->member, transform(rhs, false));
  } else if (auto e = lhs->getId()) {
    ExprPtr t = transformType(type, false);
    if (!shadow && !t) {
      auto val = ctx->find(e->value);
      if (e->value != "_" && val && val->isVar()) {
        if (val->getBase() == ctx->getBase())
          return N<UpdateStmt>(transform(lhs, false), transform(rhs, true),
                               !ctx->bases.empty() &&
                                   ctx->bases.back().attributes & FLAG_ATOMIC);
        else if (mustExist)
          error("variable '{}' is not global", e->value);
      }
    }

    // Function and type aliases are not normal assignments. They are treated like a
    // simple context renames.
    // Note: x = Ptr[byte] is not a simple alias, and is handled separately below.
    auto r = transform(rhs, true);
    if (r && r->getId()) {
      auto val = ctx->find(r->getId()->value);
      if (!val)
        error("cannot find '{}'", r->getId()->value);
      if (val->isType() || val->isFunc()) {
        ctx->add(e->value, val);
        return nullptr;
      }
    }
    // This assignment is a new variable assignment (not a rename or an update).
    // Generate new canonical variable name for this assignment and use it afterwards.
    auto canonical = ctx->generateCanonicalName(e->value);
    auto l = N<IdExpr>(canonical);
    bool global = ctx->isToplevel();
    bool isStatic = t && t->getIndex() && t->getIndex()->expr->isId("Static");
    // ctx->moduleName != MODULE_MAIN;
    // ⚠️ TODO: should we make __main__ top-level variables NOT global by default?
    // Problem: a = [1]; def foo(): a.append(2) won't work anymore as in Python.
    if (global && !isStatic)
      ctx->cache->globals.insert(canonical);
    // Handle type aliases as well!
    ctx->add(r && r->isType() ? SimplifyItem::Type : SimplifyItem::Var, e->value,
             canonical, global);
    if (global && !isStatic) {
      if (r && r->isType()) {
        preamble->globals.push_back(N<AssignStmt>(N<IdExpr>(canonical), clone(r)));
      } else {
        preamble->globals.push_back(N<AssignStmt>(N<IdExpr>(canonical), nullptr, t));
        return r ? N<UpdateStmt>(l, r) : nullptr;
      }
    } else if (isStatic) {
      preamble->globals.push_back(
          N<AssignStmt>(N<IdExpr>(canonical), clone(r), clone(t)));
    }
    return N<AssignStmt>(l, r, t);
  } else {
    error("invalid assignment");
    return nullptr;
  }
}

void SimplifyVisitor::unpackAssignments(ExprPtr lhs, ExprPtr rhs,
                                        vector<StmtPtr> &stmts, bool shadow,
                                        bool mustExist) {
  vector<ExprPtr> leftSide;
  if (auto et = lhs->getTuple()) { // (a, b) = ...
    for (auto &i : et->items)
      leftSide.push_back(i);
  } else if (auto el = lhs->getList()) { // [a, b] = ...
    for (auto &i : el->items)
      leftSide.push_back(i);
  } else { // A simple assignment.
    stmts.push_back(transformAssignment(lhs, rhs, nullptr, shadow, mustExist));
    return;
  }

  // Prepare the right-side expression
  auto srcPos = rhs.get();
  ExprPtr newRhs = nullptr; // This expression must not be deleted until the very end.
  if (!rhs->getId()) { // Store any non-trivial right-side expression (assign = rhs).
    auto var = ctx->cache->getTemporaryVar("assign");
    newRhs = Nx<IdExpr>(srcPos, var);
    stmts.push_back(transformAssignment(newRhs, rhs, nullptr, shadow, mustExist));
    rhs = newRhs;
  }

  // Process each assignment until the fist StarExpr (if any).
  int st;
  for (st = 0; st < leftSide.size(); st++) {
    if (leftSide[st]->getStar())
      break;
    // Transformation: leftSide_st = rhs[st]
    auto rightSide = Nx<IndexExpr>(srcPos, rhs->clone(), Nx<IntExpr>(srcPos, st));
    // Recursively process the assignment (as we can have cases like (a, (b, c)) = d).
    unpackAssignments(leftSide[st], rightSide, stmts, shadow, mustExist);
  }
  // If there is a StarExpr, process it and the remaining assignments after it (if
  // any).
  if (st < leftSide.size() && leftSide[st]->getStar()) {
    // StarExpr becomes SliceExpr: in (a, *b, c) = d, b is d[1:-2]
    auto rightSide = Nx<IndexExpr>(
        srcPos, rhs->clone(),
        Nx<SliceExpr>(srcPos, Nx<IntExpr>(srcPos, st),
                      // This slice is either [st:] or [st:-lhs_len + st + 1]
                      leftSide.size() == st + 1
                          ? nullptr
                          : Nx<IntExpr>(srcPos, -leftSide.size() + st + 1),
                      nullptr));
    unpackAssignments(leftSide[st]->getStar()->what, rightSide, stmts, shadow,
                      mustExist);
    st += 1;
    // Keep going till the very end. Remaining assignments use negative indices (-1,
    // -2 etc) as we are not sure how big is StarExpr.
    for (; st < leftSide.size(); st++) {
      if (leftSide[st]->getStar())
        error(leftSide[st], "multiple unpack expressions");
      rightSide = Nx<IndexExpr>(srcPos, rhs->clone(),
                                Nx<IntExpr>(srcPos, -leftSide.size() + st));
      unpackAssignments(leftSide[st], rightSide, stmts, shadow, mustExist);
    }
  }
}

StmtPtr SimplifyVisitor::transformPattern(ExprPtr var, ExprPtr pattern, StmtPtr suite) {
  auto isinstance = [&](const ExprPtr &e, const string &typ) -> ExprPtr {
    return N<CallExpr>(N<IdExpr>("isinstance"), e->clone(), N<IdExpr>(typ));
  };
  auto findEllipsis = [&](const vector<ExprPtr> &items) {
    int i = items.size();
    for (int it = 0; it < items.size(); it++)
      if (items[it]->getEllipsis()) {
        if (i != items.size())
          error("cannot have multiple ranges in a pattern");
        i = it;
      }
    return i;
  };

  if (pattern->getInt() || CAST(pattern, BoolExpr)) {
    return N<IfStmt>(isinstance(var, CAST(pattern, BoolExpr) ? "bool" : "int"),
                     N<IfStmt>(N<BinaryExpr>(var->clone(), "==", pattern), suite));
  } else if (auto er = CAST(pattern, RangeExpr)) {
    return N<IfStmt>(
        isinstance(var, "int"),
        N<IfStmt>(
            N<BinaryExpr>(var->clone(), ">=", clone(er->start)),
            N<IfStmt>(N<BinaryExpr>(var->clone(), "<=", clone(er->stop)), suite)));
  } else if (auto et = pattern->getTuple()) {
    for (int it = int(et->items.size()) - 1; it >= 0; it--)
      suite = transformPattern(N<IndexExpr>(var->clone(), N<IntExpr>(it)),
                               clone(et->items[it]), suite);
    return N<IfStmt>(
        isinstance(var, "Tuple"),
        N<IfStmt>(N<BinaryExpr>(N<CallExpr>(N<IdExpr>("staticlen"), clone(var)),
                                "==", N<IntExpr>(et->items.size())),
                  suite));
  } else if (auto el = pattern->getList()) {
    auto ellipsis = findEllipsis(el->items), sz = int(el->items.size());
    string op;
    if (ellipsis == el->items.size())
      op = "==";
    else
      op = ">=", sz -= 1;
    for (int it = int(el->items.size()) - 1; it > ellipsis; it--)
      suite = transformPattern(
          N<IndexExpr>(var->clone(), N<IntExpr>(it - el->items.size())),
          clone(el->items[it]), suite);
    for (int it = ellipsis - 1; it >= 0; it--)
      suite = transformPattern(N<IndexExpr>(var->clone(), N<IntExpr>(it)),
                               clone(el->items[it]), suite);
    return N<IfStmt>(isinstance(var, "List"),
                     N<IfStmt>(N<BinaryExpr>(N<CallExpr>(N<IdExpr>("len"), clone(var)),
                                             op, N<IntExpr>(sz)),
                               suite));
  } else if (auto eb = pattern->getBinary()) {
    if (eb->op == "|") {
      return N<SuiteStmt>(transformPattern(clone(var), clone(eb->lexpr), clone(suite)),
                          transformPattern(clone(var), clone(eb->rexpr), suite));
    }
  } else if (auto ea = CAST(pattern, AssignExpr)) {
    seqassert(ea->var->getId(), "only simple assignment expressions are supported");
    return N<SuiteStmt>(
        vector<StmtPtr>{N<AssignStmt>(clone(ea->var), clone(var)),
                        transformPattern(clone(var), clone(ea->expr), clone(suite))},
        true);
  } else if (auto ei = pattern->getId()) {
    if (ei->value != "_")
      return N<SuiteStmt>(
          vector<StmtPtr>{N<AssignStmt>(clone(pattern), clone(var)), suite}, true);
    else
      return suite;
  }
  pattern = transform(pattern); // basically check for errors
  return N<IfStmt>(
      N<CallExpr>(N<IdExpr>("hasattr"), var->clone(), N<StringExpr>("__match__"),
                  N<CallExpr>(N<IdExpr>("type"), pattern->clone())),
      N<IfStmt>(N<CallExpr>(N<DotExpr>(var->clone(), "__match__"), pattern), suite));
}

StmtPtr SimplifyVisitor::transformCImport(const string &name, const vector<Param> &args,
                                          const Expr *ret, const string &altName) {
  vector<Param> fnArgs;
  auto attr = Attr({Attr::C});
  for (int ai = 0; ai < args.size(); ai++) {
    seqassert(args[ai].name.empty(), "unexpected argument name");
    seqassert(!args[ai].deflt, "unexpected default argument");
    seqassert(args[ai].type, "missing type");
    if (dynamic_cast<EllipsisExpr *>(args[ai].type.get()) && ai + 1 == args.size()) {
      attr.set(Attr::CVarArg);
      fnArgs.emplace_back(Param{"*args", nullptr, nullptr});
    } else {
      fnArgs.emplace_back(
          Param{args[ai].name.empty() ? format("a{}", ai) : args[ai].name,
                args[ai].type->clone(), nullptr});
    }
  }
  auto f = N<FunctionStmt>(name, ret ? ret->clone() : N<IdExpr>("void"), fnArgs,
                           nullptr, attr);
  StmtPtr tf = transform(f); // Already in the preamble
  if (!altName.empty())
    ctx->add(altName, ctx->find(name));
  return tf;
}

StmtPtr SimplifyVisitor::transformCDLLImport(const Expr *dylib, const string &name,
                                             const vector<Param> &args, const Expr *ret,
                                             const string &altName) {
  // name : Function[args] = _dlsym(dylib, "name", Fn=Function[args])
  vector<ExprPtr> fnArgs{N<ListExpr>(vector<ExprPtr>{}),
                         ret ? ret->clone() : N<IdExpr>("void")};
  for (const auto &a : args) {
    seqassert(a.name.empty(), "unexpected argument name");
    seqassert(!a.deflt, "unexpected default argument");
    seqassert(a.type, "missing type");
    const_cast<ListExpr *>(fnArgs[0]->getList())->items.emplace_back(clone(a.type));
  }
  auto type = N<IndexExpr>(N<IdExpr>("Function"), N<TupleExpr>(fnArgs));
  return transform(N<AssignStmt>(
      N<IdExpr>(altName.empty() ? name : altName),
      N<CallExpr>(N<IdExpr>("_dlsym"), vector<CallExpr::Arg>{{"", dylib->clone()},
                                                             {"", N<StringExpr>(name)},
                                                             {"Fn", type}})));
}

StmtPtr SimplifyVisitor::transformPythonImport(const Expr *what,
                                               const vector<Param> &args,
                                               const Expr *ret, const string &altName) {
  // Get a module name (e.g. os.path)
  vector<string> dirs;
  auto e = what;
  while (auto d = e->getDot()) {
    dirs.push_back(d->member);
    e = d->expr.get();
  }
  seqassert(e && e->getId(), "invalid import python statement");
  dirs.push_back(e->getId()->value);
  string name = dirs[0], lib;
  for (int i = int(dirs.size()) - 1; i > 0; i--)
    lib += dirs[i] + (i > 1 ? "." : "");

  // Simple module import: from python import foo
  if (!ret && args.empty())
    // altName = pyobj._import("name")
    return transform(N<AssignStmt>(
        N<IdExpr>(altName.empty() ? name : altName),
        N<CallExpr>(N<DotExpr>("pyobj", "_import"),
                    N<StringExpr>((lib.empty() ? "" : lib + ".") + name))));

  // Typed function import: from python import foo.bar(int) -> float.
  // f = pyobj._import("lib")._getattr("name")
  auto call = N<AssignStmt>(
      N<IdExpr>("f"), N<CallExpr>(N<DotExpr>(N<CallExpr>(N<DotExpr>("pyobj", "_import"),
                                                         N<StringExpr>(lib)),
                                             "_getattr"),
                                  N<StringExpr>(name)));
  // Make a call expression: f(args...)
  vector<Param> params;
  vector<ExprPtr> callArgs;
  for (int i = 0; i < args.size(); i++) {
    params.emplace_back(Param{format("a{}", i), clone(args[i].type), nullptr});
    callArgs.emplace_back(N<IdExpr>(format("a{}", i)));
  }
  // Make a return expression: return f(args...),
  // or return retType.__from_py__(f(args...))
  ExprPtr retExpr = N<CallExpr>(N<IdExpr>("f"), callArgs);
  if (ret && !ret->isId("void"))
    retExpr = N<CallExpr>(N<DotExpr>(ret->clone(), "__from_py__"), retExpr);
  StmtPtr retStmt = nullptr;
  if (ret && ret->isId("void"))
    retStmt = N<ExprStmt>(retExpr);
  else
    retStmt = N<ReturnStmt>(retExpr);
  // Return a wrapper function
  return transform(N<FunctionStmt>(altName.empty() ? name : altName,
                                   ret ? ret->clone() : nullptr, params,
                                   N<SuiteStmt>(call, retStmt)));
}

void SimplifyVisitor::transformNewImport(const ImportFile &file) {
  // Use a clean context to parse a new file.
  if (ctx->cache->age)
    ctx->cache->age++;
  auto ictx = make_shared<SimplifyContext>(file.path, ctx->cache);
  ictx->isStdlibLoading = ctx->isStdlibLoading;
  ictx->moduleName = file;
  auto import = ctx->cache->imports.insert({file.path, {file.path, ictx}}).first;
  // __name__ = <import name> (set the Python's __name__ variable)
  auto sn =
      SimplifyVisitor(ictx, preamble)
          .transform(N<SuiteStmt>(N<AssignStmt>(N<IdExpr>("__name__"),
                                                N<StringExpr>(ictx->moduleName.module),
                                                N<IdExpr>("str"), true),
                                  parseFile(ctx->cache, file.path)));

  // If we are loading standard library, we won't wrap imports in functions as we
  // assume that standard library has no recursive imports. We will just append the
  // top-level statements as-is.
  if (ctx->isStdlibLoading) {
    resultStmt = N<SuiteStmt>(vector<StmtPtr>{sn}, true);
  } else {
    // Generate import function identifier.
    string importVar = import->second.importVar =
               ctx->cache->getTemporaryVar(format("import_{}", file.module), '.'),
           importDoneVar;
    // import_done = False (global variable that indicates if an import has been
    // loaded)
    preamble->globals.push_back(N<AssignStmt>(
        N<IdExpr>(importDoneVar = importVar + "_done"), N<BoolExpr>(false)));
    ctx->cache->globals.insert(importDoneVar);
    vector<StmtPtr> stmts;
    stmts.push_back(nullptr); // placeholder to be filled later!
    // We need to wrap all imported top-level statements (not signatures! they have
    // already been handled and are in the preamble) into a function. We also take the
    // list of global variables so that we can access them via "global" statement.
    auto processStmt = [&](StmtPtr s) {
      if (s->getAssign() && s->getAssign()->lhs->getId()) { // a = ... globals
        auto a = const_cast<AssignStmt *>(s->getAssign());
        bool isStatic =
            a->type && a->type->getIndex() && a->type->getIndex()->expr->isId("Static");
        auto val = ictx->find(a->lhs->getId()->value);
        seqassert(val, "cannot locate '{}' in imported file {}",
                  s->getAssign()->lhs->getId()->value, file.path);
        if (val->kind == SimplifyItem::Var && val->global && val->base.empty() &&
            !isStatic) {
          stmts.push_back(N<UpdateStmt>(a->lhs, a->rhs));
        } else {
          stmts.push_back(s);
        }
      } else if (!s->getFunction() && !s->getClass()) {
        stmts.push_back(s);
      }
    };
    if (auto st = const_cast<SuiteStmt *>(sn->getSuite()))
      for (auto &ss : st->stmts)
        processStmt(ss);
    else
      processStmt(sn);
    stmts[0] = N<SuiteStmt>();
    // Add a def import(): ... manually to the cache and to the preamble (it won't be
    // transformed here!).
    ctx->cache->functions[importVar].ast =
        N<FunctionStmt>(importVar, nullptr, vector<Param>{}, N<SuiteStmt>(stmts),
                        Attr({Attr::ForceRealize}));
    preamble->functions.push_back(ctx->cache->functions[importVar].ast->clone());
    ;
  }
}

StmtPtr SimplifyVisitor::transformPythonDefinition(const string &name,
                                                   const vector<Param> &args,
                                                   const Expr *ret,
                                                   const Stmt *codeStmt) {
  seqassert(codeStmt && codeStmt->getExpr() && codeStmt->getExpr()->expr->getString(),
            "invalid Python definition");
  auto code = codeStmt->getExpr()->expr->getString()->getValue();
  vector<string> pyargs;
  for (const auto &a : args)
    pyargs.emplace_back(a.name);
  code = format("def {}({}):\n{}\n", name, join(pyargs, ", "), code);
  return transform(N<SuiteStmt>(
      N<ExprStmt>(N<CallExpr>(N<DotExpr>("pyobj", "_exec"), N<StringExpr>(code))),
      N<ImportStmt>(N<IdExpr>("python"), N<DotExpr>("__main__", name), clone_nop(args),
                    ret ? ret->clone() : N<IdExpr>("pyobj"))));
}

StmtPtr SimplifyVisitor::transformLLVMDefinition(const Stmt *codeStmt) {
  seqassert(codeStmt && codeStmt->getExpr() && codeStmt->getExpr()->expr->getString(),
            "invalid LLVM definition");

  auto code = codeStmt->getExpr()->expr->getString()->getValue();
  vector<StmtPtr> items;
  auto se = N<StringExpr>("");
  string finalCode = se->getValue();
  items.push_back(N<ExprStmt>(se));

  int braceCount = 0, braceStart = 0;
  for (int i = 0; i < code.size(); i++) {
    if (i < code.size() - 1 && code[i] == '{' && code[i + 1] == '=') {
      if (braceStart < i)
        finalCode += escapeFStringBraces(code, braceStart, i - braceStart) + '{';
      if (!braceCount) {
        braceStart = i + 2;
        braceCount++;
      } else {
        error("invalid LLVM substitution");
      }
    } else if (braceCount && code[i] == '}') {
      braceCount--;
      string exprCode = code.substr(braceStart, i - braceStart);
      auto offset = getSrcInfo();
      offset.col += i;
      auto expr = transform(parseExpr(ctx->cache, exprCode, offset), true);
      items.push_back(N<ExprStmt>(expr));
      braceStart = i + 1;
      finalCode += '}';
    }
  }
  if (braceCount)
    error("invalid LLVM substitution");
  if (braceStart != code.size())
    finalCode += escapeFStringBraces(code, braceStart, int(code.size()) - braceStart);
  se->strings[0].first = finalCode;
  return N<SuiteStmt>(items);
}

StmtPtr SimplifyVisitor::codegenMagic(const string &op, const Expr *typExpr,
                                      const vector<Param> &args, bool isRecord) {
#define I(s) N<IdExpr>(s)
  assert(typExpr);
  ExprPtr ret;
  vector<Param> fargs;
  vector<StmtPtr> stmts;
  Attr attr;
  attr.set("autogenerated");
  if (op == "new") {
    // Classes: @internal def __new__() -> T
    // Tuples: @internal def __new__(a1: T1, ..., aN: TN) -> T
    ret = typExpr->clone();
    if (isRecord)
      for (auto &a : args)
        fargs.emplace_back(
            Param{a.name, clone(a.type),
                  a.deflt ? clone(a.deflt) : N<CallExpr>(clone(a.type))});
    attr.set(Attr::Internal);
  } else if (op == "init") {
    // Classes: def __init__(self: T, a1: T1, ..., aN: TN) -> void:
    //            self.aI = aI ...
    ret = I("void");
    fargs.emplace_back(Param{"self", typExpr->clone()});
    for (auto &a : args) {
      stmts.push_back(N<AssignStmt>(N<DotExpr>(I("self"), a.name), I(a.name)));
      fargs.emplace_back(Param{a.name, clone(a.type),
                               a.deflt ? clone(a.deflt) : N<CallExpr>(clone(a.type))});
    }
  } else if (op == "raw") {
    // Classes: def __raw__(self: T) -> Ptr[byte]:
    //            return __internal__.class_raw(self)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = N<IndexExpr>(I("Ptr"), I("byte"));
    stmts.emplace_back(N<ReturnStmt>(
        N<CallExpr>(N<DotExpr>(I("__internal__"), "class_raw"), I("self"))));
  } else if (op == "getitem") {
    // Tuples: def __getitem__(self: T, index: int) -> T1:
    //           return __internal__.tuple_getitem[T, T1](self, index)
    //         (error during a realizeFunc() method if T is a heterogeneous tuple)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"index", I("int")});
    ret = !args.empty() ? clone(args[0].type) : I("void");
    stmts.emplace_back(N<ReturnStmt>(
        N<CallExpr>(N<DotExpr>(I("__internal__"), "tuple_getitem"), I("self"),
                    I("index"), typExpr->clone(), ret->clone())));
  } else if (op == "iter") {
    // Tuples: def __iter__(self: T) -> Generator[T]:
    //           yield self.aI ...
    //         (error during a realizeFunc() method if T is a heterogeneous tuple)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = N<IndexExpr>(I("Generator"), !args.empty() ? clone(args[0].type) : I("int"));
    for (auto &a : args)
      stmts.emplace_back(N<YieldStmt>(N<DotExpr>("self", a.name)));
    if (args.empty()) // Hack for empty tuple: yield from List[int]()
      stmts.emplace_back(
          N<YieldFromStmt>(N<CallExpr>(N<IndexExpr>(I("List"), I("int")))));
  } else if (op == "contains") {
    // Tuples: def __contains__(self: T, what) -> bool:
    //            if isinstance(what, T1): if what == self.a1: return True ...
    //            return False
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"what", nullptr});
    ret = I("bool");
    for (auto &a : args)
      stmts.push_back(N<IfStmt>(N<CallExpr>(I("isinstance"), I("what"), clone(a.type)),
                                N<IfStmt>(N<CallExpr>(N<DotExpr>(I("what"), "__eq__"),
                                                      N<DotExpr>(I("self"), a.name)),
                                          N<ReturnStmt>(N<BoolExpr>(true)))));
    stmts.emplace_back(N<ReturnStmt>(N<BoolExpr>(false)));
  } else if (op == "eq") {
    // def __eq__(self: T, other: T) -> bool:
    //   if not self.arg1.__eq__(other.arg1): return False ...
    //   return True
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"other", typExpr->clone()});
    ret = I("bool");
    for (auto &a : args)
      stmts.push_back(N<IfStmt>(
          N<UnaryExpr>("!",
                       N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), a.name), "__eq__"),
                                   N<DotExpr>(I("other"), a.name))),
          N<ReturnStmt>(N<BoolExpr>(false))));
    stmts.emplace_back(N<ReturnStmt>(N<BoolExpr>(true)));
  } else if (op == "ne") {
    // def __ne__(self: T, other: T) -> bool:
    //   if self.arg1.__ne__(other.arg1): return True ...
    //   return False
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"other", typExpr->clone()});
    ret = I("bool");
    for (auto &a : args)
      stmts.emplace_back(
          N<IfStmt>(N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), a.name), "__ne__"),
                                N<DotExpr>(I("other"), a.name)),
                    N<ReturnStmt>(N<BoolExpr>(true))));
    stmts.push_back(N<ReturnStmt>(N<BoolExpr>(false)));
  } else if (op == "lt" || op == "gt") {
    // def __lt__(self: T, other: T) -> bool:  (same for __gt__)
    //   if self.arg1.__lt__(other.arg1): return True
    //   elif self.arg1.__eq__(other.arg1):
    //      ... (arg2, ...) ...
    //   return False
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"other", typExpr->clone()});
    ret = I("bool");
    vector<StmtPtr> *v = &stmts;
    for (int i = 0; i < (int)args.size() - 1; i++) {
      v->emplace_back(N<IfStmt>(
          N<CallExpr>(
              N<DotExpr>(N<DotExpr>(I("self"), args[i].name), format("__{}__", op)),
              N<DotExpr>(I("other"), args[i].name)),
          N<ReturnStmt>(N<BoolExpr>(true)),
          N<IfStmt>(
              N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name), "__eq__"),
                          N<DotExpr>(I("other"), args[i].name)),
              N<SuiteStmt>())));
      v = &((SuiteStmt *)(((IfStmt *)(((IfStmt *)(v->back().get()))->elseSuite.get()))
                              ->ifSuite)
                .get())
               ->stmts;
    }
    if (!args.empty())
      v->emplace_back(N<ReturnStmt>(N<CallExpr>(
          N<DotExpr>(N<DotExpr>(I("self"), args.back().name), format("__{}__", op)),
          N<DotExpr>(I("other"), args.back().name))));
    stmts.emplace_back(N<ReturnStmt>(N<BoolExpr>(false)));
  } else if (op == "le" || op == "ge") {
    // def __le__(self: T, other: T) -> bool:  (same for __ge__)
    //   if not self.arg1.__le__(other.arg1): return False
    //   elif self.arg1.__eq__(other.arg1):
    //      ... (arg2, ...) ...
    //   return True
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"other", typExpr->clone()});
    ret = I("bool");
    vector<StmtPtr> *v = &stmts;
    for (int i = 0; i < (int)args.size() - 1; i++) {
      v->emplace_back(N<IfStmt>(
          N<UnaryExpr>("!", N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name),
                                                   format("__{}__", op)),
                                        N<DotExpr>(I("other"), args[i].name))),
          N<ReturnStmt>(N<BoolExpr>(false)),
          N<IfStmt>(
              N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name), "__eq__"),
                          N<DotExpr>(I("other"), args[i].name)),
              N<SuiteStmt>())));
      v = &((SuiteStmt *)(((IfStmt *)(((IfStmt *)(v->back().get()))->elseSuite.get()))
                              ->ifSuite)
                .get())
               ->stmts;
    }
    if (!args.empty())
      v->emplace_back(N<ReturnStmt>(N<CallExpr>(
          N<DotExpr>(N<DotExpr>(I("self"), args.back().name), format("__{}__", op)),
          N<DotExpr>(I("other"), args.back().name))));
    stmts.emplace_back(N<ReturnStmt>(N<BoolExpr>(true)));
  } else if (op == "hash") {
    // def __hash__(self: T) -> int:
    //   seed = 0
    //   seed = (
    //     seed ^ ((self.arg1.__hash__() + 2654435769) + ((seed << 6) + (seed >> 2)))
    //   ) ...
    //   return seed
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = I("int");
    stmts.emplace_back(N<AssignStmt>(I("seed"), N<IntExpr>(0)));
    for (auto &a : args)
      stmts.push_back(N<AssignStmt>(
          I("seed"),
          N<BinaryExpr>(
              I("seed"), "^",
              N<BinaryExpr>(
                  N<BinaryExpr>(N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), a.name),
                                                       "__hash__")),
                                "+", N<IntExpr>(0x9e3779b9)),
                  "+",
                  N<BinaryExpr>(N<BinaryExpr>(I("seed"), "<<", N<IntExpr>(6)), "+",
                                N<BinaryExpr>(I("seed"), ">>", N<IntExpr>(2)))))));
    stmts.emplace_back(N<ReturnStmt>(I("seed")));
  } else if (op == "pickle") {
    // def __pickle__(self: T, dest: Ptr[byte]) -> void:
    //   self.arg1.__pickle__(dest) ...
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"dest", N<IndexExpr>(I("Ptr"), I("byte"))});
    ret = I("void");
    for (auto &a : args)
      stmts.emplace_back(N<ExprStmt>(N<CallExpr>(
          N<DotExpr>(N<DotExpr>(I("self"), a.name), "__pickle__"), I("dest"))));
  } else if (op == "unpickle") {
    // def __unpickle__(src: Ptr[byte]) -> T:
    //   return T(T1.__unpickle__(src),...)
    fargs.emplace_back(Param{"src", N<IndexExpr>(I("Ptr"), I("byte"))});
    ret = typExpr->clone();
    vector<ExprPtr> ar;
    for (auto &a : args)
      ar.emplace_back(N<CallExpr>(N<DotExpr>(clone(a.type), "__unpickle__"), I("src")));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(typExpr->clone(), ar)));
  } else if (op == "len") {
    // def __len__(self: T) -> int:
    //   return N (number of args)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = I("int");
    stmts.emplace_back(N<ReturnStmt>(N<IntExpr>(args.size())));
  } else if (op == "to_py") {
    // def __to_py__(self: T) -> pyobj:
    //   o = pyobj._tuple_new(N)  (number of args)
    //   o._tuple_set(1, self.arg1.__to_py__()) ...
    //   return o
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = I("pyobj");
    stmts.emplace_back(
        N<AssignStmt>(I("o"), N<CallExpr>(N<DotExpr>(I("pyobj"), "_tuple_new"),
                                          N<IntExpr>(args.size()))));
    for (int i = 0; i < args.size(); i++)
      stmts.push_back(N<ExprStmt>(N<CallExpr>(
          N<DotExpr>(I("o"), "_tuple_set"), N<IntExpr>(i),
          N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name), "__to_py__")))));
    stmts.emplace_back(N<ReturnStmt>(I("o")));
  } else if (op == "from_py") {
    // def __from_py__(src: pyobj) -> T:
    //   return T(T1.__from_py__(src._tuple_get(1)), ...)
    fargs.emplace_back(Param{"src", I("pyobj")});
    ret = typExpr->clone();
    vector<ExprPtr> ar;
    for (int i = 0; i < args.size(); i++)
      ar.push_back(
          N<CallExpr>(N<DotExpr>(clone(args[i].type), "__from_py__"),
                      N<CallExpr>(N<DotExpr>(I("src"), "_tuple_get"), N<IntExpr>(i))));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(typExpr->clone(), ar)));
  } else if (op == "str") {
    // def __str__(self: T) -> str:
    //   a = __array__[str](N)  (number of args)
    //   n = __array__[str](N)  (number of args)
    //   a.__setitem__(0, self.arg1.__str__()) ...
    //   n.__setitem__(0, "arg1") ...  (if not a Tuple.N; otherwise "")
    //   return __internal__.tuple_str(a.ptr, n.ptr, N)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = I("str");
    if (!args.empty()) {
      stmts.emplace_back(
          N<AssignStmt>(I("a"), N<CallExpr>(N<IndexExpr>(I("__array__"), I("str")),
                                            N<IntExpr>(args.size()))));
      stmts.emplace_back(
          N<AssignStmt>(I("n"), N<CallExpr>(N<IndexExpr>(I("__array__"), I("str")),
                                            N<IntExpr>(args.size()))));
      for (int i = 0; i < args.size(); i++) {
        stmts.push_back(N<ExprStmt>(N<CallExpr>(
            N<DotExpr>(I("a"), "__setitem__"), N<IntExpr>(i),
            N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name), "__str__")))));

        auto name = typExpr->getIndex() ? typExpr->getIndex()->expr->getId() : nullptr;
        stmts.push_back(N<ExprStmt>(N<CallExpr>(
            N<DotExpr>(I("n"), "__setitem__"), N<IntExpr>(i),
            N<StringExpr>(
                name && startswith(name->value, TYPE_TUPLE) ? "" : args[i].name))));
      }
      stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(
          N<DotExpr>(I("__internal__"), "tuple_str"), N<DotExpr>(I("a"), "ptr"),
          N<DotExpr>(I("n"), "ptr"), N<IntExpr>(args.size()))));
    } else {
      stmts.emplace_back(N<ReturnStmt>(N<StringExpr>("()")));
    }
  } else if (op == "dict") {
    // def __dict__(self: T):
    //   d = List[str](N)
    //   d.append('arg1')  ...
    //   return d
    fargs.emplace_back(Param{"self", typExpr->clone()});
    stmts.emplace_back(
        N<AssignStmt>(I("d"), N<CallExpr>(N<IndexExpr>(I("List"), I("str")),
                                          N<IntExpr>(args.size()))));
    for (auto &a : args)
      stmts.push_back(N<ExprStmt>(
          N<CallExpr>(N<DotExpr>(I("d"), "append"), N<StringExpr>(a.name))));
    stmts.emplace_back(N<ReturnStmt>(I("d")));
  } else if (op == "add") {
    // def __add__(self, tup):
    //   return (*self, *t)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"tup", nullptr});
    stmts.emplace_back(N<ReturnStmt>(
        N<TupleExpr>(vector<ExprPtr>{N<StarExpr>(I("self")), N<StarExpr>(I("tup"))})));
  } else {
    seqassert(false, "invalid magic {}", op);
  }
#undef I
  auto t = make_shared<FunctionStmt>(format("__{}__", op), ret, fargs,
                                     N<SuiteStmt>(stmts), attr);
  t->setSrcInfo(ctx->cache->generateSrcInfo());
  return t;
}

vector<StmtPtr> SimplifyVisitor::getClassMethods(const StmtPtr &s) {
  vector<StmtPtr> v;
  if (!s)
    return v;
  if (auto sp = s->getSuite()) {
    for (const auto &ss : sp->stmts)
      for (auto u : getClassMethods(ss))
        v.push_back(u);
  } else if (s->getExpr() && s->getExpr()->expr->getString()) {
    /// Those are doc-strings, ignore them.
  } else if (!s->getFunction() && !s->getClass()) {
    error("only function and class definitions are allowed within classes");
  } else {
    v.push_back(s);
  }
  return v;
}

} // namespace ast
} // namespace seq
