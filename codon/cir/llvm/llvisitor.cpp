// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "llvisitor.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fmt/args.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

#include "codon/cir/dsl/codegen.h"
#include "codon/cir/llvm/optimize.h"
#include "codon/cir/util/irtools.h"
#include "codon/compiler/debug_listener.h"
#include "codon/compiler/memory_manager.h"
#include "codon/parser/common.h"
#include "codon/runtime/lib.h"
#include "codon/util/common.h"

namespace codon {
namespace ir {
namespace {
const std::string EXPORT_ATTR = "std.internal.attributes.export";
const std::string INLINE_ATTR = "std.internal.attributes.inline";
const std::string NOINLINE_ATTR = "std.internal.attributes.noinline";
const std::string GPU_KERNEL_ATTR = "std.gpu.kernel";

const std::string MAIN_UNCLASH = ".main.unclash";
const std::string MAIN_CTOR = ".main.ctor";

llvm::cl::opt<bool> DisableExceptions("disable-exceptions",
                                      llvm::cl::desc("Disable exception handling"),
                                      llvm::cl::init(false));
} // namespace

llvm::DIFile *LLVMVisitor::DebugInfo::getFile(const std::string &path) {
  std::string filename;
  std::string directory;
  auto pos = path.find_last_of("/");
  if (pos != std::string::npos) {
    filename = path.substr(pos + 1);
    directory = path.substr(0, pos);
  } else {
    filename = path;
    directory = ".";
  }
  return builder->createFile(filename, directory);
}

std::string LLVMVisitor::getNameForFunction(const Func *x) {
  if (isA<ExternalFunc>(x) || util::hasAttribute(x, EXPORT_ATTR)) {
    return x->getUnmangledName();
  } else if (util::hasAttribute(x, GPU_KERNEL_ATTR)) {
    return x->getName();
  } else {
    return x->referenceString();
  }
}

std::string LLVMVisitor::getNameForVar(const Var *x) {
  if (auto *f = cast<Func>(x))
    return getNameForFunction(f);

  auto name = x->getName();
  if (x->isExternal()) {
    return name;
  } else {
    // ".Lxxx" is a linker-local name, so add an underscore if needed
    return ((!name.empty() && name[0] == 'L') ? "._" : ".") + name;
  }
}

LLVMVisitor::LLVMVisitor()
    : util::ConstVisitor(), context(std::make_unique<llvm::LLVMContext>()), M(),
      B(std::make_unique<llvm::IRBuilder<>>(*context)), func(nullptr), block(nullptr),
      value(nullptr), vars(), funcs(), coro(), loops(), trycatch(), catches(), db(),
      plugins(nullptr) {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  // Initialize passes
  auto &registry = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeCore(registry);
  llvm::initializeScalarOpts(registry);
  llvm::initializeVectorization(registry);
  llvm::initializeIPO(registry);
  llvm::initializeAnalysis(registry);
  llvm::initializeTransformUtils(registry);
  llvm::initializeInstCombine(registry);
  llvm::initializeTarget(registry);

  llvm::initializeExpandLargeDivRemLegacyPassPass(registry);
  llvm::initializeExpandLargeFpConvertLegacyPassPass(registry);
  llvm::initializeExpandMemCmpPassPass(registry);
  llvm::initializeScalarizeMaskedMemIntrinLegacyPassPass(registry);
  llvm::initializeSelectOptimizePass(registry);
  llvm::initializeCallBrPreparePass(registry);
  llvm::initializeCodeGenPreparePass(registry);
  llvm::initializeAtomicExpandPass(registry);
  llvm::initializeWinEHPreparePass(registry);
  llvm::initializeDwarfEHPrepareLegacyPassPass(registry);
  llvm::initializeSafeStackLegacyPassPass(registry);
  llvm::initializeSjLjEHPreparePass(registry);
  llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registry);
  llvm::initializeGlobalMergePass(registry);
  llvm::initializeIndirectBrExpandPassPass(registry);
  llvm::initializeInterleavedLoadCombinePass(registry);
  llvm::initializeInterleavedAccessPass(registry);
  llvm::initializeUnreachableBlockElimLegacyPassPass(registry);
  llvm::initializeExpandReductionsPass(registry);
  llvm::initializeExpandVectorPredicationPass(registry);
  llvm::initializeWasmEHPreparePass(registry);
  llvm::initializeWriteBitcodePassPass(registry);
  llvm::initializeReplaceWithVeclibLegacyPass(registry);
  llvm::initializeJMCInstrumenterPass(registry);
}

void LLVMVisitor::registerGlobal(const Var *var) {
  if (!var->isGlobal())
    return;

  if (auto *f = cast<Func>(var)) {
    insertFunc(f, makeLLVMFunction(f));
  } else {
    llvm::Type *llvmType = getLLVMType(var->getType());
    if (llvmType->isVoidTy()) {
      insertVar(var, getDummyVoidValue());
    } else {
      bool external = var->isExternal();
      auto linkage = (db.jit || external) ? llvm::GlobalValue::ExternalLinkage
                                          : llvm::GlobalValue::PrivateLinkage;
      auto *storage = new llvm::GlobalVariable(
          *M, llvmType, /*isConstant=*/false, linkage,
          external ? nullptr : llvm::Constant::getNullValue(llvmType),
          getNameForVar(var));
      insertVar(var, storage);

      if (external) {
        if (db.jit) {
          storage->setDSOLocal(true);
        } else {
          storage->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Local);
        }
      } else {
        // debug info
        auto *srcInfo = getSrcInfo(var);
        llvm::DIFile *file = db.getFile(srcInfo->file);
        llvm::DIScope *scope = db.unit;
        llvm::DIGlobalVariableExpression *debugVar =
            db.builder->createGlobalVariableExpression(
                scope, getDebugNameForVariable(var), var->getName(), file,
                srcInfo->line, getDIType(var->getType()), !var->isExternal());
        storage->addDebugInfo(debugVar);
      }
    }
  }
}

llvm::Value *LLVMVisitor::getVar(const Var *var) {
  auto it = vars.find(var->getId());
  if (db.jit && var->isGlobal()) {
    if (it != vars.end()) {
      if (!it->second) { // if value is null, it's from another module
        // see if it's in the module already
        auto name = var->getName();
        auto privName = getNameForVar(var);
        if (auto *global = M->getNamedValue(privName))
          return global;

        llvm::Type *llvmType = getLLVMType(var->getType());
        auto *storage = new llvm::GlobalVariable(*M, llvmType, /*isConstant=*/false,
                                                 llvm::GlobalValue::ExternalLinkage,
                                                 /*Initializer=*/nullptr, privName);
        storage->setExternallyInitialized(true);

        // debug info
        auto *srcInfo = getSrcInfo(var);
        llvm::DIFile *file = db.getFile(srcInfo->file);
        llvm::DIScope *scope = db.unit;
        llvm::DIGlobalVariableExpression *debugVar =
            db.builder->createGlobalVariableExpression(
                scope, getDebugNameForVariable(var), name, file, srcInfo->line,
                getDIType(var->getType()),
                /*IsLocalToUnit=*/true);
        storage->addDebugInfo(debugVar);
        insertVar(var, storage);
        return storage;
      }
    } else {
      registerGlobal(var);
      it = vars.find(var->getId());
      return it->second;
    }
  }
  return (it != vars.end()) ? it->second : nullptr;
}

llvm::Function *LLVMVisitor::getFunc(const Func *func) {
  auto it = funcs.find(func->getId());
  if (db.jit) {
    if (it != funcs.end()) {
      if (!it->second) { // if value is null, it's from another module
        // see if it's in the module already
        const std::string name = getNameForFunction(func);
        if (auto *g = M->getFunction(name))
          return g;

        auto *funcType = cast<types::FuncType>(func->getType());
        llvm::Type *returnType = getLLVMType(funcType->getReturnType());
        std::vector<llvm::Type *> argTypes;
        for (const auto &argType : *funcType) {
          argTypes.push_back(getLLVMType(argType));
        }

        auto *llvmFuncType =
            llvm::FunctionType::get(returnType, argTypes, funcType->isVariadic());
        auto *g = llvm::Function::Create(llvmFuncType, llvm::Function::ExternalLinkage,
                                         name, M.get());
        insertFunc(func, g);
        return g;
      }
    } else {
      registerGlobal(func);
      it = funcs.find(func->getId());
      return it->second;
    }
  }
  return (it != funcs.end()) ? it->second : nullptr;
}

std::unique_ptr<llvm::Module> LLVMVisitor::makeModule(llvm::LLVMContext &context,
                                                      const SrcInfo *src) {
  auto builder = llvm::EngineBuilder();
  builder.setMArch(llvm::codegen::getMArch());
  builder.setMCPU(llvm::codegen::getCPUStr());
  builder.setMAttrs(llvm::codegen::getFeatureList());

  auto target = builder.selectTarget();
  auto M = std::make_unique<llvm::Module>("codon", context);
  M->setTargetTriple(target->getTargetTriple().str());
  M->setDataLayout(target->createDataLayout());
  B = std::make_unique<llvm::IRBuilder<>>(context);

  auto *srcInfo = src ? src : getDefaultSrcInfo();
  M->setSourceFileName(srcInfo->file);
  // debug info setup
  db.builder = std::make_unique<llvm::DIBuilder>(*M);
  llvm::DIFile *file = db.getFile(srcInfo->file);
  db.unit = db.builder->createCompileUnit(llvm::dwarf::DW_LANG_C, file,
                                          ("codon version " CODON_VERSION), !db.debug,
                                          db.flags,
                                          /*RV=*/0);
  M->addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                   llvm::DEBUG_METADATA_VERSION);
  // darwin only supports dwarf2
  if (llvm::Triple(M->getTargetTriple()).isOSDarwin()) {
    M->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);
  }

  return M;
}

void LLVMVisitor::clearLLVMData() {
  B = {};
  func = nullptr;
  block = nullptr;
  value = nullptr;

  for (auto it = funcs.begin(); it != funcs.end();) {
    if (it->second && it->second->hasPrivateLinkage()) {
      it = funcs.erase(it);
    } else {
      it->second = nullptr;
      ++it;
    }
  }

  for (auto it = vars.begin(); it != vars.end();) {
    if (it->second && !llvm::isa<llvm::GlobalValue>(it->second)) {
      it = vars.erase(it);
    } else {
      it->second = nullptr;
      ++it;
    }
  }

  coro.reset();
  loops.clear();
  trycatch.clear();
  catches.clear();
  db.reset();
  context = {};
  M = {};
}

std::pair<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::LLVMContext>>
LLVMVisitor::takeModule(Module *module, const SrcInfo *src) {
  // process any new functions or globals
  if (module) {
    std::unordered_set<id_t> funcsToProcess;
    for (auto *var : *module) {
      auto id = var->getId();
      if (auto *func = cast<Func>(var)) {
        if (funcs.find(id) != funcs.end())
          continue;
        else
          funcsToProcess.insert(id);
      } else {
        if (vars.find(id) != vars.end())
          continue;
      }

      registerGlobal(var);
    }

    for (auto *var : *module) {
      if (auto *func = cast<Func>(var)) {
        if (funcsToProcess.find(func->getId()) != funcsToProcess.end()) {
          process(func);
        }
      }
    }
  }

  db.builder->finalize();
  auto currentContext = std::move(context);
  auto currentModule = std::move(M);

  // reset all LLVM fields/data -- they are owned by the context
  clearLLVMData();
  context = std::make_unique<llvm::LLVMContext>();
  M = makeModule(*context, src);
  return {std::move(currentModule), std::move(currentContext)};
}

void LLVMVisitor::setDebugInfoForNode(const Node *x) {
  if (x && func) {
    auto *srcInfo = getSrcInfo(x);
    B->SetCurrentDebugLocation(llvm::DILocation::get(
        *context, srcInfo->line, srcInfo->col, func->getSubprogram()));
  } else {
    B->SetCurrentDebugLocation(llvm::DebugLoc());
  }
}

void LLVMVisitor::process(const Node *x) {
  setDebugInfoForNode(x);
  x->accept(*this);
}

void LLVMVisitor::dump(const std::string &filename) { writeToLLFile(filename, false); }

void LLVMVisitor::runLLVMPipeline() {
  db.builder->finalize();
  optimize(M.get(), db.debug, db.jit, plugins);
}

void LLVMVisitor::writeToObjectFile(const std::string &filename, bool pic) {
  runLLVMPipeline();

  std::error_code err;
  auto out =
      std::make_unique<llvm::ToolOutputFile>(filename, err, llvm::sys::fs::OF_None);
  if (err)
    compilationError(err.message());
  llvm::raw_pwrite_stream *os = &out->os();

  auto machine = getTargetMachine(M.get(), /*setFunctionAttributes=*/false, pic);
  auto &llvmtm = static_cast<llvm::LLVMTargetMachine &>(*machine);
  auto *mmiwp = new llvm::MachineModuleInfoWrapperPass(&llvmtm);
  llvm::legacy::PassManager pm;

  llvm::TargetLibraryInfoImpl tlii(llvm::Triple(M->getTargetTriple()));
  pm.add(new llvm::TargetLibraryInfoWrapperPass(tlii));
  seqassertn(!machine->addPassesToEmitFile(pm, *os, nullptr, llvm::CGFT_ObjectFile,
                                           /*DisableVerify=*/true, mmiwp),
             "could not add passes");
  const_cast<llvm::TargetLoweringObjectFile *>(llvmtm.getObjFileLowering())
      ->Initialize(mmiwp->getMMI().getContext(), *machine);
  pm.run(*M);
  out->keep();
}

void LLVMVisitor::writeToBitcodeFile(const std::string &filename) {
  runLLVMPipeline();
  std::error_code err;
  llvm::raw_fd_ostream stream(filename, err, llvm::sys::fs::OF_None);
  llvm::WriteBitcodeToFile(*M, stream);
  if (err) {
    compilationError(err.message());
  }
}

void LLVMVisitor::writeToLLFile(const std::string &filename, bool optimize) {
  if (optimize)
    runLLVMPipeline();
  auto fo = fopen(filename.c_str(), "w");
  llvm::raw_fd_ostream fout(fileno(fo), true);
  fout << *M;
  fout.close();
}

namespace {
void executeCommand(const std::vector<std::string> &args) {
  std::vector<const char *> cArgs;
  for (auto &arg : args) {
    cArgs.push_back(arg.c_str());
  }
  LOG_USER("Executing '{}'", fmt::join(cArgs, " "));
  cArgs.push_back(nullptr);

  if (fork() == 0) {
    int status = execvp(cArgs[0], (char *const *)&cArgs[0]);
    exit(status);
  } else {
    int status;
    if (wait(&status) < 0) {
      compilationError("process for '" + args[0] + "' encountered an error in wait");
    }

    if (WEXITSTATUS(status) != 0) {
      compilationError("process for '" + args[0] + "' exited with status " +
                       std::to_string(WEXITSTATUS(status)));
    }
  }
}
} // namespace

void LLVMVisitor::setupGlobalCtorForSharedLibrary() {
  const std::string llvmCtor = "llvm.global_ctors";
  if (M->getNamedValue(llvmCtor))
    return;

  auto *main = M->getFunction(MAIN_UNCLASH);
  if (!main) {
    main = M->getFunction("main");
    if (!main)
      return;
    main->setName(MAIN_UNCLASH); // avoid clash with other main
  }

  auto *ctorFuncTy = llvm::FunctionType::get(B->getVoidTy(), {}, /*isVarArg=*/false);
  auto *ctorEntryTy = llvm::StructType::get(B->getInt32Ty(), ctorFuncTy->getPointerTo(),
                                            B->getInt8PtrTy());
  auto *ctorArrayTy = llvm::ArrayType::get(ctorEntryTy, 1);

  auto *ctor =
      cast<llvm::Function>(M->getOrInsertFunction(MAIN_CTOR, ctorFuncTy).getCallee());
  ctor->setLinkage(llvm::GlobalValue::InternalLinkage);
  auto *entry = llvm::BasicBlock::Create(*context, "entry", ctor);
  B->SetInsertPoint(entry);
  B->CreateCall({main->getFunctionType(), main},
                {B->getInt32(0),
                 llvm::ConstantPointerNull::get(B->getInt8PtrTy()->getPointerTo())});
  B->CreateRetVoid();

  const int priority = 65535; // default
  auto *ctorEntry = llvm::ConstantStruct::get(
      ctorEntryTy,
      {B->getInt32(priority), ctor, llvm::ConstantPointerNull::get(B->getInt8PtrTy())});
  new llvm::GlobalVariable(*M, ctorArrayTy,
                           /*isConstant=*/true, llvm::GlobalValue::AppendingLinkage,
                           llvm::ConstantArray::get(ctorArrayTy, {ctorEntry}),
                           llvmCtor);
}

