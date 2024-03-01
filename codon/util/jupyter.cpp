// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "codon/util/jupyter.h"
#include <cstdio>

namespace codon {
int startJupyterKernel(const std::string &argv0,
                       const std::vector<std::string> &plugins,
                       const std::string &configPath) {
  fprintf(stderr,
          "Jupyter support not included. Please install Codon Jupyter plugin.\n");
  return EXIT_FAILURE;
}

} // namespace codon
