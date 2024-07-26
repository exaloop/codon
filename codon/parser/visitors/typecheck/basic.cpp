// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
  unify(expr->getType(), ctx->instantiate(ctx->getType(TYPE_OPTIONAL)));
  if (realize(expr->getType())) {
    auto cls = expr->getClassType();

    // Realize the appropriate `Optional.__new__` for the translation stage
    auto f = ctx->forceFind(TYPE_OPTIONAL ".__new__:0")->type;
    auto t = realize(ctx->instantiate(f, cls)->getFunc());
    expr->setDone();
  }
}

/// Set type to `bool`
void TypecheckVisitor::visit(BoolExpr *expr) {
  unify(expr->getType(),
        std::make_shared<types::BoolStaticType>(ctx->cache, expr->getValue()));
  expr->setDone();
}

/// Set type to `int`
void TypecheckVisitor::visit(IntExpr *expr) { resultExpr = transformInt(expr); }

/// Set type to `float`
void TypecheckVisitor::visit(FloatExpr *expr) { resultExpr = transformFloat(expr); }

/// Set type to `str`
/// Parse a Python-like f-string into a concatenation:
///   `f"foo {x+1} bar"` -> `str.cat("foo ", str(x+1), " bar")`
/// Supports "{x=}" specifier (that prints the raw expression as well):
///   `f"{x+1=}"` -> `str.cat("x+1=", str(x+1))`

void TypecheckVisitor::visit(StringExpr *expr) {
  if (expr->strings.size() == 1 && expr->strings[0].prefix.empty()) {
    unify(expr->type,
          std::make_shared<types::StrStaticType>(ctx->cache, expr->getValue()));
    expr->setDone();
  } else {
    std::vector<Expr *> items;
    for (auto &p : expr->strings) {
      if (p.expr) {
        items.emplace_back(p.expr);
      } else if (!p.prefix.empty()) {
        /// Custom prefix strings:
        /// call `str.__prefsix_[prefix]__(str, [static length of str])`
        items.emplace_back(
            N<CallExpr>(N<DotExpr>(N<IdExpr>("str"), format("__prefix_{}__", p.prefix)),
                        N<StringExpr>(p.value), N<IntExpr>(p.value.size())));
      } else {
        items.emplace_back(N<StringExpr>(p.value));
      }
    }
    resultExpr = transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("str"), "cat"), items));
  }
}

/// Parse various integer representations depending on the integer suffix.
/// @example
///   `123u`   -> `UInt[64](123)`
///   `123i56` -> `Int[56](123)`
///   `123pf`  -> `int.__suffix_pf__(123)`
Expr *TypecheckVisitor::transformInt(IntExpr *expr) {
  auto [value, suffix] = expr->getRawData();

  if (!expr->hasStoredValue()) {
    /// TODO: currently assumes that ints are always 64-bit.
    /// Should use str constructors if available for ints with a suffix instead.
    E(Error::INT_RANGE, expr, value);
  }

  /// Handle fixed-width integers: suffixValue is a pointer to NN if the suffix
  /// is `uNNN` or `iNNN`.
  std::unique_ptr<int16_t> suffixValue = nullptr;
  if (suffix.size() > 1 && (suffix[0] == 'u' || suffix[0] == 'i') &&
      isdigit(suffix.substr(1))) {
    try {
      suffixValue = std::make_unique<int16_t>(std::stoi(suffix.substr(1)));
    } catch (...) {
    }
    if (suffixValue && *suffixValue > MAX_INT_WIDTH)
      suffixValue = nullptr;
  }

  if (suffix.empty()) {
    // A normal integer (int64_t)
    unify(expr->getType(),
          std::make_shared<types::IntStaticType>(ctx->cache, expr->getValue()));
    expr->setDone();
    return nullptr;
  } else if (suffix == "u") {
    // Unsigned integer: call `UInt[64](value)`
    return transform(N<CallExpr>(N<IndexExpr>(N<IdExpr>("UInt"), N<IntExpr>(64)),
                                 N<IntExpr>(expr->getValue())));
  } else if (suffixValue) {
    // Fixed-width numbers (with `uNNN` and `iNNN` suffixes):
    // call `UInt[NNN](value)` or `Int[NNN](value)`
    return transform(
        N<CallExpr>(N<IndexExpr>(N<IdExpr>(suffix[0] == 'u' ? "UInt" : "Int"),
                                 N<IntExpr>(*suffixValue)),
                    N<IntExpr>(expr->getValue())));
  } else {
    // Custom suffix: call `int.__suffix_[suffix]__(value)`
    return transform(
        N<CallExpr>(N<DotExpr>(N<IdExpr>("int"), format("__suffix_{}__", suffix)),
                    N<IntExpr>(expr->getValue())));
  }
}

/// Parse various float representations depending on the suffix.
/// @example
///   `123.4pf` -> `float.__suffix_pf__(123.4)`
Expr *TypecheckVisitor::transformFloat(FloatExpr *expr) {
  auto [value, suffix] = expr->getRawData();

  if (!expr->hasStoredValue()) {
    /// TODO: currently assumes that floats are always 64-bit.
    /// Should use str constructors if available for floats with suffix instead.
    E(Error::FLOAT_RANGE, expr, value);
  }

  if (suffix.empty()) {
    /// A normal float (double)
    unify(expr->getType(), ctx->getType("float"));
    expr->setDone();
    return nullptr;
  } else {
    // Custom suffix: call `float.__suffix_[suffix]__(value)`
    return transform(
        N<CallExpr>(N<DotExpr>(N<IdExpr>("float"), format("__suffix_{}__", suffix)),
                    N<FloatExpr>(expr->getValue())));
  }
}

} // namespace codon::ast