void LLVMVisitor::writeToExecutable(const std::string &filename,
                                    const std::string &argv0, bool library,
                                    const std::vector<std::string> &libs,
                                    const std::string &lflags) {
  if (library)
    setupGlobalCtorForSharedLibrary();

  const std::string objFile = filename + ".o";
  writeToObjectFile(objFile, /*pic=*/library);

  const std::string base = ast::executable_path(argv0.c_str());
  auto path = llvm::SmallString<128>(llvm::sys::path::parent_path(base));

  std::vector<std::string> relatives = {"../lib", "../lib/codon"};
  std::vector<std::string> rpaths;
  for (const auto &rel : relatives) {
    auto newPath = path;
    llvm::sys::path::append(newPath, rel);
    llvm::sys::path::remove_dots(newPath, /*remove_dot_dot=*/true);
    if (llvm::sys::fs::exists(newPath)) {
      rpaths.push_back(std::string(newPath));
    }
  }

  if (rpaths.empty()) {
    rpaths.push_back(std::string(path));
  }

  std::vector<std::string> command = {"g++"};
  // Avoid "argument unused during compilation" warning
  command.push_back("-Wno-unused-command-line-argument");
  // MUST go before -llib to compile on Linux
  command.push_back(objFile);

  if (library)
    command.push_back("-shared");

  for (const auto &rpath : rpaths) {
    if (!rpath.empty()) {
      command.push_back("-L" + rpath);
      command.push_back("-Wl,-rpath," + rpath);
    }
  }

  if (plugins) {
    for (auto *plugin : *plugins) {
      auto dylibPath = plugin->info.dylibPath;
      if (dylibPath.empty())
        continue;

      llvm::SmallString<128> rpath0 = llvm::sys::path::parent_path(dylibPath);
      llvm::sys::fs::make_absolute(rpath0);
      llvm::StringRef rpath = rpath0.str();
      if (!rpath.empty()) {
        command.push_back("-L" + rpath.str());
        command.push_back("-Wl,-rpath," + rpath.str());
      }
    }
  }

  for (const auto &lib : libs) {
    command.push_back("-l" + lib);
  }

  if (plugins) {
    for (auto *plugin : *plugins) {
      if (plugin->info.linkArgs.empty()) {
        auto dylibPath = plugin->info.dylibPath;
        if (dylibPath.empty())
          continue;

        auto stem = llvm::sys::path::stem(dylibPath);
        if (stem.startswith("lib"))
          stem = stem.substr(3);

        command.push_back("-l" + stem.str());
      } else {
        for (auto &l : plugin->info.linkArgs)
          command.push_back(l);
      }
    }
  }

  std::vector<std::string> extraArgs = {
      "-lcodonrt", "-lomp", "-lpthread", "-ldl", "-lz", "-lm", "-lc", "-o", filename};

  for (const auto &arg : extraArgs) {
    command.push_back(arg);
  }

  llvm::SmallVector<llvm::StringRef> userFlags(16);
  llvm::StringRef(lflags).split(userFlags, " ", /*MaxSplit=*/-1, /*KeepEmpty=*/false);

  for (const auto &uflag : userFlags) {
    if (!uflag.empty())
      command.push_back(uflag.str());
  }

  // Avoid "relocation R_X86_64_32 against `.bss' can not be used when making a PIE
  // object" complaints by gcc when it is built with --enable-default-pie
  if (!library)
    command.push_back("-no-pie");

  executeCommand(command);

#if __APPLE__
  if (db.debug)
    executeCommand({"dsymutil", filename});
#endif

  llvm::sys::fs::remove(objFile);
}

namespace {
// https://github.com/python/cpython/blob/main/Include/methodobject.h
constexpr int PYEXT_METH_VARARGS = 0x0001;
constexpr int PYEXT_METH_KEYWORDS = 0x0002;
constexpr int PYEXT_METH_NOARGS = 0x0004;
constexpr int PYEXT_METH_O = 0x0008;
constexpr int PYEXT_METH_CLASS = 0x0010;
constexpr int PYEXT_METH_STATIC = 0x0020;
constexpr int PYEXT_METH_COEXIST = 0x0040;
constexpr int PYEXT_METH_FASTCALL = 0x0080;
constexpr int PYEXT_METH_METHOD = 0x0200;
// https://github.com/python/cpython/blob/main/Include/modsupport.h
constexpr int PYEXT_PYTHON_ABI_VERSION = 1013;
// https://github.com/python/cpython/blob/main/Include/descrobject.h
constexpr int PYEXT_READONLY = 1;
} // namespace

llvm::Function *LLVMVisitor::createPyTryCatchWrapper(llvm::Function *func) {
  auto *wrap =
      cast<llvm::Function>(M->getOrInsertFunction((func->getName() + ".tc_wrap").str(),
                                                  func->getFunctionType())
                               .getCallee());
  wrap->setPersonalityFn(llvm::cast<llvm::Constant>(makePersonalityFunc().getCallee()));
  auto *entry = llvm::BasicBlock::Create(*context, "entry", wrap);
  auto *normal = llvm::BasicBlock::Create(*context, "normal", wrap);
  auto *unwind = llvm::BasicBlock::Create(*context, "unwind", wrap);

  B->SetInsertPoint(entry);
  std::vector<llvm::Value *> args;
  for (auto &arg : wrap->args()) {
    args.push_back(&arg);
  }
  auto *result = B->CreateInvoke(func, normal, unwind, args);

  B->SetInsertPoint(normal);
  B->CreateRet(result);

  B->SetInsertPoint(unwind);
  auto *caughtResult = B->CreateLandingPad(getPadType(), 1);
  caughtResult->setCleanup(true);
  caughtResult->addClause(getTypeIdxVar(nullptr));
  auto *unwindType = llvm::StructType::get(B->getInt64Ty()); // header only
  auto *unwindException = B->CreateExtractValue(caughtResult, 0);
  auto *unwindExceptionClass = B->CreateLoad(
      B->getInt64Ty(),
      B->CreateStructGEP(
          unwindType, B->CreatePointerCast(unwindException, unwindType->getPointerTo()),
          0));
  unwindException = B->CreateExtractValue(caughtResult, 0);
  auto *excType = llvm::StructType::get(getTypeInfoType(), B->getInt8PtrTy());
  auto *excVal =
      B->CreatePointerCast(B->CreateConstGEP1_64(B->getInt8Ty(), unwindException,
                                                 (uint64_t)seq_exc_offset()),
                           excType->getPointerTo());
  auto *loadedExc = B->CreateLoad(excType, excVal);
  auto *objPtr = B->CreateExtractValue(loadedExc, 1);

  auto *strType = llvm::StructType::get(B->getInt64Ty(), B->getInt8PtrTy());
  auto *excHeader =
      llvm::StructType::get(strType, strType, strType, strType, B->getInt64Ty(),
                            B->getInt64Ty(), B->getInt8PtrTy());
  auto *header = B->CreateLoad(excHeader, objPtr);
  auto *msg = B->CreateExtractValue(header, 1);
  auto *msgLen = B->CreateExtractValue(msg, 0);
  auto *msgPtr = B->CreateExtractValue(msg, 1);
  auto *pyType = B->CreateExtractValue(header, 6);

  // copy msg into new null-terminated buffer
  auto alloc = makeAllocFunc(/*atomic=*/true);
  auto *buf = B->CreateCall(alloc, B->CreateAdd(msgLen, B->getInt64(1)));
  B->CreateMemCpy(buf, {}, msgPtr, {}, msgLen);
  auto *last = B->CreateInBoundsGEP(B->getInt8Ty(), buf, msgLen);
  B->CreateStore(B->getInt8(0), last);

  auto *pyErrSetString = llvm::cast<llvm::Function>(
      M->getOrInsertFunction("PyErr_SetString", B->getVoidTy(), B->getInt8PtrTy(),
                             B->getInt8PtrTy())
          .getCallee());

  const std::string pyExcRuntimeErrorName = "PyExc_RuntimeError";
  llvm::Value *pyExcRuntimeError = M->getNamedValue(pyExcRuntimeErrorName);
  if (!pyExcRuntimeError) {
    auto *pyExcRuntimeErrorVar = new llvm::GlobalVariable(
        *M, B->getInt8PtrTy(), /*isConstant=*/false, llvm::GlobalValue::ExternalLinkage,
        /*Initializer=*/nullptr, pyExcRuntimeErrorName);
    pyExcRuntimeErrorVar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
    pyExcRuntimeError = pyExcRuntimeErrorVar;
  }
  pyExcRuntimeError = B->CreateLoad(B->getInt8PtrTy(), pyExcRuntimeError);

  auto *havePyType =
      B->CreateICmpNE(pyType, llvm::ConstantPointerNull::get(B->getInt8PtrTy()));
  B->CreateCall(pyErrSetString,
                {B->CreateSelect(havePyType, pyType, pyExcRuntimeError), buf});

  auto *retType = wrap->getReturnType();
  if (retType == B->getInt32Ty()) {
    B->CreateRet(B->getInt32(-1));
  } else {
    B->CreateRet(llvm::Constant::getNullValue(retType));
  }

  return wrap;
}

