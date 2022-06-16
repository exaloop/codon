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
    : compiler(std::make_unique<Compiler>(argv0, Compiler::Mode::JIT)), mode(mode) {
  if (auto e = Engine::create()) {
    engine = std::move(e.get());
  } else {
    engine = {};
    seqassertn(false, "JIT engine creation error");
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
  auto pair = llvisitor->takeModule(module);

  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}))
    return err;

  auto func = engine->lookup("main");
  if (auto err = func.takeError())
    return err;

  auto *main = (MainFunc *)func->getAddress();
  (*main)(0, nullptr);
  return llvm::Error::success();
}

llvm::Expected<std::string> JIT::run(const ir::Func *input) {
  auto *module = compiler->getModule();
  auto *pm = compiler->getPassManager();
  auto *llvisitor = compiler->getLLVMVisitor();

  Timer t1("jit/ir");
  pm->run(module);
  t1.log();

  const std::string name = ir::LLVMVisitor::getNameForFunction(input);

  Timer t2("jit/llvm");
  auto pair = llvisitor->takeModule(module);
  t2.log();

  Timer t3("jit/engine");
  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}))
    return std::move(err);

  auto func = engine->lookup(name);
  if (auto err = func.takeError())
    return std::move(err);

  auto *repl = (InputFunc *)func->getAddress();
  t3.log();

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

llvm::Expected<std::string> JIT::execute(const std::string &code) {
  auto *cache = compiler->getCache();
  ast::StmtPtr node = ast::parseCode(cache, JIT_FILENAME, code, /*startLine=*/0);

  auto sctx = cache->imports[MAIN_IMPORT].ctx;
  auto preamble = std::make_shared<ast::SimplifyVisitor::Preamble>();

  ast::Cache bCache = *cache;
  ast::SimplifyContext bSimplify = *sctx;
  ast::TypeContext bType = *(cache->typeCtx);
  ast::TranslateContext bTranslate = *(cache->codegenCtx);
  try {
    auto *e = node->getSuite() ? node->getSuite()->lastInBlock() : &node;
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
    // TODO: unroll on errors...

    auto *cache = compiler->getCache();
    auto typechecked = ast::TypecheckVisitor::apply(cache, simplified);

    // add newly realized functions
    std::vector<ast::StmtPtr> v;
    std::vector<ir::Func **> frs;
    v.push_back(typechecked);
    for (auto &p : cache->pendingRealizations) {
      v.push_back(cache->functions[p.first].ast);
      frs.push_back(&cache->functions[p.first].realizations[p.second]->ir);
    }
    auto func =
        ast::TranslateVisitor::apply(cache, std::make_shared<ast::SuiteStmt>(v));
    cache->jitCell++;

    return run(func);
  } catch (const exc::ParserException &e) {
    *cache = bCache;
    *(cache->imports[MAIN_IMPORT].ctx) = bSimplify;
    *(cache->typeCtx) = bType;
    *(cache->codegenCtx) = bTranslate;
    return llvm::make_error<error::ParserErrorInfo>(e);
  }
}

JITResult JIT::executeSafe(const std::string &code) {
  auto result = this->execute(code);
  if (auto err = result.takeError()) {
    auto errorInfo = llvm::toString(std::move(err));
    return JITResult::error(errorInfo);
  }
  return JITResult::success(result.get());
}

} // namespace jit
} // namespace codon
