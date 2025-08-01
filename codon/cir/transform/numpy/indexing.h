// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "codon/cir/transform/pass.h"
#include "codon/cir/types/types.h"

namespace codon {
namespace ir {
namespace transform {
namespace numpy {

/// NumPy bounds check elision pass
class NumPyBoundsCheckElisionPass : public OperatorPass {
private:
  /// Key of the reaching definition analysis
  std::string reachingDefKey;

public:
  static const std::string KEY;

  /// Constructs a NumPy bounds check elision pass.
  /// @param reachingDefKey the reaching definition analysis' key
  NumPyBoundsCheckElisionPass(const std::string &reachingDefKey)
      : OperatorPass(), reachingDefKey(reachingDefKey) {}

  std::string getKey() const override { return KEY; }
  void visit(ImperativeForFlow *f) override;
};

} // namespace numpy
} // namespace transform
} // namespace ir
} // namespace codon
