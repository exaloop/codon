// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "jit.h"

#include <sstream>

#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/doc/doc.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon {
namespace jit {
namespace {
typedef int MainFunc(int, char **);
typedef void InputFunc();
typedef void *PyWrapperFunc(void *);

const std::string JIT_FILENAME = "<jit>";
} // namespace

JIT::JIT(const std::string &argv0, const std::string &mode)
    : compiler(std::make_unique<Compiler>(argv0, Compiler::Mode::JIT)),
      engine(std::make_unique<Engine>()), pydata(std::make_unique<PythonData>()),
      mode(mode) {
  compiler->getLLVMVisitor()->setJIT(true);
}

llvm::Error JIT::init() {
  auto *cache = compiler->getCache();
  auto *module = compiler->getModule();
  auto *pm = compiler->getPassManager();
  auto *llvisitor = compiler->getLLVMVisitor();

  auto transformed =
      ast::SimplifyVisitor::apply(cache, std::make_shared<ast::SuiteStmt>(),
                                  JIT_FILENAME, {}, compiler->getEarlyDefines());

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

  auto *main = func->toPtr<MainFunc>();
  (*main)(0, nullptr);
  return llvm::Error::success();
}

llvm::Error JIT::compile(const ir::Func *input) {
  auto *module = compiler->getModule();
  auto *pm = compiler->getPassManager();
  auto *llvisitor = compiler->getLLVMVisitor();

  Timer t1("jit/ir");
  pm->run(module);
  t1.log();

  Timer t2("jit/llvm");
  auto pair = llvisitor->takeModule(module);
  t2.log();

  Timer t3("jit/engine");
  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}))
    return std::move(err);
  t3.log();

  return llvm::Error::success();
}

llvm::Expected<ir::Func *> JIT::compile(const std::string &code,
                                        const std::string &file, int line) {
  auto *cache = compiler->getCache();
  auto sctx = cache->imports[MAIN_IMPORT].ctx;
  auto preamble = std::make_shared<std::vector<ast::StmtPtr>>();

  ast::Cache bCache = *cache;
  ast::SimplifyContext bSimplify = *sctx;
  ast::SimplifyContext stdlibSimplify = *(cache->imports[STDLIB_IMPORT].ctx);
  ast::TypeContext bType = *(cache->typeCtx);
  ast::TranslateContext bTranslate = *(cache->codegenCtx);
  try {
    ast::StmtPtr node = ast::parseCode(cache, file.empty() ? JIT_FILENAME : file, code,
                                       /*startLine=*/line);
    auto *e = node->getSuite() ? node->getSuite()->lastInBlock() : &node;
    if (e)
      if (auto ex = const_cast<ast::ExprStmt *>((*e)->getExpr())) {
        *e = std::make_shared<ast::ExprStmt>(std::make_shared<ast::CallExpr>(
            std::make_shared<ast::IdExpr>("_jit_display"), ex->expr->clone(),
            std::make_shared<ast::StringExpr>(mode)));
      }
    auto s = ast::SimplifyVisitor(sctx, preamble).transform(node);
    if (!cache->errors.empty())
      throw exc::ParserException();
    auto simplified = std::make_shared<ast::SuiteStmt>();
    for (auto &s : *preamble)
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

    return func;
  } catch (const exc::ParserException &exc) {
    std::vector<error::Message> messages;
    if (exc.messages.empty()) {
      for (auto &e : cache->errors) {
        for (unsigned i = 0; i < e.messages.size(); i++) {
          if (!e.messages[i].empty())
            messages.emplace_back(e.messages[i], e.locations[i].file,
                                  e.locations[i].line, e.locations[i].col,
                                  e.locations[i].len, e.errorCode);
        }
      }
    }

    for (auto &f : cache->functions)
      for (auto &r : f.second.realizations)
        if (!(in(bCache.functions, f.first) &&
              in(bCache.functions[f.first].realizations, r.first)) &&
            r.second->ir) {
          cache->module->remove(r.second->ir);
        }
    *cache = bCache;
    *(cache->imports[MAIN_IMPORT].ctx) = bSimplify;
    *(cache->imports[STDLIB_IMPORT].ctx) = stdlibSimplify;
    *(cache->typeCtx) = bType;
    *(cache->codegenCtx) = bTranslate;

    if (exc.messages.empty())
      return llvm::make_error<error::ParserErrorInfo>(messages);
    else
      return llvm::make_error<error::ParserErrorInfo>(exc);
  }
}

llvm::Expected<void *> JIT::address(const ir::Func *input) {
  if (auto err = compile(input))
    return std::move(err);

  const std::string name = ir::LLVMVisitor::getNameForFunction(input);
  auto func = engine->lookup(name);
  if (auto err = func.takeError())
    return std::move(err);

  return (void *)func->getValue();
}

llvm::Expected<std::string> JIT::run(const ir::Func *input) {
  auto result = address(input);
  if (auto err = result.takeError())
    return std::move(err);

  auto *repl = (InputFunc *)result.get();
  try {
    (*repl)();
  } catch (const runtime::JITError &e) {
    return handleJITError(e);
  }
  return runtime::getCapturedOutput();
}

llvm::Expected<std::string>
JIT::execute(const std::string &code, const std::string &file, int line, bool debug) {
  if (debug)
    fmt::print(stderr, "[codon::jit::execute] code:\n{}-----\n", code);
  auto result = compile(code, file, line);
  if (auto err = result.takeError())
    return std::move(err);
  if (auto err = compile(result.get()))
    return std::move(err);
  return run(result.get());
}

