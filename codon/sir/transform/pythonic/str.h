#pragma once

#include "codon/sir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {

/// Pass to optimize str1 + str2 + ...
class StrAdditionOptimization : public OperatorPass {
public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void handle(CallInstr *v) override;
};

} // namespace pythonic
} // namespace transform
} // namespace ir
} // namespace codon
