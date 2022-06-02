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

void SimplifyVisitor::visit(NoneExpr *expr) {
  resultExpr = transform(N<CallExpr>(N<IdExpr>(TYPE_OPTIONAL)));
}

void SimplifyVisitor::visit(IntExpr *expr) { resultExpr = transformInt(expr); }

void SimplifyVisitor::visit(FloatExpr *expr) { resultExpr = transformFloat(expr); }

void SimplifyVisitor::visit(StringExpr *expr) {
  std::vector<ExprPtr> exprs;
  std::vector<std::string> concat;
  for (auto &p : expr->strings) {
    if (p.second == "f" || p.second == "F") {
      /// F-strings
      exprs.push_back(transformFString(p.first));
    } else if (!p.second.empty()) {
      /// Custom-prefix strings
      exprs.push_back(
          transform(N<CallExpr>(N<DotExpr>("str", format("__prefix_{}__", p.second)),
                                N<StringExpr>(p.first), N<IntExpr>(p.first.size()))));
    } else {
      exprs.push_back(N<StringExpr>(p.first));
      concat.push_back(p.first);
    }
  }
  if (concat.size() == expr->strings.size()) {
    resultExpr = N<StringExpr>(combine2(concat, ""));
  } else if (exprs.size() == 1) {
    resultExpr = std::move(exprs[0]);
  } else {
    resultExpr = transform(N<CallExpr>(N<DotExpr>("str", "cat"), exprs));
  }
}

/**************************************************************************************/

ExprPtr SimplifyVisitor::transformInt(IntExpr *expr) {
  // TODO: use str constructors if available?
  if (!expr->intValue)
    error("integer {} out of range", expr->value);

  std::unique_ptr<int16_t> suffixValue;
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
    return N<IntExpr>(*(expr->intValue));
  } else if (expr->suffix == "u") {
    /// Unsigned numbers: use UInt[64] for that
    return transform(N<CallExpr>(N<IndexExpr>(N<IdExpr>("UInt"), N<IntExpr>(64)),
                                 N<IntExpr>(*(expr->intValue))));
  } else if (suffixValue) {
    /// Fixed-precision numbers (uXXX and iXXX)
    /// NOTE: you cannot use binary (0bXXX) format with those numbers.
    /// TODO: implement non-string constructor for these cases.
    return transform(
        N<CallExpr>(N<IndexExpr>(N<IdExpr>(expr->suffix[0] == 'u' ? "UInt" : "Int"),
                                 N<IntExpr>(*suffixValue)),
                    N<IntExpr>(*(expr->intValue))));
  } else {
    /// Custom suffix sfx: use int.__suffix_sfx__(str) call.
    return transform(
        N<CallExpr>(N<DotExpr>("int", format("__suffix_{}__", expr->suffix)),
                    N<IntExpr>(*(expr->intValue))));
  }
}

ExprPtr SimplifyVisitor::transformFloat(FloatExpr *expr) {
  if (!expr->floatValue)
    error("float {} out of range", expr->value);

  if (expr->suffix.empty()) {
    return N<FloatExpr>(*(expr->floatValue));
  } else {
    /// Custom suffix sfx: use float.__suffix_sfx__(str) call.
    return transform(
        N<CallExpr>(N<DotExpr>("float", format("__suffix_{}__", expr->suffix)),
                    N<FloatExpr>(*(expr->floatValue))));
  }
}

ExprPtr SimplifyVisitor::transformFString(const std::string &value) {
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
          code = code.substr(0, code.size() - 1);
          items.push_back(N<StringExpr>(format("{}=", code)));
        }
        items.push_back(
            N<CallExpr>(N<IdExpr>("str"), parseExpr(ctx->cache, code, offset)));
      }
      braceStart = i + 1;
    }
  }
  if (braceCount)
    error("f-string braces are not balanced");
  if (braceStart != value.size())
    items.push_back(N<StringExpr>(value.substr(braceStart, value.size() - braceStart)));
  return transform(N<CallExpr>(N<DotExpr>("str", "cat"), items));
}

} // namespace codon::ast
