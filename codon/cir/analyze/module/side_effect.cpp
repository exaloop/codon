// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "side_effect.h"

#include <type_traits>
#include <utility>

#include "codon/cir/analyze/dataflow/capture.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/operator.h"

namespace codon {
namespace ir {
namespace analyze {
namespace module {
namespace {
template <typename T> T max(T &&t) { return std::forward<T>(t); }

template <typename T0, typename T1, typename... Ts>
typename std::common_type<T0, T1, Ts...>::type max(T0 &&val1, T1 &&val2, Ts &&...vs) {
  return (val1 > val2) ? max(val1, std::forward<Ts>(vs)...)
                       : max(val2, std::forward<Ts>(vs)...);
}

struct VarUseAnalyzer : public util::Operator {
  std::unordered_map<id_t, long> varCounts;
  std::unordered_map<id_t, long> varAssignCounts;

  void preHook(Node *v) override {
    for (auto *var : v->getUsedVariables()) {
      ++varCounts[var->getId()];
    }
  }

  void handle(AssignInstr *v) override { ++varAssignCounts[v->getLhs()->getId()]; }
};

struct SideEfectAnalyzer : public util::ConstVisitor {
  using Status = util::SideEffectStatus;

  static Status getFunctionStatusFromAttributes(const Func *v, bool *force = nullptr) {
    auto attr = [v](const auto &s) { return util::hasAttribute(v, s); };

    if (attr(util::PURE_ATTR)) {
      if (force)
        *force = true;
      return Status::PURE;
    }

    if (attr(util::NO_SIDE_EFFECT_ATTR)) {
      if (force)
        *force = true;
      return Status::NO_SIDE_EFFECT;
    }

    if (attr(util::NO_CAPTURE_ATTR)) {
      if (force)
        *force = true;
      return Status::NO_CAPTURE;
    }

    if (attr(util::NON_PURE_ATTR)) {
      if (force)
        *force = true;
      return Status::UNKNOWN;
    }

    if (force)
      *force = false;
    return Status::UNKNOWN;
  }

  VarUseAnalyzer &vua;
  dataflow::CaptureResult *cr;
  bool globalAssignmentHasSideEffects;
  std::unordered_map<id_t, Status> result;
  std::vector<const BodiedFunc *> funcStack;
  Status exprStatus;
  Status funcStatus;

  // We have to sometimes be careful with globals since future
  // IR passes might introduce globals that we've eliminated
  // or demoted earlier. Hence the distinction with whether
  // global assignments are considered to have side effects.
  SideEfectAnalyzer(VarUseAnalyzer &vua, dataflow::CaptureResult *cr,
                    bool globalAssignmentHasSideEffects)
      : util::ConstVisitor(), vua(vua), cr(cr),
        globalAssignmentHasSideEffects(globalAssignmentHasSideEffects), result(),
        funcStack(), exprStatus(Status::PURE), funcStatus(Status::PURE) {}

  template <typename T> bool has(const T *v) {
    return result.find(v->getId()) != result.end();
  }

  template <typename T> void set(const T *v, Status expr, Status func = Status::PURE) {
    result[v->getId()] = exprStatus = expr;
    funcStatus = max(funcStatus, func);
  }

  template <typename T> Status process(const T *v) {
    if (!v)
      return Status::PURE;
    if (has(v))
      return result[v->getId()];
    v->accept(*this);
    seqassertn(has(v), "node not added to results");
    return result[v->getId()];
  }

  std::pair<Status, Status> getVarAssignStatus(const Var *var) {
    if (!var)
      return {Status::PURE, Status::PURE};

    auto id = var->getId();
    auto it1 = vua.varCounts.find(id);
    auto it2 = vua.varAssignCounts.find(id);
    auto count1 = (it1 != vua.varCounts.end()) ? it1->second : 0;
    auto count2 = (it2 != vua.varAssignCounts.end()) ? it2->second : 0;

    bool global = var->isGlobal();
    bool used = (count1 != count2);
    Status defaultStatus = global ? Status::UNKNOWN : Status::NO_CAPTURE;
    auto se2stat = [&](bool b) { return b ? defaultStatus : Status::PURE; };

    if (globalAssignmentHasSideEffects || var->isExternal()) {
      return {se2stat(used || global), se2stat(global)};
    } else {
      return {se2stat(used), se2stat(used && global)};
    }
  }

