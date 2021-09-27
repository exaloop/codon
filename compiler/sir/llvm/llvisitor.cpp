#include "llvisitor.h"

#include <algorithm>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

#include "llvm/CodeGen/CommandFlags.h"

#include "coro/Coroutines.h"
#include "sir/dsl/codegen.h"
#include "util/common.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define SEQ_VERSION_STRING()                                                           \
  ("seqc version " STR(SEQ_VERSION_MAJOR) "." STR(SEQ_VERSION_MINOR) "." STR(          \
      SEQ_VERSION_PATCH))

extern "C" void seq_gc_add_roots(void *start, void *end);
extern "C" void seq_gc_remove_roots(void *start, void *end);
extern "C" int64_t seq_exc_offset();
extern "C" uint64_t seq_exc_class();

static llvm::codegen::RegisterCodeGenFlags CFG;

namespace seq {
namespace ir {
namespace {
std::string getNameForFunction(const Func *x) {
  if (auto *externalFunc = cast<ExternalFunc>(x)) {
    return x->getUnmangledName();
  } else {
    return x->referenceString();
  }
}

std::string getDebugNameForVariable(const Var *x) {
  std::string name = x->getName();
  auto pos = name.find(".");
  if (pos != 0 && pos != std::string::npos) {
    return name.substr(0, pos);
  } else {
    return name;
  }
}

const SrcInfo *getSrcInfo(const Node *x) {
  if (auto *srcInfo = x->getAttribute<SrcInfoAttribute>()) {
    return &srcInfo->info;
  } else {
    static SrcInfo defaultSrcInfo("<internal>", 0, 0, 0);
    return &defaultSrcInfo;
  }
}

llvm::Value *getDummyVoidValue(llvm::LLVMContext &context) {
  return llvm::ConstantTokenNone::get(context);
}

std::unique_ptr<llvm::TargetMachine>
getTargetMachine(llvm::Triple triple, llvm::StringRef cpuStr,
                 llvm::StringRef featuresStr, const llvm::TargetOptions &options) {
  std::string err;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(llvm::codegen::getMArch(), triple, err);

  if (!target)
    return nullptr;

  return std::unique_ptr<llvm::TargetMachine>(target->createTargetMachine(
      triple.getTriple(), cpuStr, featuresStr, options,
      llvm::codegen::getExplicitRelocModel(), llvm::codegen::getExplicitCodeModel(),
      llvm::CodeGenOpt::Aggressive));
}

/**
 * Simple extension of LLVM's SectionMemoryManager which catches data section
 * allocations and registers them with the GC. This allows the GC to know not
 * to collect globals even in JIT mode.
 */
class BoehmGCMemoryManager : public llvm::SectionMemoryManager {
private:
  /// Vector of (start, end) address pairs registered with GC.
  std::vector<std::pair<void *, void *>> roots;

  uint8_t *allocateDataSection(uintptr_t size, unsigned alignment, unsigned sectionID,
                               llvm::StringRef sectionName, bool isReadOnly) override {
    uint8_t *result = SectionMemoryManager::allocateDataSection(
        size, alignment, sectionID, sectionName, isReadOnly);
    void *start = result;
    void *end = result + size;
    seq_gc_add_roots(start, end);
    roots.emplace_back(start, end);
    return result;
  }

public:
  BoehmGCMemoryManager() : SectionMemoryManager(), roots() {}

  ~BoehmGCMemoryManager() override {
    for (const auto &root : roots) {
      seq_gc_remove_roots(root.first, root.second);
    }
  }
};

/**
 * Sometimes coroutine lowering produces hard-to-analyze loops involving
 * function pointer comparisons. This pass puts them into a somewhat
 * easier-to-analyze form.
 */
struct CoroBranchSimplifier : public llvm::LoopPass {
  static char ID;
  CoroBranchSimplifier() : llvm::LoopPass(ID) {}

  static llvm::Value *getNonNullOperand(llvm::Value *op1, llvm::Value *op2) {
    auto *ptr = llvm::dyn_cast<llvm::PointerType>(op1->getType());
    if (!ptr || !ptr->getElementType()->isFunctionTy())
      return nullptr;

    auto *c1 = llvm::dyn_cast<llvm::Constant>(op1);
    auto *c2 = llvm::dyn_cast<llvm::Constant>(op2);
    const bool isNull1 = (c1 && c1->isNullValue());
    const bool isNull2 = (c2 && c2->isNullValue());
    if (!(isNull1 ^ isNull2))
      return nullptr;
    return isNull1 ? op2 : op1;
  }

