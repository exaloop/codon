#include "side_effect.h"

#include "sir/util/irtools.h"
#include "sir/util/operator.h"

namespace seq {
namespace ir {
namespace analyze {
namespace module {
namespace {
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
  static const std::string PURE_ATTR;
  static const std::string NON_PURE_ATTR;

  VarUseAnalyzer &vua;
  bool globalAssignmentHasSideEffects;
  std::unordered_map<id_t, bool> result;
  bool exprSE;
  bool funcSE;

  // We have to sometimes be careful with globals since future
  // IR passes might introduce globals that we've eliminated
  // or demoted earlier. Hence the distinction with whether
  // global assignments are considered to have side effects.
  SideEfectAnalyzer(VarUseAnalyzer &vua, bool globalAssignmentHasSideEffects)
      : util::ConstVisitor(), vua(vua),
        globalAssignmentHasSideEffects(globalAssignmentHasSideEffects), result(),
        exprSE(true), funcSE(false) {}

  template <typename T> bool has(const T *v) {
    return result.find(v->getId()) != result.end();
  }

  template <typename T>
  void set(const T *v, bool hasSideEffect, bool hasFuncSideEffect = false) {
    result[v->getId()] = exprSE = hasSideEffect;
    funcSE |= hasFuncSideEffect;
  }

  template <typename T> bool process(const T *v) {
    if (!v)
      return false;
    if (has(v))
      return result[v->getId()];
    v->accept(*this);
    seqassert(has(v), "node not added to results");
    return result[v->getId()];
  }

  void visit(const Module *v) override {
    process(v->getMainFunc());
    for (auto *x : *v) {
      process(x);
    }
  }

  void visit(const Var *v) override { set(v, false); }

  void visit(const BodiedFunc *v) override {
    const bool pure = util::hasAttribute(v, PURE_ATTR);
    const bool nonPure = util::hasAttribute(v, NON_PURE_ATTR);
    set(v, !pure, !pure); // avoid infinite recursion
    bool oldFuncSE = funcSE;
    funcSE = false;
    process(v->getBody());
    set(v, nonPure || (funcSE && !pure));
    funcSE = oldFuncSE;
  }

  void visit(const ExternalFunc *v) override {
    set(v, !util::hasAttribute(v, PURE_ATTR));
  }

  void visit(const InternalFunc *v) override { set(v, false); }

  void visit(const LLVMFunc *v) override { set(v, !util::hasAttribute(v, PURE_ATTR)); }

  void visit(const VarValue *v) override { set(v, false); }

  void visit(const PointerValue *v) override { set(v, false); }

  void visit(const SeriesFlow *v) override {
    bool s = false;
    for (auto *x : *v) {
      s |= process(x);
    }
    set(v, s);
  }

  void visit(const IfFlow *v) override {
    set(v, process(v->getCond()) | process(v->getTrueBranch()) |
               process(v->getFalseBranch()));
  }

  void visit(const WhileFlow *v) override {
    set(v, process(v->getCond()) | process(v->getBody()));
  }

  void visit(const ForFlow *v) override {
    bool s = process(v->getIter()) | process(v->getBody());
    if (auto *sched = v->getSchedule()) {
      for (auto *x : sched->getUsedValues()) {
        s |= process(x);
      }
    }
    set(v, s);
  }

  void visit(const ImperativeForFlow *v) override {
    bool s = process(v->getStart()) | process(v->getEnd()) | process(v->getBody());
    if (auto *sched = v->getSchedule()) {
      for (auto *x : sched->getUsedValues()) {
        s |= process(x);
      }
    }
    set(v, s);
  }

  void visit(const TryCatchFlow *v) override {
    bool s = process(v->getBody()) | process(v->getFinally());
    for (auto &x : *v) {
      s |= process(x.getHandler());
    }
    set(v, s);
  }

