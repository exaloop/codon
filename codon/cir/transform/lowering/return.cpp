// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "return.h"

#include <deque>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "codon/cir/dsl/nodes.h"
#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {
namespace {

enum class ExitKind { Normal, Return, Throw, Break, Continue };
using Exit = std::pair<ExitKind, id_t>;
using Exits = std::set<Exit>;
constexpr id_t NO_TARGET = -1;

Exits exit(ExitKind kind) { return {{kind, NO_TARGET}}; }

bool has(const Exits &exits, ExitKind kind) { return exits.count({kind, NO_TARGET}); }

void merge(Exits &destination, const Exits &source) {
  destination.insert(source.begin(), source.end());
}

Exits sequence(Exits first, const Exits &second) {
  if (first.erase({ExitKind::Normal, NO_TARGET}))
    merge(first, second);
  return first;
}

class ReturnAnalysis {
  const std::unordered_set<const BodiedFunc *> &nonReturning;

  Exits branch(const Value *condition, const Value *ifTrue, const Value *ifFalse) {
    auto result = analyze(condition);
    if (!has(result, ExitKind::Normal))
      return result;
    Exits alternatives;
    auto *constant = cast<BoolConst>(condition);
    if (!constant || constant->getVal())
      merge(alternatives, analyze(ifTrue));
    if (!constant || !constant->getVal())
      merge(alternatives, analyze(ifFalse));
    return sequence(std::move(result), alternatives);
  }

  Exits loop(const Flow *flow, const Exits &body, bool mayBeEmpty) {
    Exits result;
    if (mayBeEmpty)
      result = exit(ExitKind::Normal);
    for (auto outcome : body) {
      bool local = outcome.second == NO_TARGET || outcome.second == flow->getId();
      if (outcome.first == ExitKind::Normal ||
          (outcome.first == ExitKind::Continue && local))
        continue;
      if (outcome.first == ExitKind::Break && local)
        result.insert({ExitKind::Normal, NO_TARGET});
      else
        result.insert(outcome);
    }
    return result;
  }

public:
  explicit ReturnAnalysis(const std::unordered_set<const BodiedFunc *> &nonReturning)
      : nonReturning(nonReturning) {}

  Exits analyze(const Value *value) {
    if (!value)
      return exit(ExitKind::Normal);
    if (auto *series = cast<SeriesFlow>(value)) {
      auto result = exit(ExitKind::Normal);
      for (auto *child : *series) {
        if (!has(result, ExitKind::Normal))
          break;
        result = sequence(std::move(result), analyze(child));
      }
      return result;
    }
    if (auto *conditional = cast<IfFlow>(value))
      return branch(conditional->getCond(), conditional->getTrueBranch(),
                    conditional->getFalseBranch());
    if (auto *conditional = cast<TernaryInstr>(value))
      return branch(conditional->getCond(), conditional->getTrueValue(),
                    conditional->getFalseValue());
    if (auto *whileLoop = cast<WhileFlow>(value)) {
      auto result = analyze(whileLoop->getCond());
      auto *constant = cast<BoolConst>(whileLoop->getCond());
      if (constant && !constant->getVal())
        return result;
      return sequence(std::move(result),
                      loop(whileLoop, analyze(whileLoop->getBody()), !constant));
    }
    if (auto *forLoop = cast<ForFlow>(value)) {
      auto result = analyze(forLoop->getIter());
      auto outcomes = loop(forLoop, analyze(forLoop->getBody()), true);
      outcomes.insert({ExitKind::Throw, NO_TARGET});
      return sequence(std::move(result), outcomes);
    }
    if (auto *forLoop = cast<ImperativeForFlow>(value)) {
      auto result = sequence(analyze(forLoop->getStart()), analyze(forLoop->getEnd()));
      auto *start = cast<IntConst>(forLoop->getStart());
      auto *end = cast<IntConst>(forLoop->getEnd());
      bool nonempty = start && end &&
                      ((forLoop->getStep() > 0 && start->getVal() < end->getVal()) ||
                       (forLoop->getStep() < 0 && start->getVal() > end->getVal()));
      auto body = analyze(forLoop->getBody());
      auto outcomes = loop(forLoop, body, !nonempty);
      if (forLoop->isParallel())
        outcomes.insert({ExitKind::Throw, NO_TARGET});
      if (has(body, ExitKind::Normal) || has(body, ExitKind::Continue) ||
          body.count({ExitKind::Continue, forLoop->getId()}))
        outcomes.insert({ExitKind::Normal, NO_TARGET});
      return sequence(std::move(result), outcomes);
    }
    if (auto *tryCatch = cast<TryCatchFlow>(value)) {
      auto body = analyze(tryCatch->getBody());
      auto result = sequence(body, analyze(tryCatch->getElse()));
      if (has(body, ExitKind::Throw)) {
        for (const auto &handler : *tryCatch)
          merge(result, analyze(handler.getHandler()));
      }
      if (tryCatch->getFinally() && !result.empty()) {
        auto cleanup = analyze(tryCatch->getFinally());
        if (!cleanup.erase({ExitKind::Normal, NO_TARGET}))
          result.clear();
        merge(result, cleanup);
      }
      return result;
    }
    if (auto *instruction = cast<ReturnInstr>(value))
      return sequence(analyze(instruction->getValue()), exit(ExitKind::Return));
    if (auto *instruction = cast<ThrowInstr>(value))
      return sequence(analyze(instruction->getValue()), exit(ExitKind::Throw));
    if (auto *instruction = cast<BreakInstr>(value))
      return {{ExitKind::Break,
               instruction->getLoop() ? instruction->getLoop()->getId() : NO_TARGET}};
    if (auto *instruction = cast<ContinueInstr>(value))
      return {{ExitKind::Continue,
               instruction->getLoop() ? instruction->getLoop()->getId() : NO_TARGET}};
    if (auto *instruction = cast<YieldInstr>(value)) {
      auto outcomes =
          exit(instruction->isFinal() ? ExitKind::Return : ExitKind::Normal);
      outcomes.insert({ExitKind::Throw, NO_TARGET});
      return sequence(analyze(instruction->getValue()), outcomes);
    }
    if (isA<Flow>(value) || isA<dsl::CustomInstr>(value))
      return {{ExitKind::Normal, NO_TARGET},
              {ExitKind::Return, NO_TARGET},
              {ExitKind::Throw, NO_TARGET},
              {ExitKind::Break, NO_TARGET},
              {ExitKind::Continue, NO_TARGET}};
    auto result = exit(ExitKind::Normal);
    for (auto *operand : value->getUsedValues()) {
      if (!has(result, ExitKind::Normal))
        break;
      result = sequence(std::move(result), analyze(operand));
    }
    if (auto *call = cast<CallInstr>(value)) {
      auto *callee = cast<BodiedFunc>(util::getFunc(call->getCallee()));
      auto outcomes = exit(ExitKind::Throw);
      if (!callee || !nonReturning.count(callee))
        outcomes.insert({ExitKind::Normal, NO_TARGET});
      result = sequence(std::move(result), outcomes);
    } else if (isA<AwaitInstr>(value) || isA<YieldInInstr>(value)) {
      result = sequence(std::move(result),
                        {{ExitKind::Normal, NO_TARGET}, {ExitKind::Throw, NO_TARGET}});
    }
    return result;
  }
};

} // namespace

