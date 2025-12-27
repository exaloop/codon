// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "await.h"

#include <algorithm>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {
namespace {

bool isFuture(const types::Type *type) {
  return type->getName().rfind("std.asyncio.Future.", 0) == 0;
}

bool isTask(const types::Type *type) {
  return type->getName().rfind("std.asyncio.Task.", 0) == 0;
}

const types::GeneratorType *isCoroutine(const types::Type *type) {
  return cast<types::GeneratorType>(type);
}

} // namespace

const std::string AwaitLowering::KEY = "core-await-lowering";

void AwaitLowering::handle(AwaitInstr *v) {
  auto *M = v->getModule();
  auto *value = v->getValue();
  auto *resultType = v->getType();
  auto *valueType = value->getType();
  util::CloneVisitor cv(M);

  if (isFuture(valueType) || isTask(valueType)) {
    auto *coro = M->Nr<CoroHandleInstr>();
    auto *addCallback = M->getOrRealizeMethod(valueType, "_add_done_callback",
                                              {valueType, coro->getType()});
    seqassertn(addCallback, "add-callback method not found");
    auto *getResult = M->getOrRealizeMethod(valueType, "result", {valueType});
    seqassertn(getResult, "get-result method not found");

    auto *series = M->Nr<SeriesFlow>();
    auto *futureVar =
        util::makeVar(cv.clone(value), series, cast<BodiedFunc>(getParentFunc()));
    series->push_back(util::call(addCallback, {M->Nr<VarValue>(futureVar), coro}));
    series->push_back(M->Nr<YieldInstr>());

    auto *replacement =
        M->Nr<FlowInstr>(series, util::call(getResult, {M->Nr<VarValue>(futureVar)}));
    v->replaceAll(replacement);
  } else if (auto *genType = isCoroutine(valueType)) {
    auto *var = M->Nr<Var>(genType->getBase(), /*global=*/false);
    cast<BodiedFunc>(getParentFunc())->push_back(var);
    auto *replacement = M->Nr<ForFlow>(cv.clone(value), M->Nr<SeriesFlow>(), var);
    v->replaceAll(replacement);
  } else {
    seqassertn(false, "unexpected value type '{}' in await instruction",
               valueType->getName());
  }
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