void LLVMVisitor::writeToPythonExtension(const PyModule &pymod,
                                         const std::string &filename) {
  // Setup LLVM types & constants
  auto *i64 = B->getInt64Ty();
  auto *i32 = B->getInt32Ty();
  auto *i8 = B->getInt8Ty();
  auto *ptr = B->getInt8PtrTy();
  auto *pyMethodDefType = llvm::StructType::create("PyMethodDef", ptr, ptr, i32, ptr);
  auto *pyObjectType = llvm::StructType::create("PyObject", i64, ptr);
  auto *pyVarObjectType = llvm::StructType::create("PyVarObject", pyObjectType, i64);
  auto *pyModuleDefBaseType =
      llvm::StructType::create("PyMethodDefBase", pyObjectType, ptr, i64, ptr);
  auto *pyModuleDefType =
      llvm::StructType::create("PyModuleDef", pyModuleDefBaseType, ptr, ptr, i64,
                               pyMethodDefType->getPointerTo(), ptr, ptr, ptr, ptr);
  auto *pyMemberDefType =
      llvm::StructType::create("PyMemberDef", ptr, i32, i64, i32, ptr);
  auto *pyGetSetDefType =
      llvm::StructType::create("PyGetSetDef", ptr, ptr, ptr, ptr, ptr);
  std::vector<llvm::Type *> pyNumberMethodsFields(36, ptr);
  auto *pyNumberMethodsType =
      llvm::StructType::create(*context, pyNumberMethodsFields, "PyNumberMethods");
  std::vector<llvm::Type *> pySequenceMethodsFields(10, ptr);
  auto *pySequenceMethodsType =
      llvm::StructType::create(*context, pySequenceMethodsFields, "PySequenceMethods");
  std::vector<llvm::Type *> pyMappingMethodsFields(3, ptr);
  auto *pyMappingMethodsType =
      llvm::StructType::create(*context, pyMappingMethodsFields, "PyMappingMethods");
  std::vector<llvm::Type *> pyAsyncMethodsFields(4, ptr);
  auto *pyAsyncMethodsType =
      llvm::StructType::create(*context, pyAsyncMethodsFields, "PyAsyncMethods");
  auto *pyBufferProcsType = llvm::StructType::create("PyBufferProcs", ptr, ptr);
  auto *pyTypeObjectType = llvm::StructType::create(
      "PyTypeObject", pyVarObjectType, ptr, i64, i64, ptr, i64, ptr, ptr, ptr, ptr, ptr,
      ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, i64, ptr, ptr, ptr, ptr, i64, ptr, ptr,
      ptr, ptr, ptr, ptr, ptr, ptr, ptr, i64, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr,
      ptr, ptr, ptr, i32, ptr, ptr, i8);
  auto *zero64 = B->getInt64(0);
  auto *zero32 = B->getInt32(0);
  auto *zero8 = B->getInt8(0);
  auto *null = llvm::Constant::getNullValue(ptr);
  auto *pyTypeType = new llvm::GlobalVariable(*M, ptr, /*isConstant=*/false,
                                              llvm::GlobalValue::ExternalLinkage,
                                              /*Initializer=*/nullptr, "PyType_Type");

  auto allocUncollectable = llvm::cast<llvm::Function>(
      makeAllocFunc(/*atomic=*/false, /*uncollectable=*/true).getCallee());
  auto free = llvm::cast<llvm::Function>(makeFreeFunc().getCallee());

  // Helpers
  auto pyFuncWrap = [&](Func *func, bool wrap) -> llvm::Constant * {
    if (!func)
      return null;
    auto llvmName = getNameForFunction(func);
    auto *llvmFunc = M->getFunction(llvmName);
    seqassertn(llvmFunc, "function {} not found in LLVM module", llvmName);
    if (wrap)
      llvmFunc = createPyTryCatchWrapper(llvmFunc);
    return llvmFunc;
  };

  auto pyFunc = [&](Func *func) -> llvm::Constant * { return pyFuncWrap(func, true); };

  auto pyString = [&](const std::string &str) -> llvm::Constant * {
    if (str.empty())
      return null;
    auto *var = new llvm::GlobalVariable(
        *M, llvm::ArrayType::get(i8, str.length() + 1),
        /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantDataArray::getString(*context, str), ".pyext_str");
    var->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
    return var;
  };

  auto pyFunctions = [&](const std::vector<PyFunction> &functions) -> llvm::Constant * {
    if (functions.empty())
      return null;

    std::vector<llvm::Constant *> pyMethods;
    for (auto &pyfunc : functions) {
      int flag = 0;
      if (pyfunc.keywords) {
        flag = PYEXT_METH_FASTCALL | PYEXT_METH_KEYWORDS;
      } else {
        switch (pyfunc.nargs) {
        case 0:
          flag = PYEXT_METH_NOARGS;
          break;
        case 1:
          flag = PYEXT_METH_O;
          break;
        default:
          flag = PYEXT_METH_FASTCALL;
          break;
        }
      }

      switch (pyfunc.type) {
      case PyFunction::CLASS:
        flag |= PYEXT_METH_CLASS;
        break;
      case PyFunction::STATIC:
        flag |= PYEXT_METH_STATIC;
        break;
      default:
        break;
      }

      if (pyfunc.coexist)
        flag |= PYEXT_METH_COEXIST;

      pyMethods.push_back(llvm::ConstantStruct::get(
          pyMethodDefType, pyString(pyfunc.name), pyFunc(pyfunc.func),
          B->getInt32(flag), pyString(pyfunc.doc)));
    }
    pyMethods.push_back(
        llvm::ConstantStruct::get(pyMethodDefType, null, null, zero32, null));

    auto *pyMethodDefArrayType =
        llvm::ArrayType::get(pyMethodDefType, pyMethods.size());
    auto *pyMethodDefArray = new llvm::GlobalVariable(
        *M, pyMethodDefArrayType,
        /*isConstant=*/false, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantArray::get(pyMethodDefArrayType, pyMethods), ".pyext_methods");
    return pyMethodDefArray;
  };

  auto pyMembers = [&](const std::vector<PyMember> &members,
                       llvm::StructType *type) -> llvm::Constant * {
    if (members.empty())
      return null;

    std::vector<llvm::Constant *> pyMemb;
    for (auto &memb : members) {
      // Calculate offset by creating const GEP into null ptr
      std::vector<llvm::Constant *> indexes = {zero64, B->getInt32(1)};
      for (auto idx : memb.indexes) {
        indexes.push_back(B->getInt32(idx));
      }
      auto offset = llvm::ConstantExpr::getPtrToInt(
          llvm::ConstantExpr::getGetElementPtr(type, null, indexes), i64);

      pyMemb.push_back(llvm::ConstantStruct::get(
          pyMemberDefType, pyString(memb.name), B->getInt32(memb.type), offset,
          B->getInt32(memb.readonly ? PYEXT_READONLY : 0), pyString(memb.doc)));
    }
    pyMemb.push_back(
        llvm::ConstantStruct::get(pyMemberDefType, null, zero32, zero64, zero32, null));

    auto *pyMemberDefArrayType = llvm::ArrayType::get(pyMemberDefType, pyMemb.size());
    auto *pyMemberDefArray = new llvm::GlobalVariable(
        *M, pyMemberDefArrayType,
        /*isConstant=*/false, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantArray::get(pyMemberDefArrayType, pyMemb), ".pyext_members");
    return pyMemberDefArray;
  };

  auto pyGetSet = [&](const std::vector<PyGetSet> &getset) -> llvm::Constant * {
    if (getset.empty())
      return null;

    std::vector<llvm::Constant *> pyGS;
    for (auto &gs : getset) {
      pyGS.push_back(llvm::ConstantStruct::get(pyGetSetDefType, pyString(gs.name),
                                               pyFunc(gs.get), pyFunc(gs.set),
                                               pyString(gs.doc), null));
    }
    pyGS.push_back(
        llvm::ConstantStruct::get(pyGetSetDefType, null, null, null, null, null));

    auto *pyGetSetDefArrayType = llvm::ArrayType::get(pyGetSetDefType, pyGS.size());
    auto *pyGetSetDefArray = new llvm::GlobalVariable(
        *M, pyGetSetDefArrayType,
        /*isConstant=*/false, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantArray::get(pyGetSetDefArrayType, pyGS), ".pyext_getset");
    return pyGetSetDefArray;
  };

  // Construct PyModuleDef array
  auto *pyObjectConst = llvm::ConstantStruct::get(pyObjectType, B->getInt64(1), null);
  auto *pyModuleDefBaseConst =
      llvm::ConstantStruct::get(pyModuleDefBaseType, pyObjectConst, null, zero64, null);

  auto *pyModuleDef = llvm::ConstantStruct::get(
      pyModuleDefType, pyModuleDefBaseConst, pyString(pymod.name), pyString(pymod.doc),
      B->getInt64(-1), pyFunctions(pymod.functions), null, null, null, null);
  auto *pyModuleVar =
      new llvm::GlobalVariable(*M, pyModuleDef->getType(),
                               /*isConstant=*/false, llvm::GlobalValue::PrivateLinkage,
                               pyModuleDef, ".pyext_module");

  std::unordered_map<types::Type *, llvm::GlobalVariable *> typeVars;
  for (auto &pytype : pymod.types) {
    std::vector<llvm::Constant *> numberSlots = {
        pyFunc(pytype.add),       // nb_add
        pyFunc(pytype.sub),       // nb_subtract
        pyFunc(pytype.mul),       // nb_multiply
        pyFunc(pytype.mod),       // nb_remainder
        pyFunc(pytype.divmod),    // nb_divmod
        pyFunc(pytype.pow),       // nb_power
        pyFunc(pytype.neg),       // nb_negative
        pyFunc(pytype.pos),       // nb_positive
        pyFunc(pytype.abs),       // nb_absolute
        pyFunc(pytype.bool_),     // nb_bool
        pyFunc(pytype.invert),    // nb_invert
        pyFunc(pytype.lshift),    // nb_lshift
        pyFunc(pytype.rshift),    // nb_rshift
        pyFunc(pytype.and_),      // nb_and
        pyFunc(pytype.xor_),      // nb_xor
        pyFunc(pytype.or_),       // nb_or
        pyFunc(pytype.int_),      // nb_int
        null,                     // nb_reserved
        pyFunc(pytype.float_),    // nb_float
        pyFunc(pytype.iadd),      // nb_inplace_add
        pyFunc(pytype.isub),      // nb_inplace_subtract
        pyFunc(pytype.imul),      // nb_inplace_multiply
        pyFunc(pytype.imod),      // nb_inplace_remainder
        pyFunc(pytype.ipow),      // nb_inplace_power
        pyFunc(pytype.ilshift),   // nb_inplace_lshift
        pyFunc(pytype.irshift),   // nb_inplace_rshift
        pyFunc(pytype.iand),      // nb_inplace_and
        pyFunc(pytype.ixor),      // nb_inplace_xor
        pyFunc(pytype.ior),       // nb_inplace_or
        pyFunc(pytype.floordiv),  // nb_floor_divide
        pyFunc(pytype.truediv),   // nb_true_divide
        pyFunc(pytype.ifloordiv), // nb_inplace_floor_divide
        pyFunc(pytype.itruediv),  // nb_inplace_true_divide
        pyFunc(pytype.index),     // nb_index
        pyFunc(pytype.matmul),    // nb_matrix_multiply
        pyFunc(pytype.imatmul),   // nb_inplace_matrix_multiply
    };

    std::vector<llvm::Constant *> sequenceSlots = {
        pyFunc(pytype.len),      // sq_length
        null,                    // sq_concat
        null,                    // sq_repeat
        null,                    // sq_item
        null,                    // was_sq_slice
        null,                    // sq_ass_item
        null,                    // was_sq_ass_slice
        pyFunc(pytype.contains), // sq_contains
        null,                    // sq_inplace_concat
        null,                    // sq_inplace_repeat
    };

    std::vector<llvm::Constant *> mappingSlots = {
        null,                   // mp_length
        pyFunc(pytype.getitem), // mp_subscript
        pyFunc(pytype.setitem), // mp_ass_subscript
    };

    bool needNumberSlots =
        std::find_if(numberSlots.begin(), numberSlots.end(),
                     [&](auto *v) { return v != null; }) != numberSlots.end();
    bool needSequenceSlots =
        std::find_if(sequenceSlots.begin(), sequenceSlots.end(),
                     [&](auto *v) { return v != null; }) != sequenceSlots.end();
    bool needMappingSlots =
        std::find_if(mappingSlots.begin(), mappingSlots.end(),
                     [&](auto *v) { return v != null; }) != mappingSlots.end();

    llvm::Constant *numberSlotsConst = null;
    llvm::Constant *sequenceSlotsConst = null;
    llvm::Constant *mappingSlotsConst = null;

    if (needNumberSlots) {
      auto *pyNumberSlotsVar = new llvm::GlobalVariable(
          *M, pyNumberMethodsType,
          /*isConstant=*/false, llvm::GlobalValue::PrivateLinkage,
          llvm::ConstantStruct::get(pyNumberMethodsType, numberSlots),
          ".pyext_number_slots." + pytype.name);
      numberSlotsConst = pyNumberSlotsVar;
    }

    if (needSequenceSlots) {
      auto *pySequenceSlotsVar = new llvm::GlobalVariable(
          *M, pySequenceMethodsType,
          /*isConstant=*/false, llvm::GlobalValue::PrivateLinkage,
          llvm::ConstantStruct::get(pySequenceMethodsType, sequenceSlots),
          ".pyext_sequence_slots." + pytype.name);
      sequenceSlotsConst = pySequenceSlotsVar;
    }

    if (needMappingSlots) {
      auto *pyMappingSlotsVar = new llvm::GlobalVariable(
          *M, pyMappingMethodsType,
          /*isConstant=*/false, llvm::GlobalValue::PrivateLinkage,
          llvm::ConstantStruct::get(pyMappingMethodsType, mappingSlots),
          ".pyext_mapping_slots." + pytype.name);
      mappingSlotsConst = pyMappingSlotsVar;
    }

    auto *refType = cast<types::RefType>(pytype.type);
    if (refType) {
      seqassertn(!refType->isPolymorphic(),
                 "Python extension types cannot be polymorphic");
    }
    auto *llvmType = getLLVMType(pytype.type);
    auto *objectType = llvm::StructType::get(pyObjectType, llvmType);
    auto codonSize =
        refType
            ? M->getDataLayout().getTypeAllocSize(getLLVMType(refType->getContents()))
            : 0;
    auto pySize = M->getDataLayout().getTypeAllocSize(objectType);

    auto *alloc = llvm::cast<llvm::Function>(
        M->getOrInsertFunction(pytype.name + ".py_alloc", ptr, ptr, i64).getCallee());
    {
      auto *entry = llvm::BasicBlock::Create(*context, "entry", alloc);
      B->SetInsertPoint(entry);
      auto *pythonObject = B->CreateCall(allocUncollectable, B->getInt64(pySize));
      auto *header = B->CreateInsertValue(
          llvm::ConstantStruct::get(pyObjectType, B->getInt64(1), null),
          alloc->arg_begin(), 1);
      B->CreateStore(header, pythonObject);
      if (refType) {
        auto *codonObject = B->CreateCall(
            makeAllocFunc(refType->getContents()->isAtomic()), B->getInt64(codonSize));
        B->CreateStore(codonObject, B->CreateGEP(objectType, pythonObject,
                                                 {zero64, B->getInt32(1)}));
      }
      B->CreateRet(pythonObject);
    }

    auto *delFn = pyFuncWrap(pytype.del, /*wrap=*/false);
    auto *dealloc = llvm::cast<llvm::Function>(
        M->getOrInsertFunction(pytype.name + ".py_dealloc", B->getVoidTy(), ptr)
            .getCallee());
    {
      llvm::Value *obj = dealloc->arg_begin();
      auto *entry = llvm::BasicBlock::Create(*context, "entry", dealloc);
      B->SetInsertPoint(entry);
      if (delFn != null)
        B->CreateCall(llvm::FunctionCallee(dealloc->getFunctionType(), delFn), obj);
      B->CreateCall(free, obj);
      B->CreateRetVoid();
    }

    auto *pyNew = llvm::cast<llvm::Function>(
        M->getOrInsertFunction("PyType_GenericNew", ptr, ptr, ptr, ptr).getCallee());

    std::vector<llvm::Constant *> typeSlots = {
        llvm::ConstantStruct::get(
            pyVarObjectType,
            llvm::ConstantStruct::get(pyObjectType, B->getInt64(1), pyTypeType),
            zero64),                              // PyObject_VAR_HEAD
        pyString(pymod.name + "." + pytype.name), // tp_name
        B->getInt64(pySize),                      // tp_basicsize
        zero64,                                   // tp_itemsize
        dealloc,                                  // tp_dealloc
        zero64,                                   // tp_vectorcall_offset
        null,                                     // tp_getattr
        null,                                     // tp_setattr
        null,                                     // tp_as_async
        pyFunc(pytype.repr),                      // tp_repr
        numberSlotsConst,                         // tp_as_number
        sequenceSlotsConst,                       // tp_as_sequence
        mappingSlotsConst,                        // tp_as_mapping
        pyFunc(pytype.hash),                      // tp_hash
        pyFunc(pytype.call),                      // tp_call
        pyFunc(pytype.str),                       // tp_str
        null,                                     // tp_getattro
        null,                                     // tp_setattro
        null,                                     // tp_as_buffer
        zero64,                                   // tp_flags
        pyString(pytype.doc),                     // tp_doc
        null,                                     // tp_traverse
        null,                                     // tp_clear
        pyFunc(pytype.cmp),                       // tp_richcompare
        zero64,                                   // tp_weaklistoffset
        pyFunc(pytype.iter),                      // tp_iter
        pyFunc(pytype.iternext),                  // tp_iternext
        pyFunctions(pytype.methods),              // tp_methods
        pyMembers(pytype.members, objectType),    // tp_members
        pyGetSet(pytype.getset),                  // tp_getset
        null,                                     // tp_base
        null,                                     // tp_dict
        null,                                     // tp_descr_get
        null,                                     // tp_descr_set
        zero64,                                   // tp_dictoffset
        pyFunc(pytype.init),                      // tp_init
        alloc,                                    // tp_alloc
        pyNew,                                    // tp_new
        free,                                     // tp_free
        null,                                     // tp_is_gc
        null,                                     // tp_bases
        null,                                     // tp_mro
        null,                                     // tp_cache
        null,                                     // tp_subclasses
        null,                                     // tp_weaklist
        null,                                     // tp_del
        zero32,                                   // tp_version_tag
        free,                                     // tp_finalize
        null,                                     // tp_vectorcall
        B->getInt8(0),                            // tp_watched
    };

    auto *pyTypeObjectVar = new llvm::GlobalVariable(
        *M, pyTypeObjectType,
        /*isConstant=*/false, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantStruct::get(pyTypeObjectType, typeSlots),
        ".pyext_type." + pytype.name);

    if (pytype.typePtrHook) {
      auto *hook = llvm::cast<llvm::Function>(pyFuncWrap(pytype.typePtrHook, false));
      for (auto it = llvm::inst_begin(hook), end = llvm::inst_end(hook); it != end;
           ++it) {
        if (auto *ret = llvm::dyn_cast<llvm::ReturnInst>(&*it))
          ret->setOperand(0, pyTypeObjectVar);
      }
    }

    typeVars.emplace(pytype.type, pyTypeObjectVar);
  }

  // Construct initialization hook
  auto pyIncRef = llvm::cast<llvm::Function>(
      M->getOrInsertFunction("Py_IncRef", B->getVoidTy(), ptr).getCallee());
  pyIncRef->setDoesNotThrow();

  auto pyDecRef = llvm::cast<llvm::Function>(
      M->getOrInsertFunction("Py_DecRef", B->getVoidTy(), ptr).getCallee());
  pyDecRef->setDoesNotThrow();

  auto *pyModuleCreate = llvm::cast<llvm::Function>(
      M->getOrInsertFunction("PyModule_Create2", ptr, ptr, i32).getCallee());
  pyModuleCreate->setDoesNotThrow();

  auto *pyTypeReady = llvm::cast<llvm::Function>(
      M->getOrInsertFunction("PyType_Ready", i32, ptr).getCallee());
  pyTypeReady->setDoesNotThrow();

  auto *pyModuleAddObject = llvm::cast<llvm::Function>(
      M->getOrInsertFunction("PyModule_AddObject", i32, ptr, ptr, ptr).getCallee());
  pyModuleAddObject->setDoesNotThrow();

  auto *pyModuleInit = llvm::cast<llvm::Function>(
      M->getOrInsertFunction("PyInit_" + pymod.name, ptr).getCallee());
  auto *block = llvm::BasicBlock::Create(*context, "entry", pyModuleInit);
  B->SetInsertPoint(block);

  if (auto *main = M->getFunction("main")) {
    main->setName(MAIN_UNCLASH);
    B->CreateCall({main->getFunctionType(), main}, {zero32, null});
  }

  // Set base types
  for (auto &pytype : pymod.types) {
    if (pytype.base) {
      auto subcIt = typeVars.find(pytype.type);
      auto baseIt = typeVars.find(pytype.base->type);
      seqassertn(subcIt != typeVars.end() && baseIt != typeVars.end(),
                 "types not found");
      // 30 is the index of tp_base
      B->CreateStore(baseIt->second, B->CreateConstInBoundsGEP2_64(
                                         pyTypeObjectType, subcIt->second, 0, 30));
    }
  }

  // Call PyType_Ready
  for (auto &pytype : pymod.types) {
    auto it = typeVars.find(pytype.type);
    seqassertn(it != typeVars.end(), "type not found");
    auto *typeVar = it->second;

    auto *fail = llvm::BasicBlock::Create(*context, "failure", pyModuleInit);
    block = llvm::BasicBlock::Create(*context, "success", pyModuleInit);
    auto *status = B->CreateCall(pyTypeReady, typeVar);
    B->CreateCondBr(B->CreateICmpSLT(status, zero32), fail, block);

    B->SetInsertPoint(fail);
    B->CreateRet(null);

    B->SetInsertPoint(block);
  }

  // Create module
  auto *mod = B->CreateCall(pyModuleCreate,
                            {pyModuleVar, B->getInt32(PYEXT_PYTHON_ABI_VERSION)});
  auto *fail = llvm::BasicBlock::Create(*context, "failure", pyModuleInit);
  block = llvm::BasicBlock::Create(*context, "success", pyModuleInit);

  B->CreateCondBr(B->CreateICmpEQ(mod, null), fail, block);
  B->SetInsertPoint(fail);
  B->CreateRet(null);

  B->SetInsertPoint(block);

  // Add types
  for (auto &pytype : pymod.types) {
    auto it = typeVars.find(pytype.type);
    seqassertn(it != typeVars.end(), "type not found");
    auto *typeVar = it->second;

    B->CreateCall(pyIncRef, typeVar);
    auto *status =
        B->CreateCall(pyModuleAddObject, {mod, pyString(pytype.name), typeVar});
    fail = llvm::BasicBlock::Create(*context, "failure", pyModuleInit);
    block = llvm::BasicBlock::Create(*context, "success", pyModuleInit);
    B->CreateCondBr(B->CreateICmpSLT(status, zero32), fail, block);

    B->SetInsertPoint(fail);
    B->CreateCall(pyDecRef, typeVar);
    B->CreateCall(pyDecRef, mod);
    B->CreateRet(null);

    B->SetInsertPoint(block);
  }
  B->CreateRet(mod);

  writeToObjectFile(filename);
}

void LLVMVisitor::compile(const std::string &filename, const std::string &argv0,
                          const std::vector<std::string> &libs,
                          const std::string &lflags) {
  llvm::StringRef f(filename);
  if (f.endswith(".ll")) {
    writeToLLFile(filename);
  } else if (f.endswith(".bc")) {
    writeToBitcodeFile(filename);
  } else if (f.endswith(".o") || f.endswith(".obj")) {
    writeToObjectFile(filename);
  } else if (f.endswith(".so") || f.endswith(".dylib")) {
    writeToExecutable(filename, argv0, /*library=*/true, libs, lflags);
  } else {
    writeToExecutable(filename, argv0, /*library=*/false, libs, lflags);
  }
}

void LLVMVisitor::run(const std::vector<std::string> &args,
                      const std::vector<std::string> &libs, const char *const *envp) {
  runLLVMPipeline();

  Timer t1("llvm/jitlink");
  for (auto &lib : libs) {
    std::string err;
    if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(lib.c_str(), &err)) {
      compilationError(err);
    }
  }

  DebugPlugin *dbp = nullptr;
  llvm::Triple triple(M->getTargetTriple());
  auto epc = llvm::cantFail(llvm::orc::SelfExecutorProcessControl::Create(
      std::make_shared<llvm::orc::SymbolStringPool>()));

  llvm::orc::LLJITBuilder builder;
  builder.setDataLayout(llvm::DataLayout(M.get()));
  builder.setObjectLinkingLayerCreator(
      [&epc, &dbp](llvm::orc::ExecutionSession &es, const llvm::Triple &triple)
          -> llvm::Expected<std::unique_ptr<llvm::orc::ObjectLayer>> {
        auto L = std::make_unique<llvm::orc::ObjectLinkingLayer>(
            es, llvm::cantFail(BoehmGCJITLinkMemoryManager::Create()));
        L->addPlugin(std::make_unique<llvm::orc::EHFrameRegistrationPlugin>(
            es, llvm::cantFail(llvm::orc::EPCEHFrameRegistrar::Create(es))));
        L->addPlugin(std::make_unique<llvm::orc::DebugObjectManagerPlugin>(
            es, llvm::cantFail(llvm::orc::createJITLoaderGDBRegistrar(es))));
        auto dbPlugin = std::make_unique<DebugPlugin>();
        dbp = dbPlugin.get();
        L->addPlugin(std::move(dbPlugin));
        return L;
      });
  builder.setJITTargetMachineBuilder(llvm::orc::JITTargetMachineBuilder(triple));

  auto jit = llvm::cantFail(builder.create());
  jit->getMainJITDylib().addGenerator(
      llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          jit->getDataLayout().getGlobalPrefix())));

  llvm::cantFail(jit->addIRModule({std::move(M), std::move(context)}));
  clearLLVMData();
  auto mainAddr = llvm::cantFail(jit->lookup("main"));

  if (db.debug) {
    runtime::setJITErrorCallback([dbp](const runtime::JITError &e) {
      fmt::print(stderr, "{}\n{}", e.getOutput(),
                 dbp->getPrettyBacktrace(e.getBacktrace()));
      std::abort();
    });
  } else {
    runtime::setJITErrorCallback([](const runtime::JITError &e) {
      fmt::print(stderr, "{}", e.getOutput());
      std::abort();
    });
  }
  t1.log();

  try {
    llvm::cantFail(epc->runAsMain(mainAddr, args));
  } catch (const runtime::JITError &e) {
    fmt::print(stderr, "{}\n", e.getOutput());
    std::abort();
  }
}

