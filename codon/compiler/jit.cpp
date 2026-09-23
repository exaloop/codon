// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "jit.h"

#include <sstream>

#include "llvm/Support/JSON.h"
#include "llvm/TargetParser/Host.h"

#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/doc/doc.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace {
llvm::Expected<std::unordered_map<std::string, std::string>>
parseJITDefines(const std::vector<std::string> &definitions) {
  std::unordered_map<std::string, std::string> result;
  for (const auto &definition : definitions) {
    auto equals = definition.find('=');
    if (equals == std::string::npos || equals == 0)
      return llvm::createStringError("invalid JIT definition '%s'; expected name=value",
                                     definition.c_str());
    auto name = definition.substr(0, equals);
    if (!result.emplace(name, definition.substr(equals + 1)).second)
      return llvm::createStringError("duplicate JIT definition '%s'", name.c_str());
  }
  return result;
}
} // namespace

namespace codon {
namespace jit {
namespace {
typedef int MainFunc(int, char **);
typedef void InputFunc();
typedef void *PyWrapperFunc(void *);

const std::string JIT_FILENAME = "<jit>";
} // namespace

JIT::JIT(const Options &options, const std::string &mode, const std::string &stdlibRoot)
    : compiler(std::make_unique<Compiler>(options)),
      engine(std::make_unique<Engine>(compiler->getOptions())),
      pydata(std::make_unique<PythonData>()), mode(mode), forgetful(false) {
  if (!stdlibRoot.empty())
    compiler->getCache()->fs->add_search_path(stdlibRoot);
}

void collectExecutableStmts(ast::Stmt *s, ast::SuiteStmt *final) {
  if (cast<ast::FunctionStmt>(s) || cast<ast::ClassStmt>(s) ||
      cast<ast::CommentStmt>(s))
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
    auto fs =
        std::make_shared<ast::ResourceFilesystem>(compiler->getOptions()->argv0, "",
                                                  /*allowExternal=*/false);
    compiler->getCache()->fs = fs;
  }

  auto *cache = compiler->getCache();
  auto *module = compiler->getModule();
  auto *pm = compiler->getPassManager();
  auto *llvisitor = compiler->getLLVMVisitor();