  void handleVarAssign(const Value *v, const Var *var, Status base) {
    auto pair = getVarAssignStatus(var);
    set(v, max(pair.first, base), pair.second);
  }

  void visit(const Module *v) override {
    process(v->getMainFunc());
    for (auto *x : *v) {
      process(x);
    }
  }

  void visit(const Var *v) override { set(v, Status::PURE); }

  void visit(const BodiedFunc *v) override {
    bool force;
    auto s = getFunctionStatusFromAttributes(v, &force);
    set(v, s, s); // avoid infinite recursion
    auto oldFuncStatus = funcStatus;
    funcStatus = Status::PURE;
    funcStack.push_back(v);
    process(v->getBody());
    funcStack.pop_back();
    if (force)
      funcStatus = s;
    set(v, funcStatus);
    funcStatus = oldFuncStatus;
  }

  void visit(const ExternalFunc *v) override {
    set(v, getFunctionStatusFromAttributes(v));
  }

  void visit(const InternalFunc *v) override { set(v, Status::PURE); }

  void visit(const LLVMFunc *v) override { set(v, getFunctionStatusFromAttributes(v)); }

  void visit(const VarValue *v) override { set(v, Status::PURE); }

  void visit(const PointerValue *v) override { set(v, Status::PURE); }

  void visit(const SeriesFlow *v) override {
    Status s = Status::PURE;
    for (auto *x : *v) {
      s = max(s, process(x));
    }
    set(v, s);
  }

  void visit(const IfFlow *v) override {
    set(v, max(process(v->getCond()), process(v->getTrueBranch()),
               process(v->getFalseBranch())));
  }

  void visit(const WhileFlow *v) override {
    set(v, max(process(v->getCond()), process(v->getBody())));
  }

  void visit(const ForFlow *v) override {
    auto s = max(process(v->getIter()), process(v->getBody()));
    if (auto *sched = v->getSchedule()) {
      for (auto *x : sched->getUsedValues()) {
        s = max(s, process(x));
      }
    }
    handleVarAssign(v, v->getVar(), s);
  }

  void visit(const ImperativeForFlow *v) override {
    auto s = max(process(v->getStart()), process(v->getEnd()), process(v->getBody()));
    if (auto *sched = v->getSchedule()) {
      for (auto *x : sched->getUsedValues()) {
        s = max(s, process(x));
      }
    }
    handleVarAssign(v, v->getVar(), s);
  }

  void visit(const TryCatchFlow *v) override {
    auto s = max(process(v->getBody()), process(v->getFinally()));
    auto callStatus = Status::PURE;

    for (auto &x : *v) {
      auto pair = getVarAssignStatus(x.getVar());
      s = max(s, pair.first, process(x.getHandler()));
      callStatus = max(callStatus, pair.second);
    }

    set(v, s, callStatus);
  }

  void visit(const PipelineFlow *v) override {
    auto s = Status::PURE;
    auto callStatus = Status::PURE;
    for (auto &stage : *v) {
      // make sure we're treating this as a call
      if (auto *f = util::getFunc(stage.getCallee())) {
        auto stageCallStatus = process(f);
        callStatus = max(callStatus, stageCallStatus);
        s = max(s, stageCallStatus);
      } else {
        // unknown function
        process(stage.getCallee());
        callStatus = Status::UNKNOWN;
        s = Status::UNKNOWN;
      }

      for (auto *arg : stage) {
        s = max(s, process(arg));
      }
    }
    set(v, s, callStatus);
  }

  void visit(const dsl::CustomFlow *v) override {
    set(v, v->getSideEffectStatus(/*local=*/true),
        v->getSideEffectStatus(/*local=*/false));
  }

