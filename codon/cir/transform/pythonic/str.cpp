// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "str.h"

#include <algorithm>

#include "codon/cir/analyze/dataflow/dominator.h"
#include "codon/cir/analyze/dataflow/reaching.h"
#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/parser/common.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {
namespace {
struct InspectionResult {
  bool valid = true;
  std::vector<Value *> args;
};

bool isString(Value *v) {
  auto *M = v->getModule();
  return v->getType()->is(M->getStringType());
}

void inspect(Value *v, InspectionResult &r) {
  // check if add first then go from there
  if (isString(v)) {
    if (auto *c = cast<CallInstr>(v)) {
      auto *func = util::getFunc(c->getCallee());
      if (func && func->getUnmangledName() == Module::ADD_MAGIC_NAME &&
          c->numArgs() == 2 && isString(c->front()) && isString(c->back())) {
        inspect(c->front(), r);
        inspect(c->back(), r);
        return;
      }
    }
    r.args.push_back(v);
  } else {
    r.valid = false;
  }
}

// Turn a private string accumulator into a mutable buffer:
//   result = seed             -> buffer = _str_append_seed(seed)
//   for ...: result += piece  -> for ...: _str_append_private(buffer, piece)
//   return result             -> return _str_append_finish(buffer)
// Storage is borrowed until two nonempty strings are joined, then privately owned.
// No intermediate result may escape, and the seed must dominate every use.
std::string getStringModule(Module *module) {
  if (!module->getCache())
    return "";
  // The stdlib can be filesystem-backed or embedded. Resolve its actual origin
  // for the exact module check below; a "std::" prefix check misses filesystem imports.
  auto file = ast::getImportFile(module->getCache(), "internal/types/str", "", true);
  return file ? file->path : "";
}

// Match local `result = result + piece`, also the lowering of `result += piece`.
// Require the builtin implementation: replacing a user override would lose its
// behavior.
CallInstr *getAppend(AssignInstr *assignment, const std::string &stringModule) {
  if (assignment->getLhs()->isGlobal() ||
      !assignment->getLhs()->getType()->is(assignment->getModule()->getStringType()))
    return nullptr;
  auto *call = cast<CallInstr>(assignment->getRhs());
  auto *function = call ? util::getFunc(call->getCallee()) : nullptr;
  if (!call || call->numArgs() != 2 || !function ||
      function->getUnmangledName() != Module::ADD_MAGIC_NAME ||
      !function->getParentType() ||
      !function->getParentType()->is(assignment->getModule()->getStringType()) ||
      !isString(call->back()))
    return nullptr;
  auto *attributes = function->getAttribute<KeyValueAttribute>();
  if (stringModule.empty() || !attributes || attributes->get(".module") != stringModule)
    return nullptr;
  auto *left = cast<VarValue>(call->front());
  return left && left->getVar() == assignment->getLhs() ? call : nullptr;
}

// Look for repeated concatenation first, then audit the entire containing function.
// A use outside the loop, such as saving a snapshot, can still make mutation unsafe.
struct AppendDiscovery : util::Operator {
  std::string stringModule;
  std::unordered_map<BodiedFunc *, std::unordered_set<Var *>> candidates;

  explicit AppendDiscovery(Module *module) : stringModule(getStringModule(module)) {}

  void handle(AssignInstr *assignment) override {
    auto *function = cast<BodiedFunc>(getParentFunc());
    if (function && !function->isAsync() && getAppend(assignment, stringModule) &&
        (findLast<ForFlow>() || findLast<ImperativeForFlow>() || findLast<WhileFlow>()))
      candidates[function].insert(assignment->getLhs());
  }
};

struct AppendPlan {
  AssignInstr *seed = nullptr;
  std::vector<AssignInstr *> updates;
  std::vector<ReturnInstr *> returns;
  std::vector<Value *> reads;
  bool valid = true;
};

// This is a strict use whitelist, not a general alias or last-use analysis. An
// accumulator may have one seed assignment, append updates, and direct returns;
// every other reference disqualifies it, even a read such as `len(result)`.
struct AppendUseAnalysis {
  const std::string &stringModule;
  std::unordered_map<Var *, AppendPlan> plans;
  std::unordered_set<id_t> seen;
  bool safeFlow = true;

