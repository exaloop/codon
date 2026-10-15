// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {

/// Lowers virtual calls to explicit runtime method lookups.
class RTTILowering : public OperatorPass {
public:
  /// unique pass identifier
  static const std::string KEY;
  /// @return the pass identifier
  std::string getKey() const override { return KEY; }
  /// Lowers a call with a nonzero thunk ID, leaving ordinary calls unchanged.
  /// Virtual calls must have a receiver as their first argument and belong to a
  /// bodied function.
  /// @param call the call instruction
  void handle(CallInstr *call) override;
};

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