#define ALLOC_FAMILY "seq_alloc"

llvm::FunctionCallee LLVMVisitor::makeAllocFunc(bool atomic, bool uncollectable) {
  const std::string name =
      atomic ? (uncollectable ? "seq_alloc_atomic_uncollectable" : "seq_alloc_atomic")
             : (uncollectable ? "seq_alloc_uncollectable" : "seq_alloc");
  auto f = M->getOrInsertFunction(name, B->getInt8PtrTy(), B->getInt64Ty());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotThrow();
  g->setReturnDoesNotAlias();
  g->setOnlyAccessesInaccessibleMemory();
  g->addRetAttr(llvm::Attribute::AttrKind::NoUndef);
  g->addRetAttr(llvm::Attribute::AttrKind::NonNull);
  g->addFnAttrs(
      llvm::AttrBuilder(*context)
          .addAllocKindAttr(llvm::AllocFnKind::Alloc | llvm::AllocFnKind::Uninitialized)
          .addAllocSizeAttr(0, {})
          .addAttribute("alloc-family", ALLOC_FAMILY));
  return f;
}

llvm::FunctionCallee LLVMVisitor::makeReallocFunc() {
  // note that seq_realloc takes arguments (ptr, new_size, old_size)
  auto f = M->getOrInsertFunction("seq_realloc", B->getInt8PtrTy(), B->getInt8PtrTy(),
                                  B->getInt64Ty(), B->getInt64Ty());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotThrow();
  g->addRetAttr(llvm::Attribute::AttrKind::NoUndef);
  g->addRetAttr(llvm::Attribute::AttrKind::NonNull);
  g->addParamAttr(0, llvm::Attribute::AttrKind::AllocatedPointer);
  g->addFnAttrs(llvm::AttrBuilder(*context)
                    .addAllocKindAttr(llvm::AllocFnKind::Realloc |
                                      llvm::AllocFnKind::Uninitialized)
                    .addAllocSizeAttr(1, {})
                    .addAttribute("alloc-family", ALLOC_FAMILY));
  return f;
}

llvm::FunctionCallee LLVMVisitor::makeFreeFunc() {
  auto f = M->getOrInsertFunction("seq_free", B->getVoidTy(), B->getInt8PtrTy());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotThrow();
  g->addParamAttr(0, llvm::Attribute::AttrKind::AllocatedPointer);
  g->addFnAttrs(llvm::AttrBuilder(*context)
                    .addAllocKindAttr(llvm::AllocFnKind::Free)
                    .addAttribute("alloc-family", ALLOC_FAMILY));
  return f;
}

#undef ALLOC_FAMILY

llvm::FunctionCallee LLVMVisitor::makePersonalityFunc() {
  return M->getOrInsertFunction("seq_personality", B->getInt32Ty(), B->getInt32Ty(),
                                B->getInt32Ty(), B->getInt64Ty(), B->getInt8PtrTy(),
                                B->getInt8PtrTy());
}

llvm::FunctionCallee LLVMVisitor::makeExcAllocFunc() {
  auto f = M->getOrInsertFunction("seq_alloc_exc", B->getInt8PtrTy(), B->getInt32Ty(),
                                  B->getInt8PtrTy());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotThrow();
  return f;
}

llvm::FunctionCallee LLVMVisitor::makeThrowFunc() {
  auto f = M->getOrInsertFunction("seq_throw", B->getVoidTy(), B->getInt8PtrTy());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotReturn();
  return f;
}

llvm::FunctionCallee LLVMVisitor::makeTerminateFunc() {
  auto f = M->getOrInsertFunction("seq_terminate", B->getVoidTy(), B->getInt8PtrTy());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotReturn();
  return f;
}

llvm::StructType *LLVMVisitor::getTypeInfoType() {
  return llvm::StructType::get(B->getInt32Ty());
}

llvm::StructType *LLVMVisitor::getPadType() {
  return llvm::StructType::get(B->getInt8PtrTy(), B->getInt32Ty());
}

llvm::StructType *LLVMVisitor::getExceptionType() {
  return llvm::StructType::get(getTypeInfoType(), B->getInt8PtrTy());
}

namespace {
int typeIdxLookup(const std::string &name) {
  static std::unordered_map<std::string, int> cache;
  static int next = 1000;
  if (name.empty())
    return 0;
  auto it = cache.find(name);
  if (it != cache.end()) {
    return it->second;
  } else {
    const int myID = next++;
    cache[name] = myID;
    return myID;
  }
}
} // namespace

llvm::GlobalVariable *LLVMVisitor::getTypeIdxVar(const std::string &name) {
  auto *typeInfoType = getTypeInfoType();
  const std::string typeVarName = "codon.typeidx." + (name.empty() ? "<all>" : name);
  llvm::GlobalVariable *tidx = M->getGlobalVariable(typeVarName);
  int idx = typeIdxLookup(name);
  if (!tidx) {
    tidx = new llvm::GlobalVariable(
        *M, typeInfoType, /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantStruct::get(typeInfoType, B->getInt32(idx)), typeVarName);
    tidx->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  }
  return tidx;
}

llvm::GlobalVariable *LLVMVisitor::getTypeIdxVar(types::Type *catchType) {
  return getTypeIdxVar(catchType ? catchType->getName() : "");
}

int LLVMVisitor::getTypeIdx(types::Type *catchType) {
  return typeIdxLookup(catchType ? catchType->getName() : "");
}

llvm::Value *LLVMVisitor::call(llvm::FunctionCallee callee,
                               llvm::ArrayRef<llvm::Value *> args) {
  B->SetInsertPoint(block);
  if (trycatch.empty() || DisableExceptions) {
    return B->CreateCall(callee, args);
  } else {
    auto *normalBlock = llvm::BasicBlock::Create(*context, "invoke.normal", func);
    auto *unwindBlock = trycatch.back().exceptionBlock;
    auto *result = B->CreateInvoke(callee, normalBlock, unwindBlock, args);
    block = normalBlock;
    return result;
  }
}

static int nextSequenceNumber = 0;

void LLVMVisitor::enterLoop(LoopData data) {
  loops.push_back(std::move(data));
  loops.back().sequenceNumber = nextSequenceNumber++;
}

void LLVMVisitor::exitLoop() {
  seqassertn(!loops.empty(), "no loops present");
  loops.pop_back();
}

void LLVMVisitor::enterTryCatch(TryCatchData data) {
  trycatch.push_back(std::move(data));
  trycatch.back().sequenceNumber = nextSequenceNumber++;
}

void LLVMVisitor::exitTryCatch() {
  seqassertn(!trycatch.empty(), "no try catches present");
  trycatch.pop_back();
}

void LLVMVisitor::enterCatch(CatchData data) {
  catches.push_back(std::move(data));
  catches.back().sequenceNumber = nextSequenceNumber++;
}

void LLVMVisitor::exitCatch() {
  seqassertn(!catches.empty(), "no catches present");
  catches.pop_back();
}

LLVMVisitor::TryCatchData *LLVMVisitor::getInnermostTryCatch() {
  return trycatch.empty() ? nullptr : &trycatch.back();
}

LLVMVisitor::TryCatchData *LLVMVisitor::getInnermostTryCatchBeforeLoop() {
  if (!trycatch.empty() &&
      (loops.empty() || trycatch.back().sequenceNumber > loops.back().sequenceNumber))
    return &trycatch.back();
  return nullptr;
}

/*
 * General values, M, functions, vars
 */

void LLVMVisitor::visit(const Module *x) {
  // initialize M
  M = makeModule(*context, getSrcInfo(x));

  // args variable
  seqassertn(x->getArgVar()->isGlobal(), "arg var is not global");
  registerGlobal(x->getArgVar());

  // set up global variables and initialize functions
  for (auto *var : *x) {
    registerGlobal(var);
  }

  // process functions
  for (auto *var : *x) {
    if (auto *f = cast<Func>(var)) {
      process(f);
    }
  }

  const Func *main = x->getMainFunc();
  llvm::FunctionCallee realMain = makeLLVMFunction(main);
  process(main);
  setDebugInfoForNode(nullptr);

  // build canonical main function
  auto *strType = llvm::StructType::get(*context, {B->getInt64Ty(), B->getInt8PtrTy()});
  auto *arrType =
      llvm::StructType::get(*context, {B->getInt64Ty(), strType->getPointerTo()});

  auto *initFunc = llvm::cast<llvm::Function>(
      M->getOrInsertFunction("seq_init", B->getVoidTy(), B->getInt32Ty()).getCallee());
  auto *strlenFunc = llvm::cast<llvm::Function>(
      M->getOrInsertFunction("strlen", B->getInt64Ty(), B->getInt8PtrTy()).getCallee());

  // check if main exists already as an exported function
  const std::string mainName = M->getFunction("main") ? MAIN_UNCLASH : "main";
  auto *canonicalMainFunc = llvm::cast<llvm::Function>(
      M->getOrInsertFunction(mainName, B->getInt32Ty(), B->getInt32Ty(),
                             B->getInt8PtrTy()->getPointerTo())
          .getCallee());

  canonicalMainFunc->setPersonalityFn(
      llvm::cast<llvm::Constant>(makePersonalityFunc().getCallee()));
  auto argiter = canonicalMainFunc->arg_begin();
  llvm::Value *argc = argiter++;
  llvm::Value *argv = argiter;
  argc->setName("argc");
  argv->setName("argv");

  // The following generates code to put program arguments in an array, i.e.:
  //    for (int i = 0; i < argc; i++)
  //      array[i] = {strlen(argv[i]), argv[i]}
  auto *entryBlock = llvm::BasicBlock::Create(*context, "entry", canonicalMainFunc);
  auto *loopBlock = llvm::BasicBlock::Create(*context, "loop", canonicalMainFunc);
  auto *bodyBlock = llvm::BasicBlock::Create(*context, "body", canonicalMainFunc);
  auto *exitBlock = llvm::BasicBlock::Create(*context, "exit", canonicalMainFunc);

  B->SetInsertPoint(entryBlock);
  auto allocFunc = makeAllocFunc(/*atomic=*/false);
  llvm::Value *len = B->CreateZExt(argc, B->getInt64Ty());
  llvm::Value *elemSize = B->getInt64(M->getDataLayout().getTypeAllocSize(strType));
  llvm::Value *allocSize = B->CreateMul(len, elemSize);
  llvm::Value *ptr = B->CreateCall(allocFunc, allocSize);
  ptr = B->CreateBitCast(ptr, strType->getPointerTo());
  llvm::Value *arr = llvm::UndefValue::get(arrType);
  arr = B->CreateInsertValue(arr, len, 0);
  arr = B->CreateInsertValue(arr, ptr, 1);
  B->CreateBr(loopBlock);

  B->SetInsertPoint(loopBlock);
  llvm::PHINode *control = B->CreatePHI(B->getInt32Ty(), 2, "i");
  llvm::Value *next = B->CreateAdd(control, B->getInt32(1), "next");
  llvm::Value *cond = B->CreateICmpSLT(control, argc);
  control->addIncoming(B->getInt32(0), entryBlock);
  control->addIncoming(next, bodyBlock);
  B->CreateCondBr(cond, bodyBlock, exitBlock);

  B->SetInsertPoint(bodyBlock);
  llvm::Value *arg =
      B->CreateLoad(B->getInt8PtrTy(), B->CreateGEP(B->getInt8PtrTy(), argv, control));
  llvm::Value *argLen =
      B->CreateZExtOrTrunc(B->CreateCall(strlenFunc, arg), B->getInt64Ty());
  llvm::Value *str = llvm::UndefValue::get(strType);
  str = B->CreateInsertValue(str, argLen, 0);
  str = B->CreateInsertValue(str, arg, 1);
  B->CreateStore(str, B->CreateGEP(strType, ptr, control));
  B->CreateBr(loopBlock);

  B->SetInsertPoint(exitBlock);
  llvm::Value *argStorage = getVar(x->getArgVar());
  seqassertn(argStorage, "argument storage missing");
  B->CreateStore(arr, argStorage);
  const int flags = (db.debug ? SEQ_FLAG_DEBUG : 0) |
                    (db.capture ? SEQ_FLAG_CAPTURE_OUTPUT : 0) |
                    (db.standalone ? SEQ_FLAG_STANDALONE : 0);
  B->CreateCall(initFunc, B->getInt32(flags));

  // Put the entire program in a new function
  {
    auto *proxyMainTy = llvm::FunctionType::get(B->getVoidTy(), {}, false);
    auto *proxyMain = llvm::cast<llvm::Function>(
        M->getOrInsertFunction("codon.proxy_main", proxyMainTy).getCallee());
    proxyMain->setLinkage(llvm::GlobalValue::PrivateLinkage);
    proxyMain->setPersonalityFn(
        llvm::cast<llvm::Constant>(makePersonalityFunc().getCallee()));
    auto *proxyBlockEntry = llvm::BasicBlock::Create(*context, "entry", proxyMain);
    auto *proxyBlockMain = llvm::BasicBlock::Create(*context, "main", proxyMain);
    auto *proxyBlockExit = llvm::BasicBlock::Create(*context, "exit", proxyMain);
    B->SetInsertPoint(proxyBlockEntry);

    llvm::Value *shouldExit = B->getFalse();
    B->CreateCondBr(shouldExit, proxyBlockExit, proxyBlockMain);

    B->SetInsertPoint(proxyBlockExit);
    B->CreateRetVoid();

    // invoke real main
    auto *normal = llvm::BasicBlock::Create(*context, "normal", proxyMain);
    auto *unwind = llvm::BasicBlock::Create(*context, "unwind", proxyMain);
    B->SetInsertPoint(proxyBlockMain);
    B->CreateInvoke(realMain, normal, unwind);

    B->SetInsertPoint(unwind);
    llvm::LandingPadInst *caughtResult = B->CreateLandingPad(getPadType(), 1);
    caughtResult->setCleanup(true);
    caughtResult->addClause(getTypeIdxVar(nullptr));
    llvm::Value *unwindException = B->CreateExtractValue(caughtResult, 0);
    B->CreateCall(makeTerminateFunc(), unwindException);
    B->CreateUnreachable();

    B->SetInsertPoint(normal);
    B->CreateRetVoid();

    // actually make the call
    B->SetInsertPoint(exitBlock);
    B->CreateCall(proxyMain);
  }

  B->SetInsertPoint(exitBlock);
  B->CreateRet(B->getInt32(0));

  // make sure allocation functions have the correct attributes
  if (M->getFunction("seq_alloc"))
    makeAllocFunc(/*atomic=*/false, /*uncollectable=*/false);
  if (M->getFunction("seq_alloc_atomic"))
    makeAllocFunc(/*atomic=*/true, /*uncollectable=*/false);
  if (M->getFunction("seq_alloc_uncollectable"))
    makeAllocFunc(/*atomic=*/false, /*uncollectable=*/true);
  if (M->getFunction("seq_alloc_atomic_uncollectable"))
    makeAllocFunc(/*atomic=*/true, /*uncollectable=*/true);
  if (M->getFunction("seq_realloc"))
    makeReallocFunc();
  if (M->getFunction("seq_free"))
    makeFreeFunc();
}

llvm::DISubprogram *LLVMVisitor::getDISubprogramForFunc(const Func *x) {
  auto *srcInfo = getSrcInfo(x);
  llvm::DIFile *file = db.getFile(srcInfo->file);
  auto *derivedType = llvm::cast<llvm::DIDerivedType>(getDIType(x->getType()));
  auto *subroutineType =
      llvm::cast<llvm::DISubroutineType>(derivedType->getRawBaseType());

  std::string baseName = x->getUnmangledName();
  if (auto *parent = x->getParentType())
    baseName = parent->getName() + "." + baseName;
  llvm::DISubprogram *subprogram = db.builder->createFunction(
      file, baseName, getNameForFunction(x), file, srcInfo->line, subroutineType,
      /*ScopeLine=*/0, llvm::DINode::FlagZero,
      llvm::DISubprogram::toSPFlags(/*IsLocalToUnit=*/true,
                                    /*IsDefinition=*/true, /*IsOptimized=*/!db.debug));
  return subprogram;
}