const std::string MissingReturnLowering::KEY = "core-missing-return-lowering";

void MissingReturnLowering::run(Module *module) {
  std::vector<BodiedFunc *> functions;
  if (auto *main = cast<BodiedFunc>(module->getMainFunc()))
    functions.push_back(main);
  for (auto *variable : *module)
    if (auto *function = cast<BodiedFunc>(variable))
      functions.push_back(function);

  struct Calls : public util::Operator {
    std::unordered_map<const BodiedFunc *, std::unordered_set<BodiedFunc *>> callers;
    void handle(CallInstr *call) override {
      if (auto *callee = cast<BodiedFunc>(util::getFunc(call->getCallee())))
        callers[callee].insert(cast<BodiedFunc>(getParentFunc()));
    }
  } calls;
  calls.process(module);

  std::unordered_set<const BodiedFunc *> nonReturning;
  ReturnAnalysis analysis(nonReturning);
  std::deque<BodiedFunc *> pending(functions.begin(), functions.end());
  std::unordered_set<BodiedFunc *> queued(functions.begin(), functions.end());
  while (!pending.empty()) {
    auto *function = pending.front();
    pending.pop_front();
    queued.erase(function);
    if (function->isGenerator() || function->isAsync() || !function->getBody() ||
        nonReturning.count(function))
      continue;
    auto outcomes = analysis.analyze(function->getBody());
    if (has(outcomes, ExitKind::Normal) || has(outcomes, ExitKind::Return))
      continue;
    nonReturning.insert(function);
    for (auto *caller : calls.callers[function])
      if (caller && queued.insert(caller).second)
        pending.push_back(caller);
  }

  Func *helper = nullptr;
  for (auto *function : functions) {
    if (function->isGenerator() || !function->getBody())
      continue;
    auto *result = cast<FuncType>(function->getType())->getReturnType();
    if (function->isAsync())
      result = cast<GeneratorType>(result)->getBase();
    if (result->is(module->getNoneType()) || isA<OptionalType>(result) ||
        !has(analysis.analyze(function->getBody()), ExitKind::Normal))
      continue;
    if (!helper)
      helper = module->getOrRealizeFunc(
          "_missing_return_error",
          {module->getStringType(), module->getStringType(), module->getStringType(),
           module->getIntType(), module->getIntType()},
          {}, "std.internal.types.error");
    seqassertn(helper, "missing-return helper not found");
    auto info = function->getSrcInfo();
    auto name = function->getUnmangledName();
    auto message = fmt::format(
        "function '{}' reached its end without returning a value ({}:{}:{})", name,
        info.file, info.line, info.col);
    auto *exception =
        util::call(helper, {module->getString(message), module->getString(name),
                            module->getString(info.file), module->getInt(info.line),
                            module->getInt(info.col)});
    auto *guard = module->N<ThrowInstr>(info, exception);
    auto *body = cast<SeriesFlow>(function->getBody());
    if (!body) {
      body = module->N<SeriesFlow>(info);
      body->push_back(function->getBody());
      function->setBody(body);
    }
    body->push_back(guard);
    LOG_IR("missing-return: {} [{}]", function->getName(), info);
  }
  if (helper)
    run(module);
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
