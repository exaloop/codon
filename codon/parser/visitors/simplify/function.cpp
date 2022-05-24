#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

AssignReplacementVisitor::AssignReplacementVisitor(
    Cache *c, std::map<std::string, std::pair<std::string, bool>> &r)
    : cache(c), replacements(r) {}

void AssignReplacementVisitor::visit(IdExpr *expr) {
  while (in(replacements, expr->value))
    expr->value = replacements[expr->value].first;
}

void AssignReplacementVisitor::transform(ExprPtr &e) {
  if (!e)
    return;
  AssignReplacementVisitor v{cache, replacements};
  e->accept(v);
}

void AssignReplacementVisitor::visit(ForStmt *stmt) {
  seqassertn(stmt->var->getId(), "corrupt ForStmt");
  bool addGuard = in(replacements, stmt->var->getId()->value);
  transform(stmt->var);
  auto var = stmt->var->getId()->value;
  transform(stmt->iter);
  transform(stmt->suite);
  if (addGuard) {
    stmt->suite = std::make_shared<SuiteStmt>(
        std::make_shared<UpdateStmt>(
            std::make_shared<IdExpr>(format("{}.__used__", var)),
            std::make_shared<BoolExpr>(true)),
        stmt->suite);
  }
  transform(stmt->elseSuite);
  transform(stmt->decorator);
  for (auto &a : stmt->ompArgs)
    transform(a.value);
}

void AssignReplacementVisitor::visit(TryStmt *stmt) {
  transform(stmt->suite);
  for (auto &c : stmt->catches) {
    bool addGuard = !c.var.empty() && in(replacements, c.var);
    transform(c.exc);
    transform(c.suite);
    if (addGuard) {
      while (in(replacements, c.var))
        c.var = replacements[c.var].first;
      c.suite = std::make_shared<SuiteStmt>(
          std::make_shared<UpdateStmt>(
              std::make_shared<IdExpr>(format("{}.__used__", c.var)),
              std::make_shared<BoolExpr>(true)),
          c.suite);
    }
  }
  transform(stmt->finally);
}