llvm::Function *LLVMVisitor::makeLLVMFunction(const Func *x) {
  // process LLVM functions in full immediately
  if (auto *llvmFunc = cast<LLVMFunc>(x)) {
    auto *oldFunc = func;
    process(llvmFunc);
    setDebugInfoForNode(nullptr);
    auto *newFunc = func;
    func = oldFunc;
    return newFunc;
  }

  auto *funcType = cast<types::FuncType>(x->getType());
  llvm::Type *returnType = getLLVMType(funcType->getReturnType());
  std::vector<llvm::Type *> argTypes;
  for (const auto &argType : *funcType) {
    argTypes.push_back(getLLVMType(argType));
  }

  auto *llvmFuncType =
      llvm::FunctionType::get(returnType, argTypes, funcType->isVariadic());
  const std::string functionName = getNameForFunction(x);
  auto *f = llvm::cast<llvm::Function>(
      M->getOrInsertFunction(functionName, llvmFuncType).getCallee());
  if (!cast<ExternalFunc>(x)) {
    f->setSubprogram(getDISubprogramForFunc(x));
  }
  return f;
}

void LLVMVisitor::makeYield(llvm::Value *value, bool finalYield) {
  B->SetInsertPoint(block);
  if (value) {
    seqassertn(coro.promise, "promise is null");
    B->CreateStore(value, coro.promise);
  }
  llvm::FunctionCallee coroSuspend =
      llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_suspend);
  llvm::Value *suspendResult = B->CreateCall(
      coroSuspend, {llvm::ConstantTokenNone::get(*context), B->getInt1(finalYield)});

  block = llvm::BasicBlock::Create(*context, "yield.new", func);

  llvm::SwitchInst *inst = B->CreateSwitch(suspendResult, coro.suspend, 2);
  inst->addCase(B->getInt8(0), block);
  inst->addCase(B->getInt8(1), coro.cleanup);
}

void LLVMVisitor::visit(const ExternalFunc *x) {
  func = M->getFunction(getNameForFunction(x));
  coro = {};
  seqassertn(func, "{} not inserted", *x);
  func->setDoesNotThrow();
  func->setWillReturn();
}

namespace {
// internal function type checking
template <typename ParentType>
bool internalFuncMatchesIgnoreArgs(const std::string &name, const InternalFunc *x) {
  return name == x->getUnmangledName() && cast<ParentType>(x->getParentType());
}

template <typename ParentType, typename... ArgTypes, std::size_t... Index>
bool internalFuncMatches(const std::string &name, const InternalFunc *x,
                         std::index_sequence<Index...>) {
  auto *funcType = cast<types::FuncType>(x->getType());
  if (name != x->getUnmangledName() ||
      std::distance(funcType->begin(), funcType->end()) != sizeof...(ArgTypes))
    return false;
  std::vector<types::Type *> argTypes(funcType->begin(), funcType->end());
  std::vector<bool> m = {bool(cast<ParentType>(x->getParentType())),
                         bool(cast<ArgTypes>(argTypes[Index]))...};
  const bool match = std::all_of(m.begin(), m.end(), [](bool b) { return b; });
  return match;
}

template <typename ParentType, typename... ArgTypes>
bool internalFuncMatches(const std::string &name, const InternalFunc *x) {
  return internalFuncMatches<ParentType, ArgTypes...>(
      name, x, std::make_index_sequence<sizeof...(ArgTypes)>());
}
} // namespace

void LLVMVisitor::visit(const InternalFunc *x) {
  using namespace types;
  func = M->getFunction(getNameForFunction(x));
  coro = {};
  seqassertn(func, "{} not inserted", *x);
  setDebugInfoForNode(x);

  Type *parentType = x->getParentType();
  auto *funcType = cast<FuncType>(x->getType());
  std::vector<Type *> argTypes(funcType->begin(), funcType->end());

  func->setLinkage(llvm::GlobalValue::PrivateLinkage);
  func->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
  std::vector<llvm::Value *> args;
  for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
    args.push_back(it);
  }
  block = llvm::BasicBlock::Create(*context, "entry", func);
  B->SetInsertPoint(block);
  llvm::Value *result = nullptr;

  if (internalFuncMatches<PointerType, IntType>("__new__", x)) {
    auto *pointerType = cast<PointerType>(parentType);
    Type *baseType = pointerType->getBase();
    llvm::Type *llvmBaseType = getLLVMType(baseType);
    auto allocFunc = makeAllocFunc(baseType->isAtomic());
    llvm::Value *elemSize =
        B->getInt64(M->getDataLayout().getTypeAllocSize(llvmBaseType));
    llvm::Value *allocSize = B->CreateMul(elemSize, args[0]);
    result = B->CreateCall(allocFunc, allocSize);
    result = B->CreateBitCast(result, llvmBaseType->getPointerTo());
  }

  else if (internalFuncMatches<IntType, IntNType>("__new__", x)) {
    auto *intNType = cast<IntNType>(argTypes[0]);
    if (intNType->isSigned()) {
      result = B->CreateSExtOrTrunc(args[0], B->getInt64Ty());
    } else {
      result = B->CreateZExtOrTrunc(args[0], B->getInt64Ty());
    }
  }

  else if (internalFuncMatches<IntNType, IntType>("__new__", x)) {
    auto *intNType = cast<IntNType>(parentType);
    if (intNType->isSigned()) {
      result = B->CreateSExtOrTrunc(args[0], getLLVMType(intNType));
    } else {
      result = B->CreateZExtOrTrunc(args[0], getLLVMType(intNType));
    }
  }

  else if (internalFuncMatches<GeneratorType, GeneratorType>("__promise__", x)) {
    auto *generatorType = cast<GeneratorType>(parentType);
    llvm::Type *baseType = getLLVMType(generatorType->getBase());
    if (baseType->isVoidTy()) {
      result = llvm::ConstantPointerNull::get(B->getVoidTy()->getPointerTo());
    } else {
      llvm::FunctionCallee coroPromise =
          llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_promise);
      llvm::Value *aln = B->getInt32(M->getDataLayout().getPrefTypeAlignment(baseType));
      llvm::Value *from = B->getFalse();
      llvm::Value *ptr = B->CreateCall(coroPromise, {args[0], aln, from});
      result = B->CreateBitCast(ptr, baseType->getPointerTo());
    }
  }

  else if (internalFuncMatchesIgnoreArgs<RecordType>("__new__", x)) {
    auto *recordType = cast<RecordType>(cast<FuncType>(x->getType())->getReturnType());
    seqassertn(args.size() == std::distance(recordType->begin(), recordType->end()),
               "args size does not match");
    result = llvm::UndefValue::get(getLLVMType(recordType));
    for (auto i = 0; i < args.size(); i++) {
      result = B->CreateInsertValue(result, args[i], i);
    }
  }

  seqassertn(result, "internal function {} not found", *x);
  B->CreateRet(result);
}

std::string LLVMVisitor::buildLLVMCodeString(const LLVMFunc *x) {
  auto *funcType = cast<types::FuncType>(x->getType());
  seqassertn(funcType, "{} is not a function type", *x->getType());
  std::string bufStr;
  llvm::raw_string_ostream buf(bufStr);

  // build function signature
  buf << "define ";
  getLLVMType(funcType->getReturnType())->print(buf);
  buf << " @\"" << getNameForFunction(x) << "\"(";
  const int numArgs = std::distance(x->arg_begin(), x->arg_end());
  int argIndex = 0;
  for (auto it = x->arg_begin(); it != x->arg_end(); ++it) {
    getLLVMType((*it)->getType())->print(buf);
    buf << " %" << (*it)->getName();
    if (argIndex < numArgs - 1)
      buf << ", ";
    ++argIndex;
  }
  buf << ")";
  std::string signature = buf.str();
  bufStr.clear();

  // replace literal '{' and '}'
  std::string::size_type n = 0;
  while ((n = signature.find("{", n)) != std::string::npos) {
    signature.replace(n, 1, "{{");
    n += 2;
  }
  n = 0;
  while ((n = signature.find("}", n)) != std::string::npos) {
    signature.replace(n, 1, "}}");
    n += 2;
  }

  // build remaining code
  auto body = x->getLLVMBody();
  buf << x->getLLVMDeclarations() << "\n" << signature << " {{\n" << body << "\n}}";
  return buf.str();
}

void LLVMVisitor::visit(const LLVMFunc *x) {
  func = M->getFunction(getNameForFunction(x));
  coro = {};
  if (func)
    return;

  // build code
  std::string code = buildLLVMCodeString(x);

  // format code
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  for (auto it = x->literal_begin(); it != x->literal_end(); ++it) {
    if (it->isStatic()) {
      store.push_back(it->getStaticValue());
    } else if (it->isStaticStr()) {
      store.push_back(it->getStaticStringValue());
    } else if (it->isType()) {
      llvm::Type *llvmType = getLLVMType(it->getTypeValue());
      std::string bufStr;
      llvm::raw_string_ostream buf(bufStr);
      llvmType->print(buf);
      store.push_back(buf.str());
    } else {
      seqassertn(0, "formatting failed");
    }
  }
  code = fmt::vformat(code, store);

  llvm::SMDiagnostic err;
  std::unique_ptr<llvm::MemoryBuffer> buf = llvm::MemoryBuffer::getMemBuffer(code);
  seqassertn(buf, "could not create buffer");
  std::unique_ptr<llvm::Module> sub =
      llvm::parseIR(buf->getMemBufferRef(), err, *context);
  if (!sub) {
    // LOG("-> {}", code);
    std::string bufStr;
    llvm::raw_string_ostream buf(bufStr);
    err.print("LLVM", buf);
    // LOG("-> ERR {}", x->referenceString());
    // LOG("       {}", code);
    compilationError(fmt::format("{} ({})", buf.str(), x->getName()));
  }
  sub->setDataLayout(M->getDataLayout());

  llvm::Linker L(*M);
  const bool fail = L.linkInModule(std::move(sub));
  seqassertn(!fail, "linking failed");
  func = M->getFunction(getNameForFunction(x));
  seqassertn(func, "function not linked in");
  func->setLinkage(llvm::GlobalValue::PrivateLinkage);
  func->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
  func->setSubprogram(getDISubprogramForFunc(x));

  // set up debug info
  // for now we just set all to func's source location
  auto *srcInfo = getSrcInfo(x);
  for (auto &block : *func) {
    for (auto &inst : block) {
      if (!inst.getDebugLoc()) {
        inst.setDebugLoc(llvm::DebugLoc(llvm::DILocation::get(
            *context, srcInfo->line, srcInfo->col, func->getSubprogram())));
      }
    }
  }
}

void LLVMVisitor::visit(const BodiedFunc *x) {
  func = M->getFunction(getNameForFunction(x));
  coro = {};
  seqassertn(func, "{} not inserted", *x);
  setDebugInfoForNode(x);

  auto *fnAttributes = x->getAttribute<KeyValueAttribute>();
  if (x->isJIT()) {
    func->addFnAttr(llvm::Attribute::get(*context, "jit"));
  }
  if (x->isJIT() || (fnAttributes && fnAttributes->has(EXPORT_ATTR))) {
    func->setLinkage(llvm::GlobalValue::ExternalLinkage);
  } else {
    func->setLinkage(llvm::GlobalValue::PrivateLinkage);
  }
  if (fnAttributes && fnAttributes->has(INLINE_ATTR)) {
    func->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
  }
  if (fnAttributes && fnAttributes->has(NOINLINE_ATTR)) {
    func->addFnAttr(llvm::Attribute::AttrKind::NoInline);
  }
  if (fnAttributes && fnAttributes->has(GPU_KERNEL_ATTR)) {
    func->addFnAttr(llvm::Attribute::AttrKind::NoInline);
    func->addFnAttr(llvm::Attribute::get(*context, "kernel"));
    func->setLinkage(llvm::GlobalValue::ExternalLinkage);
  }
  if (!DisableExceptions)
    func->setPersonalityFn(
        llvm::cast<llvm::Constant>(makePersonalityFunc().getCallee()));

  auto *funcType = cast<types::FuncType>(x->getType());
  seqassertn(funcType, "{} is not a function type", *x->getType());
  auto *returnType = funcType->getReturnType();
  auto *entryBlock = llvm::BasicBlock::Create(*context, "entry", func);
  B->SetInsertPoint(entryBlock);

  // set up arguments and other symbols
  seqassertn(std::distance(func->arg_begin(), func->arg_end()) ==
                 std::distance(x->arg_begin(), x->arg_end()),
             "argument length does not match");
  unsigned argIdx = 1;
  auto argIter = func->arg_begin();
  for (auto varIter = x->arg_begin(); varIter != x->arg_end(); ++varIter) {
    const Var *var = *varIter;
    llvm::Value *storage = B->CreateAlloca(getLLVMType(var->getType()));
    B->CreateStore(argIter, storage);
    insertVar(var, storage);

    // debug info
    auto *srcInfo = getSrcInfo(var);
    llvm::DIFile *file = db.getFile(srcInfo->file);
    llvm::DISubprogram *scope = func->getSubprogram();
    llvm::DILocalVariable *debugVar = db.builder->createParameterVariable(
        scope, getDebugNameForVariable(var), argIdx, file, srcInfo->line,
        getDIType(var->getType()), db.debug);
    db.builder->insertDeclare(
        storage, debugVar, db.builder->createExpression(),
        llvm::DILocation::get(*context, srcInfo->line, srcInfo->col, scope),
        entryBlock);

    ++argIter;
    ++argIdx;
  }

  for (auto *var : *x) {
    llvm::Type *llvmType = getLLVMType(var->getType());
    if (llvmType->isVoidTy()) {
      insertVar(var, getDummyVoidValue());
    } else {
      llvm::Value *storage = B->CreateAlloca(llvmType);
      insertVar(var, storage);

      // debug info
      auto *srcInfo = getSrcInfo(var);
      llvm::DIFile *file = db.getFile(srcInfo->file);
      llvm::DISubprogram *scope = func->getSubprogram();
      llvm::DILocalVariable *debugVar = db.builder->createAutoVariable(
          scope, getDebugNameForVariable(var), file, srcInfo->line,
          getDIType(var->getType()), db.debug);
      db.builder->insertDeclare(
          storage, debugVar, db.builder->createExpression(),
          llvm::DILocation::get(*context, srcInfo->line, srcInfo->col, scope),
          entryBlock);
    }
  }

  auto *startBlock = llvm::BasicBlock::Create(*context, "start", func);

  if (x->isGenerator()) {
    func->setPresplitCoroutine();
    auto *generatorType = cast<types::GeneratorType>(returnType);
    seqassertn(generatorType, "{} is not a generator type", *returnType);

    llvm::FunctionCallee coroId =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_id);
    llvm::FunctionCallee coroBegin =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_begin);
    llvm::FunctionCallee coroSize = llvm::Intrinsic::getDeclaration(
        M.get(), llvm::Intrinsic::coro_size, {B->getInt64Ty()});
    llvm::FunctionCallee coroEnd =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_end);
    llvm::FunctionCallee coroAlloc =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_alloc);
    llvm::FunctionCallee coroFree =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_free);

    coro.cleanup = llvm::BasicBlock::Create(*context, "coro.cleanup", func);
    coro.suspend = llvm::BasicBlock::Create(*context, "coro.suspend", func);
    coro.exit = llvm::BasicBlock::Create(*context, "coro.exit", func);
    auto *allocBlock = llvm::BasicBlock::Create(*context, "coro.alloc", func);
    auto *freeBlock = llvm::BasicBlock::Create(*context, "coro.free", func);

    // coro ID and promise
    llvm::Value *id = nullptr;
    llvm::Value *nullPtr = llvm::ConstantPointerNull::get(B->getInt8PtrTy());
    if (!cast<types::VoidType>(generatorType->getBase())) {
      coro.promise = B->CreateAlloca(getLLVMType(generatorType->getBase()));
      coro.promise->setName("coro.promise");
      llvm::Value *promiseRaw = B->CreateBitCast(coro.promise, B->getInt8PtrTy());
      id = B->CreateCall(coroId, {B->getInt32(0), promiseRaw, nullPtr, nullPtr});
    } else {
      id = B->CreateCall(coroId, {B->getInt32(0), nullPtr, nullPtr, nullPtr});
    }
    id->setName("coro.id");
    llvm::Value *needAlloc = B->CreateCall(coroAlloc, id);
    B->CreateCondBr(needAlloc, allocBlock, startBlock);

    // coro alloc
    B->SetInsertPoint(allocBlock);
    llvm::Value *size = B->CreateCall(coroSize);
    auto allocFunc = makeAllocFunc(/*atomic=*/false);
    llvm::Value *alloc = B->CreateCall(allocFunc, size);
    B->CreateBr(startBlock);

    // coro start
    B->SetInsertPoint(startBlock);
    llvm::PHINode *phi = B->CreatePHI(B->getInt8PtrTy(), 2);
    phi->addIncoming(nullPtr, entryBlock);
    phi->addIncoming(alloc, allocBlock);
    coro.handle = B->CreateCall(coroBegin, {id, phi});
    coro.handle->setName("coro.handle");

    // coro cleanup
    B->SetInsertPoint(coro.cleanup);
    llvm::Value *mem = B->CreateCall(coroFree, {id, coro.handle});
    llvm::Value *needFree = B->CreateIsNotNull(mem);
    B->CreateCondBr(needFree, freeBlock, coro.suspend);

    // coro free
    B->SetInsertPoint(freeBlock); // no-op: GC will free automatically
    B->CreateBr(coro.suspend);

    // coro suspend
    B->SetInsertPoint(coro.suspend);
    B->CreateCall(coroEnd, {coro.handle, B->getFalse()});
    B->CreateRet(coro.handle);

    // coro exit
    block = coro.exit;
    makeYield(nullptr, /*finalYield=*/true);
    B->SetInsertPoint(block);
    B->CreateUnreachable();

    // initial yield
    block = startBlock;
    makeYield(); // coroutine will be initially suspended
  } else {
    B->CreateBr(startBlock);
    block = startBlock;
  }

  seqassertn(x->getBody(), "{} has no body [{}]", x->getName(), x->getSrcInfo());
  process(x->getBody());
  B->SetInsertPoint(block);

  if (x->isGenerator()) {
    B->CreateBr(coro.exit);
  } else {
    if (cast<types::VoidType>(returnType)) {
      B->CreateRetVoid();
    } else {
      B->CreateRet(llvm::Constant::getNullValue(getLLVMType(returnType)));
    }
  }
}

