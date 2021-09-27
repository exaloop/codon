#pragma once

#include "sir/value.h"

namespace seq {
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

  explicit OMPSched(int code = -1, bool dynamic = false, Value *threads = nullptr,
                    Value *chunk = nullptr, bool ordered = false);
  explicit OMPSched(const std::string &code, Value *threads = nullptr,
                    Value *chunk = nullptr, bool ordered = false);
  OMPSched(const OMPSched &s)
      : code(s.code), dynamic(s.dynamic), threads(s.threads), chunk(s.chunk),
        ordered(s.ordered) {}

  std::vector<Value *> getUsedValues() const;
  int replaceUsedValue(id_t id, Value *newValue);
};

} // namespace parallel
} // namespace transform
} // namespace ir
} // namespace seq
