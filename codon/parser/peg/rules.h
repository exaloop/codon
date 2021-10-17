#pragma once

#include <any>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/util/cpp-peglib/peglib.h"

namespace codon {
namespace ast {

struct ParseContext {
  std::shared_ptr<Cache> cache;
  std::stack<int> indent;
  int parens;
  int line_offset, col_offset;
  ParseContext(std::shared_ptr<Cache> cache, int parens = 0, int line_offset = 0,
               int col_offset = 0)
      : cache(move(cache)), parens(parens), line_offset(line_offset),
        col_offset(col_offset) {}

  bool hasCustomStmtKeyword(const std::string &kwd, bool hasExpr) const {
    auto i = cache->customBlockStmts.find(kwd);
    if (i != cache->customBlockStmts.end())
      return i->second.first == hasExpr;
    return false;
  }
  bool hasCustomExprStmt(const std::string &kwd) const {
    return in(cache->customExprStmts, kwd);
  }
};

} // namespace ast
} // namespace codon

void init_codon_rules(peg::Grammar &);
void init_codon_actions(peg::Grammar &);
void init_omp_rules(peg::Grammar &);
void init_omp_actions(peg::Grammar &);