llvm::Error JIT::handleJITError(const runtime::JITError &e) {
  std::vector<std::string> backtrace;
  for (auto pc : e.getBacktrace()) {
    auto line = engine->getDebugListener()->getPrettyBacktrace(pc);
    if (line && !line->empty())
      backtrace.push_back(*line);
  }
  return llvm::make_error<error::RuntimeErrorInfo>(e.getOutput(), e.getType(), e.what(),
                                                   e.getFile(), e.getLine(), e.getCol(),
                                                   backtrace);
}

namespace {
std::string buildKey(const std::string &name, const std::vector<std::string> &types) {
  std::stringstream key;
  key << name;
  for (const auto &t : types) {
    key << "|" << t;
  }
  return key.str();
}

std::string buildPythonWrapper(const std::string &name, const std::string &wrapname,
                               const std::vector<std::string> &types,
                               const std::string &pyModule,
                               const std::vector<std::string> &pyVars) {
  std::stringstream wrap;
  wrap << "@export\n";
  wrap << "def " << wrapname << "(args: cobj) -> cobj:\n";
  for (unsigned i = 0; i < types.size(); i++) {
    wrap << "    "
         << "a" << i << " = " << types[i] << ".__from_py__(PyTuple_GetItem(args, " << i
         << "))\n";
  }
  for (unsigned i = 0; i < pyVars.size(); i++) {
    wrap << "    "
         << "py" << i << " = pyobj._get_module(\"" << pyModule << "\")._getattr(\""
         << pyVars[i] << "\")\n";
  }
  wrap << "    return " << name << "(";
  for (unsigned i = 0; i < types.size(); i++) {
    if (i > 0)
      wrap << ", ";
    wrap << "a" << i;
  }
  for (unsigned i = 0; i < pyVars.size(); i++) {
    if (i > 0 || types.size() > 0)
      wrap << ", ";
    wrap << "py" << i;
  }
  wrap << ").__to_py__()\n";

  return wrap.str();
}
} // namespace

JIT::PythonData::PythonData() : cobj(nullptr), cache() {}

ir::types::Type *JIT::PythonData::getCObjType(ir::Module *M) {
  if (cobj)
    return cobj;
  cobj = M->getPointerType(M->getByteType());
  return cobj;
}

JITResult JIT::executeSafe(const std::string &code, const std::string &file, int line,
                           bool debug) {
  auto result = execute(code, file, line, debug);
  if (auto err = result.takeError()) {
    auto errorInfo = llvm::toString(std::move(err));
    return JITResult::error(errorInfo);
  }
  return JITResult::success(nullptr);
}

JITResult JIT::executePython(const std::string &name,
                             const std::vector<std::string> &types,
                             const std::string &pyModule,
                             const std::vector<std::string> &pyVars, void *arg,
                             bool debug) {
  auto key = buildKey(name, types);
  auto &cache = pydata->cache;
  auto it = cache.find(key);
  PyWrapperFunc *wrap;

  if (it != cache.end()) {
    auto *wrapper = it->second;
    const std::string name = ir::LLVMVisitor::getNameForFunction(wrapper);
    auto func = llvm::cantFail(engine->lookup(name));
    wrap = func.toPtr<PyWrapperFunc>();
  } else {
    static int idx = 0;
    auto wrapname = "__codon_wrapped__" + name + "_" + std::to_string(idx++);
    auto wrapper = buildPythonWrapper(name, wrapname, types, pyModule, pyVars);
    if (debug)
      fmt::print(stderr, "[codon::jit::executePython] wrapper:\n{}-----\n", wrapper);
    if (auto err = compile(wrapper).takeError()) {
      auto errorInfo = llvm::toString(std::move(err));
      return JITResult::error(errorInfo);
    }

    auto *M = compiler->getModule();
    auto *func = M->getOrRealizeFunc(wrapname, {pydata->getCObjType(M)});
    seqassertn(func, "could not access wrapper func '{}'", wrapname);
    cache.emplace(key, func);

    auto result = address(func);
    if (auto err = result.takeError()) {
      auto errorInfo = llvm::toString(std::move(err));
      return JITResult::error(errorInfo);
    }
    wrap = (PyWrapperFunc *)result.get();
  }

  try {
    auto *ans = (*wrap)(arg);
    return JITResult::success(ans);
  } catch (const runtime::JITError &e) {
    auto err = handleJITError(e);
    auto errorInfo = llvm::toString(std::move(err));
    return JITResult::error(errorInfo);
  }
}

JIT *jitInit(const std::string &name) {
  auto jit = new JIT(name);
  llvm::cantFail(jit->init());
  return jit;
}

JITResult jitExecutePython(JIT *jit, const std::string &name,
                           const std::vector<std::string> &types,
                           const std::string &pyModule,
                           const std::vector<std::string> &pyVars, void *arg,
                           bool debug) {
  return jit->executePython(name, types, pyModule, pyVars, arg, debug);
}

JITResult jitExecuteSafe(JIT *jit, const std::string &code, const std::string &file,
                         int line, bool debug) {
  return jit->executeSafe(code, file, line, debug);
}

std::string getJITLibrary() { return ast::library_path(); }

} // namespace jit
} // namespace codon
