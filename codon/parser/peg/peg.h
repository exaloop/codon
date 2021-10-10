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
StmtPtr parseCode(const shared_ptr<Cache> &cache, const string &file,
                  const string &code, int line_offset = 0);
/// Parse a Seq code expression.
ExprPtr parseExpr(const shared_ptr<Cache> &cache, const string &code,
                  const codon::SrcInfo &offset);
/// Parse a Seq file.
StmtPtr parseFile(const shared_ptr<Cache> &cache, const string &file);

/// Parse a OpenMP clause.
vector<CallExpr::Arg> parseOpenMP(const shared_ptr<Cache> &cache, const string &code,
                                  const codon::SrcInfo &loc);

} // namespace ast
} // namespace codon