void LLVMVisitor::visit(const Var *x) { seqassertn(0, "cannot visit var"); }

void LLVMVisitor::visit(const VarValue *x) {
  if (auto *f = cast<Func>(x->getVar())) {
    value = getFunc(f);
    seqassertn(value, "{} value not found", *x);
  } else {
    llvm::Value *varPtr = getVar(x->getVar());
    seqassertn(varPtr, "{} value not found", *x);
    B->SetInsertPoint(block);
    value = B->CreateLoad(getLLVMType(x->getType()), varPtr);
  }
}

void LLVMVisitor::visit(const PointerValue *x) {
  llvm::Value *var = getVar(x->getVar());
  seqassertn(var, "{} variable not found", *x);
  value = var; // note: we don't load the pointer
}

/*
 * Types
 */

llvm::Type *LLVMVisitor::getLLVMType(types::Type *t) {
  if (auto *x = cast<types::IntType>(t)) {
    return B->getInt64Ty();
  }

  if (auto *x = cast<types::FloatType>(t)) {
    return B->getDoubleTy();
  }

  if (auto *x = cast<types::Float32Type>(t)) {
    return B->getFloatTy();
  }

  if (auto *x = cast<types::Float16Type>(t)) {
    return B->getHalfTy();
  }

  if (auto *x = cast<types::BFloat16Type>(t)) {
    return B->getBFloatTy();
  }

  if (auto *x = cast<types::Float128Type>(t)) {
    return llvm::Type::getFP128Ty(*context);
  }

  if (auto *x = cast<types::BoolType>(t)) {
    return B->getInt8Ty();
  }

  if (auto *x = cast<types::ByteType>(t)) {
    return B->getInt8Ty();
  }

  if (auto *x = cast<types::VoidType>(t)) {
    return B->getVoidTy();
  }

  if (auto *x = cast<types::RecordType>(t)) {
    std::vector<llvm::Type *> body;
    for (const auto &field : *x) {
      body.push_back(getLLVMType(field.getType()));
    }
    return llvm::StructType::get(*context, body);
  }

  if (auto *x = cast<types::RefType>(t)) {
    auto *p = B->getInt8PtrTy();
    if (x->isPolymorphic()) {
      return llvm::StructType::get(*context, {p, p});
    } else {
      return p;
    }
  }

  if (auto *x = cast<types::FuncType>(t)) {
    return getLLVMFuncType(x)->getPointerTo();
  }

  if (auto *x = cast<types::OptionalType>(t)) {
    if (cast<types::RefType>(x->getBase())) {
      return getLLVMType(x->getBase());
    } else {
      return llvm::StructType::get(B->getInt1Ty(), getLLVMType(x->getBase()));
    }
  }

  if (auto *x = cast<types::PointerType>(t)) {
    return getLLVMType(x->getBase())->getPointerTo();
  }

  if (auto *x = cast<types::GeneratorType>(t)) {
    return B->getInt8PtrTy();
  }

  if (auto *x = cast<types::IntNType>(t)) {
    return B->getIntNTy(x->getLen());
  }

  if (auto *x = cast<types::VectorType>(t)) {
    return llvm::VectorType::get(getLLVMType(x->getBase()), x->getCount(),
                                 /*Scalable=*/false);
  }

  if (auto *x = cast<types::UnionType>(t)) {
    auto &layout = M->getDataLayout();
    llvm::Type *largest = nullptr;
    size_t maxSize = 0;

    for (auto *t : *x) {
      auto *llvmType = getLLVMType(t);
      size_t size = layout.getTypeAllocSizeInBits(llvmType);
      if (!largest || size > maxSize) {
        largest = llvmType;
        maxSize = size;
      }
    }

    if (!largest)
      largest = llvm::StructType::get(*context, {});

    return llvm::StructType::get(*context, {B->getInt8Ty(), largest});
  }

  if (auto *x = cast<dsl::types::CustomType>(t)) {
    return x->getBuilder()->buildType(this);
  }

  seqassertn(0, "unknown type: {}", *t);
  return nullptr;
}

llvm::FunctionType *LLVMVisitor::getLLVMFuncType(types::Type *t) {
  auto *x = cast<types::FuncType>(t);
  seqassertn(x, "input type was not a func type");
  llvm::Type *returnType = getLLVMType(x->getReturnType());
  std::vector<llvm::Type *> argTypes;
  for (auto *argType : *x) {
    argTypes.push_back(getLLVMType(argType));
  }
  return llvm::FunctionType::get(returnType, argTypes, x->isVariadic());
}

llvm::DIType *LLVMVisitor::getDITypeHelper(
    types::Type *t, std::unordered_map<std::string, llvm::DICompositeType *> &cache) {
  llvm::Type *type = getLLVMType(t);
  auto &layout = M->getDataLayout();

  if (auto *x = cast<types::IntType>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type), llvm::dwarf::DW_ATE_signed);
  }

  if (auto *x = cast<types::FloatType>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type), llvm::dwarf::DW_ATE_float);
  }

  if (auto *x = cast<types::Float32Type>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type), llvm::dwarf::DW_ATE_float);
  }

  if (auto *x = cast<types::Float16Type>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type), llvm::dwarf::DW_ATE_float);
  }

  if (auto *x = cast<types::BFloat16Type>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type), llvm::dwarf::DW_ATE_float);
  }

  if (auto *x = cast<types::Float128Type>(t)) {
    return db.builder->createBasicType(x->getName(),
                                       layout.getTypeAllocSizeInBits(type),
                                       llvm::dwarf::DW_ATE_HP_float128);
  }

  if (auto *x = cast<types::BoolType>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type), llvm::dwarf::DW_ATE_boolean);
  }

  if (auto *x = cast<types::ByteType>(t)) {
    return db.builder->createBasicType(x->getName(),
                                       layout.getTypeAllocSizeInBits(type),
                                       llvm::dwarf::DW_ATE_signed_char);
  }

  if (auto *x = cast<types::VoidType>(t)) {
    return nullptr;
  }

  if (auto *x = cast<types::RecordType>(t)) {
    auto it = cache.find(x->getName());
    if (it != cache.end()) {
      return it->second;
    } else {
      auto *structType = llvm::cast<llvm::StructType>(type);
      auto *structLayout = layout.getStructLayout(structType);
      auto *srcInfo = getSrcInfo(x);
      auto *memberInfo = x->getAttribute<MemberAttribute>();
      llvm::DIFile *file = db.getFile(srcInfo->file);
      std::vector<llvm::Metadata *> members;

      llvm::DICompositeType *diType = db.builder->createStructType(
          file, x->getName(), file, srcInfo->line, structLayout->getSizeInBits(),
          /*AlignInBits=*/0, llvm::DINode::FlagZero, /*DerivedFrom=*/nullptr,
          db.builder->getOrCreateArray(members));

      // prevent infinite recursion on recursive types
      cache.emplace(x->getName(), diType);

      unsigned memberIdx = 0;
      for (const auto &field : *x) {
        auto *subSrcInfo = srcInfo;
        auto *subFile = file;
        if (memberInfo) {
          auto it = memberInfo->memberSrcInfo.find(field.getName());
          if (it != memberInfo->memberSrcInfo.end()) {
            subSrcInfo = &it->second;
            subFile = db.getFile(subSrcInfo->file);
          }
        }
        members.push_back(db.builder->createMemberType(
            diType, field.getName(), subFile, subSrcInfo->line,
            layout.getTypeAllocSizeInBits(getLLVMType(field.getType())),
            /*AlignInBits=*/0, structLayout->getElementOffsetInBits(memberIdx),
            llvm::DINode::FlagZero, getDITypeHelper(field.getType(), cache)));
        ++memberIdx;
      }

      db.builder->replaceArrays(diType, db.builder->getOrCreateArray(members));
      return diType;
    }
  }

  if (auto *x = cast<types::RefType>(t)) {
    auto *ref = db.builder->createReferenceType(
        llvm::dwarf::DW_TAG_reference_type, getDITypeHelper(x->getContents(), cache));
    if (x->isPolymorphic()) {
      auto *p = B->getInt8PtrTy();
      auto pointerSizeInBits = layout.getTypeAllocSizeInBits(p);
      auto *rtti = db.builder->createBasicType("rtti", pointerSizeInBits,
                                               llvm::dwarf::DW_ATE_address);
      auto *structType = llvm::StructType::get(p, p);
      auto *structLayout = layout.getStructLayout(structType);
      auto *srcInfo = getSrcInfo(x);
      llvm::DIFile *file = db.getFile(srcInfo->file);
      std::vector<llvm::Metadata *> members;

      llvm::DICompositeType *diType = db.builder->createStructType(
          file, x->getName(), file, srcInfo->line, structLayout->getSizeInBits(),
          /*AlignInBits=*/0, llvm::DINode::FlagZero, /*DerivedFrom=*/nullptr,
          db.builder->getOrCreateArray(members));

      members.push_back(db.builder->createMemberType(
          diType, "data", file, srcInfo->line, pointerSizeInBits,
          /*AlignInBits=*/0, structLayout->getElementOffsetInBits(0),
          llvm::DINode::FlagZero, ref));

      members.push_back(db.builder->createMemberType(
          diType, "rtti", file, srcInfo->line, pointerSizeInBits,
          /*AlignInBits=*/0, structLayout->getElementOffsetInBits(1),
          llvm::DINode::FlagZero, rtti));

      db.builder->replaceArrays(diType, db.builder->getOrCreateArray(members));
      return diType;
    } else {
      return ref;
    }
  }

  if (auto *x = cast<types::FuncType>(t)) {
    std::vector<llvm::Metadata *> argTypes = {
        getDITypeHelper(x->getReturnType(), cache)};
    for (auto *argType : *x) {
      argTypes.push_back(getDITypeHelper(argType, cache));
    }
    return db.builder->createPointerType(
        db.builder->createSubroutineType(llvm::MDTuple::get(*context, argTypes)),
        layout.getTypeAllocSizeInBits(type));
  }

  if (auto *x = cast<types::OptionalType>(t)) {
    if (cast<types::RefType>(x->getBase())) {
      return getDITypeHelper(x->getBase(), cache);
    } else {
      auto *baseType = getLLVMType(x->getBase());
      auto *structType = llvm::StructType::get(B->getInt1Ty(), baseType);
      auto *structLayout = layout.getStructLayout(structType);
      auto *srcInfo = getSrcInfo(x);
      auto i1SizeInBits = layout.getTypeAllocSizeInBits(B->getInt1Ty());
      auto *i1DebugType =
          db.builder->createBasicType("i1", i1SizeInBits, llvm::dwarf::DW_ATE_boolean);
      llvm::DIFile *file = db.getFile(srcInfo->file);
      std::vector<llvm::Metadata *> members;

      llvm::DICompositeType *diType = db.builder->createStructType(
          file, x->getName(), file, srcInfo->line, structLayout->getSizeInBits(),
          /*AlignInBits=*/0, llvm::DINode::FlagZero, /*DerivedFrom=*/nullptr,
          db.builder->getOrCreateArray(members));

      members.push_back(db.builder->createMemberType(
          diType, "has", file, srcInfo->line, i1SizeInBits,
          /*AlignInBits=*/0, structLayout->getElementOffsetInBits(0),
          llvm::DINode::FlagZero, i1DebugType));

      members.push_back(db.builder->createMemberType(
          diType, "val", file, srcInfo->line, layout.getTypeAllocSizeInBits(baseType),
          /*AlignInBits=*/0, structLayout->getElementOffsetInBits(1),
          llvm::DINode::FlagZero, getDITypeHelper(x->getBase(), cache)));

      db.builder->replaceArrays(diType, db.builder->getOrCreateArray(members));
      return diType;
    }
  }

  if (auto *x = cast<types::PointerType>(t)) {
    return db.builder->createPointerType(getDITypeHelper(x->getBase(), cache),
                                         layout.getTypeAllocSizeInBits(type));
  }

  if (auto *x = cast<types::GeneratorType>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type), llvm::dwarf::DW_ATE_address);
  }

  if (auto *x = cast<types::IntNType>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type),
        x->isSigned() ? llvm::dwarf::DW_ATE_signed : llvm::dwarf::DW_ATE_unsigned);
  }

  if (auto *x = cast<types::VectorType>(t)) {
    return db.builder->createBasicType(x->getName(),
                                       layout.getTypeAllocSizeInBits(type),
                                       llvm::dwarf::DW_ATE_unsigned);
  }

  if (auto *x = cast<types::UnionType>(t)) {
    return db.builder->createBasicType(x->getName(),
                                       layout.getTypeAllocSizeInBits(type),
                                       llvm::dwarf::DW_ATE_unsigned);
  }

  if (auto *x = cast<dsl::types::CustomType>(t)) {
    return x->getBuilder()->buildDebugType(this);
  }

  seqassertn(0, "unknown type");
  return nullptr;
}

llvm::DIType *LLVMVisitor::getDIType(types::Type *t) {
  std::unordered_map<std::string, llvm::DICompositeType *> cache;
  return getDITypeHelper(t, cache);
}

LLVMVisitor::LoopData *LLVMVisitor::getLoopData(id_t loopId) {
  for (auto &d : loops) {
    if (d.loopId == loopId)
      return &d;
  }
  return nullptr;
}

/*
 * Constants
 */

void LLVMVisitor::visit(const IntConst *x) {
  B->SetInsertPoint(block);
  value = B->getInt64(x->getVal());
}

void LLVMVisitor::visit(const FloatConst *x) {
  B->SetInsertPoint(block);
  value = llvm::ConstantFP::get(B->getDoubleTy(), x->getVal());
}

void LLVMVisitor::visit(const BoolConst *x) {
  B->SetInsertPoint(block);
  value = B->getInt8(x->getVal() ? 1 : 0);
}

void LLVMVisitor::visit(const StringConst *x) {
  B->SetInsertPoint(block);
  std::string s = x->getVal();
  auto *strVar =
      new llvm::GlobalVariable(*M, llvm::ArrayType::get(B->getInt8Ty(), s.length() + 1),
                               /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage,
                               llvm::ConstantDataArray::getString(*context, s), ".str");
  strVar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  auto *strType = llvm::StructType::get(B->getInt64Ty(), B->getInt8PtrTy());
  llvm::Value *ptr = B->CreateBitCast(strVar, B->getInt8PtrTy());
  llvm::Value *len = B->getInt64(s.length());
  llvm::Value *str = llvm::UndefValue::get(strType);
  str = B->CreateInsertValue(str, len, 0);
  str = B->CreateInsertValue(str, ptr, 1);
  value = str;
}

void LLVMVisitor::visit(const dsl::CustomConst *x) {
  x->getBuilder()->buildValue(this);
}

/*
 * Control flow
 */

void LLVMVisitor::visit(const SeriesFlow *x) {
  for (auto *value : *x) {
    process(value);
  }
}

void LLVMVisitor::visit(const IfFlow *x) {
  auto *trueBlock = llvm::BasicBlock::Create(*context, "if.true", func);
  auto *falseBlock = llvm::BasicBlock::Create(*context, "if.false", func);
  auto *exitBlock = llvm::BasicBlock::Create(*context, "if.exit", func);

  process(x->getCond());
  llvm::Value *cond = value;
  B->SetInsertPoint(block);
  cond = B->CreateTrunc(cond, B->getInt1Ty());
  B->CreateCondBr(cond, trueBlock, falseBlock);

  block = trueBlock;
  if (x->getTrueBranch()) {
    process(x->getTrueBranch());
  }
  B->SetInsertPoint(block);
  B->CreateBr(exitBlock);

  block = falseBlock;
  if (x->getFalseBranch()) {
    process(x->getFalseBranch());
  }
  B->SetInsertPoint(block);
  B->CreateBr(exitBlock);

  block = exitBlock;
}

