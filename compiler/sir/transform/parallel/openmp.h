#pragma once

#include "sir/transform/pass.h"

namespace seq {
namespace ir {
namespace transform {
namespace parallel {

class OpenMPPass : public OperatorPass {
public:
  /// Constructs an OpenMP pass.
  OpenMPPass() : OperatorPass(/*childrenFirst=*/true) {}

  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  void handle(ForFlow *) override;
  void handle(ImperativeForFlow *) override;
};

} // namespace parallel
} // namespace transform
} // namespace ir
} // namespace seq
