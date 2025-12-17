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

const std::string AwaitLowering::KEY = "core-await-lowering";

void AwaitLowering::handle(AwaitInstr *v) {
  auto *M = v->getModule();
  auto *value = v->getValue();
  auto *type = v->getType();
  auto *futureType = type; // M->getOrRealizeType(ast::getMangledClass("std.asyncio",
                           // "Future"), {type});
  seqassertn(futureType, "future type not found");

  auto *coro = M->Nr<CoroHandleInstr>();
  auto *addCallback = M->getOrRealizeMethod(futureType, "_add_done_callback",
                                            {futureType, coro->getType()});
  seqassertn(addCallback, "add-callback method not found");
  auto *getResult = M->getOrRealizeMethod(futureType, "result", {futureType});
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
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
