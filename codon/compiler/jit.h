// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/cir/llvm/llvisitor.h"
#include "codon/cir/transform/manager.h"
#include "codon/cir/var.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/engine.h"
#include "codon/compiler/error.h"
#include "codon/parser/cache.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/typecheck.h"
#include "codon/runtime/lib.h"

#include "codon/compiler/jit_extern.h"

namespace codon {
namespace jit {

class JITState {
  ast::Cache *cache;
  bool forgetful;

  ast::Cache bCache;
  ast::TypeContext mainCtx;
  ast::TypeContext stdlibCtx;
  ast::TypeContext typeCtx;
  ast::TranslateContext translateCtx;

public:
  explicit JITState(ast::Cache *cache, bool forgetful = false);

  void undo();
  void undoUnusedIR();
  void cleanUpRealizations();
};

class JIT {
public:
  struct PythonData {
    ir::types::Type *cobj;
    std::unordered_map<std::string, ir::Func *> cache;

    PythonData();
    ir::types::Type *getCObjType(ir::Module *M);
  };

  struct JITClassData {
    ir::types::Type *cobj;
    // Cache generated wrappers by native class, method name, and argument types.
    std::unordered_map<std::string, ir::Func *> constructorWrappers;
    std::unordered_map<std::string, ir::Func *> methodWrappers;

    JITClassData();
    ir::types::Type *getCObjType(ir::Module *M);
  };

  // Shared liveness token used by native instances to detect stale JIT contexts.
  struct JITContextState {
    std::atomic<bool> alive;

    JITContextState() : alive(true) {}
  };

  // RAII root for Codon GC-managed objects exposed through Python-owned instances.
  struct JITClassRoot {
    std::unique_ptr<void *> slot;

    explicit JITClassRoot(void *ptr);
    ~JITClassRoot();

    JITClassRoot(JITClassRoot &&) noexcept = default;
    JITClassRoot &operator=(JITClassRoot &&) noexcept;
    JITClassRoot(const JITClassRoot &) = delete;
    JITClassRoot &operator=(const JITClassRoot &) = delete;
  };

  // Opaque native control block owned by the Cython extension type.
  struct JITClassInstance {
    std::shared_ptr<JITContextState> contextState;
    std::string className;
    std::string nativeClassName;
    void *nativePtr;

    // Keeps the underlying Codon object alive while Python can still reach it.
    JITClassRoot root;

    JITClassInstance(std::shared_ptr<JITContextState> contextState,
                     std::string className, std::string nativeClassName,
                     void *nativePtr);

    JITClassInstance(JITClassInstance &&) noexcept = default;
    JITClassInstance &operator=(JITClassInstance &&) noexcept = default;
    JITClassInstance(const JITClassInstance &) = delete;
    JITClassInstance &operator=(const JITClassInstance &) = delete;
  };

  struct JITResult {
    void *result;
    std::string message;

    operator bool() const { return message.empty(); }
    static JITResult success(void *result = nullptr) { return {result, ""}; }
    static JITResult error(const std::string &message) { return {nullptr, message}; }
  };

private:
  std::unique_ptr<Compiler> compiler;
  std::unique_ptr<Engine> engine;
  std::unique_ptr<PythonData> pydata;
  std::unique_ptr<JITClassData> jitClassData;
  std::shared_ptr<JITContextState> contextState;
  std::string mode;
  bool forgetful = false;

public:
  explicit JIT(const std::string &argv0, const std::string &mode = "",
               const std::string &stdlibRoot = "");
  ~JIT();

  Compiler *getCompiler() const { return compiler.get(); }
  Engine *getEngine() const { return engine.get(); }

  // General
  llvm::Error init(bool forgetful = false);
  llvm::Error compile(const ir::Func *input, llvm::orc::ResourceTrackerSP rt = nullptr);
  llvm::Expected<ir::Func *> compile(const std::string &code,
                                     const std::string &file = "", int line = 0);
  llvm::Expected<void *> address(const ir::Func *input,
                                 llvm::orc::ResourceTrackerSP rt = nullptr);
  llvm::Expected<std::string> run(const ir::Func *input,
                                  llvm::orc::ResourceTrackerSP rt = nullptr);
  llvm::Expected<std::string> execute(const std::string &code,
                                      const std::string &file = "", int line = 0,
                                      bool debug = false,
                                      llvm::orc::ResourceTrackerSP rt = nullptr);

  // Python
  llvm::Expected<void *> runPythonWrapper(const ir::Func *wrapper, void *arg);
  llvm::Expected<ir::Func *> getWrapperFunc(const std::string &name,
                                            const std::vector<std::string> &types);
  JITResult executePython(const std::string &name,
                          const std::vector<std::string> &types,
                          const std::string &pyModule,
                          const std::vector<std::string> &pyVars, void *arg,
                          bool debug);
  JITResult executeSafe(const std::string &code, const std::string &file, int line,
                        bool debug);
  // Create a native jitclass object and return its opaque control block.
  JITResult jitClassNew(const std::string &className,
                        const std::string &nativeClassName,
                        const std::vector<std::string> &types, void *args, bool debug);
  // Dispatch a method or generated field accessor on an existing native instance.
  JITResult jitClassCall(const std::string &className, JITClassInstance *instance,
                         const std::string &methodName,
                         const std::vector<std::string> &types, void *args, bool debug);
  // Release an opaque jitclass control block; safe to call without a live JIT.
  static JITResult jitClassRelease(JITClassInstance *instance);

  // Errors
  llvm::Error handleJITError(const runtime::JITError &e);
};

} // namespace jit
} // namespace codon
