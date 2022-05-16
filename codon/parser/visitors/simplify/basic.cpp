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

void SimplifyVisitor::visit(IntExpr *expr) {
  resultExpr = transformInt(expr->value, expr->suffix);
}

void SimplifyVisitor::visit(FloatExpr *expr) {
  resultExpr = transformFloat(expr->value, expr->suffix);
}

void SimplifyVisitor::visit(StringExpr *expr) {
  std::vector<ExprPtr> exprs;
  std::string concat;
  int realStrings = 0;
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
      concat += p.first;
      realStrings++;
    }
  }
  if (realStrings == expr->strings.size())
    resultExpr = N<StringExpr>(concat);
  else if (exprs.size() == 1)
    resultExpr = std::move(exprs[0]);
  else
    resultExpr = transform(N<CallExpr>(N<DotExpr>("str", "cat"), exprs));
}

/**************************************************************************************/

uint64_t toInt(const std::string &s) {
  if (startswith(s, "0b") || startswith(s, "0B"))
    return std::stoull(s.substr(2), nullptr, 2);
  return std::stoull(s, nullptr, 0);
}

ExprPtr SimplifyVisitor::transformInt(const std::string &value,
                                      const std::string &suffix) {
  int64_t val;
  try {
    val = (int64_t)toInt(value);
    if (suffix.empty())
      return N<IntExpr>(val);
    /// Unsigned numbers: use UInt[64] for that
    if (suffix == "u")
      return transform(N<CallExpr>(N<IndexExpr>(N<IdExpr>("UInt"), N<IntExpr>(64)),
                                   N<IntExpr>(val)));
    /// Fixed-precision numbers (uXXX and iXXX)
    /// NOTE: you cannot use binary (0bXXX) format with those numbers.
    /// TODO: implement non-string constructor for these cases.
    if (suffix[0] == 'u' && isdigit(suffix.substr(1)))
      return transform(N<CallExpr>(
          N<IndexExpr>(N<IdExpr>("UInt"), N<IntExpr>(std::stoi(suffix.substr(1)))),
          N<IntExpr>(val)));
    if (suffix[0] == 'i' && isdigit(suffix.substr(1)))
      return transform(N<CallExpr>(
          N<IndexExpr>(N<IdExpr>("Int"), N<IntExpr>(std::stoi(suffix.substr(1)))),
          N<IntExpr>(val)));
  } catch (std::out_of_range &) {
    error("integer {} out of range", value);
  }
  /// Custom suffix sfx: use int.__suffix_sfx__(str) call.
  /// NOTE: you cannot neither use binary (0bXXX) format here.
  return transform(
      N<CallExpr>(N<DotExpr>("int", format("__suffix_{}__", suffix)), N<IntExpr>(val)));
}

ExprPtr SimplifyVisitor::transformFloat(const std::string &value,
                                        const std::string &suffix) {
  double val;
  try {
    val = std::stod(value);
  } catch (std::out_of_range &) {
    error("float {} out of range", value);
  }
  if (suffix.empty())
    return N<FloatExpr>(val);
  /// Custom suffix sfx: use float.__suffix_sfx__(str) call.
  return transform(N<CallExpr>(N<DotExpr>("float", format("__suffix_{}__", suffix)),
                               N<FloatExpr>(value)));
}

ExprPtr SimplifyVisitor::transformFString(std::string value) {
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
