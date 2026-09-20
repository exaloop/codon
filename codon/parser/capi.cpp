// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/capi.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <ranges>
#include <string>

#include <llvm/Support/Error.h>

#include "codon/compiler/compiler.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

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
                                  bool includeAttributes, int indent, int typecheck) {
  try {
    std::vector<std::string> disabledOptsVec;
    auto compiler = std::make_unique<codon::Compiler>(argv0, true, disabledOptsVec,
                                                      /*isTest=*/true, false, false);
    auto parsed = parse(compiler->getCache());
    if (!parsed)
      return failure(llvm::toString(parsed.takeError()));
    std::string str;
    if (typecheck) {
      auto abspath = (*parsed)->getSrcInfo().file;
      std::unordered_map<std::string, std::string> earlyDefines{
          {"__debug__", "1"},
          {"__py_numerics__", "0"},
          {"__py_extension__", "0"},
          {"__apple__", "1"}};
      fprintf(stderr, "-- %s\n",
              (*parsed)->toCodonString(includeAttributes, indent).c_str());
      auto node = codon::ast::TypecheckVisitor::apply(
          compiler->getCache(), *parsed, abspath,
          std::unordered_map<std::string, std::string>{}, earlyDefines, typecheck > 1);
      str = node->toCodonString(includeAttributes, indent);
      fprintf(stderr, "%s\n", str.c_str());
      str += "\n";
      for (const auto &[_, f] :
           codon::ast::sorted_view(compiler->getCache()->functions)) {
        for (const auto &[_, r] : codon::ast::sorted_view(f.realizations)) {
          if (r->ast)
            str +=
                fmt::format("{}\n", r->ast->toCodonString(includeAttributes, indent));
        }
      }
    } else {
      auto node = *parsed;
      if (auto error = codon::ast::ScopingVisitor::apply(compiler->getCache(), node))
        return failure(llvm::toString(std::move(error)));
      str = node->toCodonString(includeAttributes, indent);
    }
    return success(str);
  } catch (const std::exception &error) {
    return failure(error.what());
  } catch (...) {
    return failure("unknown C++ exception while parsing and scoping Codon code");
  }
}

} // namespace

CodonAstDumpResult codon_ast_dump_code(const char *code, const char *file,
                                       int line_offset, uint8_t include_attributes,
                                       int indent, int typecheck) {
  if (!code)
    return failure("code must not be null");
  const std::string filename = file ? file : "";
  return parseScopeDump(
      filename,
      [&](codon::ast::Cache *cache) {
        return codon::ast::parseCode(cache, filename, code, line_offset);
      },
      bool(include_attributes), indent, typecheck);
}

CodonAstDumpResult codon_ast_dump_file(const char *file, uint8_t include_attributes,
                                       int indent, int typecheck) {
  if (!file)
    return failure("file must not be null");
  const std::string filename(file);
  return parseScopeDump(
      filename,
      [&](codon::ast::Cache *cache) { return codon::ast::parseFile(cache, filename); },
      bool(include_attributes), indent, typecheck);
}

void codon_ast_dump_free(char *value) { std::free(value); }