  void visit(const IntConst *v) override { set(v, Status::PURE); }

  void visit(const FloatConst *v) override { set(v, Status::PURE); }

  void visit(const BoolConst *v) override { set(v, Status::PURE); }

  void visit(const StringConst *v) override { set(v, Status::PURE); }

  void visit(const dsl::CustomConst *v) override { set(v, Status::PURE); }

  void visit(const AssignInstr *v) override {
    handleVarAssign(v, v->getLhs(), process(v->getRhs()));
  }

  void visit(const ExtractInstr *v) override { set(v, process(v->getVal())); }

  void visit(const InsertInstr *v) override {
    process(v->getLhs());
    process(v->getRhs());

    auto *func = funcStack.back();
    auto it = cr->results.find(func->getId());
    seqassertn(it != cr->results.end(), "function not found in capture results");
    auto captureInfo = it->second;

    bool pure = true;

    for (auto &info : captureInfo) {
      if (info.externCaptures || info.modified || !info.argCaptures.empty()) {
        pure = false;
        break;
      }
    }

    if (pure) {
      // make sure the lhs does not escape
      auto escapeInfo = escapes(func, v->getLhs(), cr);
      pure = (!escapeInfo || (escapeInfo.returnCaptures && !escapeInfo.externCaptures &&
                              escapeInfo.argCaptures.empty()));
    }

    set(v, Status::UNKNOWN, pure ? Status::PURE : Status::UNKNOWN);
  }

  void visit(const CallInstr *v) override {
    auto s = Status::PURE;
    auto callStatus = Status::UNKNOWN;
    for (auto *x : *v) {
      s = max(s, process(x));
    }
    if (auto *f = util::getFunc(v->getCallee())) {
      callStatus = process(f);
      s = max(s, callStatus);
    } else {
      // unknown function
      process(v->getCallee());
      s = Status::UNKNOWN;
    }
    set(v, s, callStatus);
  }

  void visit(const StackAllocInstr *v) override { set(v, Status::PURE); }

  void visit(const TypePropertyInstr *v) override { set(v, Status::PURE); }

  void visit(const YieldInInstr *v) override { set(v, Status::NO_CAPTURE); }

  void visit(const TernaryInstr *v) override {
    set(v, max(process(v->getCond()), process(v->getTrueValue()),
               process(v->getFalseValue())));
  }

  void visit(const BreakInstr *v) override { set(v, Status::NO_CAPTURE); }

  void visit(const ContinueInstr *v) override { set(v, Status::NO_CAPTURE); }

  void visit(const ReturnInstr *v) override {
    set(v, max(Status::NO_CAPTURE, process(v->getValue())));
  }

  void visit(const YieldInstr *v) override {
    set(v, max(Status::NO_CAPTURE, process(v->getValue())));
  }

  void visit(const ThrowInstr *v) override {
    process(v->getValue());
    set(v, Status::UNKNOWN, Status::NO_CAPTURE);
  }

  void visit(const FlowInstr *v) override {
    set(v, max(process(v->getFlow()), process(v->getValue())));
  }

  void visit(const dsl::CustomInstr *v) override {
    set(v, v->getSideEffectStatus(/*local=*/true),
        v->getSideEffectStatus(/*local=*/false));
  }
};
} // namespace

const std::string SideEffectAnalysis::KEY = "core-analyses-side-effect";

bool SideEffectResult::hasSideEffect(const Value *v) const {
  auto it = result.find(v->getId());
  return it == result.end() || it->second != util::SideEffectStatus::PURE;
}

std::unique_ptr<Result> SideEffectAnalysis::run(const Module *m) {
  auto *capResult = getAnalysisResult<dataflow::CaptureResult>(capAnalysisKey);
  VarUseAnalyzer vua;
  const_cast<Module *>(m)->accept(vua);
  SideEfectAnalyzer sea(vua, capResult, globalAssignmentHasSideEffects);
  m->accept(sea);
  return std::make_unique<SideEffectResult>(sea.result);
}

} // namespace module
} // namespace analyze
} // namespace ir
} // namespace codon
