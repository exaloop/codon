// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

namespace codon::ast {

/// Shared symbolic attribute lookup for AST and CIR dumps. Unknown keys return null.
const char *attributeName(int key);

} // namespace codon::ast