void LLVMVisitor::visit(const WhileFlow *x) {
  auto *condBlock = llvm::BasicBlock::Create(*context, "while.cond", func);
  auto *bodyBlock = llvm::BasicBlock::Create(*context, "while.body", func);
  auto *exitBlock = llvm::BasicBlock::Create(*context, "while.exit", func);

  B->SetInsertPoint(block);
  B->CreateBr(condBlock);

  block = condBlock;
  process(x->getCond());
  llvm::Value *cond = value;
  B->SetInsertPoint(block);
  cond = B->CreateTrunc(cond, B->getInt1Ty());
  B->CreateCondBr(cond, bodyBlock, exitBlock);

  block = bodyBlock;
  enterLoop(
      {/*breakBlock=*/exitBlock, /*continueBlock=*/condBlock, /*loopId=*/x->getId()});
  process(x->getBody());
  exitLoop();
  B->SetInsertPoint(block);
  B->CreateBr(condBlock);

  block = exitBlock;
}

void LLVMVisitor::visit(const ForFlow *x) {
  seqassertn(!x->isParallel(), "parallel for-loop not lowered");
  llvm::Type *loopVarType = getLLVMType(x->getVar()->getType());
  llvm::Value *loopVar = getVar(x->getVar());
  seqassertn(loopVar, "{} loop variable not found", *x);

  auto *condBlock = llvm::BasicBlock::Create(*context, "for.cond", func);
  auto *bodyBlock = llvm::BasicBlock::Create(*context, "for.body", func);
  auto *cleanupBlock = llvm::BasicBlock::Create(*context, "for.cleanup", func);
  auto *exitBlock = llvm::BasicBlock::Create(*context, "for.exit", func);

  // LLVM coroutine intrinsics
  // https://prereleases.llvm.org/6.0.0/rc3/docs/Coroutines.html
  llvm::FunctionCallee coroResume =
      llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_resume);
  llvm::FunctionCallee coroDone =
      llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_done);
  llvm::FunctionCallee coroPromise =
      llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_promise);
  llvm::FunctionCallee coroDestroy =
      llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_destroy);

  process(x->getIter());
  llvm::Value *iter = value;
  B->SetInsertPoint(block);
  B->CreateBr(condBlock);

  block = condBlock;
  call(coroResume, {iter});
  B->SetInsertPoint(block);
  llvm::Value *done = B->CreateCall(coroDone, iter);
  B->CreateCondBr(done, cleanupBlock, bodyBlock);

  if (!loopVarType->isVoidTy()) {
    B->SetInsertPoint(bodyBlock);
    llvm::Value *alignment =
        B->getInt32(M->getDataLayout().getPrefTypeAlignment(loopVarType));
    llvm::Value *from = B->getFalse();
    llvm::Value *promise = B->CreateCall(coroPromise, {iter, alignment, from});
    llvm::Value *generatedValue = B->CreateLoad(loopVarType, promise);
    B->CreateStore(generatedValue, loopVar);
  }

  block = bodyBlock;
  enterLoop(
      {/*breakBlock=*/exitBlock, /*continueBlock=*/condBlock, /*loopId=*/x->getId()});
  process(x->getBody());
  exitLoop();
  B->SetInsertPoint(block);
  B->CreateBr(condBlock);

  B->SetInsertPoint(cleanupBlock);
  B->CreateCall(coroDestroy, iter);
  B->CreateBr(exitBlock);

  block = exitBlock;
}

void LLVMVisitor::visit(const ImperativeForFlow *x) {
  seqassertn(!x->isParallel(), "parallel for-loop not lowered");
  llvm::Value *loopVar = getVar(x->getVar());
  seqassertn(loopVar, "{} loop variable not found", *x);
  seqassertn(x->getStep() != 0, "step cannot be 0");

  auto *condBlock = llvm::BasicBlock::Create(*context, "imp_for.cond", func);
  auto *bodyBlock = llvm::BasicBlock::Create(*context, "imp_for.body", func);
  auto *updateBlock = llvm::BasicBlock::Create(*context, "imp_for.update", func);
  auto *exitBlock = llvm::BasicBlock::Create(*context, "imp_for.exit", func);

  process(x->getStart());
  llvm::Value *start = value;

  process(x->getEnd());
  llvm::Value *end = value;

  B->SetInsertPoint(block);
  B->CreateBr(condBlock);
  B->SetInsertPoint(condBlock);

  llvm::PHINode *phi = B->CreatePHI(B->getInt64Ty(), 2);
  phi->addIncoming(start, block);

  llvm::Value *done =
      (x->getStep() > 0) ? B->CreateICmpSGE(phi, end) : B->CreateICmpSLE(phi, end);
  B->CreateCondBr(done, exitBlock, bodyBlock);

  B->SetInsertPoint(bodyBlock);
  B->CreateStore(phi, loopVar);
  block = bodyBlock;

  enterLoop(
      {/*breakBlock=*/exitBlock, /*continueBlock=*/updateBlock, /*loopId=*/x->getId()});
  process(x->getBody());
  exitLoop();
  B->SetInsertPoint(block);
  B->CreateBr(updateBlock);

  B->SetInsertPoint(updateBlock);
  phi->addIncoming(B->CreateAdd(phi, B->getInt64(x->getStep())), updateBlock);
  B->CreateBr(condBlock);

  block = exitBlock;
}

namespace {
bool anyMatch(types::Type *type, std::vector<types::Type *> types) {
  if (type) {
    for (auto *t : types) {
      if (t && t->getName() == type->getName())
        return true;
    }
  } else {
    for (auto *t : types) {
      if (!t)
        return true;
    }
  }
  return false;
}
} // namespace

void LLVMVisitor::visit(const TryCatchFlow *x) {
  const bool isRoot = trycatch.empty();
  const bool supportBreakAndContinue = !loops.empty();
  B->SetInsertPoint(block);
  auto *entryBlock = llvm::BasicBlock::Create(*context, "trycatch.entry", func);
  B->CreateBr(entryBlock);

  TryCatchData tc;
  tc.exceptionBlock = llvm::BasicBlock::Create(*context, "trycatch.exception", func);
  tc.exceptionRouteBlock =
      llvm::BasicBlock::Create(*context, "trycatch.exception_route", func);
  tc.finallyBlock = llvm::BasicBlock::Create(*context, "trycatch.finally", func);

  auto *externalExcBlock =
      llvm::BasicBlock::Create(*context, "trycatch.exception_external", func);
  auto *unwindResumeBlock =
      llvm::BasicBlock::Create(*context, "trycatch.unwind_resume", func);
  auto *endBlock = llvm::BasicBlock::Create(*context, "trycatch.end", func);

  B->SetInsertPoint(func->getEntryBlock().getTerminator());
  auto *excStateNotThrown = B->getInt8(TryCatchData::State::NOT_THROWN);
  auto *excStateThrown = B->getInt8(TryCatchData::State::THROWN);
  auto *excStateCaught = B->getInt8(TryCatchData::State::CAUGHT);
  auto *excStateReturn = B->getInt8(TryCatchData::State::RETURN);
  auto *excStateBreak = B->getInt8(TryCatchData::State::BREAK);
  auto *excStateContinue = B->getInt8(TryCatchData::State::CONTINUE);

  llvm::StructType *padType = getPadType();
  llvm::StructType *unwindType = llvm::StructType::get(B->getInt64Ty()); // header only
  llvm::StructType *excType =
      llvm::StructType::get(getTypeInfoType(), B->getInt8PtrTy());

  if (isRoot) {
    tc.excFlag = B->CreateAlloca(B->getInt8Ty());
    tc.catchStore = B->CreateAlloca(padType);
    tc.delegateDepth = B->CreateAlloca(B->getInt64Ty());
    tc.retStore = (coro.exit || func->getReturnType()->isVoidTy())
                      ? nullptr
                      : B->CreateAlloca(func->getReturnType());
    tc.loopSequence = B->CreateAlloca(B->getInt64Ty());
    B->CreateStore(excStateNotThrown, tc.excFlag);
    B->CreateStore(llvm::ConstantAggregateZero::get(padType), tc.catchStore);
    B->CreateStore(B->getInt64(0), tc.delegateDepth);
    B->CreateStore(B->getInt64(-1), tc.loopSequence);
  } else {
    tc.excFlag = trycatch[0].excFlag;
    tc.catchStore = trycatch[0].catchStore;
    tc.delegateDepth = trycatch[0].delegateDepth;
    tc.retStore = trycatch[0].retStore;
    tc.loopSequence = trycatch[0].loopSequence;
  }

  // translate finally
  block = tc.finallyBlock;
  process(x->getFinally());
  auto *finallyBlock = block;
  B->SetInsertPoint(finallyBlock);
  llvm::Value *excFlagRead = B->CreateLoad(B->getInt8Ty(), tc.excFlag);

  if (!isRoot) {
    llvm::Value *depthRead = B->CreateLoad(B->getInt64Ty(), tc.delegateDepth);
    llvm::Value *delegate = B->CreateICmpSGT(depthRead, B->getInt64(0));
    auto *finallyNormal =
        llvm::BasicBlock::Create(*context, "trycatch.finally.normal", func);
    auto *finallyDelegate =
        llvm::BasicBlock::Create(*context, "trycatch.finally.delegate", func);
    B->CreateCondBr(delegate, finallyDelegate, finallyNormal);

    B->SetInsertPoint(finallyDelegate);
    llvm::Value *depthNew = B->CreateSub(depthRead, B->getInt64(1));
    llvm::Value *delegateNew = B->CreateICmpSGT(depthNew, B->getInt64(0));
    B->CreateStore(depthNew, tc.delegateDepth);
    B->CreateCondBr(delegateNew, trycatch.back().finallyBlock,
                    trycatch.back().exceptionRouteBlock);

    finallyBlock = finallyNormal;
    B->SetInsertPoint(finallyNormal);
  }

  B->SetInsertPoint(finallyBlock);
  llvm::SwitchInst *theSwitch =
      B->CreateSwitch(excFlagRead, endBlock, supportBreakAndContinue ? 5 : 3);
  theSwitch->addCase(excStateCaught, endBlock);
  theSwitch->addCase(excStateThrown, unwindResumeBlock);

  if (isRoot) {
    auto *finallyReturn =
        llvm::BasicBlock::Create(*context, "trycatch.finally.return", func);
    theSwitch->addCase(excStateReturn, finallyReturn);
    B->SetInsertPoint(finallyReturn);
    if (coro.exit) {
      B->CreateBr(coro.exit);
    } else if (tc.retStore) {
      llvm::Value *retVal = B->CreateLoad(func->getReturnType(), tc.retStore);
      B->CreateRet(retVal);
    } else {
      B->CreateRetVoid();
    }
  } else {
    theSwitch->addCase(excStateReturn, trycatch.back().finallyBlock);
  }

  if (supportBreakAndContinue) {
    auto prevSeq = isRoot ? -1 : trycatch.back().sequenceNumber;

    auto *finallyBreak =
        llvm::BasicBlock::Create(*context, "trycatch.finally.break", func);
    auto *finallyBreakDone =
        llvm::BasicBlock::Create(*context, "trycatch.finally.break.done", func);
    auto *finallyContinue =
        llvm::BasicBlock::Create(*context, "trycatch.finally.continue", func);
    auto *finallyContinueDone =
        llvm::BasicBlock::Create(*context, "trycatch.finally.continue.done", func);

    B->SetInsertPoint(finallyBreak);
    auto *breakSwitch =
        B->CreateSwitch(B->CreateLoad(B->getInt64Ty(), tc.loopSequence), endBlock, 0);
    B->SetInsertPoint(finallyBreakDone);
    B->CreateStore(excStateNotThrown, tc.excFlag);
    auto *breakDoneSwitch =
        B->CreateSwitch(B->CreateLoad(B->getInt64Ty(), tc.loopSequence), endBlock, 0);

    B->SetInsertPoint(finallyContinue);
    auto *continueSwitch =
        B->CreateSwitch(B->CreateLoad(B->getInt64Ty(), tc.loopSequence), endBlock, 0);
    B->SetInsertPoint(finallyContinueDone);
    B->CreateStore(excStateNotThrown, tc.excFlag);
    auto *continueDoneSwitch =
        B->CreateSwitch(B->CreateLoad(B->getInt64Ty(), tc.loopSequence), endBlock, 0);

    for (auto &l : loops) {
      if (!trycatch.empty() && l.sequenceNumber < prevSeq) {
        breakSwitch->addCase(B->getInt64(l.sequenceNumber),
                             trycatch.back().finallyBlock);
        continueSwitch->addCase(B->getInt64(l.sequenceNumber),
                                trycatch.back().finallyBlock);
      } else {
        breakSwitch->addCase(B->getInt64(l.sequenceNumber), finallyBreakDone);
        breakDoneSwitch->addCase(B->getInt64(l.sequenceNumber), l.breakBlock);
        continueSwitch->addCase(B->getInt64(l.sequenceNumber), finallyContinueDone);
        continueDoneSwitch->addCase(B->getInt64(l.sequenceNumber), l.continueBlock);
      }
    }
    theSwitch->addCase(excStateBreak, finallyBreak);
    theSwitch->addCase(excStateContinue, finallyContinue);
  }

  // try and catch translate
  std::vector<const TryCatchFlow::Catch *> catches;
  for (auto &c : *x) {
    catches.push_back(&c);
  }
  llvm::BasicBlock *catchAll = nullptr;

  for (auto *c : catches) {
    auto *catchBlock = llvm::BasicBlock::Create(*context, "trycatch.catch", func);
    tc.catchTypes.push_back(c->getType());
    tc.handlers.push_back(catchBlock);

    if (!c->getType()) {
      seqassertn(!catchAll, "cannot be catch all");
      catchAll = catchBlock;
    }
  }

  // translate try
  block = entryBlock;
  enterTryCatch(tc);
  process(x->getBody());
  exitTryCatch();

  // make sure we always get to finally block
  B->SetInsertPoint(block);
  B->CreateBr(tc.finallyBlock);

  // rethrow if uncaught
  B->SetInsertPoint(unwindResumeBlock);
  if (DisableExceptions) {
    B->CreateUnreachable();
  } else {
    B->CreateResume(B->CreateLoad(padType, tc.catchStore));
  }

  // make sure we delegate to parent try-catch if necessary
  std::vector<types::Type *> catchTypesFull(tc.catchTypes);
  std::vector<llvm::BasicBlock *> handlersFull(tc.handlers);
  std::vector<unsigned> depths(tc.catchTypes.size(), 0);
  unsigned depth = 1;

  unsigned catchAllDepth = 0;
  for (auto it = trycatch.rbegin(); it != trycatch.rend(); ++it) {
    if (catchAll) // can't ever delegate past catch-all
      break;

    seqassertn(it->catchTypes.size() == it->handlers.size(), "handler mismatch");
    for (unsigned i = 0; i < it->catchTypes.size(); i++) {
      if (!anyMatch(it->catchTypes[i], catchTypesFull)) {
        catchTypesFull.push_back(it->catchTypes[i]);
        depths.push_back(depth);

        if (!it->catchTypes[i] && !catchAll) {
          // catch-all is in parent; set finally depth
          catchAll =
              llvm::BasicBlock::Create(*context, "trycatch.fdepth_catchall", func);
          B->SetInsertPoint(catchAll);
          B->CreateStore(B->getInt64(depth), tc.delegateDepth);
          B->CreateBr(it->handlers[i]);
          handlersFull.push_back(catchAll);
          catchAllDepth = depth;
        } else {
          handlersFull.push_back(it->handlers[i]);
        }
      }
    }
    ++depth;
  }

  // exception handling
  B->SetInsertPoint(tc.exceptionBlock);
  llvm::LandingPadInst *caughtResult = nullptr;
  if (!DisableExceptions) {
    caughtResult = B->CreateLandingPad(padType, catches.size());
    caughtResult->setCleanup(true);
  }
  std::vector<llvm::Value *> typeIndices;

  for (auto *catchType : catchTypesFull) {
    seqassertn(!catchType || cast<types::RefType>(catchType), "invalid catch type");
    const std::string typeVarName =
        "codon.typeidx." + (catchType ? catchType->getName() : "<all>");
    llvm::GlobalVariable *tidx = getTypeIdxVar(catchType);
    typeIndices.push_back(tidx);
    if (caughtResult)
      caughtResult->addClause(tidx);
  }

  llvm::Value *caughtResultOrUndef = caughtResult
                                         ? llvm::cast<llvm::Value>(caughtResult)
                                         : llvm::UndefValue::get(padType);
  auto *unwindException = B->CreateExtractValue(caughtResultOrUndef, 0);
  B->CreateStore(caughtResultOrUndef, tc.catchStore);
  B->CreateStore(excStateThrown, tc.excFlag);
  llvm::Value *depthMax = B->getInt64(trycatch.size());
  B->CreateStore(depthMax, tc.delegateDepth);

  llvm::Value *unwindExceptionClass = B->CreateLoad(
      B->getInt64Ty(),
      B->CreateStructGEP(
          unwindType, B->CreatePointerCast(unwindException, unwindType->getPointerTo()),
          0));

  // check for foreign exceptions
  B->CreateCondBr(
      B->CreateICmpEQ(unwindExceptionClass, B->getInt64(SEQ_EXCEPTION_CLASS)),
      tc.exceptionRouteBlock, externalExcBlock);

  // external exception (currently assumed to be unreachable)
  B->SetInsertPoint(externalExcBlock);
  B->CreateUnreachable();

  // reroute Codon exceptions
  B->SetInsertPoint(tc.exceptionRouteBlock);
  unwindException = B->CreateExtractValue(B->CreateLoad(padType, tc.catchStore), 0);
  llvm::Value *excVal =
      B->CreatePointerCast(B->CreateConstGEP1_64(B->getInt8Ty(), unwindException,
                                                 (uint64_t)seq_exc_offset()),
                           excType->getPointerTo());

  llvm::Value *loadedExc = B->CreateLoad(excType, excVal);
  llvm::Value *objType = B->CreateExtractValue(loadedExc, 0);
  objType = B->CreateExtractValue(objType, 0);
  llvm::Value *objPtr = B->CreateExtractValue(loadedExc, 1);

  // set depth when catch-all entered
  auto *defaultRouteBlock = llvm::BasicBlock::Create(*context, "trycatch.fdepth", func);
  B->SetInsertPoint(defaultRouteBlock);
  if (catchAll)
    B->CreateStore(B->getInt64(catchAllDepth), tc.delegateDepth);
  B->CreateBr(catchAll ? (catchAllDepth > 0 ? tc.finallyBlock : catchAll)
                       : tc.finallyBlock);

  B->SetInsertPoint(tc.exceptionRouteBlock);
  llvm::SwitchInst *switchToCatchBlock =
      B->CreateSwitch(objType, defaultRouteBlock, (unsigned)handlersFull.size());
  for (unsigned i = 0; i < handlersFull.size(); i++) {
    // set finally depth
    auto *depthSet = llvm::BasicBlock::Create(*context, "trycatch.fdepth", func);
    B->SetInsertPoint(depthSet);
    B->CreateStore(B->getInt64(depths[i]), tc.delegateDepth);
    B->CreateBr((i < tc.handlers.size()) ? handlersFull[i] : tc.finallyBlock);

    if (catchTypesFull[i]) {
      switchToCatchBlock->addCase(B->getInt32((uint64_t)getTypeIdx(catchTypesFull[i])),
                                  depthSet);
    }

    // translate catch body if this block is ours (vs. a parent's)
    if (i < catches.size()) {
      block = handlersFull[i];
      B->SetInsertPoint(block);
      const Var *var = catches[i]->getVar();

      if (var) {
        llvm::Value *obj = B->CreateBitCast(objPtr, getLLVMType(catches[i]->getType()));
        llvm::Value *varPtr = getVar(var);
        seqassertn(varPtr, "could not get catch var");
        B->CreateStore(obj, varPtr);
      }

      B->CreateStore(excStateCaught, tc.excFlag);
      CatchData cd;
      cd.exception = objPtr;
      cd.typeId = objType;
      enterCatch(cd);
      process(catches[i]->getHandler());
      exitCatch();
      B->SetInsertPoint(block);
      B->CreateBr(tc.finallyBlock);
    }
  }

  block = endBlock;
}

