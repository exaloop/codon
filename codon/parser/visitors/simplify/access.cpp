// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

void SimplifyVisitor::visit(IdExpr *expr) {
  auto val = ctx->findDominatingBinding(expr->value);

  if (!val && ctx->getBase()->pyCaptures) {
    ctx->getBase()->pyCaptures->insert(expr->value);
    resultExpr = N<IndexExpr>(N<IdExpr>("__pyenv__"), N<StringExpr>(expr->value));
    return;
  } else if (!val) {
    E(Error::ID_NOT_FOUND, expr, expr->value);
  }

  // If we are accessing an outside variable, capture it or raise an error
  auto captured = checkCapture(val);
  if (captured)
    val = ctx->forceFind(expr->value);

  // Track loop variables to dominate them later. Example:
  // x = 1
  // while True:
  //   if x > 10: break
  //   x = x + 1  # x must be dominated after the loop to ensure that it gets updated
  if (ctx->getBase()->getLoop()) {
    for (size_t li = ctx->getBase()->loops.size(); li-- > 0;) {
      auto &loop = ctx->getBase()->loops[li];
      bool inside = val->scope.size() >= loop.scope.size() &&
                    val->scope[loop.scope.size() - 1] == loop.scope.back();
      if (!inside)
        loop.seenVars.insert(expr->value);
      else
        break;
    }
  }

  // Replace the variable with its canonical name
  expr->value = val->canonicalName;

  // Mark global as "seen" to prevent later creation of local variables
  // with the same name. Example:
  // x = 1
  // def foo():
  //   print(x)  # mark x as seen
  //   x = 2     # so that this is an error
  if (!val->isGeneric() && ctx->isOuter(val) &&
      !in(ctx->seenGlobalIdentifiers[ctx->getBaseName()],
          ctx->cache->rev(val->canonicalName))) {
    ctx->seenGlobalIdentifiers[ctx->getBaseName()]
                              [ctx->cache->rev(val->canonicalName)] = expr->clone();
  }

  // Flag the expression as a type expression if it points to a class or a generic
  if (val->isType())
    expr->markType();

  // Variable binding check for variables that are defined within conditional blocks
  if (!val->accessChecked.empty()) {
    bool checked = false;
    for (auto &a : val->accessChecked) {
      if (a.size() <= ctx->scope.blocks.size() &&
          a[a.size() - 1] == ctx->scope.blocks[a.size() - 1]) {
        checked = true;
        break;
      }
    }
    if (!checked) {
      // Prepend access with __internal__.undef([var]__used__, "[var name]")
      auto checkStmt = N<ExprStmt>(N<CallExpr>(
          N<IdExpr>("__internal__.undef"),
          N<IdExpr>(fmt::format("{}.__used__", val->canonicalName)),
          N<StringExpr>(ctx->cache->reverseIdentifierLookup[val->canonicalName])));
      if (!ctx->isConditionalExpr) {
        // If the expression is not conditional, we can just do the check once
        prependStmts->push_back(checkStmt);
        val->accessChecked.push_back(ctx->scope.blocks);
      } else {
        // Otherwise, this check must be always called
        resultExpr = N<StmtExpr>(checkStmt, N<IdExpr>(*expr));
      }
    }
  }
}

