#pragma once

#include "codon/sir/analyze/dataflow/reaching.h"
#include "codon/sir/sir.h"

namespace codon {
namespace ir {
namespace util {

enum EscapeResult {
  NO = 0,
  YES,
  RETURNED,
};

EscapeResult escapes(BodiedFunc *parent, Value *value,
                     analyze::dataflow::RDResult *reaching);

} // namespace util
} // namespace ir
} // namespace codon
