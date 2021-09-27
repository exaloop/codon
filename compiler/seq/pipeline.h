#pragma once

#include "sir/sir.h"
#include "sir/transform/pass.h"

namespace seq {

class PipelineSubstitutionOptimization : public ir::transform::OperatorPass {
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void handle(ir::PipelineFlow *) override;
};

class PipelinePrefetchOptimization : public ir::transform::OperatorPass {
  const unsigned SCHED_WIDTH_PREFETCH = 16;
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void handle(ir::PipelineFlow *) override;
};

class PipelineInterAlignOptimization : public ir::transform::OperatorPass {
  const unsigned SCHED_WIDTH_INTERALIGN = 2048;
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void handle(ir::PipelineFlow *) override;
};

} // namespace seq
