// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/cir.h"
#include "codon/cir/llvm/llvm.h"
#include "codon/cir/pyextension.h"
#include "codon/dsl/plugins.h"
#include "codon/util/common.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace codon {
namespace ir {

class LLVMVisitor : public util::ConstVisitor {
private:
  struct CoroData {
    /// Coroutine promise (where yielded values are stored)
    llvm::Value *promise;
    /// Coroutine handle
    llvm::Value *handle;
    /// Coroutine cleanup block
    llvm::BasicBlock *cleanup;
    /// Coroutine suspend block
    llvm::BasicBlock *suspend;
    /// Coroutine exit block
    llvm::BasicBlock *exit;

    void reset() { promise = handle = cleanup = suspend = exit = nullptr; }
  };

  struct NestableData {
    int sequenceNumber;

    NestableData() : sequenceNumber(-1) {}
  };

  struct LoopData : NestableData {
    /// Block to branch to in case of "break"
    llvm::BasicBlock *breakBlock;
    /// Block to branch to in case of "continue"
    llvm::BasicBlock *continueBlock;
    /// Loop id
    id_t loopId;

    LoopData(llvm::BasicBlock *breakBlock, llvm::BasicBlock *continueBlock, id_t loopId)
        : NestableData(), breakBlock(breakBlock), continueBlock(continueBlock),
          loopId(loopId) {}

    void reset() { breakBlock = continueBlock = nullptr; }
  };

  struct TryCatchData : NestableData {
    /// Possible try-catch states when reaching finally block
    enum State { NOT_THROWN = 0, THROWN, CAUGHT, RETURN, BREAK, CONTINUE };
    /// Exception block
    llvm::BasicBlock *exceptionBlock;
    /// Exception route block
    llvm::BasicBlock *exceptionRouteBlock;
    /// Finally start block
    llvm::BasicBlock *finallyBlock;
    /// Try-catch catch types
    std::vector<types::Type *> catchTypes;
    /// Try-catch handlers, corresponding to catch types
    std::vector<llvm::BasicBlock *> handlers;
    /// Exception state flag (see "State")
    llvm::Value *excFlag;
    /// Storage for caught exception
    llvm::Value *catchStore;
    /// How far to delegate up the finally chain
    llvm::Value *delegateDepth;
    /// Storage for postponed return
    llvm::Value *retStore;
    /// Loop being manipulated
    llvm::Value *loopSequence;

    TryCatchData()
        : NestableData(), exceptionBlock(nullptr), exceptionRouteBlock(nullptr),
          finallyBlock(nullptr), catchTypes(), handlers(), excFlag(nullptr),
          catchStore(nullptr), delegateDepth(nullptr), retStore(nullptr),
          loopSequence(nullptr) {}

    void reset() {
      exceptionBlock = exceptionRouteBlock = finallyBlock = nullptr;
      catchTypes.clear();
      handlers.clear();
      excFlag = catchStore = delegateDepth = loopSequence = nullptr;
    }
  };

  struct CatchData : NestableData {
    llvm::Value *exception;
    llvm::Value *typeId;
  };

  struct DebugInfo {
    /// LLVM debug info builder
    std::unique_ptr<llvm::DIBuilder> builder;
    /// Current compilation unit
    llvm::DICompileUnit *unit;
    /// Whether we are compiling in debug mode
    bool debug;
    /// Whether we are compiling in JIT mode
    bool jit;
    /// Whether we are compiling a standalone object/executable
    bool standalone;
    /// Whether to capture writes to stdout/stderr
    bool capture;
    /// Program command-line flags
    std::string flags;

    DebugInfo()
        : builder(), unit(nullptr), debug(false), jit(false), standalone(false),
          capture(false), flags() {}

    llvm::DIFile *getFile(const std::string &path);

    void reset() {
      builder = {};
      unit = nullptr;
    }
  };

