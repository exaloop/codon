// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/cir.h"
#include "codon/cir/transform/manager.h"
#include "codon/cir/transform/pass.h"
#include "codon/parser/cache.h"
#include "llvm/Passes/PassBuilder.h"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace codon {

/// Base class for DSL plugins. Plugins will return an instance of
/// a child of this class, which defines various characteristics of
/// the DSL, like keywords and IR passes.
class DSL {
public:
  /// General information about this plugin.
  struct Info {
    /// Extension name
    std::string name;
    /// Extension description
    std::string description;
    /// Extension version
    std::string version;
    /// Extension URL
    std::string url;
    /// Supported Codon versions (semver range)
    std::string supported;
    /// Plugin stdlib path
    std::string stdlibPath;
    /// Legacy library path, used for both compiler and runtime by default.
    std::string dylibPath;
    /// Linker arguments (to replace the default runtime library argument if present).
    std::vector<std::string> linkArgs;
    /// Compiler library: unset inherits dylibPath; empty disables loading.
    std::optional<std::string> compilerDylibPath;
    /// Runtime library: unset inherits dylibPath; empty disables default linking.
    std::optional<std::string> runtimeDylibPath;

    const std::string &getCompilerDylibPath() const {
      return compilerDylibPath ? *compilerDylibPath : dylibPath;
    }
    const std::string &getRuntimeDylibPath() const {
      return runtimeDylibPath ? *runtimeDylibPath : dylibPath;
    }
  };

  using KeywordCallback =
      std::function<ast::Stmt *(ast::TypecheckVisitor *, ast::CustomStmt *)>;

  struct ExprKeyword {
    std::string keyword;
    KeywordCallback callback;
  };

  struct BlockKeyword {
    std::string keyword;
    KeywordCallback callback;
    bool hasExpr;
  };

  virtual ~DSL() noexcept = default;

  /// Registers this DSL's IR passes with the given pass manager.
  /// @param pm the pass manager to add the passes to
  /// @param debug true if compiling in debug mode
  virtual void addIRPasses(ir::transform::PassManager *pm, bool debug) {}

  /// Registers this DSL's LLVM passes with the given pass builder.
  /// Called before analysis registration and pipeline construction, allowing
  /// plugins to register analyses as well as passes.
  /// @param pb the pass builder to add the passes to
  /// @param debug true if compiling in debug mode
  virtual void addLLVMPasses(llvm::PassBuilder *pb, bool debug) {}

  /// Returns a vector of "expression keywords", defined as keywords of
  /// the form "keyword <expr>".
  /// @return this DSL's expression keywords
  virtual std::vector<ExprKeyword> getExprKeywords() { return {}; }

  /// Returns a vector of "block keywords", defined as keywords of the
  /// form "keyword <expr>: <block of code>".
  /// @return this DSL's block keywords
  virtual std::vector<BlockKeyword> getBlockKeywords() { return {}; }
};

} // namespace codon
