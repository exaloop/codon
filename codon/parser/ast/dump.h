// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>

namespace codon::ast {

/// Shared symbolic attribute lookup for AST and CIR dumps. Unknown keys return null.
const char *attributeName(int key);

/// AST/CIR string quoting, retaining UTF-8 and escaping undecodable bytes.
std::string quoteDumpString(const std::string &value);

} // namespace codon::ast
