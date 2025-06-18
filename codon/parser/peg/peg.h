// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/util/common.h"

namespace codon::ast {

/// Parse a Seq code block with the appropriate file and position offsets.
llvm::Expected<Stmt *> parseCode(Cache *cache, const std::string &file,
                                 const std::string &code, int line_offset = 0);
/// Parse a Seq code expression.
/// @return pair of Expr * and a format specification
/// (empty if not available).
llvm::Expected<std::pair<Expr *, StringExpr::FormatSpec>>
parseExpr(Cache *cache, const std::string &code, const codon::SrcInfo &offset);
/// Parse a Seq file.
llvm::Expected<Stmt *> parseFile(Cache *cache, const std::string &file);

/// Parse a OpenMP clause.
llvm::Expected<std::vector<CallArg>> parseOpenMP(Cache *cache, const std::string &code,
                                                 const codon::SrcInfo &loc);

} // namespace codon::ast
