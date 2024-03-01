// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
using namespace codon::error;

namespace codon::ast {

/// See @c transformInt
void SimplifyVisitor::visit(IntExpr *expr) { resultExpr = transformInt(expr); }

/// See @c transformFloat
void SimplifyVisitor::visit(FloatExpr *expr) { resultExpr = transformFloat(expr); }

/// Concatenate string sequence (e.g., `"a" "b" "c"`) into a single string.
/// Parse f-strings and custom prefix strings.
/// Also see @c transformFString
void SimplifyVisitor::visit(StringExpr *expr) {
  std::vector<ExprPtr> exprs;
  std::vector<std::string> concat;
  for (auto &p : expr->strings) {
    if (p.second == "f" || p.second == "F") {
      /// Transform an F-string
      exprs.push_back(transformFString(p.first));
    } else if (!p.second.empty()) {
      /// Custom prefix strings:
      /// call `str.__prefix_[prefix]__(str, [static length of str])`
      exprs.push_back(
          transform(N<CallExpr>(N<DotExpr>("str", format("__prefix_{}__", p.second)),
                                N<StringExpr>(p.first), N<IntExpr>(p.first.size()))));
    } else {
      exprs.push_back(N<StringExpr>(p.first));
      concat.push_back(p.first);
    }
  }
  if (concat.size() == expr->strings.size()) {
    /// Simple case: statically concatenate a sequence of strings without any prefix
    expr->strings = {{combine2(concat, ""), ""}};
  } else if (exprs.size() == 1) {
    /// Simple case: only one string in a sequence
    resultExpr = std::move(exprs[0]);
  } else {
    /// Complex case: call `str.cat(str1, ...)`
    resultExpr = transform(N<CallExpr>(N<DotExpr>("str", "cat"), exprs));
  }
}

/**************************************************************************************/

/// Parse various integer representations depending on the integer suffix.
/// @example
///   `123u`   -> `UInt[64](123)`
///   `123i56` -> `Int[56](123)`
///   `123pf`  -> `int.__suffix_pf__(123)`
ExprPtr SimplifyVisitor::transformInt(IntExpr *expr) {
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
    return N<IntExpr>(*(expr->intValue));
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
ExprPtr SimplifyVisitor::transformFloat(FloatExpr *expr) {
  if (!expr->floatValue) {
    /// TODO: currently assumes that floats are always 64-bit.
    /// Should use str constructors if available for floats with suffix instead.
    E(Error::FLOAT_RANGE, expr, expr->value);
  }

  if (expr->suffix.empty()) {
    /// A normal float (double)
    return N<FloatExpr>(*(expr->floatValue));
  } else {
    // Custom suffix: call `float.__suffix_[suffix]__(value)`
    return transform(
        N<CallExpr>(N<DotExpr>("float", format("__suffix_{}__", expr->suffix)),
                    N<FloatExpr>(*(expr->floatValue))));
  }
}

/// Parse a Python-like f-string into a concatenation:
///   `f"foo {x+1} bar"` -> `str.cat("foo ", str(x+1), " bar")`
/// Supports "{x=}" specifier (that prints the raw expression as well):
///   `f"{x+1=}"` -> `str.cat("x+1=", str(x+1))`
ExprPtr SimplifyVisitor::transformFString(const std::string &value) {
  // Strings to be concatenated
  std::vector<ExprPtr> items;
  int braceCount = 0, braceStart = 0;
  for (int i = 0; i < value.size(); i++) {
    if (value[i] == '{') {
      if (braceStart < i)
        items.push_back(N<StringExpr>(value.substr(braceStart, i - braceStart)));
      if (!braceCount)
        braceStart = i + 1;
      braceCount++;
    } else if (value[i] == '}') {
      braceCount--;
      if (!braceCount) {
        std::string code = value.substr(braceStart, i - braceStart);
        auto offset = getSrcInfo();
        offset.col += i;
        if (!code.empty() && code.back() == '=') {
          // Special case: f"{x=}"
          code = code.substr(0, code.size() - 1);
          items.push_back(N<StringExpr>(fmt::format("{}=", code)));
        }
        auto [expr, format] = parseExpr(ctx->cache, code, offset);
        if (!format.empty()) {
          items.push_back(
              N<CallExpr>(N<DotExpr>(expr, "__format__"), N<StringExpr>(format)));
        } else {
          // Every expression is wrapped within `str`
          items.push_back(N<CallExpr>(N<IdExpr>("str"), expr));
        }
      }
      braceStart = i + 1;
    }
  }
  if (braceCount > 0)
    E(Error::STR_FSTRING_BALANCE_EXTRA, getSrcInfo());
  if (braceCount < 0)
    E(Error::STR_FSTRING_BALANCE_MISSING, getSrcInfo());
  if (braceStart != value.size())
    items.push_back(N<StringExpr>(value.substr(braceStart, value.size() - braceStart)));
  return transform(N<CallExpr>(N<DotExpr>("str", "cat"), items));
}

} // namespace codon::ast
