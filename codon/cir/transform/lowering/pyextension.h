// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <utility>
#include <vector>

#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {

class PythonExtensionLowering : public Pass {
public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  void run(Module *module) override;
};

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
