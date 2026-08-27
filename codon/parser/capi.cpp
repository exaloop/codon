// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/capi.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

#include <llvm/Support/Error.h>

#include "codon/parser/cache.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/scoping/scoping.h"

namespace {

char *copyString(const std::string &value) {
  auto *result = static_cast<char *>(std::malloc(value.size() + 1));
  if (!result)
    return nullptr;
  std::memcpy(result, value.data(), value.size());
  result[value.size()] = '\0';
  return result;
}

CodonAstDumpResult success(const std::string &output) {
  return {copyString(output), nullptr};
}

CodonAstDumpResult failure(const std::string &error) {
  return {nullptr, copyString(error)};
}

template <typename Parse>
CodonAstDumpResult parseScopeDump(const std::string &argv0, Parse &&parse,
                                  bool includeAttributes, int indent) {
  try {
    codon::ast::Cache cache(argv0);
    auto parsed = parse(&cache);
    if (!parsed)
      return failure(llvm::toString(parsed.takeError()));

    auto *node = *parsed;
    if (auto error = codon::ast::ScopingVisitor::apply(&cache, node))
      return failure(llvm::toString(std::move(error)));
    return success(node->toCodonString(includeAttributes, indent));
  } catch (const std::exception &error) {
    return failure(error.what());
  } catch (...) {
    return failure("unknown C++ exception while parsing and scoping Codon code");
  }
}

} // namespace

CodonAstDumpResult codon_ast_parse_scope_dump_code(const char *code, const char *file,
                                                   int line_offset,
                                                   uint8_t include_attributes,
                                                   int indent) {
  if (!code)
    return failure("code must not be null");
  const std::string filename = file ? file : "";
  return parseScopeDump(
      filename,
      [&](codon::ast::Cache *cache) {
        return codon::ast::parseCode(cache, filename, code, line_offset);
      },
      bool(include_attributes), indent);
}

CodonAstDumpResult codon_ast_parse_scope_dump_file(const char *file,
                                                   uint8_t include_attributes,
                                                   int indent) {
  if (!file)
    return failure("file must not be null");
  const std::string filename(file);
  return parseScopeDump(
      filename,
      [&](codon::ast::Cache *cache) { return codon::ast::parseFile(cache, filename); },
      bool(include_attributes), indent);
}

void codon_ast_dump_free(char *value) { std::free(value); }