  void visit(const PipelineFlow *v) override {
    bool s = false;
    bool callSE = false;
    for (auto &stage : *v) {
      // make sure we're treating this as a call
      if (auto *f = util::getFunc(stage.getCallee())) {
        bool stageCallSE = process(f);
        callSE |= stageCallSE;
        s |= stageCallSE;
      } else {
        // unknown function
        process(stage.getCallee());
        s = true;
      }

      for (auto *arg : stage) {
        s |= process(arg);
      }
    }
    set(v, s, callSE);
  }

  void visit(const dsl::CustomFlow *v) override {
    bool s = v->hasSideEffect();
    set(v, s, s);
  }

  void visit(const IntConst *v) override { set(v, false); }

  void visit(const FloatConst *v) override { set(v, false); }

  void visit(const BoolConst *v) override { set(v, false); }

  void visit(const StringConst *v) override { set(v, false); }

  void visit(const dsl::CustomConst *v) override { set(v, false); }

  void visit(const AssignInstr *v) override {
    auto id = v->getLhs()->getId();
    auto it1 = vua.varCounts.find(id);
    auto it2 = vua.varAssignCounts.find(id);
    auto count1 = (it1 != vua.varCounts.end()) ? it1->second : 0;
    auto count2 = (it2 != vua.varAssignCounts.end()) ? it2->second : 0;

    bool g = v->getLhs()->isGlobal();
    bool s = (count1 != count2);
    if (globalAssignmentHasSideEffects) {
      set(v, s | g | process(v->getRhs()), g);
    } else {
      set(v, s | process(v->getRhs()), s & g);
    }
  }

  void visit(const ExtractInstr *v) override { set(v, process(v->getVal())); }

  void visit(const InsertInstr *v) override {
    process(v->getLhs());
    process(v->getRhs());
    set(v, true, true);
  }

  void visit(const CallInstr *v) override {
    bool s = false;
    bool callSE = true;
    for (auto *x : *v) {
      s |= process(x);
    }
    if (auto *f = util::getFunc(v->getCallee())) {
      callSE = process(f);
      s |= callSE;
    } else {
      // unknown function
      process(v->getCallee());
      s = true;
    }
    set(v, s, callSE);
  }

  void visit(const StackAllocInstr *v) override { set(v, false); }

  void visit(const TypePropertyInstr *v) override { set(v, false); }

  void visit(const YieldInInstr *v) override { set(v, true); }

  void visit(const TernaryInstr *v) override {
    set(v, process(v->getCond()) | process(v->getTrueValue()) |
               process(v->getFalseValue()));
  }

  void visit(const BreakInstr *v) override { set(v, true); }

  void visit(const ContinueInstr *v) override { set(v, true); }

  void visit(const ReturnInstr *v) override {
    process(v->getValue());
    set(v, true);
  }

  void visit(const YieldInstr *v) override {
    process(v->getValue());
    set(v, true);
  }

  void visit(const ThrowInstr *v) override {
    process(v->getValue());
    set(v, true, true);
  }

  void visit(const FlowInstr *v) override {
    set(v, process(v->getFlow()) | process(v->getValue()));
  }

  void visit(const dsl::CustomInstr *v) override {
    bool s = v->hasSideEffect();
    set(v, s, s);
  }
};

const std::string SideEfectAnalyzer::PURE_ATTR = "std.internal.attributes.pure";
const std::string SideEfectAnalyzer::NON_PURE_ATTR = "std.internal.attributes.nonpure";
} // namespace

const std::string SideEffectAnalysis::KEY = "core-analyses-side-effect";

bool SideEffectResult::hasSideEffect(Value *v) const {
  auto it = result.find(v->getId());
  return it == result.end() || it->second;
}

std::unique_ptr<Result> SideEffectAnalysis::run(const Module *m) {
  VarUseAnalyzer vua;
  const_cast<Module *>(m)->accept(vua);
  SideEfectAnalyzer sea(vua, globalAssignmentHasSideEffects);
  m->accept(sea);
  return std::make_unique<SideEffectResult>(sea.result);
}

} // namespace module
} // namespace analyze
} // namespace ir
} // namespace seq
