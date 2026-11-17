// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/capi.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <ranges>
#include <string>

#include "codon/cir/llvm/llvm.h"
#include "codon/cir/util/format.h"
#include "codon/compiler/compiler.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

#include "llvm/Support/JSON.h"

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
    auto options = codon::Options::getDefault(argv0);
    auto compiler = std::make_unique<codon::Compiler>(*options);
    auto parsed = parse(compiler->getCache());
    if (!parsed)
      return failure(llvm::toString(parsed.takeError()));
    std::string str;
    if (typecheck) {
      auto abspath = (*parsed)->getSrcInfo().file;
      auto earlyDefines = compiler->getEarlyDefines();
      auto node = codon::ast::TypecheckVisitor::apply(
          compiler->getCache(), *parsed, abspath,
          std::unordered_map<std::string, std::string>{}, earlyDefines, typecheck > 1);
      str = node->toCodonString(includeAttributes, indent);
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

char *codon_ast_dump_defines(void) {
  auto options = codon::Options::getDefault("");
  codon::Compiler compiler(*options);
  std::string result;
  for (const auto &[name, value] : compiler.getEarlyDefines())
    result += fmt::format("{}={}\n", name, value);
  return copyString(result);
}

CodonAstDumpResult codon_ir_dump_stages(const char *file, uint8_t include_attributes,
                                        int indent, uint8_t types, int ast_indent) {
  if (!file)
    return failure("file must not be null");
  try {
    auto options = codon::Options::getDefault(file);
    options->debug = true;
    auto compiler = std::make_unique<codon::Compiler>(*options);
    auto *cache = compiler->getCache();
    cache->fs->set_module0(file);
    auto parsed = codon::ast::parseFile(cache, file);
    if (!parsed)
      return failure(llvm::toString(parsed.takeError()));
    auto checked = codon::ast::TypecheckVisitor::apply(
        cache, *parsed, file, {}, compiler->getEarlyDefines(), false);
    std::string typed =
        checked->toCodonString(bool(include_attributes), ast_indent) + "\n";
    for (const auto &[_, fn] : codon::ast::sorted_view(cache->functions))
      for (const auto &[_, real] : codon::ast::sorted_view(fn.realizations))
        if (real->ast)
          typed +=
              real->ast->toCodonString(bool(include_attributes), ast_indent) + "\n";
    llvm::json::Object stages;
    stages["typecheck"] = std::move(typed);
    codon::ast::TranslateVisitor::apply(cache, checked);
    auto dump = [&] {
      return codon::ir::util::format(compiler->getModule(), true,
                                     bool(include_attributes), indent, bool(types));
    };
    stages["translate"] = dump();
    auto *manager = compiler->getPassManager();
    for (const auto &key : manager->getExecutionOrder()) {
      manager->runPass(compiler->getModule(), key);
      stages[key] = dump();
    }
    return success(llvm::formatv("{0}", llvm::json::Value(std::move(stages))).str());
  } catch (const std::exception &error) {
    return failure(error.what());
  } catch (...) {
    return failure("unknown C++ exception while dumping CIR stages");
  }
}

void codon_ast_dump_free(char *value) { std::free(value); }
