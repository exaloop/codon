#include "jit.h"

#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/doc/doc.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/typecheck.h"
#include "codon/runtime/lib.h"

namespace codon {
namespace jit {
namespace {
typedef int MainFunc(int, char **);
typedef void InputFunc();

const std::string JIT_FILENAME = "<jit>";
} // namespace

JIT::JIT(const std::string &argv0)
    : compiler(std::make_unique<Compiler>(argv0, /*debug=*/true)) {
  if (auto e = Engine::create()) {
    engine = std::move(e.get());
  } else {
    engine = {};
    seqassert(false, "JIT engine creation error");
  }
  compiler->getLLVMVisitor()->setJIT(true);
}

llvm::Error JIT::init() {
  auto *cache = compiler->getCache();
  auto *module = compiler->getModule();
  auto *llvisitor = compiler->getLLVMVisitor();

  auto transformed = ast::SimplifyVisitor::apply(
      cache, std::make_shared<ast::SuiteStmt>(), JIT_FILENAME, {});
  auto typechecked = ast::TypecheckVisitor::apply(cache, std::move(transformed));
  ast::TranslateVisitor::apply(cache, std::move(typechecked));
  cache->isJit = true; // we still need main(), so set isJit after it has been set
  module->setSrcInfo({JIT_FILENAME, 0, 0, 0});

  module->accept(*llvisitor);
  auto pair = llvisitor->takeModule();

  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}))
    return err;

  auto func = engine->lookup("main");
  if (auto err = func.takeError())
    return err;

  auto *main = (MainFunc *)func->getAddress();
  (*main)(0, nullptr);
  return llvm::Error::success();
}

llvm::Error JIT::run(const ir::Func *input, const std::vector<ir::Var *> &newGlobals) {
  auto *llvisitor = compiler->getLLVMVisitor();
  const std::string name = ir::LLVMVisitor::getNameForFunction(input);
  llvisitor->registerGlobal(input);
  for (auto *var : newGlobals) {
    llvisitor->registerGlobal(var);
  }
  for (auto *var : newGlobals) {
    if (auto *func = ir::cast<ir::Func>(var))
      func->accept(*llvisitor);
  }
  input->accept(*llvisitor);
  auto pair = llvisitor->takeModule();
  llvm::StripDebugInfo(*pair.first); // TODO: needed?

  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}))
    return err;

  auto func = engine->lookup(name);
  if (auto err = func.takeError())
    return err;

  auto *repl = (InputFunc *)func->getAddress();
  try {
    (*repl)();
  } catch (const seq_jit_error &err) {
    return llvm::make_error<error::RuntimeErrorInfo>(
        err.getType(), err.what(), err.getFile(), err.getLine(), err.getCol());
  }
  return llvm::Error::success();
}

std::pair<ir::Func *, std::vector<ir::Var *>>
JIT::transformSimplified(const ast::StmtPtr &simplified) {
  auto *cache = compiler->getCache();
  auto typechecked = ast::TypecheckVisitor::apply(cache, simplified);
  std::vector<std::string> globalNames;
  for (auto &g : cache->globals) {
    if (!g.second)
      globalNames.push_back(g.first);
  }
  // add newly realized functions
  std::vector<ast::StmtPtr> v;
  std::vector<ir::Func **> frs;
  v.push_back(typechecked);
  for (auto &p : cache->pendingRealizations) {
    v.push_back(cache->functions[p.first].ast);
    frs.push_back(&cache->functions[p.first].realizations[p.second]->ir);
  }
  auto func =
      ast::TranslateVisitor::apply(cache, std::make_shared<ast::SuiteStmt>(v, false));
  cache->jitCell++;

  std::vector<ir::Var *> globalVars;
  for (auto &g : globalNames) {
    seqassert(cache->globals[g], "JIT global {} not set", g);
    globalVars.push_back(cache->globals[g]);
  }
  for (auto &i : frs) {
    seqassert(*i, "JIT fn not set");
    globalVars.push_back(*i);
  }
  return {func, globalVars};
}

llvm::Error JIT::exec(const std::string &code) {
  auto *cache = compiler->getCache();
  ast::StmtPtr node = ast::parseCode(cache, JIT_FILENAME, code, /*startLine=*/0);

  auto sctx = cache->imports[MAIN_IMPORT].ctx;
  auto preamble = std::make_shared<ast::SimplifyVisitor::Preamble>();
  try {
    auto s = ast::SimplifyVisitor(sctx, preamble).transform(node);
    auto simplified = std::make_shared<ast::SuiteStmt>();
    for (auto &s : preamble->globals)
      simplified->stmts.push_back(s);
    for (auto &s : preamble->functions)
      simplified->stmts.push_back(s);
    simplified->stmts.push_back(s);
    // TODO: unroll on errors...

    auto p = transformSimplified(simplified);
    return run(p.first, p.second);
  } catch (const exc::ParserException &e) {
    return llvm::make_error<error::ParserErrorInfo>(e);
  }
}

} // namespace jit
} // namespace codon