void AssignReplacementVisitor::transform(StmtPtr &e) {
  if (!e)
    return;
  AssignReplacementVisitor v{cache, replacements};
  if (auto i = CAST(e, AssignStmt)) {
    if (i->lhs->getId() && in(replacements, i->lhs->getId()->value)) {
      auto value = i->lhs->getId()->value;
      bool hasUsed = false;
      while (in(replacements, value)) {
        hasUsed = replacements[value].second;
        value = replacements[value].first;
      }

      e = std::make_shared<UpdateStmt>(i->lhs, i->rhs);
      e->setSrcInfo(i->getSrcInfo());

      auto lb = i->lhs->clone();
      lb->getId()->value = fmt::format("{}.__used__", value);
      auto f = std::make_shared<UpdateStmt>(lb, std::make_shared<BoolExpr>(true));
      f->setSrcInfo(i->getSrcInfo());

      if (i->rhs && hasUsed)
        e = std::make_shared<SuiteStmt>(e, f);
      else if (i->rhs)
        e = std::make_shared<SuiteStmt>(e);
      else if (hasUsed)
        e = std::make_shared<SuiteStmt>(f);
      else
        e = nullptr;
      if (e)
        e->setSrcInfo(i->getSrcInfo());
    }
  }
  if (e)
    e->accept(v);
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

void SimplifyVisitor::visit(GlobalStmt *stmt) {
  if (!ctx->inFunction())
    error("global or nonlocal outside of a function");
  auto val = ctx->findDominatingBinding(stmt->var);
  if (!val || !val->isVar())
    error("identifier '{}' not found", stmt->var);
  if (val->getBase() == ctx->getBase())
    error("identifier '{}' already defined", stmt->var);
  if (!stmt->nonLocal && !val->getBase().empty())
    error("not a global variable");
  else if (stmt->nonLocal && val->getBase().empty())
    error("not a nonlocal variable");
  seqassert(!val->canonicalName.empty(), "'{}' does not have a canonical name",
            stmt->var);
  if (!stmt->nonLocal && !in(ctx->cache->globals, val->canonicalName))
    ctx->cache->globals[val->canonicalName] = nullptr;
  // TODO: capture otherwise?
  val = ctx->addVar(stmt->var, val->canonicalName, stmt->getSrcInfo());
  // val->scope = ctx->scope;
  val->base = ctx->getBase();
  val->noShadow = true;
}

void SimplifyVisitor::visit(FunctionStmt *stmt) {
  std::vector<ExprPtr> decorators;
  Attr attr = stmt->attributes;
  for (auto &d : stmt->decorators) {
    if (d->isId("__attribute__")) {
      if (stmt->decorators.size() != 1)
        error("__attribute__ cannot be mixed with other decorators");
      attr.isAttribute = true;
    } else if (d->isId(Attr::LLVM)) {
      attr.set(Attr::LLVM);
    } else if (d->isId(Attr::Python)) {
      attr.set(Attr::Python);
    } else if (d->isId(Attr::Internal)) {
      attr.set(Attr::Internal);
    } else if (d->isId(Attr::Atomic)) {
      attr.set(Attr::Atomic);
    } else if (d->isId(Attr::Property)) {
      attr.set(Attr::Property);
    } else if (d->isId(Attr::ForceRealize)) {
      attr.set(Attr::ForceRealize);
    } else {
      // Let's check if this is a attribute
      auto dt = transform(clone(d));
      if (dt && dt->getId()) {
        auto ci = ctx->find(dt->getId()->value);
        if (ci && ci->isFunc()) {
          if (ctx->cache->overloads[ci->canonicalName].size() == 1)
            if (ctx->cache->functions[ctx->cache->overloads[ci->canonicalName][0].name]
                    .ast->attributes.isAttribute) {
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
  bool overload = attr.has(Attr::Overload);
  bool isClassMember = ctx->inClass();
  std::string rootName;
  if (isClassMember) {
    auto &m = ctx->cache->classes[ctx->bases.back().name].methods;
    auto i = m.find(stmt->name);
    if (i != m.end())
      rootName = i->second;
  } else if (overload) {
    if (auto c = ctx->find(stmt->name))
      if (c->isFunc() && c->getModule() == ctx->getModule() &&
          c->getBase() == ctx->getBase())
        rootName = c->canonicalName;
  }
  if (rootName.empty())
    rootName = ctx->generateCanonicalName(stmt->name, true);
  auto canonicalName =
      format("{}:{}", rootName, ctx->cache->overloads[rootName].size());
  ctx->cache->reverseIdentifierLookup[canonicalName] = stmt->name;
  bool isEnclosedFunc = ctx->inFunction();

  if (attr.has(Attr::ForceRealize) && (ctx->getLevel() || isClassMember))
    error("builtins must be defined at the toplevel");

  if (!isClassMember) {
    auto funcVal = ctx->find(stmt->name);
    if (funcVal && funcVal->noShadow)
      error("cannot update global/nonlocal");
    funcVal = ctx->addFunc(stmt->name, rootName, stmt->getSrcInfo());
    ctx->addAlwaysVisible(funcVal);
  }
  ctx->bases.emplace_back(SimplifyContext::Base{canonicalName}); // Add new base...
  if (isClassMember && ctx->bases[ctx->bases.size() - 2].deducedMembers)
    ctx->bases.back().deducedMembers = ctx->bases[ctx->bases.size() - 2].deducedMembers;
  ctx->addBlock(); // ... and a block!
  // Set atomic flag if @atomic attribute is present.
  if (attr.has(Attr::Atomic))
    ctx->bases.back().attributes |= FLAG_ATOMIC;
  if (attr.has(Attr::Test))
    ctx->bases.back().attributes |= FLAG_TEST;
  // Add generic identifiers to the context
  std::unordered_set<std::string> seenArgs;
  // Parse function arguments and add them to the context.
  std::vector<Param> args;
  bool defaultsStarted = false, hasStarArg = false, hasKwArg = false;
  // Add generics first
  for (int ia = 0; ia < stmt->args.size(); ia++) {
    auto &a = stmt->args[ia];
    // check if this is a generic!
    if (a.type && (a.type->isId("type") || a.type->isId("TypeVar") ||
                   (a.type->getIndex() && a.type->getIndex()->expr->isId("Static"))))
      a.generic = true;
    std::string varName = a.name;
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

    auto name = ctx->generateCanonicalName(varName);

    auto typeAst = a.type;
    if (!typeAst && isClassMember && ia == 0 && a.name == "self") {
      typeAst = ctx->bases[ctx->bases.size() - 2].ast;
      ctx->bases.back().selfName = name;
      attr.set(".changedSelf");
      attr.set(Attr::Method);
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
    auto deflt = a.deflt;
    if (typeAst && typeAst->getIndex() && typeAst->getIndex()->expr->isId("Callable") &&
        deflt && deflt->getNone())
      deflt = N<CallExpr>(N<IdExpr>("NoneType"));
    if (typeAst && (typeAst->isId("type") || typeAst->isId("TypeVar")) && deflt &&
        deflt->getNone())
      deflt = N<IdExpr>("NoneType");
    args.emplace_back(Param{std::string(stars, '*') + name, typeAst, deflt, a.generic});
    // if (deflt) {
    //   defltCanonicalName =
    //       ctx->generateCanonicalName(format("{}.{}", canonicalName, name));
    //   stmts.push_back(N<AssignStmt>(N<IdExpr>(defltCanonicalName), deflt));
    // }
    // args.emplace_back(Param{std::string(stars, '*') + name, typeAst,
    //                         deflt ? N<IdExpr>(defltCanonicalName) : nullptr,
    //                         a.generic});
    if (a.generic) {
      if (a.type->getIndex() && a.type->getIndex()->expr->isId("Static"))
        ctx->addVar(varName, name, stmt->getSrcInfo());
      else
        ctx->addType(varName, name, stmt->getSrcInfo());
    }
  }
  for (auto &a : args) {
    a.type = transformType(a.type, false);
    a.deflt = transform(a.deflt, true);
  }
  // Delay adding to context to prevent "def foo(a, b=a)"
  for (auto &a : args) {
    if (!a.generic) {
      std::string canName = a.name;
      trimStars(canName);
      ctx->addVar(ctx->cache->reverseIdentifierLookup[canName], canName,
                  stmt->getSrcInfo());
    }
  }
  // Parse the return type.
  if (!stmt->ret && (attr.has(Attr::LLVM) || attr.has(Attr::C)))
    error("LLVM functions must have a return type");
  auto ret = transformType(stmt->ret, false);
  // Parse function body.
  StmtPtr suite = nullptr;
  std::map<std::string, std::string> captures;
  if (!attr.has(Attr::Internal) && !attr.has(Attr::C)) {
    ctx->addBlock();
    if (attr.has(Attr::LLVM)) {
      suite = transformLLVMDefinition(stmt->suite->firstInBlock());
    } else if (attr.has(Attr::C)) {
    } else {
      if ((isEnclosedFunc || attr.has(Attr::Capture)) && !isClassMember)
        ctx->captures.emplace_back(std::map<std::string, std::string>{});
      suite = SimplifyVisitor(ctx, preamble).transformInScope(stmt->suite);
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
  ctx->popBlock();
  attr.module =
      format("{}{}", ctx->moduleName.status == ImportFile::STDLIB ? "std::" : "::",
             ctx->moduleName.module);

  if (isClassMember) { // If this is a method...
    // ... set the enclosing class name...
    attr.parentClass = ctx->bases.back().name;
    // ... add the method to class' method list ...
    ctx->cache->classes[ctx->bases.back().name].methods[stmt->name] = rootName;
    // ... and if the function references outer class variable (by definition a
    // generic), mark it as not static as it needs fully instantiated class to be
    // realized. For example, in class A[T]: def foo(): pass, A.foo() can be realized
    // even if T is unknown. However, def bar(): return T() cannot because it needs T
    // (and is thus accordingly marked with ATTR_IS_METHOD).
    if (isMethod)
      attr.set(Attr::Method);
  }
  ctx->cache->overloads[rootName].push_back({canonicalName, ctx->cache->age});

  std::vector<CallExpr::Arg> partialArgs;
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
  // Make sure to cache this (generic) AST for later realization.
  ctx->cache->functions[canonicalName].ast = f;
  AssignReplacementVisitor(ctx->cache, ctx->scopeRenames).transform(f->suite);
  resultStmt = f;

  ExprPtr finalExpr;
  if (!captures.empty())
    finalExpr = N<CallExpr>(N<IdExpr>(stmt->name), partialArgs);
  if (isClassMember && !decorators.empty())
    error("decorators cannot be applied to class methods");
  for (int j = int(decorators.size()) - 1; j >= 0; j--) {
    finalExpr =
        N<CallExpr>(decorators[j], finalExpr ? finalExpr : N<IdExpr>(stmt->name));
  }
  if (finalExpr)
    resultStmt = N<SuiteStmt>(
        resultStmt, transform(N<AssignStmt>(N<IdExpr>(stmt->name), finalExpr)));
}

StmtPtr SimplifyVisitor::transformPythonDefinition(const std::string &name,
                                                   const std::vector<Param> &args,
                                                   const Expr *ret, Stmt *codeStmt) {
  seqassert(codeStmt && codeStmt->getExpr() && codeStmt->getExpr()->expr->getString(),
            "invalid Python definition");
  auto code = codeStmt->getExpr()->expr->getString()->getValue();
  std::vector<std::string> pyargs;
  for (const auto &a : args)
    pyargs.emplace_back(a.name);
  code = format("def {}({}):\n{}\n", name, join(pyargs, ", "), code);
  return transform(N<SuiteStmt>(
      N<ExprStmt>(N<CallExpr>(N<DotExpr>("pyobj", "_exec"), N<StringExpr>(code))),
      N<ImportStmt>(N<IdExpr>("python"), N<DotExpr>("__main__", name), clone_nop(args),
                    ret ? ret->clone() : N<IdExpr>("pyobj"))));
}

StmtPtr SimplifyVisitor::transformLLVMDefinition(Stmt *codeStmt) {
  seqassert(codeStmt && codeStmt->getExpr() && codeStmt->getExpr()->expr->getString(),
            "invalid LLVM definition");

  auto code = codeStmt->getExpr()->expr->getString()->getValue();
  std::vector<StmtPtr> items;
  auto se = N<StringExpr>("");
  std::string finalCode = se->getValue();
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
      std::string exprCode = code.substr(braceStart, i - braceStart);
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

} // namespace codon::ast