  /// LLVM context used for compilation
  std::unique_ptr<llvm::LLVMContext> context;
  /// Module we are compiling
  std::unique_ptr<llvm::Module> M;
  /// LLVM IR builder used for constructing LLVM IR
  std::unique_ptr<llvm::IRBuilder<>> B;
  /// Current function we are compiling
  llvm::Function *func;
  /// Current basic block we are compiling
  llvm::BasicBlock *block;
  /// Last compiled value
  llvm::Value *value;
  /// LLVM values corresponding to IR variables
  std::unordered_map<id_t, llvm::Value *> vars;
  /// LLVM functions corresponding to IR functions
  std::unordered_map<id_t, llvm::Function *> funcs;
  /// Coroutine data, if current function is a coroutine
  CoroData coro;
  /// Loop data stack, containing break/continue blocks
  std::vector<LoopData> loops;
  /// Try-catch data stack
  std::vector<TryCatchData> trycatch;
  /// Catch-block data stack
  std::vector<CatchData> catches;
  /// Debug information
  DebugInfo db;
  /// Plugin manager
  PluginManager *plugins;

  llvm::DIType *
  getDITypeHelper(types::Type *t,
                  std::unordered_map<std::string, llvm::DICompositeType *> &cache);

  /// GC allocation functions
  llvm::FunctionCallee makeAllocFunc(bool atomic, bool uncollectable = false);
  // GC reallocation function
  llvm::FunctionCallee makeReallocFunc();
  // GC free function
  llvm::FunctionCallee makeFreeFunc();
  /// Personality function for exception handling
  llvm::FunctionCallee makePersonalityFunc();
  /// Exception allocation function
  llvm::FunctionCallee makeExcAllocFunc();
  /// Exception throw function
  llvm::FunctionCallee makeThrowFunc();
  /// Program termination function
  llvm::FunctionCallee makeTerminateFunc();

  // Try-catch types and utilities
  llvm::StructType *getTypeInfoType();
  llvm::StructType *getPadType();
  llvm::StructType *getExceptionType();
  llvm::GlobalVariable *getTypeIdxVar(const std::string &name);
  llvm::GlobalVariable *getTypeIdxVar(types::Type *catchType);
  int getTypeIdx(types::Type *catchType = nullptr);

  // General function helpers
  llvm::Value *call(llvm::FunctionCallee callee, llvm::ArrayRef<llvm::Value *> args);
  llvm::Function *makeLLVMFunction(const Func *);
  void makeYield(llvm::Value *value = nullptr, bool finalYield = false);
  std::string buildLLVMCodeString(const LLVMFunc *);
  void callStage(const PipelineFlow::Stage *stage);
  void codegenPipeline(const std::vector<const PipelineFlow::Stage *> &stages,
                       unsigned where = 0);

  // Loop and try-catch state
  void enterLoop(LoopData data);
  void exitLoop();
  void enterTryCatch(TryCatchData data);
  void exitTryCatch();
  void enterCatch(CatchData data);
  void exitCatch();
  TryCatchData *getInnermostTryCatch();
  TryCatchData *getInnermostTryCatchBeforeLoop();

  // Shared library setup
  void setupGlobalCtorForSharedLibrary();

  // Python extension setup
  llvm::Function *createPyTryCatchWrapper(llvm::Function *func);

  // LLVM passes
  void runLLVMPipeline();

  llvm::Value *getVar(const Var *var);
  void insertVar(const Var *var, llvm::Value *x) { vars.emplace(var->getId(), x); }
  llvm::Function *getFunc(const Func *func);
  void insertFunc(const Func *func, llvm::Function *x) {
    funcs.emplace(func->getId(), x);
  }
  llvm::Value *getDummyVoidValue() { return llvm::ConstantTokenNone::get(*context); }
  llvm::DISubprogram *getDISubprogramForFunc(const Func *x);
  void clearLLVMData();

public:
  static std::string getNameForFunction(const Func *x);
  static std::string getNameForVar(const Var *x);

  static std::string getDebugNameForVariable(const Var *x) {
    std::string name = x->getName();
    auto pos = name.find(".");
    if (pos != 0 && pos != std::string::npos) {
      return name.substr(0, pos);
    } else {
      return name;
    }
  }

