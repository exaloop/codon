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
private:
  /// vector of original function (1st) and generated
  /// extension wrapper (2nd)
  std::vector<std::pair<Func *, Func *>> extFuncs;

public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  /// Constructs a PythonExtensionLowering pass.
  PythonExtensionLowering() : Pass(), extFuncs() {}

  void run(Module *module) override;

  /// @return extension function (original, generated) pairs
  std::vector<std::pair<Func *, Func *>> getExtensionFunctions() const {
    return extFuncs;
  }
};

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
