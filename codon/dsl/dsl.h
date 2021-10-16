#pragma once

#include "codon/parser/cache.h"
#include "codon/sir/sir.h"
#include "codon/sir/transform/manager.h"
#include "codon/sir/transform/pass.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <functional>
#include <string>
#include <vector>

namespace codon {

/// Base class for DSL plugins. Plugins will return an instance of
/// a child of this class, which defines various characteristics of
/// the DSL, like keywords and IR passes.
class DSL {
public:
  using KeywordCallback =
      std::function<ast::StmtPtr(ast::SimplifyVisitor *, ast::CustomStmt *)>;

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

  /// @return the name of this DSL
  virtual std::string getName() const = 0;

  /// @param major major version number (i.e. X in X.Y.Z)
  /// @param minor minor version number (i.e. Y in X.Y.Z)
  /// @param patch patch version number (i.e. Z in X.Y.Z)
  /// @return true if the given major, minor and patch versions are supported
  virtual bool isVersionSupported(unsigned major, unsigned minor, unsigned patch) {
    return true;
  }

  /// Registers this DSL's IR passes with the given pass manager.
  /// @param pm the pass manager to add the passes to
  /// @param debug true if compiling in debug mode
  virtual void addIRPasses(ir::transform::PassManager *pm, bool debug) {}

  /// Registers this DSL's LLVM passes with the given pass manager builder.
  /// @param pmb the pass manager builder to add the passes to
  /// @param debug true if compining in debug mode
  virtual void addLLVMPasses(llvm::PassManagerBuilder *pmb, bool debug) {}

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