/// Flatten imports.
/// @example
///   `a.b.c`      -> canonical name of `c` in `a.b` if `a.b` is an import
///   `a.B.c`      -> canonical name of `c` in class `a.B`
///   `python.foo` -> internal.python._get_identifier("foo")
/// Other cases are handled during the type checking.
void SimplifyVisitor::visit(DotExpr *expr) {
  // First flatten the imports:
  // transform Dot(Dot(a, b), c...) to {a, b, c, ...}
  std::vector<std::string> chain;
  Expr *root = expr;
  for (; root->getDot(); root = root->getDot()->expr.get())
    chain.push_back(root->getDot()->member);

  if (auto id = root->getId()) {
    // Case: a.bar.baz
    chain.push_back(id->value);
    std::reverse(chain.begin(), chain.end());
    auto p = getImport(chain);

    if (!p.second) {
      seqassert(ctx->getBase()->pyCaptures, "unexpected py capture");
      ctx->getBase()->pyCaptures->insert(chain[0]);
      resultExpr = N<IndexExpr>(N<IdExpr>("__pyenv__"), N<StringExpr>(chain[0]));
    } else if (p.second->getModule() == "std.python") {
      resultExpr = transform(N<CallExpr>(
          N<DotExpr>(N<DotExpr>(N<IdExpr>("internal"), "python"), "_get_identifier"),
          N<StringExpr>(chain[p.first++])));
    } else if (p.second->getModule() == ctx->getModule() && p.first == 1) {
      resultExpr = transform(N<IdExpr>(chain[0]), true);
    } else {
      resultExpr = N<IdExpr>(p.second->canonicalName);
      if (p.second->isType() && p.first == chain.size())
        resultExpr->markType();
    }
    for (auto i = p.first; i < chain.size(); i++)
      resultExpr = N<DotExpr>(resultExpr, chain[i]);
  } else {
    // Case: a[x].foo.bar
    transform(expr->expr, true);
  }
}

/// Access identifiers from outside of the current function/class scope.
/// Either use them as-is (globals), capture them if allowed (nonlocals),
/// or raise an error.
bool SimplifyVisitor::checkCapture(const SimplifyContext::Item &val) {
  if (!ctx->isOuter(val))
    return false;
  if ((val->isType() && !val->isGeneric()) || val->isFunc())
    return false;

  // Ensure that outer variables can be captured (i.e., do not cross no-capture
  // boundary). Example:
  // def foo():
  //   x = 1
  //   class T:      # <- boundary (classes cannot capture locals)
  //     t: int = x  # x cannot be accessed
  //     def bar():  # <- another boundary
  //                 # (class methods cannot capture locals except class generics)
  //       print(x)  # x cannot be accessed
  bool crossCaptureBoundary = false;
  bool localGeneric = val->isGeneric() && val->getBaseName() == ctx->getBaseName();
  bool parentClassGeneric =
      val->isGeneric() && !ctx->getBase()->isType() &&
      (ctx->bases.size() > 1 && ctx->bases[ctx->bases.size() - 2].isType() &&
       ctx->bases[ctx->bases.size() - 2].name == val->getBaseName());
  auto i = ctx->bases.size();
  for (; i-- > 0;) {
    if (ctx->bases[i].name == val->getBaseName())
      break;
    if (!localGeneric && !parentClassGeneric && !ctx->bases[i].captures)
      crossCaptureBoundary = true;
  }

  // Mark methods (class functions that access class generics)
  if (parentClassGeneric)
    ctx->getBase()->attributes->set(Attr::Method);

  // Ignore generics
  if (parentClassGeneric || localGeneric)
    return false;

  // Case: a global variable that has not been marked with `global` statement
  if (val->isVar() && val->getBaseName().empty() && val->scope.size() == 1) {
    val->noShadow = true;
    if (!val->isStatic())
      ctx->cache->addGlobal(val->canonicalName);
    return false;
  }

  // Check if a real variable (not a static) is defined outside the current scope
  if (crossCaptureBoundary)
    E(Error::ID_CANNOT_CAPTURE, getSrcInfo(), ctx->cache->rev(val->canonicalName));

  // Case: a nonlocal variable that has not been marked with `nonlocal` statement
  //       and capturing is enabled
  auto captures = ctx->getBase()->captures;
  if (captures && !in(*captures, val->canonicalName)) {
    // Captures are transformed to function arguments; generate new name for that
    // argument
    ExprPtr typ = nullptr;
    if (val->isType())
      typ = N<IdExpr>("type");
    if (auto st = val->isStatic())
      typ = N<IndexExpr>(N<IdExpr>("Static"),
                         N<IdExpr>(st == StaticValue::INT ? "int" : "str"));
    auto [newName, _] = (*captures)[val->canonicalName] = {
        ctx->generateCanonicalName(val->canonicalName), typ};
    ctx->cache->reverseIdentifierLookup[newName] = newName;
    // Add newly generated argument to the context
    std::shared_ptr<SimplifyItem> newVal = nullptr;
    if (val->isType())
      newVal = ctx->addType(ctx->cache->rev(val->canonicalName), newName, getSrcInfo());
    else
      newVal = ctx->addVar(ctx->cache->rev(val->canonicalName), newName, getSrcInfo());
    newVal->baseName = ctx->getBaseName();
    newVal->noShadow = true;
    newVal->scope = ctx->getBase()->scope;
    return true;
  }

  // Case: a nonlocal variable that has not been marked with `nonlocal` statement
  //       and capturing is *not* enabled
  E(Error::ID_NONLOCAL, getSrcInfo(), ctx->cache->rev(val->canonicalName));
  return false;
}

