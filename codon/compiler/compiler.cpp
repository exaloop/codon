// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "compiler.h"

#include "codon/compiler/error.h"
#include "codon/parser/cache.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/doc/doc.h"
#include "codon/parser/visitors/format/format.h"
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
    auto nodeOrErr = isCode ? ast::parseCode(cache.get(), abspath, code, startLine)
                            : ast::parseFile(cache.get(), abspath);
    if (!nodeOrErr)
      throw exc::ParserException(nodeOrErr.takeError());
    auto codeStmt = *nodeOrErr;

    cache->module0 = file;

    Timer t2("typecheck");
    t2.logged = true;
    auto typechecked = ast::TypecheckVisitor::apply(
        cache.get(), codeStmt, abspath, defines, getEarlyDefines(), (testFlags > 1));
    LOG_TIME("[T] parse = {:.1f}", totalPeg);
    LOG_TIME("[T] typecheck = {:.1f}", t2.elapsed() - totalPeg);

    // std::unordered_map<std::string, double> wxt = cache->_timings;
    // std::vector<std::pair<std::string, double>> q(wxt.begin(), wxt.end());
    // sort(q.begin(), q.end(),
    //      [](const auto &a, const auto &b) { return b.second < a.second; });
    // double s = 0;
    // int _x = 0;
    // for (auto &[k, v] : q) {
    //   s += v;
    //   LOG_TIME("  [->] {:60} = {:10.3f} / {:10.3f}", k, v, s);
    //   if (_x++ > 10)
    //     break;
    // }

    if (codon::getLogger().flags & codon::Logger::FLAG_USER) {
      auto fo = fopen("_dump_typecheck.sexp", "w");
      fmt::print(fo, "{}\n", typechecked->toString(0));
      for (auto &f : cache->functions)
        for (auto &r : f.second.realizations) {
          fmt::print(fo, "{}\n", r.second->ast->toString(0));
        }
      fclose(fo);

      fo = fopen("_dump_typecheck.htm", "w");
      auto s = ast::FormatVisitor::apply(typechecked, cache.get(), true);
      fmt::print(fo, "{}\n", s);
      fclose(fo);
    }

    Timer t4("translate");
    ast::TranslateVisitor::apply(cache.get(), std::move(typechecked));
    t4.log();
  } catch (const exc::ParserException &exc) {
    return llvm::make_error<error::ParserErrorInfo>(exc.getErrors());
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
  } catch (exc::ParserException &exc) {
    return llvm::make_error<error::ParserErrorInfo>(exc.getErrors());
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
