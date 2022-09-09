#pragma once

#include "codon/sir/value.h"

namespace codon {
namespace ir {

class Value;

namespace transform {
namespace parallel {

struct OMPSched {
  int code;
  bool dynamic;
  Value *threads;
  Value *chunk;
  bool ordered;
  int64_t collapse;

  explicit OMPSched(int code = -1, bool dynamic = false, Value *threads = nullptr,
                    Value *chunk = nullptr, bool ordered = false, int64_t collapse = 0);
  explicit OMPSched(const std::string &code, Value *threads = nullptr,
                    Value *chunk = nullptr, bool ordered = false, int64_t collapse = 0);
  OMPSched(const OMPSched &s)
      : code(s.code), dynamic(s.dynamic), threads(s.threads), chunk(s.chunk),
        ordered(s.ordered), collapse(s.collapse) {}

  std::vector<Value *> getUsedValues() const;
  int replaceUsedValue(id_t id, Value *newValue);
};

} // namespace parallel
} // namespace transform
} // namespace ir
} // namespace codon
