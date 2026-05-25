// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

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
typedef void *JITClassNewFunc(void *);
typedef void *JITClassMethodFunc(void *, void *);

const std::string JIT_FILENAME = "<jit>";
} // namespace

JIT::JIT(const std::string &argv0, const std::string &mode,
         const std::string &stdlibRoot)
    : compiler(std::make_unique<Compiler>(argv0, Compiler::Mode::JIT,
                                          /*disabledPasses=*/std::vector<std::string>{},
                                          /*isTest=*/false,
                                          /*pyNumerics=*/false, /*pyExtension=*/false)),
      engine(std::make_unique<Engine>()), pydata(std::make_unique<PythonData>()),
      jitClassData(std::make_unique<JITClassData>()), mode(mode), forgetful(false) {
  if (!stdlibRoot.empty())
    compiler->getCache()->fs->add_search_path(stdlibRoot);
  compiler->getLLVMVisitor()->setJIT(true);
}

JIT::JITClassRoot::JITClassRoot(void *ptr) : slot(std::make_unique<void *>(ptr)) {
  seq_gc_add_roots(slot.get(), slot.get() + 1);
}

JIT::JITClassRoot::~JITClassRoot() {
  if (slot)
    seq_gc_remove_roots(slot.get(), slot.get() + 1);
}

JIT::JITClassRoot &JIT::JITClassRoot::operator=(JITClassRoot &&other) noexcept {
  if (this != &other) {
    if (slot)
      seq_gc_remove_roots(slot.get(), slot.get() + 1);
    slot = std::move(other.slot);
  }
  return *this;
}

JIT::JITClassInstance::JITClassInstance(std::string className,
                                        std::string nativeClassName, void *nativePtr)
    : className(std::move(className)), nativeClassName(std::move(nativeClassName)),
      nativePtr(nativePtr), root(nativePtr) {}

void collectExecutableStmts(ast::Stmt *s, ast::SuiteStmt *final) {
  if (ast::cast<ast::FunctionStmt>(s) || ast::cast<ast::ClassStmt>(s) ||
      ast::cast<ast::CommentStmt>(s))
    return;
  if (auto ss = ast::cast<ast::SuiteStmt>(s)) {
    for (auto &si : *ss)
      collectExecutableStmts(si, final);
  } else if (s) {
    final->addStmt(ast::clean_clone(s));
  }
}

llvm::Error JIT::init(bool forgetful) {
  if (forgetful) {
    this->forgetful = true;
    auto fs = std::make_shared<ast::ResourceFilesystem>(compiler->getArgv0(), "",
                                                        /*allowExternal=*/false);
    compiler->getCache()->fs = fs;
  }

  auto *cache = compiler->getCache();
  auto *module = compiler->getModule();
  auto *pm = compiler->getPassManager();
  auto *llvisitor = compiler->getLLVMVisitor();

  cache->isJit = true;
  auto typechecked = ast::TypecheckVisitor::apply(
      cache, cache->N<ast::SuiteStmt>(), JIT_FILENAME, {}, compiler->getEarlyDefines());
  cache->isJit =
      false; // we still need main(), so pause isJit first time during translation
  ast::TranslateVisitor::apply(cache, std::move(typechecked));
  cache->isJit = true;
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

llvm::Error JIT::compile(const ir::Func *input, llvm::orc::ResourceTrackerSP rt) {
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
  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}, rt))
    return std::move(err);
  t3.log();

  return llvm::Error::success();
}

JITState::JITState(ast::Cache *cache, bool forgetful)
    : cache(cache), forgetful(forgetful), bCache(*cache),
      mainCtx(*(cache->imports[MAIN_IMPORT].ctx)),
      stdlibCtx(*(cache->imports[STDLIB_IMPORT].ctx)), typeCtx(*(cache->typeCtx)),
      translateCtx(*(cache->codegenCtx)) {}

void JITState::undo() {
  if (!forgetful)
    undoUnusedIR();

  *cache = bCache;
  *(cache->imports[MAIN_IMPORT].ctx) = mainCtx;
  *(cache->imports[STDLIB_IMPORT].ctx) = stdlibCtx;
  *(cache->typeCtx) = typeCtx;
  *(cache->codegenCtx) = translateCtx;

  if (forgetful)
    cleanUpRealizations();
}

void JITState::undoUnusedIR() {
  // Clean-up unused IR nodes made before Typechecker raised an error
  for (auto &f : cache->functions) {
    for (auto &r : f.second.realizations) {
      if (!(in(bCache.functions, f.first) &&
            in(bCache.functions[f.first].realizations, r.first)) &&
          r.second->ir) {
        cache->module->remove(r.second->ir);
      }
    }
  }
}

void JITState::cleanUpRealizations() {
  // Clean-up IR nodes after single JIT input
  // Nothing should be done here with a proper arena support.
}

