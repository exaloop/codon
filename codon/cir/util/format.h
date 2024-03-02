// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <iostream>

#include "codon/cir/cir.h"

namespace codon {
namespace ir {
namespace util {

/// Formats an IR node.
/// @param node the node
/// @return the formatted node
std::string format(const Node *node);

/// Formats an IR node to an IO stream.
/// @param os the output stream
/// @param node the node
/// @return the resulting output stream
std::ostream &format(std::ostream &os, const Node *node);

} // namespace util
} // namespace ir
} // namespace codon
