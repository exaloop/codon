#pragma once

#include "llvm.h"
#include "sir/sir.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace seq {
namespace ir {

class LLVMVisitor : public util::ConstVisitor {
private:
  template <typename V> using CacheBase = std::unordered_map<id_t, V *>;
  template <typename K, typename V> class Cache : public CacheBase<V> {
  public:
    using CacheBase<V>::CacheBase;

    V *operator[](const K *key) {
      auto it = CacheBase<V>::find(key->getId());
      return (it != CacheBase<V>::end()) ? it->second : nullptr;
    }

    void insert(const K *key, V *value) { CacheBase<V>::emplace(key->getId(), value); }
  };

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
  };

  struct DebugInfo {
    /// LLVM debug info builder
    std::unique_ptr<llvm::DIBuilder> builder;
    /// Current compilation unit
    llvm::DICompileUnit *unit;
    /// Whether we are compiling in debug mode
    bool debug;
    /// Program command-line flags
    std::string flags;

    explicit DebugInfo(bool debug, const std::string &flags)
        : builder(), unit(nullptr), debug(debug), flags(flags) {}

    llvm::DIFile *getFile(const std::string &path);
  };

  /// LLVM context used for compilation
  llvm::LLVMContext context;
  /// LLVM IR builder used for constructing LLVM IR
  llvm::IRBuilder<> builder;
  /// Module we are compiling
  std::unique_ptr<llvm::Module> module;
  /// Current function we are compiling
  llvm::Function *func;
  /// Current basic block we are compiling
  llvm::BasicBlock *block;
  /// Last compiled value
  llvm::Value *value;
  /// LLVM values corresponding to IR variables
  Cache<Var, llvm::Value> vars;
  /// LLVM functions corresponding to IR functions
  Cache<Func, llvm::Function> funcs;
  /// Coroutine data, if current function is a coroutine
  CoroData coro;
  /// Loop data stack, containing break/continue blocks
  std::vector<LoopData> loops;
  /// Try-catch data stack
  std::vector<TryCatchData> trycatch;
  /// Debug information
  DebugInfo db;
  /// LLVM target machine
  std::unique_ptr<llvm::TargetMachine> machine;

  llvm::DIType *
  getDITypeHelper(types::Type *t,
                  std::unordered_map<std::string, llvm::DICompositeType *> &cache);

  /// GC allocation functions
  llvm::FunctionCallee makeAllocFunc(bool atomic);
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
  void makeLLVMFunction(const Func *);
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
  TryCatchData *getInnermostTryCatch();
  TryCatchData *getInnermostTryCatchBeforeLoop();

  // LLVM passes
  void applyDebugTransformations();
  void runLLVMOptimizationPasses();
  void runLLVMPipeline();

public:
  LLVMVisitor(bool debug = false, const std::string &flags = "");

  llvm::LLVMContext &getContext() { return context; }
  llvm::IRBuilder<> &getBuilder() { return builder; }
  llvm::Module *getModule() { return module.get(); }
  llvm::FunctionCallee getFunc() { return func; }
  llvm::BasicBlock *getBlock() { return block; }
  llvm::Value *getValue() { return value; }
  Cache<Var, llvm::Value> &getVars() { return vars; }
  Cache<Func, llvm::Function> &getFuncs() { return funcs; }
  CoroData &getCoro() { return coro; }
  std::vector<LoopData> &getLoops() { return loops; }
  std::vector<TryCatchData> &getTryCatch() { return trycatch; }
  DebugInfo &getDebugInfo() { return db; }

  void setFunc(llvm::Function *f) { func = f; }
  void setBlock(llvm::BasicBlock *b) { block = b; }
  void setValue(llvm::Value *v) { value = v; }

  /// Sets current debug info based on a given node.
  /// @param node the node whose debug info to use
  void setDebugInfoForNode(const Node *node);

  /// Compiles a given IR node, updating the internal
  /// LLVM value and/or function as a result.
  /// @param node the node to compile
  void process(const Node *node);

  /// Performs LLVM's module verification on the contained module.
  /// Causes an assertion failure if verification fails.
  void verify();
  /// Dumps the unoptimized module IR to a file.
  /// @param filename name of file to write IR to
  void dump(const std::string &filename = "_dump.ll");
  /// Writes module as native object file.
  /// @param filename the .o file to write to
  void writeToObjectFile(const std::string &filename);
  /// Writes module as LLVM bitcode file.
  /// @param filename the .bc file to write to
  void writeToBitcodeFile(const std::string &filename);
  /// Writes module as LLVM IR file.
  /// @param filename the .ll file to write to
  void writeToLLFile(const std::string &filename, bool optimize = true);
  /// Writes module as native executable. Invokes an
  /// external linker to generate the final executable.
  /// @param filename the file to write to
  /// @param libs library names to link
  void writeToExecutable(const std::string &filename,
                         const std::vector<std::string> &libs = {});
  /// Runs optimization passes on module and writes the result
  /// to the specified file. The output type is determined by
  /// the file extension (.ll for LLVM IR, .bc for LLVM bitcode
  /// .o or .obj for object file, other for executable).
  /// @param filename name of the file to write to
  /// @param libs library names to link to, if creating executable
  void compile(const std::string &filename, const std::vector<std::string> &libs = {});
  /// Runs optimization passes on module and executes it.
  /// @param args vector of arguments to program
  /// @param libs vector of libraries to load
  /// @param envp program environment
  void run(const std::vector<std::string> &args = {},
           const std::vector<std::string> &libs = {},
           const char *const *envp = nullptr);

  /// Get LLVM type from IR type
  /// @param t the IR type
  /// @return corresponding LLVM type
  llvm::Type *getLLVMType(types::Type *t);
  /// Get the LLVM debug info type from the IR type
  /// @param t the IR type
  /// @return corresponding LLVM DI type
  llvm::DIType *getDIType(types::Type *t);

  LoopData *getLoopData(id_t loopId);

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
} // namespace seq
