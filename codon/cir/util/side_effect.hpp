// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>

namespace codon {
namespace ir {
namespace util {

/// Function side effect status. "Pure" functions by definition give the same
/// output for the same inputs and have no side effects. "No side effect"
/// functions have no side effects, but can give different outputs for the
/// same input (e.g. time() is one such function). "No capture" functions do
/// not capture any of their arguments; note that capturing an argument is
/// considered a side effect. Therefore, we have pure < no_side_effect <
/// no_capture < unknown, where "<" denotes subset. The enum values are also
/// ordered in this way, which is relied on by the implementation.
enum SideEffectStatus {
  PURE = 0,
  NO_SIDE_EFFECT,
  NO_CAPTURE,
  UNKNOWN,
};

extern const std::string NON_PURE_ATTR;
extern const std::string PURE_ATTR;
extern const std::string NO_SIDE_EFFECT_ATTR;
extern const std::string NO_CAPTURE_ATTR;
extern const std::string DERIVES_ATTR;
extern const std::string SELF_CAPTURES_ATTR;

} // namespace util
} // namespace ir
} // namespace codon
