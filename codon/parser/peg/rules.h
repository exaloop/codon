// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <any>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <peglib.h>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"

namespace codon::ast {

struct ParseContext {
  Cache *cache;
  std::stack<int> indent;
  int parens;
  int line_offset, col_offset;
  ParseContext(Cache *cache, int parens = 0, int line_offset = 0, int col_offset = 0)
      : cache(cache), parens(parens), line_offset(line_offset), col_offset(col_offset) {
  }

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

} // namespace codon::ast

void init_codon_rules(peg::Grammar &);
void init_codon_actions(peg::Grammar &);
void init_omp_rules(peg::Grammar &);
void init_omp_actions(peg::Grammar &);
