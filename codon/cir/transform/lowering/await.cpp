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

bool isCoroutine(const types::Type *type) { return isA<types::GeneratorType>(type); }

} // namespace

const std::string AwaitLowering::KEY = "core-await-lowering";

void AwaitLowering::handle(AwaitInstr *v) {
  auto *M = v->getModule();
  auto *value = v->getValue();
  auto *resultType = v->getType();
  auto *valueType = value->getType();

  if (isFuture(valueType)) {
    auto *coro = M->Nr<CoroHandleInstr>();
    auto *addCallback = M->getOrRealizeMethod(valueType, "_add_done_callback",
                                              {valueType, coro->getType()});
    seqassertn(addCallback, "add-callback method not found");
    auto *getResult = M->getOrRealizeMethod(valueType, "result", {valueType});
    seqassertn(getResult, "get-result method not found");

    util::CloneVisitor cv(M);
    auto *series = M->Nr<SeriesFlow>();
    auto *futureVar =
        util::makeVar(cv.clone(value), series, cast<BodiedFunc>(getParentFunc()));
    series->push_back(util::call(addCallback, {M->Nr<VarValue>(futureVar), coro}));
    series->push_back(M->Nr<YieldInstr>());

    auto *replacement =
        M->Nr<FlowInstr>(series, util::call(getResult, {M->Nr<VarValue>(futureVar)}));
    v->replaceAll(replacement);
  } else if (isTask(valueType)) {
    seqassertn(false, "TODO");
  } else if (isCoroutine(valueType)) {
    seqassertn(false, "TODO");
  } else {
    seqassertn(false, "unexpected value type '{}' in await instruction",
               valueType->getName());
  }
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
