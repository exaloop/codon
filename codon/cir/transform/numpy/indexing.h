// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/cir/transform/numpy/numpy.h"
#include "codon/cir/type.h"

namespace codon {
namespace ir {
namespace transform {
namespace numpy {

/// NumPy bounds check elision pass
class NumPyBoundsCheckElisionPass : public NumPyPass {
private:
  /// Key of the reaching definition analysis
  std::string reachingDefKey;

public:
  static const std::string KEY;

  /// Constructs a NumPy bounds check elision pass.
  /// @param reachingDefKey the reaching definition analysis' key
  NumPyBoundsCheckElisionPass(const std::string &reachingDefKey)
      : NumPyPass(), reachingDefKey(reachingDefKey) {}

  std::string getKey() const override { return KEY; }
  void visit(ImperativeForFlow *f) override;
};

} // namespace numpy
} // namespace transform
} // namespace ir
} // namespace codon
