// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "compiler.h"

#include "codon/compiler/error.h"
#include "codon/parser/cache.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/doc/doc.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

extern double totalPeg;

namespace codon {
namespace {
ir::transform::PassManager::Init getPassManagerInit(Compiler::Mode mode, bool isTest) {
  using ir::transform::PassManager;
  switch (mode) {
  case Compiler::Mode::DEBUG:
    return isTest ? PassManager::Init::RELEASE : PassManager::Init::DEBUG;
  case Compiler::Mode::RELEASE:
    return PassManager::Init::RELEASE;
  case Compiler::Mode::JIT:
    return PassManager::Init::JIT;
  default:
    return PassManager::Init::EMPTY;
  }
}
} // namespace

Compiler::Compiler(const std::string &argv0, Compiler::Mode mode,
                   const std::vector<std::string> &disabledPasses, bool isTest,
                   bool pyNumerics, bool pyExtension)
    : argv0(argv0), debug(mode == Mode::DEBUG), pyNumerics(pyNumerics),
      pyExtension(pyExtension), input(), plm(std::make_unique<PluginManager>(argv0)),
      cache(std::make_unique<ast::Cache>(argv0)),
      module(std::make_unique<ir::Module>()),
      pm(std::make_unique<ir::transform::PassManager>(
          getPassManagerInit(mode, isTest), disabledPasses, pyNumerics, pyExtension)),
      llvisitor(std::make_unique<ir::LLVMVisitor>()) {
  cache->module = module.get();
  cache->pythonExt = pyExtension;
  cache->pythonCompat = pyNumerics;
  module->setCache(cache.get());
  llvisitor->setDebug(debug);
  llvisitor->setPluginManager(plm.get());
}

llvm::Error Compiler::load(const std::string &plugin) {
  auto result = plm->load(plugin);
  if (auto err = result.takeError())
    return err;

  auto *p = *result;
  if (!p->info.stdlibPath.empty()) {
    cache->pluginImportPaths.push_back(p->info.stdlibPath);
  }
  for (auto &kw : p->dsl->getExprKeywords()) {
    cache->customExprStmts[kw.keyword] = kw.callback;
  }
  for (auto &kw : p->dsl->getBlockKeywords()) {
    cache->customBlockStmts[kw.keyword] = {kw.hasExpr, kw.callback};
  }
  p->dsl->addIRPasses(pm.get(), debug);
  return llvm::Error::success();
}

llvm::Error
Compiler::parse(bool isCode, const std::string &file, const std::string &code,
                int startLine, int testFlags,
                const std::unordered_map<std::string, std::string> &defines) {
  input = file;
  std::string abspath = (file != "-") ? ast::getAbsolutePath(file) : file;
  try {
    ast::StmtPtr codeStmt = isCode
                                ? ast::parseCode(cache.get(), abspath, code, startLine)
                                : ast::parseFile(cache.get(), abspath);

    cache->module0 = file;

    Timer t2("simplify");
    t2.logged = true;
    auto transformed =
        ast::SimplifyVisitor::apply(cache.get(), std::move(codeStmt), abspath, defines,
                                    getEarlyDefines(), (testFlags > 1));
    LOG_TIME("[T] parse = {:.1f}", totalPeg);
    LOG_TIME("[T] simplify = {:.1f}", t2.elapsed() - totalPeg);

    if (codon::getLogger().flags & codon::Logger::FLAG_USER) {
      auto fo = fopen("_dump_simplify.sexp", "w");
      fmt::print(fo, "{}\n", transformed->toString(0));
      fclose(fo);
    }

    Timer t3("typecheck");
    auto typechecked =
        ast::TypecheckVisitor::apply(cache.get(), std::move(transformed));
    t3.log();
    if (codon::getLogger().flags & codon::Logger::FLAG_USER) {
      auto fo = fopen("_dump_typecheck.sexp", "w");
      fmt::print(fo, "{}\n", typechecked->toString(0));
      for (auto &f : cache->functions)
        for (auto &r : f.second.realizations) {
          fmt::print(fo, "{}\n", r.second->ast->toString(0));
        }
      fclose(fo);
    }

    Timer t4("translate");
    ast::TranslateVisitor::apply(cache.get(), std::move(typechecked));
    t4.log();
  } catch (const exc::ParserException &exc) {
    std::vector<error::Message> messages;
    if (exc.messages.empty()) {
      const int MAX_ERRORS = 5;
      int ei = 0;
      for (auto &e : cache->errors) {
        for (unsigned i = 0; i < e.messages.size(); i++) {
          if (!e.messages[i].empty())
            messages.emplace_back(e.messages[i], e.locations[i].file,
                                  e.locations[i].line, e.locations[i].col,
                                  e.locations[i].len, e.errorCode);
        }
        if (ei++ > MAX_ERRORS)
          break;
      }
      return llvm::make_error<error::ParserErrorInfo>(messages);
    } else {
      return llvm::make_error<error::ParserErrorInfo>(exc);
    }
  }
  module->setSrcInfo({abspath, 0, 0, 0});
  if (codon::getLogger().flags & codon::Logger::FLAG_USER) {
    auto fo = fopen("_dump_ir.sexp", "w");
    fmt::print(fo, "{}\n", *module);
    fclose(fo);
  }
  return llvm::Error::success();
}

llvm::Error
Compiler::parseFile(const std::string &file, int testFlags,
                    const std::unordered_map<std::string, std::string> &defines) {
  return parse(/*isCode=*/false, file, /*code=*/"", /*startLine=*/0, testFlags,
               defines);
}

llvm::Error
Compiler::parseCode(const std::string &file, const std::string &code, int startLine,
                    int testFlags,
                    const std::unordered_map<std::string, std::string> &defines) {
  return parse(/*isCode=*/true, file, code, startLine, testFlags, defines);
}

llvm::Error Compiler::compile() {
  pm->run(module.get());
  if (codon::getLogger().flags & codon::Logger::FLAG_USER) {
    auto fo = fopen("_dump_ir_opt.sexp", "w");
    fmt::print(fo, "{}\n", *module);
    fclose(fo);
  }
  llvisitor->visit(module.get());
  if (codon::getLogger().flags & codon::Logger::FLAG_USER) {
    auto fo = fopen("_dump_llvm.ll", "w");
    std::string str;
    llvm::raw_string_ostream os(str);
    os << *(llvisitor->getModule());
    os.flush();
    fmt::print(fo, "{}\n", str);
    fclose(fo);
  }
  return llvm::Error::success();
}

llvm::Expected<std::string> Compiler::docgen(const std::vector<std::string> &files) {
  try {
    auto j = ast::DocVisitor::apply(argv0, files);
    return j->toString();
  } catch (exc::ParserException &e) {
    return llvm::make_error<error::ParserErrorInfo>(e);
  }
}

std::unordered_map<std::string, std::string> Compiler::getEarlyDefines() {
  std::unordered_map<std::string, std::string> earlyDefines;
  earlyDefines.emplace("__debug__", debug ? "1" : "0");
  earlyDefines.emplace("__py_numerics__", pyNumerics ? "1" : "0");
  earlyDefines.emplace("__py_extension__", pyExtension ? "1" : "0");
  earlyDefines.emplace("__apple__",
#if __APPLE__
                       "1"
#else
                       "0"
#endif
  );
  return earlyDefines;
}

} // namespace codon
