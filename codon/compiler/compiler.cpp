// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

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

Compiler::Compiler(const Options &options, const std::shared_ptr<ast::IFilesystem> &fs)
    : input(), options(std::make_unique<Options>(options)),
      plm(std::make_unique<PluginManager>(options.argv0)),
      cache(std::make_unique<ast::Cache>(options.argv0, fs)),
      module(std::make_unique<ir::Module>(cache.get())),
      pm(std::make_unique<ir::transform::PassManager>(getOptions())),
      llvisitor(std::make_unique<ir::LLVMVisitor>(getOptions())) {
  cache->module = module.get();
  cache->compiler = this;
  llvisitor->setPluginManager(plm.get());
}

llvm::Error Compiler::load(const std::string &plugin) {
  auto result = plm->load(plugin);
  if (auto err = result.takeError())
    return err;

  auto *p = *result;
  if (!p->info.stdlibPath.empty()) {
    cache->fs->add_search_path(p->info.stdlibPath);
  }
  for (auto &kw : p->dsl->getExprKeywords()) {
    cache->customExprStmts[kw.keyword] = kw.callback;
  }
  for (auto &kw : p->dsl->getBlockKeywords()) {
    cache->customBlockStmts[kw.keyword] = {kw.hasExpr, kw.callback};
  }
  p->dsl->addIRPasses(pm.get(), options->debug);

  loadedPlugins.insert(plugin);

  return llvm::Error::success();
}

/// Checks if a plugin is already loaded.
bool Compiler::isPluginLoaded(const std::string &path) const {
  return loadedPlugins.find(path) != loadedPlugins.end();
}

llvm::Error
Compiler::parse(bool isCode, const std::string &file, const std::string &code,
                int startLine, int testFlags,
                const std::unordered_map<std::string, std::string> &defines) {
  input = file;
  std::string abspath = (file != "-") ? cache->fs->canonical(file).generic_string() : file;
  try {
    auto nodeOrErr = isCode ? ast::parseCode(cache.get(), abspath, code, startLine)
                            : ast::parseFile(cache.get(), abspath);
    if (!nodeOrErr)
      throw exc::ParserException(nodeOrErr.takeError());
    auto codeStmt = *nodeOrErr;

    cache->fs->set_module0(file);

    Timer t2("typecheck");
    t2.logged = true;
    auto typechecked = ast::TypecheckVisitor::apply(
        cache.get(), codeStmt, abspath, defines, getEarlyDefines(), (testFlags > 1));
    LOG_TIME("[T] parse = {:.1f}", totalPeg);
    LOG_TIME("[T] typecheck = {:.1f}", t2.elapsed() - totalPeg);

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
    auto j = ast::DocVisitor::apply(options->argv0, files);
    return j->toString();
  } catch (exc::ParserException &exc) {
    return llvm::make_error<error::ParserErrorInfo>(exc.getErrors());
  }
}

std::unordered_map<std::string, std::string> Compiler::getEarlyDefines() {
  std::unordered_map<std::string, std::string> earlyDefines;
  earlyDefines.emplace("__debug__", options->debug ? "1" : "0");
  earlyDefines.emplace("__py_numerics__", options->pynum ? "1" : "0");
  earlyDefines.emplace("__py_extension__", options->pyext ? "1" : "0");
  earlyDefines.emplace("__dict_unordered__", options->unordereddict ? "1" : "0");
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
