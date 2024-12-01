// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <vector>

namespace codon {
int startJupyterKernel(const std::string &argv0,
                       const std::vector<std::string> &plugins,
                       const std::string &configPath);
} // namespace codon
