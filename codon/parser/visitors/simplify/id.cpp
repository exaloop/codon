#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(IdExpr *expr) {
  if (ctx->substitutions) {
    auto it = ctx->substitutions->find(expr->value);
    if (it != ctx->substitutions->end()) {
      resultExpr = transform(it->second, true);
      return;
    }
  }

  if (in(std::set<std::string>{"type", "TypeVar", "Callable"}, expr->value)) {
    resultExpr = N<IdExpr>(expr->value == "TypeVar" ? "type" : expr->value);
    resultExpr->markType();
    return;
  }
  auto val = ctx->findDominatingBinding(expr->value);
  if (!val) {
    // ctx->dump();
    error("identifier '{}' not found", expr->value);
  }

  // If we are accessing a nonlocal variable, capture it or raise an error
  bool captured = false;
  bool isClassGeneric = ctx->bases.size() >= 2 &&
                        ctx->bases[ctx->bases.size() - 2].isType() &&
                        ctx->bases[ctx->bases.size() - 2].name == val->getBase();
  auto newName = val->canonicalName;
  if (val->isVar() &&
      (ctx->getBase() != val->getBase() || ctx->getModule() != val->getModule()) &&
      !isClassGeneric) {
    // LOG("-> {} : {} {} | {}", expr->value, ctx->getBase(), val->getBase(),
    //     val->scope.size() == 1 && !in(ctx->cache->globals, val->canonicalName));
    if (val->getBase().empty()) {
      val->noShadow = true;
      if (val->scope.size() == 1 && !in(ctx->cache->globals, val->canonicalName))
        ctx->cache->globals[val->canonicalName] = nullptr;
    } else if (!ctx->captures.empty()) {
      // LOG("-- {} -> {} {}", expr->value, val->getBase(), ctx->getBase());
      captured = true;
      if (!in(ctx->captures.back(), val->canonicalName)) {
        ctx->captures.back()[val->canonicalName] = newName =
            ctx->generateCanonicalName(val->canonicalName);
        ctx->cache->reverseIdentifierLookup[newName] = newName;
      }
      newName = ctx->captures.back()[val->canonicalName];
      val->noShadow = true;
    } else {
      // ctx->dump();
      error("cannot access nonlocal variable '{}'",
            ctx->cache->reverseIdentifierLookup[expr->value]);
    }
  }

  // Replace the variable with its canonical name. Do not canonize captured
  // variables (they will be later passed as argument names).
  resultExpr = N<IdExpr>(newName);
  if (val->getBase() != ctx->getBase() &&
      !in(ctx->seenGlobalIdentifiers[ctx->getBase()],
          ctx->cache->reverseIdentifierLookup[val->canonicalName]) &&
      !isClassGeneric) {
    ctx->seenGlobalIdentifiers
        [ctx->getBase()][ctx->cache->reverseIdentifierLookup[val->canonicalName]] =
        expr->clone();
  }
  // Flag the expression as a type expression if it points to a class name or a generic.
  if (val->isType())
    resultExpr->markType();
  if (!val->accessChecked) {
    // LOG("{} {}", expr->toString(), expr->getSrcInfo());
    /// TODO: what if later access removes the check?!
    auto checkStmt = N<ExprStmt>(N<CallExpr>(
        N<DotExpr>("__internal__", "undef"),
        N<IdExpr>(fmt::format("{}.__used__", val->canonicalName)),
        N<StringExpr>(ctx->cache->reverseIdentifierLookup[val->canonicalName])));
    if (!ctx->shortCircuit) {
      prependStmts->push_back(checkStmt);
      val->accessChecked = true;
    } else {
      resultExpr = N<StmtExpr>(checkStmt, resultExpr);
    }
  }
  if (ctx->inLoop()) {
    bool inside =
        val->scope.size() >= ctx->getLoop()->scope.size() &&
        val->scope[ctx->getLoop()->scope.size() - 1] == ctx->getLoop()->scope.back();
    if (!inside)
      ctx->getLoop()->seenVars.insert(expr->value);
  }

  // The only variables coming from the enclosing base must be class generics.
  //  seqassert(!val->isFunc() || val->getBase().empty(), "{} has invalid base ({})",
  //           expr->value, val->getBase());
  if (!val->getBase().empty() && ctx->getBase() != val->getBase()) {
    // Assumption: only 2 bases are available (class -> function)
    if (isClassGeneric) {
      ctx->bases.back().attributes |= FLAG_METHOD;
      return;
    }
  }
  // If that is not the case, we are probably having a class accessing its enclosing
  // function variable (generic or other identifier). We do not like that!
  if (!captured && ctx->getBase() != val->getBase() && !val->getBase().empty())
    error("identifier '{}' not found (cannot access outer function identifiers)",
          expr->value);
}

} // namespace codon::ast