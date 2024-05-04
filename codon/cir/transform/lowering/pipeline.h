// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {

/// Converts pipelines to for-loops
class PipelineLowering : public OperatorPass {
public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void handle(PipelineFlow *v) override;
};

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
