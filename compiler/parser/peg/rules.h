/*
 * peg.h --- PEG parser interface.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#pragma once

#include <any>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "parser/ast.h"
#include "parser/cache.h"
#include "parser/common.h"
#include "util/peglib.h"

namespace seq {
namespace ast {

struct ParseContext {
  shared_ptr<Cache> cache;
  std::stack<int> indent;
  int parens;
  int line_offset, col_offset;
  ParseContext(shared_ptr<Cache> cache, int parens = 0, int line_offset = 0,
               int col_offset = 0)
      : cache(move(cache)), parens(parens), line_offset(line_offset),
        col_offset(col_offset) {}

  bool hasCustomStmtKeyword(const string &kwd, bool hasExpr) const {
    auto i = cache->customBlockStmts.find(kwd);
    if (i != cache->customBlockStmts.end())
      return i->second.first == hasExpr;
    return false;
  }
  bool hasCustomExprStmt(const string &kwd) const {
    return in(cache->customExprStmts, kwd);
  }
};

} // namespace ast
} // namespace seq

void init_seq_rules(peg::Grammar &);
void init_seq_actions(peg::Grammar &);
void init_omp_rules(peg::Grammar &);
void init_omp_actions(peg::Grammar &);
