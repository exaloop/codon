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

JIT::JIT(const std::string &argv0, const std::string &mode)
    : compiler(std::make_unique<Compiler>(argv0, /*debug=*/true)), mode(mode) {
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
  auto *pm = compiler->getPassManager();
  auto *llvisitor = compiler->getLLVMVisitor();

  auto transformed = ast::SimplifyVisitor::apply(
      cache, std::make_shared<ast::SuiteStmt>(), JIT_FILENAME, {});

  auto typechecked = ast::TypecheckVisitor::apply(cache, std::move(transformed));
  ast::TranslateVisitor::apply(cache, std::move(typechecked));
  cache->isJit = true; // we still need main(), so set isJit after it has been set
  module->setSrcInfo({JIT_FILENAME, 0, 0, 0});

  pm->run(module);
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

llvm::Expected<std::string> JIT::run(const ir::Func *input,
                                     const std::vector<ir::Var *> &newGlobals) {
  auto *module = compiler->getModule();
  auto *pm = compiler->getPassManager();
  auto *llvisitor = compiler->getLLVMVisitor();
  pm->run(module);

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

  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}))
    return std::move(err);

  auto func = engine->lookup(name);
  if (auto err = func.takeError())
    return std::move(err);

  auto *repl = (InputFunc *)func->getAddress();
  try {
    (*repl)();
  } catch (const JITError &e) {
    std::vector<std::string> backtrace;
    for (auto pc : e.getBacktrace()) {
      auto line = engine->getDebugListener()->getPrettyBacktrace(pc);
      if (line && !line->empty())
        backtrace.push_back(*line);
    }
    return llvm::make_error<error::RuntimeErrorInfo>(e.getOutput(), e.getType(),
                                                     e.what(), e.getFile(), e.getLine(),
                                                     e.getCol(), backtrace);
  }
  return getCapturedOutput();
}

llvm::Expected<std::string> JIT::exec(const std::string &code) {
  auto *cache = compiler->getCache();
  ast::StmtPtr node = ast::parseCode(cache, JIT_FILENAME, code, /*startLine=*/0);

  auto sctx = cache->imports[MAIN_IMPORT].ctx;
  auto preamble = std::make_shared<ast::SimplifyVisitor::Preamble>();


  ast::Cache bCache = *cache;
  ast::SimplifyContext bSimplify = *sctx;
  ast::TypeContext bType = *(cache->typeCtx);
  ast::TranslateContext bTranslate = *(cache->codegenCtx);
  try {
    auto *e = node->getSuite()
                  ? const_cast<ast::SuiteStmt *>(node->getSuite())->lastInBlock()
                  : &node;
    if (e)
      if (auto ex = const_cast<ast::ExprStmt *>((*e)->getExpr())) {
        *e = std::make_shared<ast::IfStmt>(
            std::make_shared<ast::CallExpr>(std::make_shared<ast::IdExpr>("isinstance"),
                                            ex->expr->clone(),
                                            std::make_shared<ast::IdExpr>("void")),
            ex->clone(),
            std::make_shared<ast::ExprStmt>(std::make_shared<ast::CallExpr>(
                std::make_shared<ast::IdExpr>("_jit_display"), ex->expr->clone(),
                std::make_shared<ast::StringExpr>(mode))));
      }
    auto s = ast::SimplifyVisitor(sctx, preamble).transform(node);
    auto simplified = std::make_shared<ast::SuiteStmt>();
    for (auto &s : preamble->globals)
      simplified->stmts.push_back(s);
    for (auto &s : preamble->functions)
      simplified->stmts.push_back(s);
    simplified->stmts.push_back(s);
    // LOG("{}\n", simplified->toString(1));
    // TODO: unroll on errors...

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
    return run(func, globalVars);
  } catch (const exc::ParserException &e) {
    *cache = bCache;
    *(cache->imports[MAIN_IMPORT].ctx) = bSimplify;
    *(cache->typeCtx) = bType;
    *(cache->codegenCtx) = bTranslate;
    return llvm::make_error<error::ParserErrorInfo>(e);
  }
}

} // namespace jit
} // namespace codon
