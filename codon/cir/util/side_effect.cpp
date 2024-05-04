// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "side_effect.h"

namespace codon {
namespace ir {
namespace util {

const std::string NON_PURE_ATTR = "std.internal.attributes.nonpure.0:0";
const std::string PURE_ATTR = "std.internal.attributes.pure.0:0";
const std::string NO_SIDE_EFFECT_ATTR = "std.internal.attributes.no_side_effect.0:0";
const std::string NO_CAPTURE_ATTR = "std.internal.attributes.nocapture.0:0";
const std::string DERIVES_ATTR = "std.internal.attributes.derives.0:0";
const std::string SELF_CAPTURES_ATTR = "std.internal.attributes.self_captures.0:0";

} // namespace util
} // namespace ir
} // namespace codon
