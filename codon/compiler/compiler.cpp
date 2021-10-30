#include "compiler.h"

#include <filesystem>

#include "codon/parser/cache.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/doc/doc.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon {

Compiler::Compiler(const std::string &argv0, bool debug,
                   const std::vector<std::string> &disabledPasses)
    : debug(debug), input(), plm(std::make_unique<PluginManager>()),
      cache(std::make_unique<ast::Cache>(argv0)),
      module(std::make_unique<ir::Module>()),
      pm(std::make_unique<ir::transform::PassManager>(debug, disabledPasses)),
      llvisitor(std::make_unique<ir::LLVMVisitor>(debug)) {
  cache->module = module.get();
  module->setCache(cache.get());
  llvisitor->setPluginManager(plm.get());
}

bool Compiler::load(const std::string &plugin, std::string *errMsg) {
  if (auto *p = plm->load(plugin, errMsg)) {
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
    return true;
  }
  return false;
}

Compiler::ParserError
Compiler::parseFile(const std::string &file, int isTest,
                    const std::unordered_map<std::string, std::string> &defines) {
  input = file;
  std::string abspath =
      (file != "-") ? std::filesystem::absolute(std::filesystem::path(file)).string()
                    : file;
  try {
    ast::StmtPtr codeStmt = ast::parseFile(cache.get(), abspath);
    cache->module0 = file;
    if (isTest)
      cache->testFlags = isTest;

    auto transformed = ast::SimplifyVisitor::apply(cache.get(), std::move(codeStmt),
                                                   abspath, defines, (isTest > 1));
    auto typechecked =
        ast::TypecheckVisitor::apply(cache.get(), std::move(transformed));
    ast::TranslateVisitor::apply(cache.get(), std::move(typechecked));
  } catch (const exc::ParserException &e) {
    auto result = Compiler::ParserError::failure();
    for (int i = 0; i < e.messages.size(); i++) {
      if (!e.messages[i].empty())
        result.messages.push_back({e.messages[i], e.locations[i].file,
                                   e.locations[i].line, e.locations[i].col});
    }
    return result;
  }
  module->setSrcInfo({abspath, 0, 0, 0});
  return Compiler::ParserError::success();
}

Compiler::ParserError
Compiler::parseCode(const std::string &file, const std::string &code, int isTest,
                    int startLine,
                    const std::unordered_map<std::string, std::string> &defines) {
  input = file;
  std::string abspath =
      (file != "-") ? std::filesystem::absolute(std::filesystem::path(file)).string()
                    : file;
  try {
    ast::StmtPtr codeStmt = ast::parseCode(cache.get(), abspath, code, startLine);
    cache->module0 = file;
    if (isTest)
      cache->testFlags = isTest;

    auto transformed = ast::SimplifyVisitor::apply(cache.get(), std::move(codeStmt),
                                                   abspath, defines, (isTest > 1));
    auto typechecked =
        ast::TypecheckVisitor::apply(cache.get(), std::move(transformed));
    ast::TranslateVisitor::apply(cache.get(), std::move(typechecked));
  } catch (const exc::ParserException &e) {
    auto result = Compiler::ParserError::failure();
    for (int i = 0; i < e.messages.size(); i++) {
      if (!e.messages[i].empty())
        result.messages.push_back({e.messages[i], e.locations[i].file,
                                   e.locations[i].line, e.locations[i].col});
    }
    return result;
  }
  module->setSrcInfo({abspath, 0, 0, 0});
  return Compiler::ParserError::success();
}

void Compiler::compile() {
  pm->run(module.get());
  llvisitor->visit(module.get());
}

} // namespace codon
