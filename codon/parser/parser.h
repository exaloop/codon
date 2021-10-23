#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "codon/dsl/plugins.h"
#include "codon/sir/sir.h"
#include "codon/util/common.h"

namespace codon {

codon::ir::Module *parse(const std::string &argv0, const std::string &file,
                         const std::string &code = "", bool isCode = false,
                         int isTest = 0, int startLine = 0,
                         const std::unordered_map<std::string, std::string> &defines =
                             std::unordered_map<std::string, std::string>{},
                         PluginManager *plm = nullptr);

void generateDocstr(const std::string &argv0);
int jitLoop(const std::string &argv0);

} // namespace codon