  AppendUseAnalysis(const std::unordered_set<Var *> &candidates,
                    const std::string &stringModule)
      : stringModule(stringModule) {
    for (auto *variable : candidates)
      plans.emplace(variable, AppendPlan{});
  }

  void inspect(Value *value) {
    if (!value || !seen.insert(value->getId()).second)
      return;
    // Keep the proof within ordinary serial control flow. Exception handlers,
    // suspension, parallel execution, and custom flows need additional reasoning,
    // so their presence rejects the whole function, not just the nearest loop.
    if (isA<TryCatchFlow>(value) || isA<YieldInstr>(value) ||
        isA<YieldInInstr>(value) || isA<AwaitInstr>(value) ||
        isA<PipelineFlow>(value) || isA<dsl::CustomFlow>(value) ||
        isA<dsl::CustomInstr>(value)) {
      safeFlow = false;
      return;
    }
    if (auto *loop = cast<ForFlow>(value))
      safeFlow &= !loop->isParallel() && !loop->isAsync();
    if (auto *loop = cast<ImperativeForFlow>(value))
      safeFlow &= !loop->isParallel();

    if (auto *assignment = cast<AssignInstr>(value)) {
      auto found = plans.find(assignment->getLhs());
      if (found != plans.end()) {
        auto &plan = found->second;
        if (auto *call = getAppend(assignment, stringModule)) {
          plan.updates.push_back(assignment);
          plan.reads.push_back(call->front());
          // The left operand is the permitted accumulator read; audit the RHS
          // normally so `result += result` or `result += consume(result)` rejects.
          inspect(call->back());
        } else {
          // A second non-append assignment, e.g. `result = ""` after an append,
          // starts another lifetime that this single-seed plan does not model.
          if (plan.seed)
            plan.valid = false;
          plan.seed = assignment;
          inspect(assignment->getRhs());
        }
        return;
      }
    }
    if (auto *ret = cast<ReturnInstr>(value)) {
      auto *read = cast<VarValue>(ret->getValue());
      auto found = read ? plans.find(read->getVar()) : plans.end();
      if (found != plans.end()) {
        // A direct return exposes storage only after the last mutation on that
        // path, so early returns are safe too. Compound returns such as
        // `return (result, 1)` deliberately fall through to the rejection below.
        found->second.returns.push_back(ret);
        found->second.reads.push_back(read);
        return;
      }
    }
    // Other references can observe or retain the old immutable string. For example,
    // `snapshot = result; result += piece` must preserve snapshot's original contents.
    // Calls, slices, pointer-taking, and closure captures are rejected by this rule
    // too.
    for (auto *variable : value->getUsedVariables()) {
      auto found = plans.find(variable);
      if (found != plans.end())
        found->second.valid = false;
    }
    for (auto *child : value->getUsedValues())
      inspect(child);
  }
};

bool proveAppendPlan(Var *variable, const AppendPlan &plan,
                     analyze::dataflow::CFGraph *graph,
                     analyze::dataflow::RDInspector *reaching,
                     analyze::dataflow::DominatorInspector *dominators) {
  if (!plan.valid || !plan.seed || plan.updates.empty() || plan.returns.empty() ||
      reaching->isInvalid(variable) || !graph->getBlock(plan.seed))
    return false;
  std::unordered_set<const Instr *> definitions = {plan.seed};
  definitions.insert(plan.updates.begin(), plan.updates.end());
  for (auto *read : plan.reads) {
    // A unique seed in the syntax is not enough: it must execute before every read.
    // For example, a seed in only one branch cannot initialize an unconditional append.
    if (!graph->getBlock(read) || !dominators->isDominated(read, plan.seed))
      return false;
    // At branch joins and loop backedges, every incoming value must belong to the
    // audited seed/update chain. Unknown or unrelated definitions are not a buffer.
    auto incoming = reaching->getReachingDefinitions(variable, read);
    if (incoming.empty())
      return false;
    for (const auto &definition : incoming)
      if (!definition.known() || !definitions.count(definition.assignment))
        return false;
  }
  return true;
}
} // namespace

const std::string StrAdditionOptimization::KEY = "core-pythonic-str-addition-opt";

