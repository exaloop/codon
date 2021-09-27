#pragma once

#include "sir/sir.h"
#include "sir/transform/manager.h"
#include "sir/transform/pass.h"
#include <functional>
#include <string>
#include <vector>

namespace seq {

/// Base class for DSL plugins. Plugins will return an instance of
/// a child of this class, which defines various characteristics of
/// the DSL, like keywords and IR passes.
class DSL {
public:
  /// Represents a keyword, consisting of the keyword
  /// name (e.g. "if", "while", etc.) and a callback
  /// to generate the resulting IR node.
  template <typename Callback> struct Keyword {
    /// keyword name
    std::string name;
    /// callback to produce IR node
    Callback callback;
  };

  using ExprKeywordCallback =
      std::function<ir::Node *(ir::Module *M, const std::vector<ir::Value *> &values)>;
  using BlockKeywordCallback = std::function<ir::Node *(
      ir::Module *M, const std::vector<ir::Value *> &values, ir::SeriesFlow *block)>;
  using BinaryKeywordCallback =
      std::function<ir::Node *(ir::Module *M, ir::Value *lhs, ir::Value *rhs)>;

  using ExprKeyword = Keyword<ExprKeywordCallback>;
  using BlockKeyword = Keyword<BlockKeywordCallback>;
  using BinaryKeyword = Keyword<BinaryKeywordCallback>;

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

  /// Returns a vector of "expression keywords", defined as keywords of
  /// the form "keyword <expr1> ... <exprN>".
  /// @return this DSL's expression keywords
  virtual std::vector<ExprKeyword> getExprKeywords() { return {}; }

  /// Returns a vector of "block keywords", defined as keywords of the
  /// form "keyword <expr1> ... <exprN>: <block of code>".
  /// @return this DSL's block keywords
  virtual std::vector<BlockKeyword> getBlockKeywords() { return {}; }

  /// Returns a vector of "binary keywords", defined as keywords of the
  /// form "<expr1> keyword <expr2>".
  /// @return this DSL's binary keywords
  virtual std::vector<BinaryKeyword> getBinaryKeywords() { return {}; }
};

} // namespace seq