llvm::Expected<ir::Func *> JIT::compile(const std::string &code,
                                        const std::string &file, int line) {
  auto *cache = compiler->getCache();
  auto preamble = cache->N<ast::SuiteStmt>();

  JITState state(cache, forgetful);

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
    auto sctx = cache->imports[MAIN_IMPORT].ctx;
    if (auto err = ast::ScopingVisitor::apply(sctx->cache, node, &sctx->globalShadows))
      throw exc::ParserException(std::move(err));
    auto tv = ast::TypecheckVisitor::apply(sctx, node, JIT_FILENAME);
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
    state.undo();

    return llvm::make_error<error::ParserErrorInfo>(exc.getErrors());
  }
}

llvm::Expected<void *> JIT::address(const ir::Func *input,
                                    llvm::orc::ResourceTrackerSP rt) {
  if (auto err = compile(input, rt))
    return std::move(err);

  const std::string name = ir::LLVMVisitor::getNameForFunction(input);
  auto func = engine->lookup(name);
  if (auto err = func.takeError())
    return std::move(err);

  return (void *)func->getValue();
}

llvm::Expected<std::string> JIT::run(const ir::Func *input,
                                     llvm::orc::ResourceTrackerSP rt) {
  auto result = address(input, rt);
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

llvm::Expected<std::string> JIT::execute(const std::string &code,
                                         const std::string &file, int line, bool debug,
                                         llvm::orc::ResourceTrackerSP rt) {
  if (debug)
    fmt::print(stderr, "[codon::jit::execute] code:\n{}-----\n", code);

  std::unique_ptr<JITState> state = nullptr;
  if (forgetful)
    state = std::make_unique<JITState>(compiler->getCache(), forgetful);

  auto result = compile(code, file, line);
  if (auto err = result.takeError())
    return std::move(err);
  if (auto err = compile(result.get(), rt))
    return std::move(err);
  auto r = run(result.get());

  if (state)
    state->undo();

  return r;
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
std::string buildJITClassKey(const std::string &kind,
                             const std::string &nativeClassName,
                             const std::vector<std::string> &types,
                             const std::string &methodName = "") {
  std::stringstream key;
  key << kind << "|" << nativeClassName;
  if (!methodName.empty())
    key << "|" << methodName;
  for (const auto &t : types)
    key << "|" << t;
  return key.str();
}

std::string buildJITClassNewWrapper(const std::string &nativeClassName,
                                    const std::string &wrapname,
                                    const std::vector<std::string> &types) {
  std::stringstream wrap;
  wrap << "from internal.python import PyTuple_GetItem\n";
  wrap << "@export\n";
  wrap << "def " << wrapname << "(args: cobj) -> cobj:\n";
  for (unsigned i = 0; i < types.size(); i++) {
    wrap << "    a" << i << " = " << types[i] << ".__from_py__(PyTuple_GetItem(args, "
         << i << "))\n";
  }
  wrap << "    obj = " << nativeClassName << "(";
  for (unsigned i = 0; i < types.size(); i++) {
    if (i > 0)
      wrap << ", ";
    wrap << "a" << i;
  }
  wrap << ")\n";
  wrap << "    return obj.__raw__()\n";
  return wrap.str();
}

std::string buildJITClassMethodWrapper(const std::string &nativeClassName,
                                       const std::string &methodName,
                                       const std::string &wrapname,
                                       const std::vector<std::string> &types) {
  std::stringstream wrap;
  wrap << "from internal.python import PyTuple_GetItem\n";
  wrap << "@export\n";
  wrap << "def " << wrapname << "(self_obj: cobj, args: cobj) -> cobj:\n";
  wrap << "    self = type._force_cast(self_obj, " << nativeClassName << ")\n";
  for (unsigned i = 0; i < types.size(); i++) {
    wrap << "    a" << i << " = " << types[i] << ".__from_py__(PyTuple_GetItem(args, "
         << i << "))\n";
  }
  wrap << "    return self." << methodName << "(";
  for (unsigned i = 0; i < types.size(); i++) {
    if (i > 0)
      wrap << ", ";
    wrap << "a" << i;
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

JIT::JITClassData::JITClassData() : cobj(nullptr), ctorWrappers(), methodWrappers() {}

ir::types::Type *JIT::JITClassData::getCObjType(ir::Module *M) {
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

JIT::JITResult JIT::jitClassNew(const std::string &className,
                                const std::string &nativeClassName,
                                const std::vector<std::string> &types, void *args,
                                bool debug) {
  auto key = buildJITClassKey("new", nativeClassName, types);
  auto &cache = jitClassData->ctorWrappers;
  auto it = cache.find(key);
  JITClassNewFunc *wrap;

  if (it != cache.end()) {
    const std::string name = ir::LLVMVisitor::getNameForFunction(it->second);
    auto func = llvm::cantFail(engine->lookup(name));
    wrap = func.toPtr<JITClassNewFunc>();
  } else {
    auto idx = cache.size();
    auto wrapname = "__codon_jitclass_new_" + std::to_string(idx);
    auto wrapper = buildJITClassNewWrapper(nativeClassName, wrapname, types);
    if (debug)
      fmt::print(stderr, "[codon::jit::jitClassNew] wrapper:\n{}-----\n", wrapper);
    if (auto err = compile(wrapper).takeError()) {
      auto errorInfo = llvm::toString(std::move(err));
      return JITResult::error(errorInfo);
    }

    auto *M = compiler->getModule();
    auto *func = M->getOrRealizeFunc(wrapname, {jitClassData->getCObjType(M)});
    seqassertn(func, "could not access jitclass constructor wrapper '{}'", wrapname);
    cache.emplace(key, func);

    auto result = address(func);
    if (auto err = result.takeError()) {
      auto errorInfo = llvm::toString(std::move(err));
      return JITResult::error(errorInfo);
    }
    wrap = (JITClassNewFunc *)result.get();
  }

  try {
    auto *nativePtr = (*wrap)(args);
    auto *instance = new JITClassInstance(className, nativeClassName, nativePtr);
    return JITResult::success(instance);
  } catch (const runtime::JITError &e) {
    auto err = handleJITError(e);
    auto errorInfo = llvm::toString(std::move(err));
    return JITResult::error(errorInfo);
  }
}

JIT::JITResult JIT::jitClassCall(const std::string &className,
                                 JITClassInstance *instance,
                                 const std::string &methodName,
                                 const std::vector<std::string> &types, void *args,
                                 bool debug) {
  if (!instance)
    return JITResult::error("jitclass object has been released");
  if (instance->className != className)
    return JITResult::error(
        fmt::format("jitclass instance has type '{}', expected '{}'",
                    instance->className, className));

  auto key = buildJITClassKey("method", instance->nativeClassName, types, methodName);
  auto &cache = jitClassData->methodWrappers;
  auto it = cache.find(key);
  JITClassMethodFunc *wrap;

  if (it != cache.end()) {
    const std::string name = ir::LLVMVisitor::getNameForFunction(it->second);
    auto func = llvm::cantFail(engine->lookup(name));
    wrap = func.toPtr<JITClassMethodFunc>();
  } else {
    auto idx = cache.size();
    auto wrapname = "__codon_jitclass_method_" + std::to_string(idx);
    auto wrapper = buildJITClassMethodWrapper(instance->nativeClassName, methodName,
                                              wrapname, types);
    if (debug)
      fmt::print(stderr, "[codon::jit::jitClassCall] wrapper:\n{}-----\n", wrapper);
    if (auto err = compile(wrapper).takeError()) {
      auto errorInfo = llvm::toString(std::move(err));
      return JITResult::error(errorInfo);
    }

    auto *M = compiler->getModule();
    auto *cobj = jitClassData->getCObjType(M);
    auto *func = M->getOrRealizeFunc(wrapname, {cobj, cobj});
    seqassertn(func, "could not access jitclass method wrapper '{}'", wrapname);
    cache.emplace(key, func);

    auto result = address(func);
    if (auto err = result.takeError()) {
      auto errorInfo = llvm::toString(std::move(err));
      return JITResult::error(errorInfo);
    }
    wrap = (JITClassMethodFunc *)result.get();
  }

  try {
    auto *ans = (*wrap)(instance->nativePtr, args);
    return JITResult::success(ans);
  } catch (const runtime::JITError &e) {
    auto err = handleJITError(e);
    auto errorInfo = llvm::toString(std::move(err));
    return JITResult::error(errorInfo);
  }
}

JIT::JITResult JIT::jitClassRelease(JITClassInstance *instance) {
  // Deleting the control block also removes the RAII GC root.
  delete instance;
  return JITResult::success();
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

CJITResult jitclass_new(void *jit, char *class_name, char *native_class_name,
                        char **types, size_t types_size, void *args, uint8_t debug) {
  std::vector<std::string> cppTypes;
  cppTypes.reserve(types_size);
  for (size_t i = 0; i < types_size; i++)
    cppTypes.emplace_back(types[i]);
  auto t = ((codon::jit::JIT *)jit)
               ->jitClassNew(std::string(class_name), std::string(native_class_name),
                             cppTypes, args, bool(debug));
  void *result = t.result;
  char *message =
      t.message.empty() ? nullptr : strndup(t.message.c_str(), t.message.size());
  return {result, message};
}

CJITResult jitclass_call(void *jit, char *class_name, void *instance, char *method_name,
                         char **types, size_t types_size, void *args, uint8_t debug) {
  std::vector<std::string> cppTypes;
  cppTypes.reserve(types_size);
  for (size_t i = 0; i < types_size; i++)
    cppTypes.emplace_back(types[i]);
  auto t =
      ((codon::jit::JIT *)jit)
          ->jitClassCall(std::string(class_name),
                         static_cast<codon::jit::JIT::JITClassInstance *>(instance),
                         std::string(method_name), cppTypes, args, bool(debug));
  void *result = t.result;
  char *message =
      t.message.empty() ? nullptr : strndup(t.message.c_str(), t.message.size());
  return {result, message};
}

CJITResult jitclass_release(void *instance) {
  auto t = codon::jit::JIT::jitClassRelease(
      static_cast<codon::jit::JIT::JITClassInstance *>(instance));
  void *result = t.result;
  char *message =
      t.message.empty() ? nullptr : strndup(t.message.c_str(), t.message.size());
  return {result, message};
}

char *get_jit_library() {
  auto t = codon::ast::library_path();
  return strndup(t.c_str(), t.size());
}
