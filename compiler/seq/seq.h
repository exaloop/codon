#pragma once

#include "dsl/dsl.h"

namespace seq {

class Seq : public DSL {
public:
  std::string getName() const override { return "Seq"; }
  void addIRPasses(ir::transform::PassManager *pm, bool debug) override;
};

} // namespace seq