  bool runOnLoop(llvm::Loop *loop, llvm::LPPassManager &lpm) override {
    if (auto *exit = loop->getExitingBlock()) {
      if (auto *br = llvm::dyn_cast<llvm::BranchInst>(exit->getTerminator())) {
        if (!br->isConditional() || br->getNumSuccessors() != 2 ||
            loop->contains(br->getSuccessor(0)) || !loop->contains(br->getSuccessor(1)))
          return false;

        auto *cond = br->getCondition();
        if (auto *cmp = llvm::dyn_cast<llvm::CmpInst>(cond)) {
          if (cmp->getPredicate() != llvm::CmpInst::Predicate::ICMP_EQ)
            return false;

          if (auto *f = getNonNullOperand(cmp->getOperand(0), cmp->getOperand(1))) {
            if (auto *sel = llvm::dyn_cast<llvm::SelectInst>(f)) {
              if (auto *g =
                      getNonNullOperand(sel->getTrueValue(), sel->getFalseValue())) {
                // If we can deduce that g is not null, we can replace the condition.
                if (auto *phi = llvm::dyn_cast<llvm::PHINode>(g)) {
                  bool ok = true;
                  for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
                    auto *phiBlock = phi->getIncomingBlock(i);
                    auto *phiValue = phi->getIncomingValue(i);

                    if (auto *c = llvm::dyn_cast<llvm::Constant>(phiValue)) {
                      if (c->isNullValue()) {
                        ok = false;
                        break;
                      }
                    } else {
                      // There is no way for the value to be null if the incoming phi
                      // value is predicated on this exit condition, which checks for a
                      // non-null function pointer.

                      if (phiBlock != exit || phiValue != f) {
                        ok = false;
                        break;
                      }
                    }
                  }
                  if (!ok)
                    return false;

                  br->setCondition(sel->getCondition());
                  return true;
                }
              }
            }
          }
        }
      }
    }
    return false;
  }
};

void addCoroutineBranchSimplifier(const llvm::PassManagerBuilder &builder,
                                  llvm::legacy::PassManagerBase &pm) {
  pm.add(new CoroBranchSimplifier());
}

char CoroBranchSimplifier::ID = 0;
llvm::RegisterPass<CoroBranchSimplifier> X("coro-br-simpl",
                                           "Coroutine Branch Simplifier");
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

LLVMVisitor::LLVMVisitor(bool debug, const std::string &flags)
    : util::ConstVisitor(), context(), builder(context), module(), func(nullptr),
      block(nullptr), value(nullptr), vars(), funcs(), coro(), loops(), trycatch(),
      db(debug, flags), machine() {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  auto &registry = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeCore(registry);
  llvm::initializeCoroutines(registry);
  llvm::initializeScalarOpts(registry);
  llvm::initializeObjCARCOpts(registry);
  llvm::initializeVectorization(registry);
  llvm::initializeIPO(registry);
  llvm::initializeAnalysis(registry);
  llvm::initializeTransformUtils(registry);
  llvm::initializeInstCombine(registry);
  llvm::initializeAggressiveInstCombine(registry);
  llvm::initializeInstrumentation(registry);
  llvm::initializeTarget(registry);

  llvm::initializeExpandMemCmpPassPass(registry);
  llvm::initializeScalarizeMaskedMemIntrinLegacyPassPass(registry);
  llvm::initializeCodeGenPreparePass(registry);
  llvm::initializeAtomicExpandPass(registry);
  llvm::initializeRewriteSymbolsLegacyPassPass(registry);
  llvm::initializeWinEHPreparePass(registry);
  llvm::initializeDwarfEHPrepareLegacyPassPass(registry);
  llvm::initializeSafeStackLegacyPassPass(registry);
  llvm::initializeSjLjEHPreparePass(registry);
  llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registry);
  llvm::initializeGlobalMergePass(registry);
  llvm::initializeIndirectBrExpandPassPass(registry);
  llvm::initializeInterleavedLoadCombinePass(registry);
  llvm::initializeInterleavedAccessPass(registry);
  llvm::initializeEntryExitInstrumenterPass(registry);
  llvm::initializePostInlineEntryExitInstrumenterPass(registry);
  llvm::initializeUnreachableBlockElimLegacyPassPass(registry);
  llvm::initializeExpandReductionsPass(registry);
  llvm::initializeWasmEHPreparePass(registry);
  llvm::initializeWriteBitcodePassPass(registry);
  llvm::initializeHardwareLoopsPass(registry);
  llvm::initializeTypePromotionPass(registry);
}

void LLVMVisitor::setDebugInfoForNode(const Node *x) {
  if (x && func) {
    auto *srcInfo = getSrcInfo(x);
    builder.SetCurrentDebugLocation(llvm::DILocation::get(
        context, srcInfo->line, srcInfo->col, func->getSubprogram()));
  } else {
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
  }
}

void LLVMVisitor::process(const Node *x) {
  setDebugInfoForNode(x);
  x->accept(*this);
}

void LLVMVisitor::verify() {
  const bool broken = llvm::verifyModule(*module, &llvm::errs());
  seqassert(!broken, "module broken");
}

void LLVMVisitor::dump(const std::string &filename) { writeToLLFile(filename, false); }

void LLVMVisitor::applyDebugTransformations() {
  if (db.debug) {
    // remove tail calls and fix linkage for stack traces
    for (auto &f : *module) {
      f.setLinkage(llvm::GlobalValue::ExternalLinkage);
      if (f.hasFnAttribute(llvm::Attribute::AttrKind::AlwaysInline)) {
        f.removeFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
      }
      f.addFnAttr(llvm::Attribute::AttrKind::NoInline);
      f.setHasUWTable();
      f.addFnAttr("no-frame-pointer-elim", "true");
      f.addFnAttr("no-frame-pointer-elim-non-leaf");
      f.addFnAttr("no-jump-tables", "false");

      for (auto &block : f.getBasicBlockList()) {
        for (auto &inst : block) {
          if (auto *call = llvm::dyn_cast<llvm::CallInst>(&inst)) {
            call->setTailCall(false);
          }
        }
      }
    }
  } else {
    llvm::StripDebugInfo(*module);
  }
}

void LLVMVisitor::runLLVMOptimizationPasses() {
  applyDebugTransformations();

  llvm::Triple moduleTriple(module->getTargetTriple());
  std::string cpuStr, featuresStr;
  const llvm::TargetOptions options =
      llvm::codegen::InitTargetOptionsFromCodeGenFlags(moduleTriple);
  llvm::TargetLibraryInfoImpl tlii(moduleTriple);

  auto pm = std::make_unique<llvm::legacy::PassManager>();
  auto fpm = std::make_unique<llvm::legacy::FunctionPassManager>(module.get());
  pm->add(new llvm::TargetLibraryInfoWrapperPass(tlii));

  if (moduleTriple.getArch()) {
    cpuStr = llvm::codegen::getCPUStr();
    featuresStr = llvm::codegen::getFeaturesStr();
    machine = getTargetMachine(moduleTriple, cpuStr, featuresStr, options);
  }

  llvm::codegen::setFunctionAttributes(cpuStr, featuresStr, *module);
  pm->add(llvm::createTargetTransformInfoWrapperPass(
      machine ? machine->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));
  fpm->add(llvm::createTargetTransformInfoWrapperPass(
      machine ? machine->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));

  if (machine) {
    auto &ltm = dynamic_cast<llvm::LLVMTargetMachine &>(*machine);
    llvm::Pass *tpc = ltm.createPassConfig(*pm);
    pm->add(tpc);
  }

  unsigned optLevel = 3;
  unsigned sizeLevel = 0;
  llvm::PassManagerBuilder pmb;

  if (!db.debug) {
    pmb.OptLevel = optLevel;
    pmb.SizeLevel = sizeLevel;
    pmb.Inliner = llvm::createFunctionInliningPass(optLevel, sizeLevel, false);
    pmb.DisableUnrollLoops = false;
    pmb.LoopVectorize = true;
    pmb.SLPVectorize = true;
    // pmb.MergeFunctions = true;
  } else {
    pmb.OptLevel = 0;
  }

  if (machine) {
    machine->adjustPassManager(pmb);
  }

  coro::addCoroutinePassesToExtensionPoints(pmb);
  if (!db.debug) {
    pmb.addExtension(llvm::PassManagerBuilder::EP_LateLoopOptimizations,
                     addCoroutineBranchSimplifier);
  }

  pmb.populateModulePassManager(*pm);
  pmb.populateFunctionPassManager(*fpm);

  fpm->doInitialization();
  for (llvm::Function &f : *module) {
    fpm->run(f);
  }
  fpm->doFinalization();
  pm->run(*module);
  applyDebugTransformations();
}

void LLVMVisitor::runLLVMPipeline() {
  using namespace std::chrono;
  auto t = high_resolution_clock::now();
  verify();
  runLLVMOptimizationPasses();
  LOG_TIME("[T] llvm/opt = {:.1f}",
           duration_cast<milliseconds>(high_resolution_clock::now() - t).count() /
               1000.0);
  if (!db.debug) {
    t = high_resolution_clock::now();
    runLLVMOptimizationPasses();
    LOG_TIME("[T] llvm/opt2 = {:.1f}",
             duration_cast<milliseconds>(high_resolution_clock::now() - t).count() /
                 1000.0);
  }
  verify();
}

void LLVMVisitor::writeToObjectFile(const std::string &filename) {
  runLLVMPipeline();

  std::error_code err;
  auto out =
      std::make_unique<llvm::ToolOutputFile>(filename, err, llvm::sys::fs::OF_None);
  if (err)
    compilationError(err.message());
  llvm::raw_pwrite_stream *os = &out->os();

  auto &llvmtm = static_cast<llvm::LLVMTargetMachine &>(*machine);
  auto *mmiwp = new llvm::MachineModuleInfoWrapperPass(&llvmtm);
  llvm::legacy::PassManager pm;

  llvm::TargetLibraryInfoImpl tlii(llvm::Triple(module->getTargetTriple()));
  pm.add(new llvm::TargetLibraryInfoWrapperPass(tlii));
  seqassert(!machine->addPassesToEmitFile(pm, *os, nullptr, llvm::CGFT_ObjectFile,
                                          /*DisableVerify=*/true, mmiwp),
            "could not add passes");
  const_cast<llvm::TargetLoweringObjectFile *>(llvmtm.getObjFileLowering())
      ->Initialize(mmiwp->getMMI().getContext(), *machine);
  pm.run(*module);
  out->keep();
}

void LLVMVisitor::writeToBitcodeFile(const std::string &filename) {
  runLLVMPipeline();
  std::error_code err;
  llvm::raw_fd_ostream stream(filename, err, llvm::sys::fs::F_None);
  llvm::WriteBitcodeToFile(*module, stream);
  if (err) {
    compilationError(err.message());
  }
}

void LLVMVisitor::writeToLLFile(const std::string &filename, bool optimize) {
  if (optimize)
    runLLVMPipeline();
  auto fo = fopen(filename.c_str(), "w");
  llvm::raw_fd_ostream fout(fileno(fo), true);
  fout << *module;
  fout.close();
}

namespace {
void executeCommand(const std::vector<std::string> &args) {
  std::vector<const char *> cArgs;
  for (auto &arg : args) {
    cArgs.push_back(arg.c_str());
  }
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

void addEnvVarPathsToLinkerArgs(std::vector<std::string> &args,
                                const std::string &var) {
  if (const char *path = getenv(var.c_str())) {
    llvm::StringRef pathStr(path);
    llvm::SmallVector<llvm::StringRef, 16> split;
    pathStr.split(split, ":");

    for (const auto &subPath : split) {
      args.push_back(("-L" + subPath).str());
    }
  }
}
} // namespace

void LLVMVisitor::writeToExecutable(const std::string &filename,
                                    const std::vector<std::string> &libs) {
  const std::string objFile = filename + ".o";
  writeToObjectFile(objFile);

  std::vector<std::string> command = {"clang"};
  addEnvVarPathsToLinkerArgs(command, "LIBRARY_PATH");
  addEnvVarPathsToLinkerArgs(command, "LD_LIBRARY_PATH");
  addEnvVarPathsToLinkerArgs(command, "DYLD_LIBRARY_PATH");
  addEnvVarPathsToLinkerArgs(command, "SEQ_LIBRARY_PATH");
  for (const auto &lib : libs) {
    command.push_back("-l" + lib);
  }
  std::vector<std::string> extraArgs = {"-lseqrt", "-lomp", "-lpthread", "-ldl",
                                        "-lz",     "-lm",   "-lc",       "-o",
                                        filename,  objFile};
  for (const auto &arg : extraArgs) {
    command.push_back(arg);
  }

  executeCommand(command);

#if __APPLE__
  if (db.debug) {
    executeCommand({"dsymutil", filename});
  }
#endif
}

void LLVMVisitor::compile(const std::string &filename,
                          const std::vector<std::string> &libs) {
  llvm::StringRef f(filename);
  if (f.endswith(".ll")) {
    writeToLLFile(filename);
  } else if (f.endswith(".bc")) {
    writeToBitcodeFile(filename);
  } else if (f.endswith(".o") || f.endswith(".obj")) {
    writeToObjectFile(filename);
  } else {
    writeToExecutable(filename, libs);
  }
}

void LLVMVisitor::run(const std::vector<std::string> &args,
                      const std::vector<std::string> &libs, const char *const *envp) {
  runLLVMPipeline();

  std::vector<std::string> functionNames;
  if (db.debug) {
    for (auto &f : *module) {
      functionNames.push_back(f.getName().str());
    }
  }

  llvm::Function *main = module->getFunction("main");
  llvm::EngineBuilder EB(std::move(module));
  EB.setMCJITMemoryManager(std::make_unique<BoehmGCMemoryManager>());
  llvm::ExecutionEngine *eng = EB.create();

  std::string err;
  for (auto &lib : libs) {
    if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(lib.c_str(), &err)) {
      compilationError(err);
    }
  }

