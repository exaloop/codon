// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <unordered_map>

#include "codon/cir/analyze/analysis.h"

namespace codon {
namespace ir {
namespace analyze {
namespace module {

struct GlobalVarsResult : public Result {
  std::unordered_map<id_t, id_t> assignments;
  explicit GlobalVarsResult(std::unordered_map<id_t, id_t> assignments)
      : assignments(std::move(assignments)) {}
};

class GlobalVarsAnalyses : public Analysis {
  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  std::unique_ptr<Result> run(const Module *m) override;
};

} // namespace module
} // namespace analyze
} // namespace ir
} // namespace codon
