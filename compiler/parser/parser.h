/*
 * parser.h --- Seq AST parser.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "sir/sir.h"
#include "util/common.h"

namespace seq {

seq::ir::Module *parse(const std::string &argv0, const std::string &file,
                       const std::string &code = "", bool isCode = false,
                       int isTest = 0, int startLine = 0,
                       const std::unordered_map<std::string, std::string> &defines =
                           std::unordered_map<std::string, std::string>{});

void generateDocstr(const std::string &argv0);

} // namespace seq
