#pragma once

#include "sir/transform/pass.h"

namespace seq {
namespace ir {
namespace transform {
namespace lowering {

class ImperativeForFlowLowering : public OperatorPass {
public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void handle(ForFlow *v) override;
};

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace seq
