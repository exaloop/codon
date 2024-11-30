// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <vector>

namespace codon {
namespace ir {
namespace util {

/// Utility function to strip parameter packs.
/// @param dst the destination vector
/// @param first the value
template <typename Desired>
void stripPack(std::vector<Desired *> &dst, Desired &first) {
  dst.push_back(&first);
}

/// Utility function to strip parameter packs.
/// @param dst the destination vector
template <typename Desired> void stripPack(std::vector<Desired *> &dst) {}

/// Utility function to strip parameter packs.
/// @param dst the destination vector
/// @param first the value
/// @param args the argument pack
template <typename Desired, typename... Args>
void stripPack(std::vector<Desired *> &dst, Desired &first, Args &&...args) {
  dst.push_back(&first);
  stripPack<Desired>(dst, std::forward<Args>(args)...);
}

} // namespace util
} // namespace ir
} // namespace codon