  auto definitions = parseJITDefines(compiler->getOptions()->defines);
  if (!definitions)
    return definitions.takeError();
  compiler->getOptions()->jit = true;
  auto typechecked =
      ast::TypecheckVisitor::apply(cache, cache->N<ast::SuiteStmt>(), JIT_FILENAME,
                                   *definitions, compiler->getEarlyDefines());
  compiler->getOptions()->jit =
      false; // we still need main(), so pause jit first time during translation
  ast::TranslateVisitor::apply(cache, std::move(typechecked));
  compiler->getOptions()->jit = true;
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
} // namespace

JIT::PythonData::PythonData() : cobj(nullptr), cache() {}

ir::Type *JIT::PythonData::getCObjType(ir::Module *M) {
  if (cobj)
    return cobj;
  cobj = M->getPointerType();
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

namespace {
llvm::Expected<std::unique_ptr<codon::Options>> parseJITOptions(const char *name,
                                                                const char *settings) {
  auto options = codon::Options::getDefault(name);
  options->jit = true;
  options->debug = false;

  auto parsed = llvm::json::parse(settings);
  if (!parsed)
    return parsed.takeError();
  auto *object = parsed->getAsObject();
  if (!object)
    return llvm::createStringError("JIT options must be an object");

  using Options = codon::Options;
  const std::pair<const char *, bool Options::*> booleans[] = {
      {"debug", &Options::debug},       {"pmempty", &Options::pmempty},
      {"capture", &Options::capture},   {"native", &Options::native},
      {"pynum", &Options::pynum},       {"noexc", &Options::noexc},
      {"fastmath", &Options::fastmath}, {"autopy", &Options::autopy},
      {"autofree", &Options::autofree}, {"unordereddict", &Options::unordereddict},
  };
  for (const auto &[key, member] : booleans) {
    if (auto *value = object->get(key)) {
      auto setting = value->getAsBoolean();
      if (!setting)
        return llvm::createStringError("JIT option '%s' must be a bool", key);
      options.get()->*member = *setting;
      object->erase(key);
    }
  }
  const std::pair<const char *, std::string Options::*> strings[] = {
      {"libdevice", &Options::libdevice},
      {"gpuName", &Options::gpuName},
      {"gpuFeat", &Options::gpuFeat},
      {"gpuOutput", &Options::gpuOutput},
      {"log", &Options::log},
      {"march", &Options::march},
      {"mcpu", &Options::mcpu},
  };
  for (const auto &[key, member] : strings) {
    if (auto *value = object->get(key)) {
      auto setting = value->getAsString();
      if (!setting || setting->contains('\0'))
        return llvm::createStringError(
            "JIT option '%s' must be a string without NUL bytes", key);
      options.get()->*member = setting->str();
      object->erase(key);
    }
  }
  const std::pair<const char *, std::vector<std::string> Options::*> lists[] = {
      {"plugins", &Options::plugins},
      {"defines", &Options::defines},
      {"disabled", &Options::disabled},
      {"mattrs", &Options::mattrs},
  };
  for (const auto &[key, member] : lists) {
    if (auto *value = object->get(key)) {
      auto *settings = value->getAsArray();
      if (!settings)
        return llvm::createStringError("JIT option '%s' must be a list of strings",
                                       key);
      for (const auto &element : *settings) {
        auto setting = element.getAsString();
        if (!setting || setting->contains('\0'))
          return llvm::createStringError(
              "JIT option '%s' must contain strings without NUL bytes", key);
        (options.get()->*member).push_back(setting->str());
      }
      object->erase(key);
    }
  }
  if (!object->empty())
    return llvm::createStringError("unknown or unsupported JIT option '%s'",
                                   object->begin()->first.str().c_str());
  auto definitions = parseJITDefines(options->defines);
  if (!definitions)
    return definitions.takeError();
  if (options->march == "native") {
    options->march.clear();
    if (options->mcpu.empty()) {
      auto hostCPU = llvm::sys::getHostCPUName();
      if (!hostCPU.empty() && hostCPU != "generic")
        options->mcpu = hostCPU.str();
    }
    if (options->mattrs.empty())
      for (const auto &[feature, enabled] : llvm::sys::getHostCPUFeatures())
        options->mattrs.push_back((enabled ? "+" : "-") + feature.str());
  }
  return std::move(options);
}

CJITResult jitError(llvm::Error error) {
  auto message = llvm::toString(std::move(error));
  return {nullptr, strndup(message.c_str(), message.size())};
}
} // namespace

CJITResult jit_validate_options(const char *settings) {
  auto options = parseJITOptions("codon jit", settings);
  if (!options)
    return jitError(options.takeError());
  return {nullptr, nullptr};
}

CJITResult jit_init_with_options(const char *name, const char *settings) {
  auto options = parseJITOptions(name, settings);
  if (!options)
    return jitError(options.takeError());
  try {
    codon::getLogger().parse((*options)->log);
    if (auto *debug = getenv("CODON_DEBUG"))
      codon::getLogger().parse(debug);
    auto jit = std::make_unique<codon::jit::JIT>(**options);
    for (const auto &plugin : (*options)->plugins)
      if (auto error = jit->getCompiler()->load(plugin))
        return jitError(std::move(error));
    if (auto error = jit->init())
      return jitError(std::move(error));
    return {jit.release(), nullptr};
  } catch (const codon::exc::ParserException &error) {
    return jitError(llvm::make_error<codon::error::ParserErrorInfo>(error.getErrors()));
  }
}

void *jit_init(char *name) {
  auto result = jit_init_with_options(name, "{}");
  if (result.error) {
    auto error = llvm::createStringError("%s", result.error);
    free(result.error);
    llvm::cantFail(std::move(error));
  }
  return result.result;
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
