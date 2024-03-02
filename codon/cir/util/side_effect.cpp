// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "side_effect.h"

namespace codon {
namespace ir {
namespace util {

const std::string NON_PURE_ATTR = "std.internal.attributes.nonpure";
const std::string PURE_ATTR = "std.internal.attributes.pure";
const std::string NO_SIDE_EFFECT_ATTR = "std.internal.attributes.no_side_effect";
const std::string NO_CAPTURE_ATTR = "std.internal.attributes.nocapture";
const std::string DERIVES_ATTR = "std.internal.attributes.derives";
const std::string SELF_CAPTURES_ATTR = "std.internal.attributes.self_captures";

} // namespace util
} // namespace ir
} // namespace codon
