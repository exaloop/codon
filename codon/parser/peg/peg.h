#pragma once

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/util/common.h"

namespace codon {
namespace ast {

/// Parse a Seq code block with the appropriate file and position offsets.
StmtPtr parseCode(Cache *cache, const std::string &file, const std::string &code,
                  int line_offset = 0);
/// Parse a Seq code expression.
ExprPtr parseExpr(Cache *cache, const std::string &code, const codon::SrcInfo &offset);
/// Parse a Seq file.
StmtPtr parseFile(Cache *cache, const std::string &file);

/// Parse a OpenMP clause.
std::vector<CallExpr::Arg> parseOpenMP(Cache *cache, const std::string &code,
                                       const codon::SrcInfo &loc);

} // namespace ast
} // namespace codon
