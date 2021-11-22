#include "jit.h"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

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

class CaptureOutput {
private:
  std::vector<char> buf;
  int outpipe[2];
  int saved;
  bool stopped;
  std::string result;

  llvm::Error err(const std::string &msg) {
    return llvm::make_error<error::IOErrorInfo>(msg);
  }

public:
  static constexpr size_t BUFFER_SIZE = 65536;

  CaptureOutput() : buf(BUFFER_SIZE), outpipe(), saved(0), stopped(false), result() {}

  std::string getResult() const { return result; }

  llvm::Error start() {
    if (stopped)
      return llvm::Error::success();

    saved = dup(STDOUT_FILENO);
    if (saved == -1)
      return err("dup(STDOUT_FILENO) call failed");

    if (pipe(outpipe) != 0)
      return err("pipe(outpipe) call failed");

    if (dup2(outpipe[1], STDOUT_FILENO) == -1)
      return err("dup2(outpipe[1], STDOUT_FILENO) call failed");

    if (close(outpipe[1]) == -1)
      return err("close(outpipe[1]) call failed");

    return llvm::Error::success();
  }

  llvm::Error stop() {
    if (stopped)
      return llvm::Error::success();
    stopped = true;

    if (fflush(stdout) != 0)
      return err("fflush(stdout) call failed");

    auto count = read(outpipe[0], buf.data(), buf.size() - 1);
    if (count == -1)
      return err("read(outpipe[0], buf.data(), buf.size() - 1) call failed");

    if (dup2(saved, STDOUT_FILENO) == -1)
      return err("dup2(saved, STDOUT_FILENO) call failed");

    result = std::string(buf.data(), count);
    return llvm::Error::success();
  }

  ~CaptureOutput() {
    // seqassert(dup2(saved, STDOUT_FILENO) != -1, "IO error when capturing stdout");
  }
};
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

llvm::Expected<std::string> JIT::run(const ir::Func *input,
                                     const std::vector<ir::Var *> &newGlobals) {
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

  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}))
    return std::move(err);

  auto func = engine->lookup(name);
  if (auto err = func.takeError())
    return std::move(err);

  auto *repl = (InputFunc *)func->getAddress();
  std::string output;
  try {
    CaptureOutput capture;
    if (auto err = capture.start())
      return std::move(err);
    (*repl)();
    if (auto err = capture.stop())
      return std::move(err);
    output = capture.getResult();
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
  return output;
}

llvm::Expected<std::string> JIT::exec(const std::string &code) {
  auto *cache = compiler->getCache();
  ast::StmtPtr node = ast::parseCode(cache, JIT_FILENAME, code, /*startLine=*/0);

  auto sctx = cache->imports[MAIN_IMPORT].ctx;
  auto preamble = std::make_shared<ast::SimplifyVisitor::Preamble>();
  try {
    auto *e = node->getSuite()
                  ? const_cast<ast::SuiteStmt *>(node->getSuite())->lastInBlock()
                  : &node;
    if (e)
      if (auto ex = (*e)->getExpr()) {
        *e = std::make_shared<ast::PrintStmt>(
            std::vector<ast::ExprPtr>{std::make_shared<ast::CallExpr>(
                std::make_shared<ast::IdExpr>("_jit_display"), ex->expr,
                std::make_shared<ast::StringExpr>(mode))},
            false);
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
    return llvm::make_error<error::ParserErrorInfo>(e);
  }
}

} // namespace jit
} // namespace codon
