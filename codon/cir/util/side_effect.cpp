// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "side_effect.h"
#include "codon/parser/common.h"

namespace codon {
namespace ir {
namespace util {

const std::string NON_PURE_ATTR =
    ast::getMangledFunc("std.internal.attributes", "nonpure");
const std::string PURE_ATTR = ast::getMangledFunc("std.internal.core", "pure");
const std::string NO_SIDE_EFFECT_ATTR =
    ast::getMangledFunc("std.internal.attributes", "no_side_effect");
const std::string NO_CAPTURE_ATTR =
    ast::getMangledFunc("std.internal.attributes", "nocapture");
const std::string DERIVES_ATTR = ast::getMangledFunc("std.internal.core", "derives");
const std::string SELF_CAPTURES_ATTR =
    ast::getMangledFunc("std.internal.attributes", "self_captures");

} // namespace util
} // namespace ir
} // namespace codon
