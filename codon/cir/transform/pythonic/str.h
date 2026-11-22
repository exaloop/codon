// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {

/// Pass to optimize str1 + str2 + ...
class StrAdditionOptimization : public OperatorPass {
public:
  static CODON_API const std::string KEY;
  std::string getKey() const override { return KEY; }
  void handle(CallInstr *v) override;
};

class StrAppendOptimization : public Pass {
private:
  std::string reachingKey;
  std::string dominatorKey;

public:
  static CODON_API const std::string KEY;
  StrAppendOptimization(std::string reachingKey, std::string dominatorKey)
      : reachingKey(std::move(reachingKey)), dominatorKey(std::move(dominatorKey)) {}
  std::string getKey() const override { return KEY; }
  bool shouldRun(Module *module) const override;
  void run(Module *module) override;
};

} // namespace pythonic
} // namespace transform
} // namespace ir
} // namespace codon
