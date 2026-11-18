// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <functional>
#include <iostream>

#include "codon/cir/cir.h"

namespace codon {
namespace ir {
namespace util {

/// Formats an IR node.
/// @param node the node
/// @param annotateFields print field names, as in Python ast.dump
/// @param includeAttributes include symbolic attributes and their payloads
/// @param indent positive spaces per indentation level; otherwise compact
/// @param types include expression type and frontend type metadata
/// @return the formatted node
std::string format(const Node *node, bool annotateFields = true,
                   bool includeAttributes = false, int indent = -1, bool types = false);

/// Emit the identical dump in chunks, bounding compact-module buffering to a
/// declaration instead of the entire module. Indented/non-module dumps use the
/// regular formatter. The callback must consume its temporary string immediately.
void formatChunks(const Node *node,
                  const std::function<void(const std::string &)> &write,
                  bool annotateFields = true, bool includeAttributes = false,
                  int indent = -1, bool types = false);

/// Formats an IR node to an IO stream.
/// @param os the output stream
/// @param node the node
/// @param annotateFields print field names, as in Python ast.dump
/// @param includeAttributes include symbolic attributes and their payloads
/// @param indent positive spaces per indentation level; otherwise compact
/// @param types include expression type and frontend type metadata
/// @return the resulting output stream
std::ostream &format(std::ostream &os, const Node *node, bool annotateFields = true,
                     bool includeAttributes = false, int indent = -1,
                     bool types = false);

} // namespace util
} // namespace ir
} // namespace codon
