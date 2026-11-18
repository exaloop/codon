// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/capi.h"

#include <algorithm>
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

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Base64.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/SHA256.h"

namespace {

/// LLVM 20 SHA256 stores a 32-bit byte count and shifts it before widening in
/// pad(), producing incorrect digests above 512 MiB. Bound every independent
/// hash to 64 MiB, preserving byte order and exact total length in the report.
class DumpFingerprint {
  static constexpr size_t CHUNK_BYTES = 64 * 1024 * 1024;
  llvm::SHA256 hash;
  size_t offset = 0;
  int64_t bytes = 0;
  llvm::json::Array chunks;

  void commit() {
    auto digest = hash.final();
    chunks.push_back(llvm::toHex(llvm::ArrayRef<uint8_t>(digest), true));
    hash.init();
    offset = 0;
  }

public:
  void update(llvm::StringRef text) {
    bytes += text.size();
    while (!text.empty()) {
      auto size = std::min(CHUNK_BYTES - offset, text.size());
      hash.update(text.take_front(size));
      text = text.drop_front(size);
      offset += size;
      if (offset == CHUNK_BYTES)
        commit();
    }
  }

  llvm::json::Object finish() {
    if (offset || chunks.empty())
      commit();
    return llvm::json::Object{{"bytes", bytes}, {"sha256_chunks", std::move(chunks)}};
  }
};

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

static CodonAstDumpResult dumpCompare(const char *code, const char *file,
                                      int line_offset, uint8_t barebones,
                                      uint8_t summary, int indent, int ast_indent,
                                      CodonDumpWriter writer) {
  if (!file)
    return failure("file must not be null");
  llvm::json::Object stages;
  std::string phase = "parse";
  auto capture = [&](const std::string &key, const std::string &text) {
    DumpFingerprint hash;
    hash.update(llvm::StringRef(text));
    auto record = hash.finish();
    if (!summary)
      record["base64"] = llvm::encodeBase64(llvm::StringRef(text));
    stages[key] = std::move(record);
  };
  auto captureIR = [&](codon::ir::Module *module) {
    if (!summary) {
      capture(phase, codon::ir::util::format(module, true, true, indent, true));
      return;
    }
    DumpFingerprint hash;
    codon::ir::util::formatChunks(
        module,
        [&](const std::string &text) {
          hash.update(llvm::StringRef(text));
          if (writer)
            writer(phase.c_str(), text.data(), text.size());
        },
        true, true, indent, true);
    stages[phase] = hash.finish();
  };
  try {
    auto options = codon::Options::getDefault(file);
    options->debug = true;
    auto compiler = std::make_unique<codon::Compiler>(*options);
    auto *cache = compiler->getCache();
    cache->fs->set_module0(file);
    auto parsed = code ? codon::ast::parseCode(cache, file, code, line_offset)
                       : codon::ast::parseFile(cache, file);
    if (!parsed)
      throw codon::exc::ParserException(parsed.takeError());
    phase = "typecheck";
    auto checked = codon::ast::TypecheckVisitor::apply(
        cache, *parsed, file, {}, compiler->getEarlyDefines(), bool(barebones));
    DumpFingerprint typedHash;
    std::string typed;
    auto appendTyped = [&](const std::string &text) {
      // Closure type graphs can make the complete forest several gigabytes.
      // Hash every byte without retaining all realized-function dumps at once.
      typedHash.update(llvm::StringRef(text));
      if (writer)
        writer(phase.c_str(), text.data(), text.size());
      if (!summary)
        typed += text;
    };
    appendTyped(checked->toCodonString(true, ast_indent));
    appendTyped("\n");
    for (const auto &[_, fn] : codon::ast::sorted_view(cache->functions))
      for (const auto &[_, real] : codon::ast::sorted_view(fn.realizations))
        if (real->ast) {
          appendTyped(real->ast->toCodonString(true, ast_indent));
          appendTyped("\n");
        }
    auto typedRecord = typedHash.finish();
    if (!summary)
      typedRecord["base64"] = llvm::encodeBase64(llvm::StringRef(typed));
    stages[phase] = std::move(typedRecord);
    std::string().swap(typed);
    phase = "translate";
    codon::ast::TranslateVisitor::apply(cache, checked);
    captureIR(compiler->getModule());
    phase = "ir";
    compiler->getPassManager()->run(compiler->getModule());
    captureIR(compiler->getModule());
  } catch (const codon::exc::ParserException &error) {
    stages["_error"] = llvm::json::Object{
        {"stage", phase}, {"message", std::string(error.what())}, {"kind", "compiler"}};
  } catch (const std::exception &error) {
    stages["_error"] = llvm::json::Object{
        {"stage", phase}, {"message", std::string(error.what())}, {"kind", "internal"}};
  } catch (...) {
    stages["_error"] = llvm::json::Object{
        {"stage", phase}, {"message", "unknown C++ exception"}, {"kind", "internal"}};
  }
  return success(llvm::formatv("{0}", llvm::json::Value(std::move(stages))).str());
}

CodonAstDumpResult codon_ir_dump_compare(const char *code, const char *file,
                                         int line_offset, uint8_t barebones,
                                         uint8_t summary, int indent, int ast_indent) {
  return dumpCompare(code, file, line_offset, barebones, summary, indent, ast_indent,
                     nullptr);
}

CodonAstDumpResult codon_ir_dump_compare_write(const char *code, const char *file,
                                               int line_offset, uint8_t barebones,
                                               CodonDumpWriter writer, int indent,
                                               int ast_indent) {
  return dumpCompare(code, file, line_offset, barebones, true, indent, ast_indent,
                     writer);
}
