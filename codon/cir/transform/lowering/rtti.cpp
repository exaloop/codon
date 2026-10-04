// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/cir/transform/lowering/rtti.h"

#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {

const std::string RTTILowering::KEY = "core-rtti-lowering";

void RTTILowering::handle(CallInstr *call) {
  if (!call->getThunkID())
    return;
  auto *module = call->getModule();
  auto *parent = cast<BodiedFunc>(getParentFunc());
  seqassertn(parent && call->numArgs(), "virtual call requires a receiver");
  auto *receiver = call->front();
  auto *temporary =
      module->Nr<Var>(receiver->getType(), false, false, false, "rtti.self");
  parent->push_back(temporary);
  auto *lookup = module->Nr<InternalFunc>("rtti.lookup");
  lookup->setIntrinsic(InternalFunc::Intrinsic::VIRTUAL_LOOKUP);
  lookup->setGlobal();
  lookup->realize(
      cast<FuncType>(module->unsafeGetFuncType(
          "rtti.lookup." + call->getCallee()->getType()->getName(),
          call->getCallee()->getType(), {receiver->getType(), module->getIntType()})),
      {"self", "thunk"});
  auto *prefix = module->Nr<SeriesFlow>();
  prefix->push_back(call->getCallee());
  prefix->push_back(module->Nr<AssignInstr>(temporary, receiver));
  auto *target = module->Nr<FlowInstr>(
      prefix, util::call(lookup, {module->Nr<VarValue>(temporary),
                                  module->getInt(call->getThunkID())}));
  std::vector<Value *> arguments(call->begin(), call->end());
  arguments[0] = module->Nr<VarValue>(temporary);
  call->replaceAll(module->N<CallInstr>(call, target, arguments, call->getName()));
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
