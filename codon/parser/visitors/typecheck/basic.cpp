// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Set type to `Optional[?]`
void TypecheckVisitor::visit(NoneExpr *expr) {
  unify(expr->type, ctx->instantiate(ctx->getType(TYPE_OPTIONAL)));
  if (realize(expr->type)) {
    // Realize the appropriate `Optional.__new__` for the translation stage
    auto cls = expr->type->getClass();
    auto f = ctx->forceFind(TYPE_OPTIONAL ".__new__:0")->type;
    auto t = realize(ctx->instantiate(f, cls)->getFunc());
    expr->setDone();
  }
}

/// Set type to `bool`
void TypecheckVisitor::visit(BoolExpr *expr) {
  unify(expr->type, ctx->getType("bool"));
  expr->setDone();
}

/// Set type to `int`
void TypecheckVisitor::visit(IntExpr *expr) { resultExpr = transformInt(expr); }

/// Set type to `float`
void TypecheckVisitor::visit(FloatExpr *expr) { resultExpr = transformFloat(expr); }

/// Set type to `str`
void TypecheckVisitor::visit(StringExpr *expr) {
  seqassert(expr->strings.size() == 1 && expr->strings[0].second.empty(), "bad string");
  unify(expr->type,
        std::make_shared<types::StrStaticType>(ctx->cache, expr->getValue()));
  expr->setDone();
}

/// Parse various integer representations depending on the integer suffix.
/// @example
///   `123u`   -> `UInt[64](123)`
///   `123i56` -> `Int[56](123)`
///   `123pf`  -> `int.__suffix_pf__(123)`
ExprPtr TypecheckVisitor::transformInt(IntExpr *expr) {
  if (!expr->intValue) {
    /// TODO: currently assumes that ints are always 64-bit.
    /// Should use str constructors if available for ints with a suffix instead.
    E(Error::INT_RANGE, expr, expr->value);
  }

  /// Handle fixed-width integers: suffixValue is a pointer to NN if the suffix
  /// is `uNNN` or `iNNN`.
  std::unique_ptr<int16_t> suffixValue = nullptr;
  if (expr->suffix.size() > 1 && (expr->suffix[0] == 'u' || expr->suffix[0] == 'i') &&
      isdigit(expr->suffix.substr(1))) {
    try {
      suffixValue = std::make_unique<int16_t>(std::stoi(expr->suffix.substr(1)));
    } catch (...) {
    }
    if (suffixValue && *suffixValue > MAX_INT_WIDTH)
      suffixValue = nullptr;
  }

  if (expr->suffix.empty()) {
    // A normal integer (int64_t)
    unify(expr->type,
          std::make_shared<types::IntStaticType>(ctx->cache, *(expr->intValue)));
    expr->setDone();
    return nullptr;
  } else if (expr->suffix == "u") {
    // Unsigned integer: call `UInt[64](value)`
    return transform(N<CallExpr>(N<IndexExpr>(N<IdExpr>("UInt"), N<IntExpr>(64)),
                                 N<IntExpr>(*(expr->intValue))));
  } else if (suffixValue) {
    // Fixed-width numbers (with `uNNN` and `iNNN` suffixes):
    // call `UInt[NNN](value)` or `Int[NNN](value)`
    return transform(
        N<CallExpr>(N<IndexExpr>(N<IdExpr>(expr->suffix[0] == 'u' ? "UInt" : "Int"),
                                 N<IntExpr>(*suffixValue)),
                    N<IntExpr>(*(expr->intValue))));
  } else {
    // Custom suffix: call `int.__suffix_[suffix]__(value)`
    return transform(
        N<CallExpr>(N<DotExpr>("int", format("__suffix_{}__", expr->suffix)),
                    N<IntExpr>(*(expr->intValue))));
  }
}

/// Parse various float representations depending on the suffix.
/// @example
///   `123.4pf` -> `float.__suffix_pf__(123.4)`
ExprPtr TypecheckVisitor::transformFloat(FloatExpr *expr) {
  if (!expr->floatValue) {
    /// TODO: currently assumes that floats are always 64-bit.
    /// Should use str constructors if available for floats with suffix instead.
    E(Error::FLOAT_RANGE, expr, expr->value);
  }

  if (expr->suffix.empty()) {
    /// A normal float (double)
    unify(expr->type, ctx->getType("float"));
    expr->setDone();
    return nullptr;
  } else {
    // Custom suffix: call `float.__suffix_[suffix]__(value)`
    return transform(
        N<CallExpr>(N<DotExpr>("float", format("__suffix_{}__", expr->suffix)),
                    N<FloatExpr>(*(expr->floatValue))));
  }
}

} // namespace codon::ast