void StrAdditionOptimization::handle(CallInstr *v) {
  auto *M = v->getModule();

  auto *f = util::getFunc(v->getCallee());
  if (!f || f->getUnmangledName() != Module::ADD_MAGIC_NAME)
    return;

  InspectionResult r;
  inspect(v, r);

  if (r.valid && r.args.size() > 2) {
    std::vector<Value *> args;
    util::CloneVisitor cv(M);

    for (auto *arg : r.args) {
      args.push_back(cv.clone(arg));
    }

    auto *arg = util::makeTuple(args, M);
    args = {arg};
    auto *replacementFunc =
        M->getOrRealizeMethod(M->getStringType(), "cat", {arg->getType()});
    seqassertn(replacementFunc, "could not find cat function [{}]", v->getSrcInfo());
    v->replaceAll(util::call(replacementFunc, args));
  }
}

const std::string StrAppendOptimization::KEY = "core-pythonic-str-append-opt";

bool StrAppendOptimization::shouldRun(Module *module) const {
  AppendDiscovery discovery(module);
  discovery.process(module);
  return !discovery.candidates.empty();
}

void StrAppendOptimization::run(Module *module) {
  AppendDiscovery discovery(module);
  discovery.process(module);
  auto *reaching = getAnalysisResult<analyze::dataflow::RDResult>(reachingKey);
  auto *dominators =
      getAnalysisResult<analyze::dataflow::DominatorResult>(dominatorKey);
  struct Rewrite {
    BodiedFunc *function;
    AppendPlan plan;
  };

  // Prove all candidates against the original IR before realizing helpers or
  // rewriting anything; mutations would invalidate the analyses used by later plans.
  std::vector<Rewrite> rewrites;
  for (const auto &[function, candidates] : discovery.candidates) {
    auto graph = reaching->cfgResult->graphs.find(function->getId());
    auto definitions = reaching->results.find(function->getId());
    auto domination = dominators->results.find(function->getId());
    if (graph == reaching->cfgResult->graphs.end() ||
        definitions == reaching->results.end() ||
        domination == dominators->results.end())
      continue;
    AppendUseAnalysis uses(candidates, discovery.stringModule);
    uses.inspect(function->getBody());
    if (!uses.safeFlow)
      continue;
    for (const auto &[variable, plan] : uses.plans)
      if (proveAppendPlan(variable, plan, graph->second.get(),
                          definitions->second.get(), domination->second.get()))
        rewrites.push_back({function, plan});
  }

  if (rewrites.empty())
    return;

  // Avoid public strbuf method dispatch: user @extend overrides must not intercept
  // compiler-generated operations implementing builtin string concatenation.
  const std::string strbufModule = "std.internal.types.strbuf";
  auto *seed = module->getOrRealizeFunc("_str_append_seed", {module->getStringType()},
                                        {}, strbufModule);
  if (!seed)
    return;
  auto *bufferType = cast<FuncType>(seed->getType())->getReturnType();
  auto *append = module->getOrRealizeFunc(
      "_str_append_private", {bufferType, module->getStringType()}, {}, strbufModule);
  auto *finish =
      module->getOrRealizeFunc("_str_append_finish", {bufferType}, {}, strbufModule);

  if (!append || !finish)
    return;

  for (const auto &rewrite : rewrites) {
    auto *buffer = module->Nr<Var>(bufferType);
    rewrite.function->push_back(buffer);
    const auto &plan = rewrite.plan;
    plan.seed->replaceAll(module->N<AssignInstr>(
        plan.seed->getSrcInfo(), buffer, util::call(seed, {plan.seed->getRhs()})));

    for (auto *update : plan.updates) {
      auto *call = cast<CallInstr>(update->getRhs());
      // Keep the original RHS as one argument: its effects or exception occur once,
      // before the buffer is mutated, just as before the original string assignment.
      update->replaceAll(
          util::call(append, {module->Nr<VarValue>(buffer), call->back()}));
    }

    for (auto *ret : plan.returns)
      ret->setValue(util::call(finish, {module->Nr<VarValue>(buffer)}));
  }
}

} // namespace pythonic
} // namespace transform
} // namespace ir
} // namespace codon
