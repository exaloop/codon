// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Result of parsing, scoping and dumping a Codon AST. Exactly one of output and
/// error is non-null. Both strings are owned by the caller and must be released
/// with codon_ast_dump_free().
struct CodonAstDumpResult {
  char *output;
  char *error;
};

/// Parse code from memory, run the scoping pass and return its Codon AST dump.
struct CodonAstDumpResult
codon_ast_parse_scope_dump_code(const char *code, const char *file, int line_offset,
                                uint8_t include_attributes, int indent);

/// Parse a file, run the scoping pass and return its Codon AST dump.
struct CodonAstDumpResult codon_ast_parse_scope_dump_file(const char *file,
                                                          uint8_t include_attributes,
                                                          int indent);

/// Release a string returned in CodonAstDumpResult.
void codon_ast_dump_free(char *value);

#ifdef __cplusplus
}
#endif
