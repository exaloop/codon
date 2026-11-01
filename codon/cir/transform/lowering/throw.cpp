// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "throw.h"

#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {

const std::string ThrowLowering::KEY = "core-throw-lowering";

void ThrowLowering::handle(ThrowInstr *value) {
  if (value->getValue() || value->getFallback())
    return;
  auto *module = value->getModule();
  auto *fallback = module->getOrRealizeFunc("_raise_no_active_exception", {}, {},
                                            "std.internal.types.error");
  seqassertn(fallback, "empty-raise fallback not found");
  value->setFallback(util::call(fallback, {}));
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
