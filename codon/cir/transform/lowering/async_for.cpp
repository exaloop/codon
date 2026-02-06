// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "async_for.h"

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {

const std::string AsyncForLowering::KEY = "core-async-for-lowering";

void AsyncForLowering::handle(ForFlow *v) {
  if (!v->isAsync())
    return;

  auto *M = v->getModule();
  auto *coro = v->getIter();
  auto *coroType = coro->getType();
  util::CloneVisitor cv(M);

  auto *promise = M->getOrRealizeFunc("_promise", {coroType}, {}, "std.asyncio");
  seqassertn(promise, "promise func not found");
  auto *done = M->getOrRealizeFunc("_done", {coroType}, {}, "std.asyncio");
  seqassertn(done, "done func not found");
  auto *resume = M->getOrRealizeFunc("_resume", {coroType}, {}, "std.asyncio");
  seqassertn(resume, "resume func not found");
  auto *currTask = M->getOrRealizeFunc("_curr_task", {}, {}, "std.asyncio");
  seqassertn(currTask, "curr-task func not found");
  auto *waiting = M->getOrRealizeFunc("_is_waiting", {util::getReturnType(currTask)},
                                      {}, "std.asyncio");
  seqassertn(waiting, "is-waiting func not found");

  // Construct the following:
  //   task = curr_task()
  //   if not coro.__done__():
  //       while True:
  //           coro.__resume__()
  //           if coro.__done__():
  //               break
  //           if is_waiting(task):
  //               yield
  //           else:
  //               i = coro.__promise__()
  //               <BODY>
  auto *series = M->Nr<SeriesFlow>();
  auto *taskVar = util::makeVar(util::call(currTask, {}), series,
                                cast<BodiedFunc>(getParentFunc()));
  auto *coroVar =
      util::makeVar(cv.clone(coro), series, cast<BodiedFunc>(getParentFunc()));

  series->push_back(M->Nr<IfFlow>(
      ~*util::call(done, {M->Nr<VarValue>(coroVar)}),
      util::series(M->Nr<WhileFlow>(
          M->getBool(true),
          util::series(
              util::call(resume, {M->Nr<VarValue>(coroVar)}),
              M->Nr<IfFlow>(util::call(done, {M->Nr<VarValue>(coroVar)}),
                            util::series(M->Nr<BreakInstr>())),
              M->Nr<IfFlow>(
                  util::call(waiting, {M->Nr<VarValue>(taskVar)}),
                  util::series(M->Nr<YieldInstr>()),
                  util::series(
                      M->Nr<AssignInstr>(
                          v->getVar(), util::call(promise, {M->Nr<VarValue>(coroVar)})),
                      cv.clone(v->getBody()))))))));

  v->replaceAll(series);
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
