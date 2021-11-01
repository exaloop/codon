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
                   const std::vector<std::string> &disabledPasses, bool isTest)
    : argv0(argv0), debug(debug), input(), plm(std::make_unique<PluginManager>()),
      cache(std::make_unique<ast::Cache>(argv0)),
      module(std::make_unique<ir::Module>()),
      pm(std::make_unique<ir::transform::PassManager>(debug && !isTest,
                                                      disabledPasses)),
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
Compiler::parse(bool isCode, const std::string &file, const std::string &code,
                int startLine, int testFlags,
                const std::unordered_map<std::string, std::string> &defines) {
  input = file;
  std::string abspath =
      (file != "-") ? std::filesystem::absolute(std::filesystem::path(file)).string()
                    : file;
  try {
    Timer t1("parse");
    ast::StmtPtr codeStmt = isCode
                                ? ast::parseCode(cache.get(), abspath, code, startLine)
                                : ast::parseFile(cache.get(), abspath);
    t1.log();

    cache->module0 = file;
    if (testFlags)
      cache->testFlags = testFlags;

    Timer t2("simplify");
    auto transformed = ast::SimplifyVisitor::apply(cache.get(), std::move(codeStmt),
                                                   abspath, defines, (testFlags > 1));
    t2.log();

    Timer t3("typecheck");
    auto typechecked =
        ast::TypecheckVisitor::apply(cache.get(), std::move(transformed));
    t3.log();

    Timer t4("translate");
    ast::TranslateVisitor::apply(cache.get(), std::move(typechecked));
    t4.log();
  } catch (const exc::ParserException &e) {
    auto result = Compiler::ParserError::failure();
    for (unsigned i = 0; i < e.messages.size(); i++) {
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
Compiler::parseFile(const std::string &file, int testFlags,
                    const std::unordered_map<std::string, std::string> &defines) {
  return parse(/*isCode=*/false, file, /*code=*/"", /*startLine=*/0, testFlags,
               defines);
}

Compiler::ParserError
Compiler::parseCode(const std::string &file, const std::string &code, int startLine,
                    int testFlags,
                    const std::unordered_map<std::string, std::string> &defines) {
  return parse(/*isCode=*/true, file, code, startLine, testFlags, defines);
}

void Compiler::compile() {
  pm->run(module.get());
  llvisitor->visit(module.get());
}

Compiler::ParserError Compiler::docgen(const std::vector<std::string> &files,
                                       std::string *output) {
  try {
    auto j = ast::DocVisitor::apply(argv0, files);
    if (output)
      *output = j->toString();
  } catch (exc::ParserException &e) {
    auto result = Compiler::ParserError::failure();
    for (unsigned i = 0; i < e.messages.size(); i++) {
      if (!e.messages[i].empty())
        result.messages.push_back({e.messages[i], e.locations[i].file,
                                   e.locations[i].line, e.locations[i].col});
    }
    return result;
  }
  return Compiler::ParserError::success();
}

} // namespace codon