  eng->runFunctionAsMain(main, args, envp);
  delete eng;
}

llvm::FunctionCallee LLVMVisitor::makeAllocFunc(bool atomic) {
  auto f = module->getOrInsertFunction(atomic ? "seq_alloc_atomic" : "seq_alloc",
                                       builder.getInt8PtrTy(), builder.getInt64Ty());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotThrow();
  g->setReturnDoesNotAlias();
  g->setOnlyAccessesInaccessibleMemory();
  return f;
}

llvm::FunctionCallee LLVMVisitor::makePersonalityFunc() {
  return module->getOrInsertFunction("seq_personality", builder.getInt32Ty(),
                                     builder.getInt32Ty(), builder.getInt32Ty(),
                                     builder.getInt64Ty(), builder.getInt8PtrTy(),
                                     builder.getInt8PtrTy());
}

llvm::FunctionCallee LLVMVisitor::makeExcAllocFunc() {
  auto f = module->getOrInsertFunction("seq_alloc_exc", builder.getInt8PtrTy(),
                                       builder.getInt32Ty(), builder.getInt8PtrTy());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotThrow();
  return f;
}

llvm::FunctionCallee LLVMVisitor::makeThrowFunc() {
  auto f = module->getOrInsertFunction("seq_throw", builder.getVoidTy(),
                                       builder.getInt8PtrTy());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotReturn();
  return f;
}

llvm::FunctionCallee LLVMVisitor::makeTerminateFunc() {
  auto f = module->getOrInsertFunction("seq_terminate", builder.getVoidTy(),
                                       builder.getInt8PtrTy());
  auto *g = cast<llvm::Function>(f.getCallee());
  g->setDoesNotReturn();
  return f;
}

llvm::StructType *LLVMVisitor::getTypeInfoType() {
  return llvm::StructType::get(builder.getInt32Ty());
}

llvm::StructType *LLVMVisitor::getPadType() {
  return llvm::StructType::get(builder.getInt8PtrTy(), builder.getInt32Ty());
}

llvm::StructType *LLVMVisitor::getExceptionType() {
  return llvm::StructType::get(getTypeInfoType(), builder.getInt8PtrTy());
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
  const std::string typeVarName = "seq.typeidx." + (name.empty() ? "<all>" : name);
  llvm::GlobalVariable *tidx = module->getGlobalVariable(typeVarName);
  int idx = typeIdxLookup(name);
  if (!tidx) {
    tidx = new llvm::GlobalVariable(
        *module, typeInfoType, /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantStruct::get(typeInfoType, builder.getInt32(idx)), typeVarName);
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
  builder.SetInsertPoint(block);
  if (trycatch.empty()) {
    return builder.CreateCall(callee, args);
  } else {
    auto *normalBlock = llvm::BasicBlock::Create(context, "invoke.normal", func);
    auto *unwindBlock = trycatch.back().exceptionBlock;
    auto *result = builder.CreateInvoke(callee, normalBlock, unwindBlock, args);
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
  seqassert(!loops.empty(), "no loops present");
  loops.pop_back();
}

void LLVMVisitor::enterTryCatch(TryCatchData data) {
  trycatch.push_back(std::move(data));
  trycatch.back().sequenceNumber = nextSequenceNumber++;
}

void LLVMVisitor::exitTryCatch() {
  seqassert(!trycatch.empty(), "no try catches present");
  trycatch.pop_back();
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
 * General values, module, functions, vars
 */

void LLVMVisitor::visit(const Module *x) {
  module = std::make_unique<llvm::Module>("seq", context);
  module->setTargetTriple(
      llvm::EngineBuilder().selectTarget()->getTargetTriple().str());
  module->setDataLayout(llvm::EngineBuilder().selectTarget()->createDataLayout());
  auto *srcInfo = getSrcInfo(x->getMainFunc());
  module->setSourceFileName(srcInfo->file);

  // debug info setup
  db.builder = std::make_unique<llvm::DIBuilder>(*module);
  llvm::DIFile *file = db.getFile(srcInfo->file);
  db.unit =
      db.builder->createCompileUnit(llvm::dwarf::DW_LANG_C, file, SEQ_VERSION_STRING(),
                                    !db.debug, db.flags, /*RV=*/0);
  module->addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                        llvm::DEBUG_METADATA_VERSION);
  // darwin only supports dwarf2
  if (llvm::Triple(module->getTargetTriple()).isOSDarwin()) {
    module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);
  }

  // args variable
  const Var *argVar = x->getArgVar();
  llvm::Type *argVarType = getLLVMType(argVar->getType());
  auto *argStorage = new llvm::GlobalVariable(
      *module, argVarType, /*isConstant=*/false, llvm::GlobalValue::PrivateLinkage,
      llvm::Constant::getNullValue(argVarType), argVar->getName());
  vars.insert(argVar, argStorage);

  // set up global variables and initialize functions
  for (auto *var : *x) {
    if (!var->isGlobal())
      continue;

    if (auto *f = cast<Func>(var)) {
      makeLLVMFunction(f);
      funcs.insert(f, func);
    } else {
      llvm::Type *llvmType = getLLVMType(var->getType());
      if (llvmType->isVoidTy()) {
        vars.insert(var, getDummyVoidValue(context));
      } else {
        auto *storage = new llvm::GlobalVariable(
            *module, llvmType, /*isConstant=*/false,
            llvm::GlobalVariable::PrivateLinkage,
            llvm::Constant::getNullValue(llvmType), var->getName());
        vars.insert(var, storage);

        // debug info
        auto *srcInfo = getSrcInfo(var);
        llvm::DIFile *file = db.getFile(srcInfo->file);
        llvm::DIScope *scope = db.unit;
        llvm::DIGlobalVariableExpression *debugVar =
            db.builder->createGlobalVariableExpression(
                scope, getDebugNameForVariable(var), var->getName(), file,
                srcInfo->line, getDIType(var->getType()),
                /*IsLocalToUnit=*/true);
        storage->addDebugInfo(debugVar);
      }
    }
  }

  // process functions
  for (auto *var : *x) {
    if (auto *f = cast<Func>(var)) {
      process(f);
    }
  }

  const Func *main = x->getMainFunc();
  makeLLVMFunction(main);
  llvm::FunctionCallee realMain = func;
  process(main);
  setDebugInfoForNode(nullptr);

  // build canonical main function
  auto *strType =
      llvm::StructType::get(context, {builder.getInt64Ty(), builder.getInt8PtrTy()});
  auto *arrType =
      llvm::StructType::get(context, {builder.getInt64Ty(), strType->getPointerTo()});

  auto *initFunc = llvm::cast<llvm::Function>(
      module->getOrInsertFunction("seq_init", builder.getVoidTy(), builder.getInt32Ty())
          .getCallee());
  auto *strlenFunc = llvm::cast<llvm::Function>(
      module
          ->getOrInsertFunction("strlen", builder.getInt64Ty(), builder.getInt8PtrTy())
          .getCallee());

  auto *canonicalMainFunc = llvm::cast<llvm::Function>(
      module
          ->getOrInsertFunction("main", builder.getInt32Ty(), builder.getInt32Ty(),
                                builder.getInt8PtrTy()->getPointerTo())
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
  auto *entryBlock = llvm::BasicBlock::Create(context, "entry", canonicalMainFunc);
  auto *loopBlock = llvm::BasicBlock::Create(context, "loop", canonicalMainFunc);
  auto *bodyBlock = llvm::BasicBlock::Create(context, "body", canonicalMainFunc);
  auto *exitBlock = llvm::BasicBlock::Create(context, "exit", canonicalMainFunc);

  builder.SetInsertPoint(entryBlock);
  auto allocFunc = makeAllocFunc(/*atomic=*/false);
  llvm::Value *len = builder.CreateZExt(argc, builder.getInt64Ty());
  llvm::Value *elemSize =
      builder.getInt64(module->getDataLayout().getTypeAllocSize(strType));
  llvm::Value *allocSize = builder.CreateMul(len, elemSize);
  llvm::Value *ptr = builder.CreateCall(allocFunc, allocSize);
  ptr = builder.CreateBitCast(ptr, strType->getPointerTo());
  llvm::Value *arr = llvm::UndefValue::get(arrType);
  arr = builder.CreateInsertValue(arr, len, 0);
  arr = builder.CreateInsertValue(arr, ptr, 1);
  builder.CreateBr(loopBlock);

  builder.SetInsertPoint(loopBlock);
  llvm::PHINode *control = builder.CreatePHI(builder.getInt32Ty(), 2, "i");
  llvm::Value *next = builder.CreateAdd(control, builder.getInt32(1), "next");
  llvm::Value *cond = builder.CreateICmpSLT(control, argc);
  control->addIncoming(builder.getInt32(0), entryBlock);
  control->addIncoming(next, bodyBlock);
  builder.CreateCondBr(cond, bodyBlock, exitBlock);

  builder.SetInsertPoint(bodyBlock);
  llvm::Value *arg = builder.CreateLoad(builder.CreateGEP(argv, control));
  llvm::Value *argLen = builder.CreateZExtOrTrunc(builder.CreateCall(strlenFunc, arg),
                                                  builder.getInt64Ty());
  llvm::Value *str = llvm::UndefValue::get(strType);
  str = builder.CreateInsertValue(str, argLen, 0);
  str = builder.CreateInsertValue(str, arg, 1);
  builder.CreateStore(str, builder.CreateGEP(ptr, control));
  builder.CreateBr(loopBlock);

  builder.SetInsertPoint(exitBlock);
  builder.CreateStore(arr, argStorage);
  builder.CreateCall(initFunc, builder.getInt32(db.debug ? 1 : 0));

  // Put the entire program in a new function
  {
    auto *proxyMainTy = llvm::FunctionType::get(builder.getVoidTy(), {}, false);
    auto *proxyMain = llvm::cast<llvm::Function>(
        module->getOrInsertFunction("seq.proxy_main", proxyMainTy).getCallee());
    proxyMain->setLinkage(llvm::GlobalValue::PrivateLinkage);
    proxyMain->setPersonalityFn(
        llvm::cast<llvm::Constant>(makePersonalityFunc().getCallee()));
    auto *proxyBlockEntry = llvm::BasicBlock::Create(context, "entry", proxyMain);
    auto *proxyBlockMain = llvm::BasicBlock::Create(context, "main", proxyMain);
    auto *proxyBlockExit = llvm::BasicBlock::Create(context, "exit", proxyMain);
    builder.SetInsertPoint(proxyBlockEntry);

    llvm::Value *shouldExit = builder.getFalse();
    builder.CreateCondBr(shouldExit, proxyBlockExit, proxyBlockMain);

    builder.SetInsertPoint(proxyBlockExit);
    builder.CreateRetVoid();

    // invoke real main
    auto *normal = llvm::BasicBlock::Create(context, "normal", proxyMain);
    auto *unwind = llvm::BasicBlock::Create(context, "unwind", proxyMain);
    builder.SetInsertPoint(proxyBlockMain);
    builder.CreateInvoke(realMain, normal, unwind);

    builder.SetInsertPoint(unwind);
    llvm::LandingPadInst *caughtResult = builder.CreateLandingPad(getPadType(), 1);
    caughtResult->setCleanup(true);
    caughtResult->addClause(getTypeIdxVar(nullptr));
    llvm::Value *unwindException = builder.CreateExtractValue(caughtResult, 0);
    builder.CreateCall(makeTerminateFunc(), unwindException);
    builder.CreateUnreachable();

    builder.SetInsertPoint(normal);
    builder.CreateRetVoid();

    // actually make the call
    builder.SetInsertPoint(exitBlock);
    builder.CreateCall(proxyMain);
  }

  builder.SetInsertPoint(exitBlock);
  builder.CreateRet(builder.getInt32(0));
  db.builder->finalize();
}

void LLVMVisitor::makeLLVMFunction(const Func *x) {
  auto *srcInfo = getSrcInfo(x);
  llvm::DIFile *file = db.getFile(srcInfo->file);
  auto *derivedType = llvm::cast<llvm::DIDerivedType>(getDIType(x->getType()));
  auto *subroutineType =
      llvm::cast<llvm::DISubroutineType>(derivedType->getRawBaseType());
  llvm::DISubprogram *subprogram = db.builder->createFunction(
      file, x->getUnmangledName(), getNameForFunction(x), file, srcInfo->line,
      subroutineType, /*ScopeLine=*/0, llvm::DINode::FlagZero,
      llvm::DISubprogram::toSPFlags(/*IsLocalToUnit=*/true,
                                    /*IsDefinition=*/true, /*IsOptimized=*/!db.debug));

  // process LLVM functions in full immediately
  if (auto *llvmFunc = cast<LLVMFunc>(x)) {
    process(llvmFunc);
    func = module->getFunction(getNameForFunction(x));
    func->setSubprogram(subprogram);
    setDebugInfoForNode(nullptr);
    return;
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
  func = llvm::cast<llvm::Function>(
      module->getOrInsertFunction(functionName, llvmFuncType).getCallee());
  if (!cast<ExternalFunc>(x)) {
    func->setSubprogram(subprogram);
  }
}

void LLVMVisitor::makeYield(llvm::Value *value, bool finalYield) {
  builder.SetInsertPoint(block);
  if (value) {
    seqassert(coro.promise, "promise is null");
    builder.CreateStore(value, coro.promise);
  }
  llvm::FunctionCallee coroSuspend =
      llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_suspend);
  llvm::Value *suspendResult =
      builder.CreateCall(coroSuspend, {llvm::ConstantTokenNone::get(context),
                                       builder.getInt1(finalYield)});

  block = llvm::BasicBlock::Create(context, "yield.new", func);

  llvm::SwitchInst *inst = builder.CreateSwitch(suspendResult, coro.suspend, 2);
  inst->addCase(builder.getInt8(0), block);
  inst->addCase(builder.getInt8(1), coro.cleanup);
}

void LLVMVisitor::visit(const ExternalFunc *x) {
  func = module->getFunction(getNameForFunction(x)); // inserted during module visit
  coro = {};
  seqassert(func, "{} not inserted", *x);
  func->setDoesNotThrow();
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
  func = module->getFunction(getNameForFunction(x)); // inserted during module visit
  coro = {};
  seqassert(func, "{} not inserted", *x);
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
  block = llvm::BasicBlock::Create(context, "entry", func);
  builder.SetInsertPoint(block);
  llvm::Value *result = nullptr;

  if (internalFuncMatches<PointerType, IntType>("__new__", x)) {
    auto *pointerType = cast<PointerType>(parentType);
    Type *baseType = pointerType->getBase();
    llvm::Type *llvmBaseType = getLLVMType(baseType);
    auto allocFunc = makeAllocFunc(baseType->isAtomic());
    llvm::Value *elemSize =
        builder.getInt64(module->getDataLayout().getTypeAllocSize(llvmBaseType));
    llvm::Value *allocSize = builder.CreateMul(elemSize, args[0]);
    result = builder.CreateCall(allocFunc, allocSize);
    result = builder.CreateBitCast(result, llvmBaseType->getPointerTo());
  }

  else if (internalFuncMatches<IntType, IntNType>("__new__", x)) {
    auto *intNType = cast<IntNType>(argTypes[0]);
    if (intNType->isSigned()) {
      result = builder.CreateSExtOrTrunc(args[0], builder.getInt64Ty());
    } else {
      result = builder.CreateZExtOrTrunc(args[0], builder.getInt64Ty());
    }
  }

  else if (internalFuncMatches<IntNType, IntType>("__new__", x)) {
    auto *intNType = cast<IntNType>(parentType);
    if (intNType->isSigned()) {
      result = builder.CreateSExtOrTrunc(args[0], getLLVMType(intNType));
    } else {
      result = builder.CreateZExtOrTrunc(args[0], getLLVMType(intNType));
    }
  }

  else if (internalFuncMatches<RefType>("__new__", x)) {
    auto *refType = cast<RefType>(parentType);
    auto allocFunc = makeAllocFunc(refType->getContents()->isAtomic());
    llvm::Value *size = builder.getInt64(
        module->getDataLayout().getTypeAllocSize(getLLVMType(refType->getContents())));
    result = builder.CreateCall(allocFunc, size);
  }

  else if (internalFuncMatches<GeneratorType, GeneratorType>("__promise__", x)) {
    auto *generatorType = cast<GeneratorType>(parentType);
    llvm::Type *baseType = getLLVMType(generatorType->getBase());
    if (baseType->isVoidTy()) {
      result = llvm::ConstantPointerNull::get(builder.getVoidTy()->getPointerTo());
    } else {
      llvm::FunctionCallee coroPromise =
          llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_promise);
      llvm::Value *aln =
          builder.getInt32(module->getDataLayout().getPrefTypeAlignment(baseType));
      llvm::Value *from = builder.getFalse();
      llvm::Value *ptr = builder.CreateCall(coroPromise, {args[0], aln, from});
      result = builder.CreateBitCast(ptr, baseType->getPointerTo());
    }
  }

  else if (internalFuncMatchesIgnoreArgs<RecordType>("__new__", x)) {
    auto *recordType = cast<RecordType>(parentType);
    seqassert(args.size() == std::distance(recordType->begin(), recordType->end()),
              "args size does not match");
    result = llvm::UndefValue::get(getLLVMType(recordType));
    for (auto i = 0; i < args.size(); i++) {
      result = builder.CreateInsertValue(result, args[i], i);
    }
  }

  seqassert(result, "internal function {} not found", *x);
  builder.CreateRet(result);
}

std::string LLVMVisitor::buildLLVMCodeString(const LLVMFunc *x) {
  auto *funcType = cast<types::FuncType>(x->getType());
  seqassert(funcType, "{} is not a function type", *x->getType());
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
  buf << x->getLLVMDeclarations() << "\n"
      << signature << " {{\n"
      << x->getLLVMBody() << "\n}}";
  return buf.str();
}

void LLVMVisitor::visit(const LLVMFunc *x) {
  func = module->getFunction(getNameForFunction(x));
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
    } else if (it->isType()) {
      llvm::Type *llvmType = getLLVMType(it->getTypeValue());
      std::string bufStr;
      llvm::raw_string_ostream buf(bufStr);
      llvmType->print(buf);
      store.push_back(buf.str());
    } else {
      seqassert(0, "formatting failed");
    }
  }
  code = fmt::vformat(code, store);

