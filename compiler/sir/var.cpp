#include "var.h"

#include "module.h"

namespace seq {
namespace ir {

const char Var::NodeId = 0;

int Var::doReplaceUsedType(const std::string &name, types::Type *newType) {
  if (type->getName() == name) {
    type = newType;
    return 1;
  }
  return 0;
}

const char VarValue::NodeId = 0;

int VarValue::doReplaceUsedVariable(id_t id, Var *newVar) {
  if (val->getId() == id) {
    val = newVar;
    return 1;
  }
  return 0;
}

const char PointerValue::NodeId = 0;

types::Type *PointerValue::doGetType() const {
  return getModule()->getPointerType(val->getType());
}

int PointerValue::doReplaceUsedVariable(id_t id, Var *newVar) {
  if (val->getId() == id) {
    val = newVar;
    return 1;
  }
  return 0;
}

} // namespace ir
} // namespace seq
