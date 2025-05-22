// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "jit.h"

#include <sstream>

#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/doc/doc.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/scoping/scoping.h"
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

  auto typechecked = ast::TypecheckVisitor::apply(
      cache, cache->N<ast::SuiteStmt>(), JIT_FILENAME, {}, compiler->getEarlyDefines());
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
  auto preamble = cache->N<ast::SuiteStmt>();

  ast::Cache bCache = *cache;
  auto bTypecheck = *sctx;
  auto stdlibTypecheck = *(cache->imports[STDLIB_IMPORT].ctx);
  ast::TypeContext bType = *(cache->typeCtx);
  ast::TranslateContext bTranslate = *(cache->codegenCtx);
  try {
    auto nodeOrErr = ast::parseCode(cache, file.empty() ? JIT_FILENAME : file, code,
                                    /*startLine=*/line);
    if (!nodeOrErr)
      throw exc::ParserException(nodeOrErr.takeError());
    auto *node = *nodeOrErr;

    ast::Stmt **e = &node;
    while (auto se = ast::cast<ast::SuiteStmt>(*e)) {
      if (se->empty())
        break;
      e = &se->back();
    }
    if (e)
      if (auto ex = ast::cast<ast::ExprStmt>(*e)) {
        *e = cache->N<ast::ExprStmt>(cache->N<ast::CallExpr>(
            cache->N<ast::IdExpr>("_jit_display"), clone(ex->getExpr()),
            cache->N<ast::StringExpr>(mode)));
      }
    auto tv = ast::TypecheckVisitor(sctx, preamble);
    if (auto err = ast::ScopingVisitor::apply(sctx->cache, node))
      throw exc::ParserException(std::move(err));
    node = tv.transform(node);

    if (!cache->errors.empty())
      throw exc::ParserException(cache->errors);
    auto typechecked = cache->N<ast::SuiteStmt>();
    for (auto &s : *preamble)
      typechecked->addStmt(s);
    typechecked->addStmt(node);
    // TODO: unroll on errors...

    // add newly realized functions
    std::vector<ast::Stmt *> v;
    std::vector<ir::Func **> frs;
    v.push_back(typechecked);
    for (auto &p : cache->pendingRealizations) {
      v.push_back(cache->functions[p.first].ast);
      frs.push_back(&cache->functions[p.first].realizations[p.second]->ir);
    }
    auto func = ast::TranslateVisitor::apply(cache, cache->N<ast::SuiteStmt>(v));
    cache->jitCell++;

    return func;
  } catch (const exc::ParserException &exc) {
    for (auto &f : cache->functions)
      for (auto &r : f.second.realizations)
        if (!(in(bCache.functions, f.first) &&
              in(bCache.functions[f.first].realizations, r.first)) &&
            r.second->ir) {
          cache->module->remove(r.second->ir);
        }
    *cache = bCache;
    *(cache->imports[MAIN_IMPORT].ctx) = bTypecheck;
    *(cache->imports[STDLIB_IMPORT].ctx) = stdlibTypecheck;
    *(cache->typeCtx) = bType;
    *(cache->codegenCtx) = bTranslate;

    return llvm::make_error<error::ParserErrorInfo>(exc.getErrors());
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

JIT::JITResult JIT::executeSafe(const std::string &code, const std::string &file,
                                int line, bool debug) {
  auto result = execute(code, file, line, debug);
  if (auto err = result.takeError()) {
    auto errorInfo = llvm::toString(std::move(err));
    return JITResult::error(errorInfo);
  }
  return JITResult::success();
}

JIT::JITResult JIT::executePython(const std::string &name,
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

} // namespace jit
} // namespace codon

void *jit_init(char *name) {
  auto jit = new codon::jit::JIT(std::string(name));
  llvm::cantFail(jit->init());
  return jit;
}

void jit_exit(void *jit) { delete ((codon::jit::JIT *)jit); }

CJITResult jit_execute_python(void *jit, char *name, char **types, size_t types_size,
                              char *pyModule, char **py_vars, size_t py_vars_size,
                              void *arg, uint8_t debug) {
  std::vector<std::string> cppTypes;
  cppTypes.reserve(types_size);
  for (size_t i = 0; i < types_size; i++)
    cppTypes.emplace_back(types[i]);
  std::vector<std::string> cppPyVars;
  cppPyVars.reserve(py_vars_size);
  for (size_t i = 0; i < py_vars_size; i++)
    cppPyVars.emplace_back(py_vars[i]);
  auto t = ((codon::jit::JIT *)jit)
               ->executePython(std::string(name), cppTypes, std::string(pyModule),
                               cppPyVars, arg, bool(debug));
  void *result = t.result;
  char *message =
      t.message.empty() ? nullptr : strndup(t.message.c_str(), t.message.size());
  return {result, message};
}

CJITResult jit_execute_safe(void *jit, char *code, char *file, int32_t line,
                            uint8_t debug) {
  auto t = ((codon::jit::JIT *)jit)
               ->executeSafe(std::string(code), std::string(file), line, bool(debug));
  void *result = t.result;
  char *message =
      t.message.empty() ? nullptr : strndup(t.message.c_str(), t.message.size());
  return {result, message};
}

char *get_jit_library() {
  auto t = codon::ast::library_path();
  return strndup(t.c_str(), t.size());
}