  llvm::SMDiagnostic err;
  std::unique_ptr<llvm::MemoryBuffer> buf = llvm::MemoryBuffer::getMemBuffer(code);
  seqassert(buf, "could not create buffer");
  std::unique_ptr<llvm::Module> sub =
      llvm::parseIR(buf->getMemBufferRef(), err, context);
  if (!sub) {
    std::string bufStr;
    llvm::raw_string_ostream buf(bufStr);
    err.print("LLVM", buf);
    compilationError(buf.str());
  }
  sub->setDataLayout(module->getDataLayout());

  llvm::Linker L(*module);
  const bool fail = L.linkInModule(std::move(sub));
  seqassert(!fail, "linking failed");
  func = module->getFunction(getNameForFunction(x));
  seqassert(func, "function not linked in");
  func->setLinkage(llvm::GlobalValue::PrivateLinkage);
  func->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
}

void LLVMVisitor::visit(const BodiedFunc *x) {
  func = module->getFunction(getNameForFunction(x)); // inserted during module visit
  coro = {};
  seqassert(func, "{} not inserted", *x);
  setDebugInfoForNode(x);

  auto *fnAttributes = x->getAttribute<KeyValueAttribute>();
  if (fnAttributes && fnAttributes->has("std.internal.attributes.export")) {
    func->setLinkage(llvm::GlobalValue::ExternalLinkage);
  } else {
    func->setLinkage(llvm::GlobalValue::PrivateLinkage);
  }
  if (fnAttributes && fnAttributes->has("std.internal.attributes.inline")) {
    func->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
  }
  if (fnAttributes && fnAttributes->has("std.internal.attributes.noinline")) {
    func->addFnAttr(llvm::Attribute::AttrKind::NoInline);
  }
  func->setPersonalityFn(llvm::cast<llvm::Constant>(makePersonalityFunc().getCallee()));

  auto *funcType = cast<types::FuncType>(x->getType());
  seqassert(funcType, "{} is not a function type", *x->getType());
  auto *returnType = funcType->getReturnType();
  auto *entryBlock = llvm::BasicBlock::Create(context, "entry", func);
  builder.SetInsertPoint(entryBlock);
  builder.SetCurrentDebugLocation(llvm::DebugLoc());

  // set up arguments and other symbols
  seqassert(std::distance(func->arg_begin(), func->arg_end()) ==
                std::distance(x->arg_begin(), x->arg_end()),
            "argument length does not match");
  unsigned argIdx = 1;
  auto argIter = func->arg_begin();
  for (auto varIter = x->arg_begin(); varIter != x->arg_end(); ++varIter) {
    const Var *var = *varIter;
    llvm::Value *storage = builder.CreateAlloca(getLLVMType(var->getType()));
    builder.CreateStore(argIter, storage);
    vars.insert(var, storage);

    // debug info
    auto *srcInfo = getSrcInfo(var);
    llvm::DIFile *file = db.getFile(srcInfo->file);
    llvm::DISubprogram *scope = func->getSubprogram();
    llvm::DILocalVariable *debugVar = db.builder->createParameterVariable(
        scope, getDebugNameForVariable(var), argIdx, file, srcInfo->line,
        getDIType(var->getType()), db.debug);
    db.builder->insertDeclare(
        storage, debugVar, db.builder->createExpression(),
        llvm::DILocation::get(context, srcInfo->line, srcInfo->col, scope), entryBlock);

    ++argIter;
    ++argIdx;
  }

  for (auto *var : *x) {
    llvm::Type *llvmType = getLLVMType(var->getType());
    if (llvmType->isVoidTy()) {
      vars.insert(var, getDummyVoidValue(context));
    } else {
      llvm::Value *storage = builder.CreateAlloca(llvmType);
      vars.insert(var, storage);

      // debug info
      auto *srcInfo = getSrcInfo(var);
      llvm::DIFile *file = db.getFile(srcInfo->file);
      llvm::DISubprogram *scope = func->getSubprogram();
      llvm::DILocalVariable *debugVar = db.builder->createAutoVariable(
          scope, getDebugNameForVariable(var), file, srcInfo->line,
          getDIType(var->getType()), db.debug);
      db.builder->insertDeclare(
          storage, debugVar, db.builder->createExpression(),
          llvm::DILocation::get(context, srcInfo->line, srcInfo->col, scope),
          entryBlock);
    }
  }

  auto *startBlock = llvm::BasicBlock::Create(context, "start", func);

  if (x->isGenerator()) {
    auto *generatorType = cast<types::GeneratorType>(returnType);
    seqassert(generatorType, "{} is not a generator type", *returnType);

    llvm::FunctionCallee coroId =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_id);
    llvm::FunctionCallee coroBegin =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_begin);
    llvm::FunctionCallee coroSize = llvm::Intrinsic::getDeclaration(
        module.get(), llvm::Intrinsic::coro_size, {builder.getInt64Ty()});
    llvm::FunctionCallee coroEnd =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_end);
    llvm::FunctionCallee coroAlloc =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_alloc);
    llvm::FunctionCallee coroFree =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_free);

    coro.cleanup = llvm::BasicBlock::Create(context, "coro.cleanup", func);
    coro.suspend = llvm::BasicBlock::Create(context, "coro.suspend", func);
    coro.exit = llvm::BasicBlock::Create(context, "coro.exit", func);
    auto *allocBlock = llvm::BasicBlock::Create(context, "coro.alloc", func);
    auto *freeBlock = llvm::BasicBlock::Create(context, "coro.free", func);

    // coro ID and promise
    llvm::Value *id = nullptr;
    llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getInt8PtrTy());
    if (!cast<types::VoidType>(generatorType->getBase())) {
      coro.promise = builder.CreateAlloca(getLLVMType(generatorType->getBase()));
      coro.promise->setName("coro.promise");
      llvm::Value *promiseRaw =
          builder.CreateBitCast(coro.promise, builder.getInt8PtrTy());
      id = builder.CreateCall(coroId,
                              {builder.getInt32(0), promiseRaw, nullPtr, nullPtr});
    } else {
      id = builder.CreateCall(coroId, {builder.getInt32(0), nullPtr, nullPtr, nullPtr});
    }
    id->setName("coro.id");
    llvm::Value *needAlloc = builder.CreateCall(coroAlloc, id);
    builder.CreateCondBr(needAlloc, allocBlock, startBlock);

    // coro alloc
    builder.SetInsertPoint(allocBlock);
    llvm::Value *size = builder.CreateCall(coroSize);
    auto allocFunc = makeAllocFunc(/*atomic=*/false);
    llvm::Value *alloc = builder.CreateCall(allocFunc, size);
    builder.CreateBr(startBlock);

    // coro start
    builder.SetInsertPoint(startBlock);
    llvm::PHINode *phi = builder.CreatePHI(builder.getInt8PtrTy(), 2);
    phi->addIncoming(nullPtr, entryBlock);
    phi->addIncoming(alloc, allocBlock);
    coro.handle = builder.CreateCall(coroBegin, {id, phi});
    coro.handle->setName("coro.handle");

    // coro cleanup
    builder.SetInsertPoint(coro.cleanup);
    llvm::Value *mem = builder.CreateCall(coroFree, {id, coro.handle});
    llvm::Value *needFree = builder.CreateIsNotNull(mem);
    builder.CreateCondBr(needFree, freeBlock, coro.suspend);

    // coro free
    builder.SetInsertPoint(freeBlock); // no-op: GC will free automatically
    builder.CreateBr(coro.suspend);

    // coro suspend
    builder.SetInsertPoint(coro.suspend);
    builder.CreateCall(coroEnd, {coro.handle, builder.getFalse()});
    builder.CreateRet(coro.handle);

    // coro exit
    block = coro.exit;
    makeYield(nullptr, /*finalYield=*/true);
    builder.SetInsertPoint(block);
    builder.CreateUnreachable();

    // initial yield
    block = startBlock;
    makeYield(); // coroutine will be initially suspended
  } else {
    builder.CreateBr(startBlock);
    block = startBlock;
  }

  process(x->getBody());
  builder.SetInsertPoint(block);

  if (x->isGenerator()) {
    builder.CreateBr(coro.exit);
  } else {
    if (cast<types::VoidType>(returnType)) {
      builder.CreateRetVoid();
    } else {
      builder.CreateRet(llvm::Constant::getNullValue(getLLVMType(returnType)));
    }
  }
}

