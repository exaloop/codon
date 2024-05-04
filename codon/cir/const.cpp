// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "const.h"

namespace codon {
namespace ir {

const char Const::NodeId = 0;

int Const::doReplaceUsedType(const std::string &name, types::Type *newType) {
  if (type->getName() == name) {
    type = newType;
    return 1;
  }
  return 0;
}

const char TemplatedConst<std::string>::NodeId = 0;

} // namespace ir
} // namespace codon
