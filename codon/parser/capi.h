// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <stddef.h>
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
struct CodonAstDumpResult codon_ast_dump_code(const char *code, const char *file,
                                              int line_offset,
                                              uint8_t include_attributes, int indent,
                                              int typecheck);

/// Parse a file, run the scoping pass and return its Codon AST dump.
struct CodonAstDumpResult codon_ast_dump_file(const char *file,
                                              uint8_t include_attributes, int indent,
                                              int typecheck);

/// Default compiler defines as key=value lines, in the typechecker's iteration order.
/// The caller must release the returned string with codon_ast_dump_free().
char *codon_ast_dump_defines(void);

/// Typecheck and translate a file, then dump translation and each debug IR pass.
/// Returns a JSON object mapping stage names to dump strings. No LLVM emission.
/// Ownership and error handling are identical to the AST dump APIs.
struct CodonAstDumpResult codon_ir_dump_stages(const char *file,
                                               uint8_t include_attributes, int indent,
                                               uint8_t types, int ast_indent);

/// Compare-test reporting: typecheck, translate, and final debug IR only.
/// A null code pointer reads the file; otherwise parse code with its original
/// filename and line offset. Summary mode returns ordered 64-MiB SHA-256 chunks
/// and byte counts; full mode
/// also returns base64 text, preserving even non-UTF-8 literal payloads.
struct CodonAstDumpResult codon_ir_dump_compare(const char *code, const char *file,
                                                int line_offset, uint8_t barebones,
                                                uint8_t summary, int indent,
                                                int ast_indent);

/// Stream exact dump chunks to a caller-provided writer while returning only
/// stage digests. The writer must consume each temporary byte buffer immediately.
typedef void (*CodonDumpWriter)(const char *stage, const char *data, size_t size);
struct CodonAstDumpResult codon_ir_dump_compare_write(const char *code,
                                                      const char *file, int line_offset,
                                                      uint8_t barebones,
                                                      CodonDumpWriter writer,
                                                      int indent, int ast_indent);

/// Release a string returned in CodonAstDumpResult.
void codon_ast_dump_free(char *value);

#ifdef __cplusplus
}
#endif