void LLVMVisitor::visit(const Var *x) { seqassert(0, "cannot visit var"); }

void LLVMVisitor::visit(const VarValue *x) {
  if (auto *f = cast<Func>(x->getVar())) {
    value = funcs[f];
    seqassert(value, "{} value not found", *x);
  } else {
    llvm::Value *varPtr = vars[x->getVar()];
    seqassert(varPtr, "{} value not found", *x);
    builder.SetInsertPoint(block);
    value = builder.CreateLoad(varPtr);
  }
}

void LLVMVisitor::visit(const PointerValue *x) {
  llvm::Value *var = vars[x->getVar()];
  seqassert(var, "{} variable not found", *x);
  value = var; // note: we don't load the pointer
}

/*
 * Types
 */

llvm::Type *LLVMVisitor::getLLVMType(types::Type *t) {
  if (auto *x = cast<types::IntType>(t)) {
    return builder.getInt64Ty();
  }

  if (auto *x = cast<types::FloatType>(t)) {
    return builder.getDoubleTy();
  }

  if (auto *x = cast<types::BoolType>(t)) {
    return builder.getInt8Ty();
  }

  if (auto *x = cast<types::ByteType>(t)) {
    return builder.getInt8Ty();
  }

  if (auto *x = cast<types::VoidType>(t)) {
    return builder.getVoidTy();
  }

  if (auto *x = cast<types::RecordType>(t)) {
    std::vector<llvm::Type *> body;
    for (const auto &field : *x) {
      body.push_back(getLLVMType(field.getType()));
    }
    return llvm::StructType::get(context, body);
  }

  if (auto *x = cast<types::RefType>(t)) {
    return builder.getInt8PtrTy();
  }

  if (auto *x = cast<types::FuncType>(t)) {
    llvm::Type *returnType = getLLVMType(x->getReturnType());
    std::vector<llvm::Type *> argTypes;
    for (auto *argType : *x) {
      argTypes.push_back(getLLVMType(argType));
    }
    return llvm::FunctionType::get(returnType, argTypes, x->isVariadic())
        ->getPointerTo();
  }

  if (auto *x = cast<types::OptionalType>(t)) {
    if (cast<types::RefType>(x->getBase())) {
      return getLLVMType(x->getBase());
    } else {
      return llvm::StructType::get(builder.getInt1Ty(), getLLVMType(x->getBase()));
    }
  }

  if (auto *x = cast<types::PointerType>(t)) {
    return getLLVMType(x->getBase())->getPointerTo();
  }

  if (auto *x = cast<types::GeneratorType>(t)) {
    return builder.getInt8PtrTy();
  }

  if (auto *x = cast<types::IntNType>(t)) {
    return builder.getIntNTy(x->getLen());
  }

  if (auto *x = cast<dsl::types::CustomType>(t)) {
    return x->getBuilder()->buildType(this);
  }

  seqassert(0, "unknown type");
  return nullptr;
}