  static const SrcInfo *getDefaultSrcInfo() {
    static SrcInfo defaultSrcInfo("<internal>", 0, 0, 0);
    return &defaultSrcInfo;
  }

  static const SrcInfo *getSrcInfo(const Node *x) {
    if (auto *srcInfo = x->getAttribute<SrcInfoAttribute>()) {
      return &srcInfo->info;
    } else {
      return getDefaultSrcInfo();
    }
  }

  /// Constructs an LLVM visitor.
  LLVMVisitor();

  /// @return true if in debug mode, false otherwise
  bool getDebug() const { return db.debug; }
  /// Sets debug status.
  /// @param d true if debug mode
  void setDebug(bool d = true) { db.debug = d; }

  /// @return true if in JIT mode, false otherwise
  bool getJIT() const { return db.jit; }
  /// Sets JIT status.
  /// @param j true if JIT mode
  void setJIT(bool j = true) { db.jit = j; }

  /// @return true if in standalone mode, false otherwise
  bool getStandalone() const { return db.standalone; }
  /// Sets standalone status.
  /// @param s true if standalone
  void setStandalone(bool s = true) { db.standalone = s; }

  /// @return true if capturing outputs, false otherwise
  bool getCapture() const { return db.capture; }
  /// Sets capture status.
  /// @param c true to capture
  void setCapture(bool c = true) { db.capture = c; }

  /// @return program flags
  std::string getFlags() const { return db.flags; }
  /// Sets program flags.
  /// @param f flags
  void setFlags(const std::string &f) { db.flags = f; }

  llvm::LLVMContext &getContext() { return *context; }
  llvm::IRBuilder<> &getBuilder() { return *B; }
  llvm::Module *getModule() { return M.get(); }
  llvm::FunctionCallee getFunc() { return func; }
  llvm::BasicBlock *getBlock() { return block; }
  llvm::Value *getValue() { return value; }
  std::unordered_map<id_t, llvm::Value *> &getVars() { return vars; }
  std::unordered_map<id_t, llvm::Function *> &getFuncs() { return funcs; }
  CoroData &getCoro() { return coro; }
  std::vector<LoopData> &getLoops() { return loops; }
  std::vector<TryCatchData> &getTryCatch() { return trycatch; }
  DebugInfo &getDebugInfo() { return db; }

  void setFunc(llvm::Function *f) { func = f; }
  void setBlock(llvm::BasicBlock *b) { block = b; }
  void setValue(llvm::Value *v) { value = v; }

  /// Registers a new global variable or function with
  /// this visitor.
  /// @param var the global variable (or function) to register
  void registerGlobal(const Var *var);

  /// Returns a new LLVM module initialized for the host
  /// architecture.
  /// @param context LLVM context used for creating module
  /// @param src source information for the new module
  /// @return a new module
  std::unique_ptr<llvm::Module> makeModule(llvm::LLVMContext &context,
                                           const SrcInfo *src = nullptr);

  /// Returns the current module/LLVM context and replaces them
  /// with new, fresh ones. References to variables or functions
  /// from the old module will be included as "external".
  /// @param module the IR module
  /// @param src source information for the new module
  /// @return the current module/context, replaced internally
  std::pair<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::LLVMContext>>
  takeModule(Module *module, const SrcInfo *src = nullptr);

  /// Sets current debug info based on a given node.
  /// @param node the node whose debug info to use
  void setDebugInfoForNode(const Node *node);

  /// Compiles a given IR node, updating the internal
  /// LLVM value and/or function as a result.
  /// @param node the node to compile
  void process(const Node *node);

