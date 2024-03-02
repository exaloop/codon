// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/transform/pass.h"

namespace codon {
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
} // namespace codon