llvm::DIType *LLVMVisitor::getDITypeHelper(
    types::Type *t, std::unordered_map<std::string, llvm::DICompositeType *> &cache) {
  llvm::Type *type = getLLVMType(t);
  auto &layout = module->getDataLayout();

  if (auto *x = cast<types::IntType>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type), llvm::dwarf::DW_ATE_signed);
  }

  if (auto *x = cast<types::FloatType>(t)) {
    return db.builder->createBasicType(
        x->getName(), layout.getTypeAllocSizeInBits(type), llvm::dwarf::DW_ATE_float);
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
    return db.builder->createReferenceType(llvm::dwarf::DW_TAG_reference_type,
                                           getDITypeHelper(x->getContents(), cache));
  }

  if (auto *x = cast<types::FuncType>(t)) {
    std::vector<llvm::Metadata *> argTypes = {
        getDITypeHelper(x->getReturnType(), cache)};
    for (auto *argType : *x) {
      argTypes.push_back(getDITypeHelper(argType, cache));
    }
    return db.builder->createPointerType(
        db.builder->createSubroutineType(llvm::MDTuple::get(context, argTypes)),
        layout.getTypeAllocSizeInBits(type));
  }

  if (auto *x = cast<types::OptionalType>(t)) {
    if (cast<types::RefType>(x->getBase())) {
      return getDITypeHelper(x->getBase(), cache);
    } else {
      auto *baseType = getLLVMType(x->getBase());
      auto *structType = llvm::StructType::get(builder.getInt1Ty(), baseType);
      auto *structLayout = layout.getStructLayout(structType);
      auto *srcInfo = getSrcInfo(x);
      auto i1SizeInBits = layout.getTypeAllocSizeInBits(builder.getInt1Ty());
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

  if (auto *x = cast<dsl::types::CustomType>(t)) {
    return x->getBuilder()->buildDebugType(this);
  }

  seqassert(0, "unknown type");
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
  builder.SetInsertPoint(block);
  value = builder.getInt64(x->getVal());
}

void LLVMVisitor::visit(const FloatConst *x) {
  builder.SetInsertPoint(block);
  value = llvm::ConstantFP::get(builder.getDoubleTy(), x->getVal());
}

void LLVMVisitor::visit(const BoolConst *x) {
  builder.SetInsertPoint(block);
  value = builder.getInt8(x->getVal() ? 1 : 0);
}

void LLVMVisitor::visit(const StringConst *x) {
  builder.SetInsertPoint(block);
  std::string s = x->getVal();
  auto *strVar = new llvm::GlobalVariable(
      *module, llvm::ArrayType::get(builder.getInt8Ty(), s.length() + 1),
      /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage,
      llvm::ConstantDataArray::getString(context, s), "str_literal");
  auto *strType = llvm::StructType::get(builder.getInt64Ty(), builder.getInt8PtrTy());
  llvm::Value *ptr = builder.CreateBitCast(strVar, builder.getInt8PtrTy());
  llvm::Value *len = builder.getInt64(s.length());
  llvm::Value *str = llvm::UndefValue::get(strType);
  str = builder.CreateInsertValue(str, len, 0);
  str = builder.CreateInsertValue(str, ptr, 1);
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
  auto *trueBlock = llvm::BasicBlock::Create(context, "if.true", func);
  auto *falseBlock = llvm::BasicBlock::Create(context, "if.false", func);
  auto *exitBlock = llvm::BasicBlock::Create(context, "if.exit", func);

  process(x->getCond());
  llvm::Value *cond = value;
  builder.SetInsertPoint(block);
  cond = builder.CreateTrunc(cond, builder.getInt1Ty());
  builder.CreateCondBr(cond, trueBlock, falseBlock);

  block = trueBlock;
  if (x->getTrueBranch()) {
    process(x->getTrueBranch());
  }
  builder.SetInsertPoint(block);
  builder.CreateBr(exitBlock);

  block = falseBlock;
  if (x->getFalseBranch()) {
    process(x->getFalseBranch());
  }
  builder.SetInsertPoint(block);
  builder.CreateBr(exitBlock);

  block = exitBlock;
}

void LLVMVisitor::visit(const WhileFlow *x) {
  auto *condBlock = llvm::BasicBlock::Create(context, "while.cond", func);
  auto *bodyBlock = llvm::BasicBlock::Create(context, "while.body", func);
  auto *exitBlock = llvm::BasicBlock::Create(context, "while.exit", func);

  builder.SetInsertPoint(block);
  builder.CreateBr(condBlock);

  block = condBlock;
  process(x->getCond());
  llvm::Value *cond = value;
  builder.SetInsertPoint(block);
  cond = builder.CreateTrunc(cond, builder.getInt1Ty());
  builder.CreateCondBr(cond, bodyBlock, exitBlock);

  block = bodyBlock;
  enterLoop(
      {/*breakBlock=*/exitBlock, /*continueBlock=*/condBlock, /*loopId=*/x->getId()});
  process(x->getBody());
  exitLoop();
  builder.SetInsertPoint(block);
  builder.CreateBr(condBlock);

  block = exitBlock;
}

void LLVMVisitor::visit(const ForFlow *x) {
  seqassert(!x->isParallel(), "parallel for-loop not lowered");
  llvm::Type *loopVarType = getLLVMType(x->getVar()->getType());
  llvm::Value *loopVar = vars[x->getVar()];
  seqassert(loopVar, "{} loop variable not found", *x);

  auto *condBlock = llvm::BasicBlock::Create(context, "for.cond", func);
  auto *bodyBlock = llvm::BasicBlock::Create(context, "for.body", func);
  auto *cleanupBlock = llvm::BasicBlock::Create(context, "for.cleanup", func);
  auto *exitBlock = llvm::BasicBlock::Create(context, "for.exit", func);

  // LLVM coroutine intrinsics
  // https://prereleases.llvm.org/6.0.0/rc3/docs/Coroutines.html
  llvm::FunctionCallee coroResume =
      llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_resume);
  llvm::FunctionCallee coroDone =
      llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_done);
  llvm::FunctionCallee coroPromise =
      llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_promise);
  llvm::FunctionCallee coroDestroy =
      llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_destroy);

  process(x->getIter());
  llvm::Value *iter = value;
  builder.SetInsertPoint(block);
  builder.CreateBr(condBlock);

  block = condBlock;
  call(coroResume, {iter});
  builder.SetInsertPoint(block);
  llvm::Value *done = builder.CreateCall(coroDone, iter);
  builder.CreateCondBr(done, cleanupBlock, bodyBlock);

  if (!loopVarType->isVoidTy()) {
    builder.SetInsertPoint(bodyBlock);
    llvm::Value *alignment =
        builder.getInt32(module->getDataLayout().getPrefTypeAlignment(loopVarType));
    llvm::Value *from = builder.getFalse();
    llvm::Value *promise = builder.CreateCall(coroPromise, {iter, alignment, from});
    promise = builder.CreateBitCast(promise, loopVarType->getPointerTo());
    llvm::Value *generatedValue = builder.CreateLoad(promise);
    builder.CreateStore(generatedValue, loopVar);
  }

  block = bodyBlock;
  enterLoop(
      {/*breakBlock=*/exitBlock, /*continueBlock=*/condBlock, /*loopId=*/x->getId()});
  process(x->getBody());
  exitLoop();
  builder.SetInsertPoint(block);
  builder.CreateBr(condBlock);

  builder.SetInsertPoint(cleanupBlock);
  builder.CreateCall(coroDestroy, iter);
  builder.CreateBr(exitBlock);

  block = exitBlock;
}

