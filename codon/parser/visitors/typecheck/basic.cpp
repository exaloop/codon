// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Set type to `Optional[?]`
void TypecheckVisitor::visit(NoneExpr *expr) {
  unify(expr->getType(), instantiateType(getStdLibType(TYPE_OPTIONAL)));
  if (realize(expr->getType())) {
    // Realize the appropriate `Optional.__new__` for the translation stage
    auto f = ctx->forceFind(TYPE_OPTIONAL ".__new__:0")->getType();
    auto t = realize(instantiateType(f, extractClassType(expr)));
    expr->setDone();
  }
}

/// Set type to `bool`
void TypecheckVisitor::visit(BoolExpr *expr) {
  unify(expr->getType(), instantiateStatic(expr->getValue()));
  expr->setDone();
}

/// Set type to `int`
void TypecheckVisitor::visit(IntExpr *expr) { resultExpr = transformInt(expr); }

/// Set type to `float`
void TypecheckVisitor::visit(FloatExpr *expr) { resultExpr = transformFloat(expr); }

/// Set type to `str`. Concatinate strings in list and apply appropriate transformations
///   (e.g., `str` wrap).
void TypecheckVisitor::visit(StringExpr *expr) {
  if (expr->isSimple()) {
    unify(expr->getType(), instantiateStatic(expr->getValue()));
    expr->setDone();
  } else {
    std::vector<Expr *> items;
    for (auto &p : *expr) {
      if (p.expr) {
        if (!p.format.conversion.empty()) {
          switch (p.format.conversion[0]) {
          case 'r':
            p.expr = N<CallExpr>(N<IdExpr>("repr"), p.expr);
            break;
          case 's':
            p.expr = N<CallExpr>(N<IdExpr>("str"), p.expr);
            break;
          case 'a':
            p.expr = N<CallExpr>(N<IdExpr>("ascii"), p.expr);
            break;
          default:
            // TODO: error?
            break;
          }
        }
        if (!p.format.spec.empty()) {
          p.expr = N<CallExpr>(N<DotExpr>(p.expr, "__format__"),
                               N<StringExpr>(p.format.spec));
        }
        p.expr = N<CallExpr>(N<IdExpr>("str"), p.expr);
        if (!p.format.text.empty()) {
          p.expr = N<CallExpr>(N<DotExpr>(N<IdExpr>("str"), "cat"),
                               N<StringExpr>(p.format.text), p.expr);
        }
        items.emplace_back(p.expr);
      } else if (!p.prefix.empty()) {
        /// Custom prefix strings:
        /// call `str.__prefsix_[prefix]__(str, [static length of str])`
        items.emplace_back(N<CallExpr>(
            N<DotExpr>(N<IdExpr>("str"), fmt::format("__prefix_{}__", p.prefix)),
            N<StringExpr>(p.value), N<IntExpr>(p.value.size())));
      } else {
        items.emplace_back(N<StringExpr>(p.value));
      }
    }
    if (items.size() == 1)
      resultExpr = transform(items.front());
    else
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
  Expr *holder = nullptr;
  if (!expr->hasStoredValue()) {
    holder = N<StringExpr>(value);
    if (suffix.empty())
      suffix = "i64";
  } else {
    holder = N<IntExpr>(expr->getValue());
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
    unify(expr->getType(), instantiateStatic(expr->getValue()));
    expr->setDone();
    return nullptr;
  } else if (suffix == "u") {
    // Unsigned integer: call `UInt[64](value)`
    return transform(
        N<CallExpr>(N<IndexExpr>(N<IdExpr>("UInt"), N<IntExpr>(64)), holder));
  } else if (suffixValue) {
    // Fixed-width numbers (with `uNNN` and `iNNN` suffixes):
    // call `UInt[NNN](value)` or `Int[NNN](value)`
    return transform(
        N<CallExpr>(N<IndexExpr>(N<IdExpr>(suffix[0] == 'u' ? "UInt" : "Int"),
                                 N<IntExpr>(*suffixValue)),
                    holder));
  } else {
    // Custom suffix: call `int.__suffix_[suffix]__(value)`
    return transform(N<CallExpr>(
        N<DotExpr>(N<IdExpr>("int"), fmt::format("__suffix_{}__", suffix)), holder));
  }
}

/// Parse various float representations depending on the suffix.
/// @example
///   `123.4pf` -> `float.__suffix_pf__(123.4)`
Expr *TypecheckVisitor::transformFloat(FloatExpr *expr) {
  auto [value, suffix] = expr->getRawData();

  Expr *holder = nullptr;
  if (!expr->hasStoredValue()) {
    holder = N<StringExpr>(value);
  } else {
    holder = N<FloatExpr>(expr->getValue());
  }

  if (suffix.empty() && expr->hasStoredValue()) {
    // A normal float (double)
    unify(expr->getType(), getStdLibType("float"));
    expr->setDone();
    return nullptr;
  } else if (suffix.empty()) {
    return transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("float"), "__new__"), holder));
  } else {
    // Custom suffix: call `float.__suffix_[suffix]__(value)`
    return transform(N<CallExpr>(
        N<DotExpr>(N<IdExpr>("float"), fmt::format("__suffix_{}__", suffix)), holder));
  }
}

} // namespace codon::ast