  /// Dumps the unoptimized module IR to a file.
  /// @param filename name of file to write IR to
  void dump(const std::string &filename = "_dump.ll");
  /// Writes module as native object file.
  /// @param filename the .o file to write to
  /// @param pic true to write position-independent code
  void writeToObjectFile(const std::string &filename, bool pic = false);
  /// Writes module as LLVM bitcode file.
  /// @param filename the .bc file to write to
  void writeToBitcodeFile(const std::string &filename);
  /// Writes module as LLVM IR file.
  /// @param filename the .ll file to write to
  void writeToLLFile(const std::string &filename, bool optimize = true);
  /// Writes module as native executable. Invokes an
  /// external linker to generate the final executable.
  /// @param filename the file to write to
  /// @param argv0 compiler's argv[0] used to set rpath
  /// @param library whether to make a shared library
  /// @param libs library names to link
  /// @param lflags extra flags to pass linker
  void writeToExecutable(const std::string &filename, const std::string &argv0,
                         bool library = false,
                         const std::vector<std::string> &libs = {},
                         const std::string &lflags = "");
  /// Writes module as Python extension object.
  /// @param pymod extension module
  /// @param filename the file to write to
  void writeToPythonExtension(const PyModule &pymod, const std::string &filename);
  /// Runs optimization passes on module and writes the result
  /// to the specified file. The output type is determined by
  /// the file extension (.ll for LLVM IR, .bc for LLVM bitcode
  /// .o or .obj for object file, other for executable).
  /// @param filename name of the file to write to
  /// @param argv0 compiler's argv[0] used to set rpath
  /// @param libs library names to link to, if creating executable
  /// @param lflags extra flags to pass linker, if creating executable
  void compile(const std::string &filename, const std::string &argv0,
               const std::vector<std::string> &libs = {},
               const std::string &lflags = "");
  /// Runs optimization passes on module and executes it.
  /// @param args vector of arguments to program
  /// @param libs vector of libraries to load
  /// @param envp program environment
  void run(const std::vector<std::string> &args = {},
           const std::vector<std::string> &libs = {},
           const char *const *envp = nullptr);

  /// Gets LLVM type from IR type
  /// @param t the IR type
  /// @return corresponding LLVM type
  llvm::Type *getLLVMType(types::Type *t);
  /// Gets LLVM function type from IR function type
  /// @param t the IR type (must be FuncType)
  /// @return corresponding LLVM function type
  llvm::FunctionType *getLLVMFuncType(types::Type *t);
  /// Gets the LLVM debug info type from the IR type
  /// @param t the IR type
  /// @return corresponding LLVM DI type
  llvm::DIType *getDIType(types::Type *t);
  /// Gets loop data for a given loop id
  /// @param loopId the IR id of the loop
  /// @return the loop's datas
  LoopData *getLoopData(id_t loopId);

  /// Sets the plugin manager
  /// @param p the plugin manager
  void setPluginManager(PluginManager *p) { plugins = p; }
  /// @return the plugin manager
  PluginManager *getPluginManager() { return plugins; }

  void visit(const Module *) override;
  void visit(const BodiedFunc *) override;
  void visit(const ExternalFunc *) override;
  void visit(const InternalFunc *) override;
  void visit(const LLVMFunc *) override;
  void visit(const Var *) override;
  void visit(const VarValue *) override;
  void visit(const PointerValue *) override;

  void visit(const IntConst *) override;
  void visit(const FloatConst *) override;
  void visit(const BoolConst *) override;
  void visit(const StringConst *) override;
  void visit(const dsl::CustomConst *) override;

  void visit(const SeriesFlow *) override;
  void visit(const IfFlow *) override;
  void visit(const WhileFlow *) override;
  void visit(const ForFlow *) override;
  void visit(const ImperativeForFlow *) override;
  void visit(const TryCatchFlow *) override;
  void visit(const PipelineFlow *) override;
  void visit(const dsl::CustomFlow *) override;

  void visit(const AssignInstr *) override;
  void visit(const ExtractInstr *) override;
  void visit(const InsertInstr *) override;
  void visit(const CallInstr *) override;
  void visit(const StackAllocInstr *) override;
  void visit(const TypePropertyInstr *) override;
  void visit(const YieldInInstr *) override;
  void visit(const TernaryInstr *) override;
  void visit(const BreakInstr *) override;
  void visit(const ContinueInstr *) override;
  void visit(const ReturnInstr *) override;
  void visit(const YieldInstr *) override;
  void visit(const ThrowInstr *) override;
  void visit(const FlowInstr *) override;
  void visit(const dsl::CustomInstr *) override;
};

} // namespace ir
} // namespace codon