void LLVMVisitor::visit(const ImperativeForFlow *x) {
  seqassert(!x->isParallel(), "parallel for-loop not lowered");
  llvm::Value *loopVar = vars[x->getVar()];
  seqassert(loopVar, "{} loop variable not found", *x);
  seqassert(x->getStep() != 0, "step cannot be 0");

  auto *condBlock = llvm::BasicBlock::Create(context, "imp_for.cond", func);
  auto *bodyBlock = llvm::BasicBlock::Create(context, "imp_for.body", func);
  auto *updateBlock = llvm::BasicBlock::Create(context, "imp_for.update", func);
  auto *exitBlock = llvm::BasicBlock::Create(context, "imp_for.exit", func);

  process(x->getStart());
  builder.SetInsertPoint(block);
  builder.CreateStore(value, loopVar);
  process(x->getEnd());
  auto *end = value;
  builder.SetInsertPoint(block);
  builder.CreateBr(condBlock);
  builder.SetInsertPoint(condBlock);

  llvm::Value *done;
  if (x->getStep() > 0)
    done = builder.CreateICmpSGE(builder.CreateLoad(loopVar), end);
  else
    done = builder.CreateICmpSLE(builder.CreateLoad(loopVar), end);

  builder.CreateCondBr(done, exitBlock, bodyBlock);

  block = bodyBlock;
  enterLoop(
      {/*breakBlock=*/exitBlock, /*continueBlock=*/updateBlock, /*loopId=*/x->getId()});
  process(x->getBody());
  exitLoop();
  builder.SetInsertPoint(block);
  builder.CreateBr(updateBlock);

  builder.SetInsertPoint(updateBlock);
  builder.CreateStore(
      builder.CreateAdd(builder.CreateLoad(loopVar), builder.getInt64(x->getStep())),
      loopVar);
  builder.CreateBr(condBlock);

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
  builder.SetInsertPoint(block);
  auto *entryBlock = llvm::BasicBlock::Create(context, "trycatch.entry", func);
  builder.CreateBr(entryBlock);

  TryCatchData tc;
  tc.exceptionBlock = llvm::BasicBlock::Create(context, "trycatch.exception", func);
  tc.exceptionRouteBlock =
      llvm::BasicBlock::Create(context, "trycatch.exception_route", func);
  tc.finallyBlock = llvm::BasicBlock::Create(context, "trycatch.finally", func);

  auto *externalExcBlock =
      llvm::BasicBlock::Create(context, "trycatch.exception_external", func);
  auto *unwindResumeBlock =
      llvm::BasicBlock::Create(context, "trycatch.unwind_resume", func);
  auto *endBlock = llvm::BasicBlock::Create(context, "trycatch.end", func);

  builder.SetInsertPoint(func->getEntryBlock().getTerminator());
  auto *excStateNotThrown = builder.getInt8(TryCatchData::State::NOT_THROWN);
  auto *excStateThrown = builder.getInt8(TryCatchData::State::THROWN);
  auto *excStateCaught = builder.getInt8(TryCatchData::State::CAUGHT);
  auto *excStateReturn = builder.getInt8(TryCatchData::State::RETURN);
  auto *excStateBreak = builder.getInt8(TryCatchData::State::BREAK);
  auto *excStateContinue = builder.getInt8(TryCatchData::State::CONTINUE);

  llvm::StructType *padType = getPadType();
  llvm::StructType *unwindType =
      llvm::StructType::get(builder.getInt64Ty()); // header only
  llvm::StructType *excType =
      llvm::StructType::get(getTypeInfoType(), builder.getInt8PtrTy());

  if (isRoot) {
    tc.excFlag = builder.CreateAlloca(builder.getInt8Ty());
    tc.catchStore = builder.CreateAlloca(padType);
    tc.delegateDepth = builder.CreateAlloca(builder.getInt64Ty());
    tc.retStore = (coro.exit || func->getReturnType()->isVoidTy())
                      ? nullptr
                      : builder.CreateAlloca(func->getReturnType());
    tc.loopSequence = builder.CreateAlloca(builder.getInt64Ty());
    builder.CreateStore(excStateNotThrown, tc.excFlag);
    builder.CreateStore(llvm::ConstantAggregateZero::get(padType), tc.catchStore);
    builder.CreateStore(builder.getInt64(0), tc.delegateDepth);
    builder.CreateStore(builder.getInt64(-1), tc.loopSequence);
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
  builder.SetInsertPoint(finallyBlock);
  llvm::Value *excFlagRead = builder.CreateLoad(tc.excFlag);

  if (!isRoot) {
    llvm::Value *depthRead = builder.CreateLoad(tc.delegateDepth);
    llvm::Value *delegate = builder.CreateICmpSGT(depthRead, builder.getInt64(0));
    auto *finallyNormal =
        llvm::BasicBlock::Create(context, "trycatch.finally.normal", func);
    auto *finallyDelegate =
        llvm::BasicBlock::Create(context, "trycatch.finally.delegate", func);
    builder.CreateCondBr(delegate, finallyDelegate, finallyNormal);

    builder.SetInsertPoint(finallyDelegate);
    llvm::Value *depthNew = builder.CreateSub(depthRead, builder.getInt64(1));
    llvm::Value *delegateNew = builder.CreateICmpSGT(depthNew, builder.getInt64(0));
    builder.CreateStore(depthNew, tc.delegateDepth);
    builder.CreateCondBr(delegateNew, trycatch.back().finallyBlock,
                         trycatch.back().exceptionRouteBlock);

    finallyBlock = finallyNormal;
    builder.SetInsertPoint(finallyNormal);
  }

  builder.SetInsertPoint(finallyBlock);
  llvm::SwitchInst *theSwitch =
      builder.CreateSwitch(excFlagRead, endBlock, supportBreakAndContinue ? 5 : 3);
  theSwitch->addCase(excStateCaught, endBlock);
  theSwitch->addCase(excStateThrown, unwindResumeBlock);

  if (isRoot) {
    auto *finallyReturn =
        llvm::BasicBlock::Create(context, "trycatch.finally.return", func);
    theSwitch->addCase(excStateReturn, finallyReturn);
    builder.SetInsertPoint(finallyReturn);
    if (coro.exit) {
      builder.CreateBr(coro.exit);
    } else if (tc.retStore) {
      llvm::Value *retVal = builder.CreateLoad(tc.retStore);
      builder.CreateRet(retVal);
    } else {
      builder.CreateRetVoid();
    }
  } else {
    theSwitch->addCase(excStateReturn, trycatch.back().finallyBlock);
  }

  if (supportBreakAndContinue) {
    auto prevSeq = isRoot ? -1 : trycatch.back().sequenceNumber;

    auto *finallyBreak =
        llvm::BasicBlock::Create(context, "trycatch.finally.break", func);
    auto *finallyBreakDone =
        llvm::BasicBlock::Create(context, "trycatch.finally.break.done", func);
    auto *finallyContinue =
        llvm::BasicBlock::Create(context, "trycatch.finally.continue", func);
    auto *finallyContinueDone =
        llvm::BasicBlock::Create(context, "trycatch.finally.continue.done", func);

    builder.SetInsertPoint(finallyBreak);
    auto *breakSwitch =
        builder.CreateSwitch(builder.CreateLoad(tc.loopSequence), endBlock, 0);
    builder.SetInsertPoint(finallyBreakDone);
    builder.CreateStore(excStateNotThrown, tc.excFlag);
    auto *breakDoneSwitch =
        builder.CreateSwitch(builder.CreateLoad(tc.loopSequence), endBlock, 0);

    builder.SetInsertPoint(finallyContinue);
    auto *continueSwitch =
        builder.CreateSwitch(builder.CreateLoad(tc.loopSequence), endBlock, 0);
    builder.SetInsertPoint(finallyContinueDone);
    builder.CreateStore(excStateNotThrown, tc.excFlag);
    auto *continueDoneSwitch =
        builder.CreateSwitch(builder.CreateLoad(tc.loopSequence), endBlock, 0);

    for (auto &l : loops) {
      if (!trycatch.empty() && l.sequenceNumber < prevSeq) {
        breakSwitch->addCase(builder.getInt64(l.sequenceNumber),
                             trycatch.back().finallyBlock);
        continueSwitch->addCase(builder.getInt64(l.sequenceNumber),
                                trycatch.back().finallyBlock);
      } else {
        breakSwitch->addCase(builder.getInt64(l.sequenceNumber), finallyBreakDone);
        breakDoneSwitch->addCase(builder.getInt64(l.sequenceNumber), l.breakBlock);
        continueSwitch->addCase(builder.getInt64(l.sequenceNumber),
                                finallyContinueDone);
        continueDoneSwitch->addCase(builder.getInt64(l.sequenceNumber),
                                    l.continueBlock);
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
    auto *catchBlock = llvm::BasicBlock::Create(context, "trycatch.catch", func);
    tc.catchTypes.push_back(c->getType());
    tc.handlers.push_back(catchBlock);

    if (!c->getType()) {
      seqassert(!catchAll, "cannot be catch all");
      catchAll = catchBlock;
    }
  }

  // translate try
  block = entryBlock;
  enterTryCatch(tc);
  process(x->getBody());
  exitTryCatch();

  // make sure we always get to finally block
  builder.SetInsertPoint(block);
  builder.CreateBr(tc.finallyBlock);

  // rethrow if uncaught
  builder.SetInsertPoint(unwindResumeBlock);
  builder.CreateResume(builder.CreateLoad(tc.catchStore));

  // make sure we delegate to parent try-catch if necessary
  std::vector<types::Type *> catchTypesFull(tc.catchTypes);
  std::vector<llvm::BasicBlock *> handlersFull(tc.handlers);
  std::vector<unsigned> depths(tc.catchTypes.size(), 0);
  unsigned depth = 1;

  unsigned catchAllDepth = 0;
  for (auto it = trycatch.rbegin(); it != trycatch.rend(); ++it) {
    if (catchAll) // can't ever delegate past catch-all
      break;

    seqassert(it->catchTypes.size() == it->handlers.size(), "handler mismatch");
    for (unsigned i = 0; i < it->catchTypes.size(); i++) {
      if (!anyMatch(it->catchTypes[i], catchTypesFull)) {
        catchTypesFull.push_back(it->catchTypes[i]);
        depths.push_back(depth);

        if (!it->catchTypes[i] && !catchAll) {
          // catch-all is in parent; set finally depth
          catchAll =
              llvm::BasicBlock::Create(context, "trycatch.fdepth_catchall", func);
          builder.SetInsertPoint(catchAll);
          builder.CreateStore(builder.getInt64(depth), tc.delegateDepth);
          builder.CreateBr(it->handlers[i]);
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
  builder.SetInsertPoint(tc.exceptionBlock);
  llvm::LandingPadInst *caughtResult =
      builder.CreateLandingPad(padType, catches.size());
  caughtResult->setCleanup(true);
  std::vector<llvm::Value *> typeIndices;

  for (auto *catchType : catchTypesFull) {
    seqassert(!catchType || cast<types::RefType>(catchType), "invalid catch type");
    const std::string typeVarName =
        "seq.typeidx." + (catchType ? catchType->getName() : "<all>");
    llvm::GlobalVariable *tidx = getTypeIdxVar(catchType);
    typeIndices.push_back(tidx);
    caughtResult->addClause(tidx);
  }

  llvm::Value *unwindException = builder.CreateExtractValue(caughtResult, 0);
  builder.CreateStore(caughtResult, tc.catchStore);
  builder.CreateStore(excStateThrown, tc.excFlag);
  llvm::Value *depthMax = builder.getInt64(trycatch.size());
  builder.CreateStore(depthMax, tc.delegateDepth);

  llvm::Value *unwindExceptionClass = builder.CreateLoad(builder.CreateStructGEP(
      unwindType,
      builder.CreatePointerCast(unwindException, unwindType->getPointerTo()), 0));

  // check for foreign exceptions
  builder.CreateCondBr(
      builder.CreateICmpEQ(unwindExceptionClass, builder.getInt64(seq_exc_class())),
      tc.exceptionRouteBlock, externalExcBlock);

  // external exception (currently assumed to be unreachable)
  builder.SetInsertPoint(externalExcBlock);
  builder.CreateUnreachable();

  // reroute Seq exceptions
  builder.SetInsertPoint(tc.exceptionRouteBlock);
  unwindException = builder.CreateExtractValue(builder.CreateLoad(tc.catchStore), 0);
  llvm::Value *excVal = builder.CreatePointerCast(
      builder.CreateConstGEP1_64(unwindException, (uint64_t)seq_exc_offset()),
      excType->getPointerTo());

  llvm::Value *loadedExc = builder.CreateLoad(excVal);
  llvm::Value *objType = builder.CreateExtractValue(loadedExc, 0);
  objType = builder.CreateExtractValue(objType, 0);
  llvm::Value *objPtr = builder.CreateExtractValue(loadedExc, 1);

  // set depth when catch-all entered
  auto *defaultRouteBlock = llvm::BasicBlock::Create(context, "trycatch.fdepth", func);
  builder.SetInsertPoint(defaultRouteBlock);
  if (catchAll)
    builder.CreateStore(builder.getInt64(catchAllDepth), tc.delegateDepth);
  builder.CreateBr(catchAll ? (catchAllDepth > 0 ? tc.finallyBlock : catchAll)
                            : tc.finallyBlock);

  builder.SetInsertPoint(tc.exceptionRouteBlock);
  llvm::SwitchInst *switchToCatchBlock =
      builder.CreateSwitch(objType, defaultRouteBlock, (unsigned)handlersFull.size());
  for (unsigned i = 0; i < handlersFull.size(); i++) {
    // set finally depth
    auto *depthSet = llvm::BasicBlock::Create(context, "trycatch.fdepth", func);
    builder.SetInsertPoint(depthSet);
    builder.CreateStore(builder.getInt64(depths[i]), tc.delegateDepth);
    builder.CreateBr((i < tc.handlers.size()) ? handlersFull[i] : tc.finallyBlock);

    if (catchTypesFull[i]) {
      switchToCatchBlock->addCase(
          builder.getInt32((uint64_t)getTypeIdx(catchTypesFull[i])), depthSet);
    }

    // translate catch body if this block is ours (vs. a parent's)
    if (i < catches.size()) {
      block = handlersFull[i];
      builder.SetInsertPoint(block);
      const Var *var = catches[i]->getVar();

      if (var) {
        llvm::Value *obj =
            builder.CreateBitCast(objPtr, getLLVMType(catches[i]->getType()));
        llvm::Value *varPtr = vars[var];
        seqassert(varPtr, "could not get catch var");
        builder.CreateStore(obj, varPtr);
      }

      builder.CreateStore(excStateCaught, tc.excFlag);
      process(catches[i]->getHandler());
      builder.SetInsertPoint(block);
      builder.CreateBr(tc.finallyBlock);
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

  auto *funcType = cast<llvm::PointerType>(getLLVMType(stage->getCallee()->getType()));
  value = call({cast<llvm::FunctionType>(funcType->getElementType()), f}, args);
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
    seqassert(generatorType, "{} is not a generator type", *prevStage->getOutputType());
    auto *baseType = getLLVMType(generatorType->getBase());

    auto *condBlock = llvm::BasicBlock::Create(context, "pipeline.cond", func);
    auto *bodyBlock = llvm::BasicBlock::Create(context, "pipeline.body", func);
    auto *cleanupBlock = llvm::BasicBlock::Create(context, "pipeline.cleanup", func);
    auto *exitBlock = llvm::BasicBlock::Create(context, "pipeline.exit", func);

    // LLVM coroutine intrinsics
    // https://prereleases.llvm.org/6.0.0/rc3/docs/Coroutines.html
    llvm::FunctionCallee coroResume =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_resume);
    llvm::FunctionCallee coroDone =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_done);
    llvm::FunctionCallee coroPromise =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_promise);
    llvm::FunctionCallee coroDestroy =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_destroy);

    llvm::Value *iter = value;
    builder.SetInsertPoint(block);
    builder.CreateBr(condBlock);

    block = condBlock;
    call(coroResume, {iter});
    builder.SetInsertPoint(block);
    llvm::Value *done = builder.CreateCall(coroDone, iter);
    builder.CreateCondBr(done, cleanupBlock, bodyBlock);

    builder.SetInsertPoint(bodyBlock);
    llvm::Value *alignment =
        builder.getInt32(module->getDataLayout().getPrefTypeAlignment(baseType));
    llvm::Value *from = builder.getFalse();
    llvm::Value *promise = builder.CreateCall(coroPromise, {iter, alignment, from});
    promise = builder.CreateBitCast(promise, baseType->getPointerTo());
    value = builder.CreateLoad(promise);

    block = bodyBlock;
    callStage(stage);
    codegenPipeline(stages, where + 1);

    builder.SetInsertPoint(block);
    builder.CreateBr(condBlock);

    builder.SetInsertPoint(cleanupBlock);
    builder.CreateCall(coroDestroy, iter);
    builder.CreateBr(exitBlock);

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
  builder.SetInsertPoint(block);
  value = x->getBuilder()->buildValue(this);
}

/*
 * Instructions
 */

void LLVMVisitor::visit(const AssignInstr *x) {
  llvm::Value *var = vars[x->getLhs()];
  seqassert(var, "could not find {} var", *x);
  process(x->getRhs());
  if (var != getDummyVoidValue(context)) {
    builder.SetInsertPoint(block);
    builder.CreateStore(value, var);
  }
}

void LLVMVisitor::visit(const ExtractInstr *x) {
  auto *memberedType = cast<types::MemberedType>(x->getVal()->getType());
  seqassert(memberedType, "{} is not a membered type", *x->getVal()->getType());
  const int index = memberedType->getMemberIndex(x->getField());
  seqassert(index >= 0, "invalid index");

  process(x->getVal());
  builder.SetInsertPoint(block);
  if (auto *refType = cast<types::RefType>(memberedType)) {
    value = builder.CreateBitCast(value,
                                  getLLVMType(refType->getContents())->getPointerTo());
    value = builder.CreateLoad(value);
  }
  value = builder.CreateExtractValue(value, index);
}

void LLVMVisitor::visit(const InsertInstr *x) {
  auto *refType = cast<types::RefType>(x->getLhs()->getType());
  seqassert(refType, "{} is not a reference type", *x->getLhs()->getType());
  const int index = refType->getMemberIndex(x->getField());
  seqassert(index >= 0, "invalid index");

  process(x->getLhs());
  llvm::Value *lhs = value;
  process(x->getRhs());
  llvm::Value *rhs = value;

  builder.SetInsertPoint(block);
  lhs = builder.CreateBitCast(lhs, getLLVMType(refType->getContents())->getPointerTo());
  llvm::Value *load = builder.CreateLoad(lhs);
  load = builder.CreateInsertValue(load, rhs, index);
  builder.CreateStore(load, lhs);
}

void LLVMVisitor::visit(const CallInstr *x) {
  builder.SetInsertPoint(block);
  process(x->getCallee());
  llvm::Value *f = value;

  std::vector<llvm::Value *> args;
  for (auto *arg : *x) {
    builder.SetInsertPoint(block);
    process(arg);
    args.push_back(value);
  }

  auto *funcType = cast<llvm::PointerType>(getLLVMType(x->getCallee()->getType()));
  value = call({cast<llvm::FunctionType>(funcType->getElementType()), f}, args);
}

void LLVMVisitor::visit(const TypePropertyInstr *x) {
  builder.SetInsertPoint(block);
  switch (x->getProperty()) {
  case TypePropertyInstr::Property::SIZEOF:
    value = builder.getInt64(
        module->getDataLayout().getTypeAllocSize(getLLVMType(x->getInspectType())));
    break;
  case TypePropertyInstr::Property::IS_ATOMIC:
    value = builder.getInt8(x->getInspectType()->isAtomic() ? 1 : 0);
    break;
  default:
    seqassert(0, "unknown type property");
  }
}

void LLVMVisitor::visit(const YieldInInstr *x) {
  builder.SetInsertPoint(block);
  if (x->isSuspending()) {
    llvm::FunctionCallee coroSuspend =
        llvm::Intrinsic::getDeclaration(module.get(), llvm::Intrinsic::coro_suspend);
    llvm::Value *tok = llvm::ConstantTokenNone::get(context);
    llvm::Value *final = builder.getFalse();
    llvm::Value *susp = builder.CreateCall(coroSuspend, {tok, final});

    block = llvm::BasicBlock::Create(context, "yieldin.new", func);
    llvm::SwitchInst *inst = builder.CreateSwitch(susp, coro.suspend, 2);
    inst->addCase(builder.getInt8(0), block);
    inst->addCase(builder.getInt8(1), coro.cleanup);
    builder.SetInsertPoint(block);
  }
  value = builder.CreateLoad(coro.promise);
}

void LLVMVisitor::visit(const StackAllocInstr *x) {
  auto *arrayType = llvm::cast<llvm::StructType>(getLLVMType(x->getType()));
  builder.SetInsertPoint(func->getEntryBlock().getTerminator());
  llvm::Value *len = builder.getInt64(x->getCount());
  llvm::Value *ptr = builder.CreateAlloca(
      llvm::cast<llvm::PointerType>(arrayType->getElementType(1))->getElementType(),
      len);
  llvm::Value *arr = llvm::UndefValue::get(arrayType);
  arr = builder.CreateInsertValue(arr, len, 0);
  arr = builder.CreateInsertValue(arr, ptr, 1);
  value = arr;
}

void LLVMVisitor::visit(const TernaryInstr *x) {
  auto *trueBlock = llvm::BasicBlock::Create(context, "ternary.true", func);
  auto *falseBlock = llvm::BasicBlock::Create(context, "ternary.false", func);
  auto *exitBlock = llvm::BasicBlock::Create(context, "ternary.exit", func);

  llvm::Type *valueType = getLLVMType(x->getType());
  process(x->getCond());
  llvm::Value *cond = value;

  builder.SetInsertPoint(block);
  cond = builder.CreateTrunc(cond, builder.getInt1Ty());
  builder.CreateCondBr(cond, trueBlock, falseBlock);

  block = trueBlock;
  process(x->getTrueValue());
  llvm::Value *trueValue = value;
  trueBlock = block;
  builder.SetInsertPoint(trueBlock);
  builder.CreateBr(exitBlock);

  block = falseBlock;
  process(x->getFalseValue());
  llvm::Value *falseValue = value;
  falseBlock = block;
  builder.SetInsertPoint(falseBlock);
  builder.CreateBr(exitBlock);

  builder.SetInsertPoint(exitBlock);
  llvm::PHINode *phi = builder.CreatePHI(valueType, 2);
  phi->addIncoming(trueValue, trueBlock);
  phi->addIncoming(falseValue, falseBlock);
  value = phi;
  block = exitBlock;
}

void LLVMVisitor::visit(const BreakInstr *x) {
  seqassert(!loops.empty(), "not in a loop");
  builder.SetInsertPoint(block);

  auto *loop = !x->getLoop() ? &loops.back() : getLoopData(x->getLoop()->getId());

  if (trycatch.empty() || trycatch.back().sequenceNumber < loop->sequenceNumber) {
    builder.CreateBr(loop->breakBlock);
  } else {
    auto *tc = &trycatch.back();
    auto *excStateBreak = builder.getInt8(TryCatchData::State::BREAK);
    builder.CreateStore(excStateBreak, tc->excFlag);
    builder.CreateStore(builder.getInt64(loop->sequenceNumber), tc->loopSequence);
    builder.CreateBr(tc->finallyBlock);
  }

  block = llvm::BasicBlock::Create(context, "break.new", func);
}

void LLVMVisitor::visit(const ContinueInstr *x) {
  seqassert(!loops.empty(), "not in a loop");
  builder.SetInsertPoint(block);
  auto *loop = !x->getLoop() ? &loops.back() : getLoopData(x->getLoop()->getId());

  if (trycatch.empty() || trycatch.back().sequenceNumber < loop->sequenceNumber) {
    builder.CreateBr(loop->continueBlock);
  } else {
    auto *tc = &trycatch.back();
    auto *excStateContinue = builder.getInt8(TryCatchData::State::CONTINUE);
    builder.CreateStore(excStateContinue, tc->excFlag);
    builder.CreateStore(builder.getInt64(loop->sequenceNumber), tc->loopSequence);
    builder.CreateBr(tc->finallyBlock);
  }

  block = llvm::BasicBlock::Create(context, "continue.new", func);
}

void LLVMVisitor::visit(const ReturnInstr *x) {
  if (x->getValue()) {
    process(x->getValue());
  }
  builder.SetInsertPoint(block);
  if (coro.exit) {
    if (auto *tc = getInnermostTryCatch()) {
      auto *excStateReturn = builder.getInt8(TryCatchData::State::RETURN);
      builder.CreateStore(excStateReturn, tc->excFlag);
      builder.CreateBr(tc->finallyBlock);
    } else {
      builder.CreateBr(coro.exit);
    }
  } else {
    if (auto *tc = getInnermostTryCatch()) {
      auto *excStateReturn = builder.getInt8(TryCatchData::State::RETURN);
      builder.CreateStore(excStateReturn, tc->excFlag);
      if (tc->retStore) {
        seqassert(value, "no return value storage");
        builder.CreateStore(value, tc->retStore);
      }
      builder.CreateBr(tc->finallyBlock);
    } else {
      if (x->getValue()) {
        builder.CreateRet(value);
      } else {
        builder.CreateRetVoid();
      }
    }
  }
  block = llvm::BasicBlock::Create(context, "return.new", func);
}

void LLVMVisitor::visit(const YieldInstr *x) {
  if (x->isFinal()) {
    if (x->getValue()) {
      seqassert(coro.promise, "no coroutine promise");
      process(x->getValue());
      builder.SetInsertPoint(block);
      builder.CreateStore(value, coro.promise);
    }
    builder.SetInsertPoint(block);
    if (auto *tc = getInnermostTryCatch()) {
      auto *excStateReturn = builder.getInt8(TryCatchData::State::RETURN);
      builder.CreateStore(excStateReturn, tc->excFlag);
      builder.CreateBr(tc->finallyBlock);
    } else {
      builder.CreateBr(coro.exit);
    }
    block = llvm::BasicBlock::Create(context, "yield.new", func);
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
  // note: exception header should be set in the frontend
  auto excAllocFunc = makeExcAllocFunc();
  auto throwFunc = makeThrowFunc();
  process(x->getValue());
  builder.SetInsertPoint(block);
  llvm::Value *exc = builder.CreateCall(
      excAllocFunc, {builder.getInt32(getTypeIdx(x->getValue()->getType())), value});
  call(throwFunc, exc);
}

void LLVMVisitor::visit(const FlowInstr *x) {
  process(x->getFlow());
  process(x->getValue());
}

void LLVMVisitor::visit(const dsl::CustomInstr *x) {
  builder.SetInsertPoint(block);
  value = x->getBuilder()->buildValue(this);
}

} // namespace ir
} // namespace seq
