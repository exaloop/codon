#pragma once

#include <memory>
#include <unordered_map>

#include "rule.h"

#include "sir/transform/pass.h"

namespace seq {
namespace ir {
namespace transform {
namespace folding {

class FoldingPass : public OperatorPass, public Rewriter {
public:
  /// Constructs a folding pass.
  FoldingPass() : OperatorPass(/*childrenFirst=*/true) {}

  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  void run(Module *m) override;
  void handle(CallInstr *v) override;

private:
  void registerStandardRules(Module *m);
};

} // namespace folding
} // namespace transform
} // namespace ir
} // namespace seq