void LLVMVisitor::callStage(const PipelineFlow::Stage *stage) {
  llvm::Value *output = value;
  process(stage->getCallee());
  llvm::Value *f = value;
  std::vector<llvm::Value *> args;
  for (const auto *arg : *stage) {
    if (arg) {
      process(arg);
      args.push_back(value);
    } else {
      args.push_back(output);
    }
  }

  auto *funcType = getLLVMFuncType(stage->getCallee()->getType());
  value = call({funcType, f}, args);
}

void LLVMVisitor::codegenPipeline(
    const std::vector<const PipelineFlow::Stage *> &stages, unsigned where) {
  if (where >= stages.size()) {
    return;
  }

  auto *stage = stages[where];

  if (where == 0) {
    process(stage->getCallee());
    codegenPipeline(stages, where + 1);
    return;
  }

  auto *prevStage = stages[where - 1];
  const bool generator = prevStage->isGenerator();

  if (generator) {
    auto *generatorType = cast<types::GeneratorType>(prevStage->getOutputType());
    seqassertn(generatorType, "{} is not a generator type",
               *prevStage->getOutputType());
    auto *baseType = getLLVMType(generatorType->getBase());

    auto *condBlock = llvm::BasicBlock::Create(*context, "pipeline.cond", func);
    auto *bodyBlock = llvm::BasicBlock::Create(*context, "pipeline.body", func);
    auto *cleanupBlock = llvm::BasicBlock::Create(*context, "pipeline.cleanup", func);
    auto *exitBlock = llvm::BasicBlock::Create(*context, "pipeline.exit", func);

    // LLVM coroutine intrinsics
    // https://prereleases.llvm.org/6.0.0/rc3/docs/Coroutines.html
    llvm::FunctionCallee coroResume =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_resume);
    llvm::FunctionCallee coroDone =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_done);
    llvm::FunctionCallee coroPromise =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_promise);
    llvm::FunctionCallee coroDestroy =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_destroy);

    llvm::Value *iter = value;
    B->SetInsertPoint(block);
    B->CreateBr(condBlock);

    block = condBlock;
    call(coroResume, {iter});
    B->SetInsertPoint(block);
    llvm::Value *done = B->CreateCall(coroDone, iter);
    B->CreateCondBr(done, cleanupBlock, bodyBlock);

    B->SetInsertPoint(bodyBlock);
    llvm::Value *alignment =
        B->getInt32(M->getDataLayout().getPrefTypeAlignment(baseType));
    llvm::Value *from = B->getFalse();
    llvm::Value *promise = B->CreateCall(coroPromise, {iter, alignment, from});
    promise = B->CreateBitCast(promise, baseType->getPointerTo());
    value = B->CreateLoad(baseType, promise);

    block = bodyBlock;
    callStage(stage);
    codegenPipeline(stages, where + 1);

    B->SetInsertPoint(block);
    B->CreateBr(condBlock);

    B->SetInsertPoint(cleanupBlock);
    B->CreateCall(coroDestroy, iter);
    B->CreateBr(exitBlock);

    block = exitBlock;
  } else {
    callStage(stage);
    codegenPipeline(stages, where + 1);
  }
}

void LLVMVisitor::visit(const PipelineFlow *x) {
  std::vector<const PipelineFlow::Stage *> stages;
  for (const auto &stage : *x) {
    stages.push_back(&stage);
  }
  codegenPipeline(stages);
}

void LLVMVisitor::visit(const dsl::CustomFlow *x) {
  B->SetInsertPoint(block);
  value = x->getBuilder()->buildValue(this);
}

/*
 * Instructions
 */

void LLVMVisitor::visit(const AssignInstr *x) {
  llvm::Value *var = getVar(x->getLhs());
  seqassertn(var, "could not find {} var", *x->getLhs());
  process(x->getRhs());
  if (var != getDummyVoidValue()) {
    B->SetInsertPoint(block);
    B->CreateStore(value, var);
  }
}

void LLVMVisitor::visit(const ExtractInstr *x) {
  auto *memberedType = cast<types::MemberedType>(x->getVal()->getType());
  seqassertn(memberedType, "{} is not a membered type", *x->getVal()->getType());
  const int index = memberedType->getMemberIndex(x->getField());
  seqassertn(index >= 0, "invalid index");

  process(x->getVal());
  B->SetInsertPoint(block);
  if (auto *refType = cast<types::RefType>(memberedType)) {
    if (refType->isPolymorphic())
      value =
          B->CreateExtractValue(value, 0); // polymorphic ref type is tuple (data, rtti)
    value = B->CreateLoad(getLLVMType(refType->getContents()), value);
  }
  value = B->CreateExtractValue(value, index);
}

void LLVMVisitor::visit(const InsertInstr *x) {
  auto *refType = cast<types::RefType>(x->getLhs()->getType());
  seqassertn(refType, "{} is not a reference type", *x->getLhs()->getType());
  const int index = refType->getMemberIndex(x->getField());
  seqassertn(index >= 0, "invalid index");

  process(x->getLhs());
  llvm::Value *lhs = value;
  process(x->getRhs());
  llvm::Value *rhs = value;

  B->SetInsertPoint(block);
  if (refType->isPolymorphic())
    lhs = B->CreateExtractValue(lhs, 0); // polymorphic ref type is tuple (data, rtti)
  llvm::Value *load = B->CreateLoad(getLLVMType(refType->getContents()), lhs);
  load = B->CreateInsertValue(load, rhs, index);
  B->CreateStore(load, lhs);
}

void LLVMVisitor::visit(const CallInstr *x) {
  B->SetInsertPoint(block);
  process(x->getCallee());
  llvm::Value *f = value;

  std::vector<llvm::Value *> args;
  for (auto *arg : *x) {
    B->SetInsertPoint(block);
    process(arg);
    args.push_back(value);
  }

  auto *funcType = getLLVMFuncType(x->getCallee()->getType());
  value = call({funcType, f}, args);
}

void LLVMVisitor::visit(const TypePropertyInstr *x) {
  B->SetInsertPoint(block);
  switch (x->getProperty()) {
  case TypePropertyInstr::Property::SIZEOF:
    value = B->getInt64(
        M->getDataLayout().getTypeAllocSize(getLLVMType(x->getInspectType())));
    break;
  case TypePropertyInstr::Property::IS_ATOMIC:
    value = B->getInt8(x->getInspectType()->isAtomic() ? 1 : 0);
    break;
  case TypePropertyInstr::Property::IS_CONTENT_ATOMIC:
    value = B->getInt8(x->getInspectType()->isContentAtomic() ? 1 : 0);
    break;
  default:
    seqassertn(0, "unknown type property");
  }
}

void LLVMVisitor::visit(const YieldInInstr *x) {
  B->SetInsertPoint(block);
  if (x->isSuspending()) {
    llvm::FunctionCallee coroSuspend =
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::coro_suspend);
    llvm::Value *tok = llvm::ConstantTokenNone::get(*context);
    llvm::Value *final = B->getFalse();
    llvm::Value *susp = B->CreateCall(coroSuspend, {tok, final});

    block = llvm::BasicBlock::Create(*context, "yieldin.new", func);
    llvm::SwitchInst *inst = B->CreateSwitch(susp, coro.suspend, 2);
    inst->addCase(B->getInt8(0), block);
    inst->addCase(B->getInt8(1), coro.cleanup);
    B->SetInsertPoint(block);
  }
  value = B->CreateLoad(getLLVMType(x->getType()), coro.promise);
}

void LLVMVisitor::visit(const StackAllocInstr *x) {
  auto *recordType = cast<types::RecordType>(x->getType());
  seqassertn(recordType, "stack alloc does not have record type");
  auto *ptrType = cast<types::PointerType>(recordType->back().getType());
  seqassertn(ptrType, "array did not have ptr type");

  auto *arrayType = llvm::cast<llvm::StructType>(getLLVMType(x->getType()));
  B->SetInsertPoint(func->getEntryBlock().getTerminator());
  llvm::Value *len = B->getInt64(x->getCount());
  llvm::Value *ptr = B->CreateAlloca(getLLVMType(ptrType->getBase()), len);
  llvm::Value *arr = llvm::UndefValue::get(arrayType);
  arr = B->CreateInsertValue(arr, len, 0);
  arr = B->CreateInsertValue(arr, ptr, 1);
  value = arr;
}

void LLVMVisitor::visit(const TernaryInstr *x) {
  auto *trueBlock = llvm::BasicBlock::Create(*context, "ternary.true", func);
  auto *falseBlock = llvm::BasicBlock::Create(*context, "ternary.false", func);
  auto *exitBlock = llvm::BasicBlock::Create(*context, "ternary.exit", func);

  llvm::Type *valueType = getLLVMType(x->getType());
  process(x->getCond());
  llvm::Value *cond = value;

  B->SetInsertPoint(block);
  cond = B->CreateTrunc(cond, B->getInt1Ty());
  B->CreateCondBr(cond, trueBlock, falseBlock);

  block = trueBlock;
  process(x->getTrueValue());
  llvm::Value *trueValue = value;
  trueBlock = block;
  B->SetInsertPoint(trueBlock);
  B->CreateBr(exitBlock);

  block = falseBlock;
  process(x->getFalseValue());
  llvm::Value *falseValue = value;
  falseBlock = block;
  B->SetInsertPoint(falseBlock);
  B->CreateBr(exitBlock);

  B->SetInsertPoint(exitBlock);
  llvm::PHINode *phi = B->CreatePHI(valueType, 2);
  phi->addIncoming(trueValue, trueBlock);
  phi->addIncoming(falseValue, falseBlock);
  value = phi;
  block = exitBlock;
}

void LLVMVisitor::visit(const BreakInstr *x) {
  seqassertn(!loops.empty(), "not in a loop");
  B->SetInsertPoint(block);

  auto *loop = !x->getLoop() ? &loops.back() : getLoopData(x->getLoop()->getId());

  if (trycatch.empty() || trycatch.back().sequenceNumber < loop->sequenceNumber) {
    B->CreateBr(loop->breakBlock);
  } else {
    auto *tc = &trycatch.back();
    auto *excStateBreak = B->getInt8(TryCatchData::State::BREAK);
    B->CreateStore(excStateBreak, tc->excFlag);
    B->CreateStore(B->getInt64(loop->sequenceNumber), tc->loopSequence);
    B->CreateBr(tc->finallyBlock);
  }

  block = llvm::BasicBlock::Create(*context, "break.new", func);
}

void LLVMVisitor::visit(const ContinueInstr *x) {
  seqassertn(!loops.empty(), "not in a loop");
  B->SetInsertPoint(block);
  auto *loop = !x->getLoop() ? &loops.back() : getLoopData(x->getLoop()->getId());

  if (trycatch.empty() || trycatch.back().sequenceNumber < loop->sequenceNumber) {
    B->CreateBr(loop->continueBlock);
  } else {
    auto *tc = &trycatch.back();
    auto *excStateContinue = B->getInt8(TryCatchData::State::CONTINUE);
    B->CreateStore(excStateContinue, tc->excFlag);
    B->CreateStore(B->getInt64(loop->sequenceNumber), tc->loopSequence);
    B->CreateBr(tc->finallyBlock);
  }

  block = llvm::BasicBlock::Create(*context, "continue.new", func);
}

void LLVMVisitor::visit(const ReturnInstr *x) {
  if (x->getValue()) {
    process(x->getValue());
  }
  B->SetInsertPoint(block);
  if (coro.exit) {
    if (auto *tc = getInnermostTryCatch()) {
      auto *excStateReturn = B->getInt8(TryCatchData::State::RETURN);
      B->CreateStore(excStateReturn, tc->excFlag);
      B->CreateBr(tc->finallyBlock);
    } else {
      B->CreateBr(coro.exit);
    }
  } else {
    if (auto *tc = getInnermostTryCatch()) {
      auto *excStateReturn = B->getInt8(TryCatchData::State::RETURN);
      B->CreateStore(excStateReturn, tc->excFlag);
      if (tc->retStore) {
        seqassertn(value, "no return value storage");
        B->CreateStore(value, tc->retStore);
      }
      B->CreateBr(tc->finallyBlock);
    } else {
      if (x->getValue()) {
        B->CreateRet(value);
      } else {
        B->CreateRetVoid();
      }
    }
  }
  block = llvm::BasicBlock::Create(*context, "return.new", func);
}

void LLVMVisitor::visit(const YieldInstr *x) {
  if (x->isFinal()) {
    if (x->getValue()) {
      seqassertn(coro.promise, "no coroutine promise");
      process(x->getValue());
      B->SetInsertPoint(block);
      B->CreateStore(value, coro.promise);
    }
    B->SetInsertPoint(block);
    if (auto *tc = getInnermostTryCatch()) {
      auto *excStateReturn = B->getInt8(TryCatchData::State::RETURN);
      B->CreateStore(excStateReturn, tc->excFlag);
      B->CreateBr(tc->finallyBlock);
    } else {
      B->CreateBr(coro.exit);
    }
    block = llvm::BasicBlock::Create(*context, "yield.new", func);
  } else {
    if (x->getValue()) {
      process(x->getValue());
      makeYield(value);
    } else {
      makeYield(nullptr);
    }
  }
}

void LLVMVisitor::visit(const ThrowInstr *x) {
  if (DisableExceptions) {
    B->SetInsertPoint(block);
    B->CreateUnreachable();
    block = llvm::BasicBlock::Create(*context, "throw_unreachable.new", func);
    return;
  }

  // note: exception header should be set in the frontend
  auto excAllocFunc = makeExcAllocFunc();
  auto throwFunc = makeThrowFunc();
  llvm::Value *obj = nullptr;
  llvm::Value *typ = nullptr;

  if (x->getValue()) {
    process(x->getValue());
    obj = value;
    typ = B->getInt32(getTypeIdx(x->getValue()->getType()));
  } else {
    seqassertn(!catches.empty(), "empty raise outside of except block");
    obj = catches.back().exception;
    typ = catches.back().typeId;
  }

  B->SetInsertPoint(block);
  llvm::Value *exc = B->CreateCall(excAllocFunc, {typ, obj});
  call(throwFunc, exc);
}

void LLVMVisitor::visit(const FlowInstr *x) {
  process(x->getFlow());
  process(x->getValue());
}

void LLVMVisitor::visit(const dsl::CustomInstr *x) {
  B->SetInsertPoint(block);
  value = x->getBuilder()->buildValue(this);
}

} // namespace ir
} // namespace codon