/// Check if a access chain (a.b.c.d...) contains an import or class prefix.
std::pair<size_t, SimplifyContext::Item>
SimplifyVisitor::getImport(const std::vector<std::string> &chain) {
  size_t importEnd = 0;
  std::string importName;

  // Find the longest prefix that corresponds to the existing import
  // (e.g., `a.b.c.d` -> `a.b.c` if there is `import a.b.c`)
  SimplifyContext::Item val = nullptr, importVal = nullptr;
  for (auto i = chain.size(); i-- > 0;) {
    val = ctx->find(join(chain, "/", 0, i + 1));
    if (val && val->isImport()) {
      importVal = val;
      importName = val->importPath, importEnd = i + 1;
      break;
    }
  }

  if (importEnd != chain.size()) { // false when a.b.c points to import itself
    // Find the longest prefix that corresponds to the existing class
    // (e.g., `a.b.c` -> `a.b` if there is `class a: class b:`)
    std::string itemName;
    size_t itemEnd = 0;
    auto fctx = importName.empty() ? ctx : ctx->cache->imports[importName].ctx;
    for (auto i = chain.size(); i-- > importEnd;) {
      if (fctx->getModule() == "std.python" && importEnd < chain.size()) {
        // Special case: importing from Python.
        // Fake SimplifyItem that indicates std.python access
        val = std::make_shared<SimplifyItem>(SimplifyItem::Var, "", "",
                                             fctx->getModule(), std::vector<int>{});
        return {importEnd, val};
      } else {
        val = fctx->find(join(chain, ".", importEnd, i + 1));
        if (val && i + 1 != chain.size() && val->isImport()) {
          importVal = val;
          importName = val->importPath;
          importEnd = i + 1;
          fctx = ctx->cache->imports[importName].ctx;
          i = chain.size();
          continue;
        }
        if (val && (importName.empty() || val->isType() || !val->isConditional())) {
          itemName = val->canonicalName, itemEnd = i + 1;
          break;
        }
      }
    }
    if (itemName.empty() && importName.empty()) {
      if (ctx->getBase()->pyCaptures)
        return {1, nullptr};
      E(Error::IMPORT_NO_MODULE, getSrcInfo(), chain[importEnd]);
    } else if (itemName.empty()) {
      if (!ctx->isStdlibLoading && endswith(importName, "__init__.codon")) {
        auto import = ctx->cache->imports[importName];
        auto file =
            getImportFile(ctx->cache->argv0, chain[importEnd], importName, false,
                          ctx->cache->module0, ctx->cache->pluginImportPaths);
        if (file) {
          auto s = SimplifyVisitor(import.ctx, preamble)
                       .transform(N<ImportStmt>(N<IdExpr>(chain[importEnd]), nullptr));
          prependStmts->push_back(s);
          return getImport(chain);
        }
      }

      E(Error::IMPORT_NO_NAME, getSrcInfo(), chain[importEnd],
        ctx->cache->imports[importName].moduleName);
    }
    importEnd = itemEnd;
  }
  return {importEnd, val};
}

} // namespace codon::ast
