#include "escape.h"

#include <unordered_map>
#include <utility>

namespace codon {
namespace ir {
namespace util {
namespace {

/*
LAMBDA_VISIT(IfFlow);
LAMBDA_VISIT(WhileFlow);
LAMBDA_VISIT(ForFlow);
LAMBDA_VISIT(ImperativeForFlow);
LAMBDA_VISIT(TryCatchFlow);
LAMBDA_VISIT(PipelineFlow);
LAMBDA_VISIT(dsl::CustomFlow);

LAMBDA_VISIT(TemplatedConst<int64_t>);
LAMBDA_VISIT(TemplatedConst<double>);
LAMBDA_VISIT(TemplatedConst<bool>);
LAMBDA_VISIT(TemplatedConst<std::string>);
LAMBDA_VISIT(dsl::CustomConst);

LAMBDA_VISIT(Instr);
LAMBDA_VISIT(AssignInstr);
LAMBDA_VISIT(ExtractInstr);
LAMBDA_VISIT(InsertInstr);
LAMBDA_VISIT(CallInstr);
LAMBDA_VISIT(StackAllocInstr);
LAMBDA_VISIT(TypePropertyInstr);
LAMBDA_VISIT(YieldInInstr);
LAMBDA_VISIT(TernaryInstr);
LAMBDA_VISIT(BreakInstr);
LAMBDA_VISIT(ContinueInstr);
LAMBDA_VISIT(ReturnInstr);
LAMBDA_VISIT(YieldInstr);
LAMBDA_VISIT(ThrowInstr);
LAMBDA_VISIT(FlowInstr);
LAMBDA_VISIT(dsl::CustomInstr);
*/

enum DerivedKind {
  ORIGIN = 0,
  ASSIGN,
  MEMBER,
  INSERT,
};

struct DerivedFinder : public Operator {
  analyze::dataflow::RDInspector *rd;
  std::unordered_map<id_t, DerivedKind> derivedValues;
  std::unordered_map<id_t, std::pair<DerivedKind, Value *>>
      derivedVars; // pair of kind and value leading to derivation
  bool escapes;

  bool isDerived(Var *v) const {
    return derivedVars.find(v->getId()) != derivedVars.end();
  }

  bool isDerived(Value *v) const {
    if (derivedValues.find(v->getId()) != derivedValues.end())
      return true;

    if (auto *x = cast<VarValue>(v))
      return isDerived(x->getVar());

    if (auto *x = cast<PointerValue>(v))
      return isDerived(x->getVar());

    return false;
  }

  void setDerived(Var *v, DerivedKind kind, Value *cause) {
    derivedVars.emplace(v->getId(), std::make_pair(kind, cause));
  }

  void setDerived(Value *v, DerivedKind kind) {
    derivedValues.emplace(v->getId(), kind);
  }

  DerivedFinder(Value *value, analyze::dataflow::RDInspector *rd)
      : Operator(), rd(rd), derivedValues(), derivedVars(), escapes(false) {
    setDerived(value, DerivedKind::ORIGIN);
  }

  unsigned size() const { return derivedValues.size() + derivedVars.size(); }

  void handle(AssignInstr *v) override {
    if (isDerived(v->getRhs()))
      setDerived(v->getLhs(), DerivedKind::ASSIGN, v);
  }

  void handle(ExtractInstr *v) override {
    if (isDerived(v->getVal()) && !v->getType()->isAtomic())
      setDerived(v, DerivedKind::MEMBER);
  }

  void handle(InsertInstr *v) override {
    // We handle cases like "a.b = c" by expanding our
    // search to all the reaching definitions of "a", since
    // if any of these escape, then our object might escape
    // as well.

    if (isDerived(v->getRhs())) {
      // Only handle cases where lhs is a variable, otherwise
      // just assume we escape in cases like e.g. "foo().x = y".
      auto *lhs = cast<VarValue>(v->getLhs());
      Var *var = lhs ? lhs->getVar() : nullptr;
      if (var && !var->isGlobal()) {
        setDerived(var, DerivedKind::INSERT, v);
      } else {
        escapes = true;
      }
    }
  }

  void handle(CallInstr *v) override {
    // TODO
  }

  void handle(dsl::CustomInstr *v) override {
    // TODO
  }
};

struct EscapeFinder : public Operator {
  DerivedFinder &dfinder;
  EscapeResult result;

  EscapeFinder(DerivedFinder &dfinder) : Operator(), dfinder(dfinder), result(EscapeResult::NO) {}

  void update(EscapeResult r) {
    if (r == EscapeResult::YES) {
      result = EscapeResult::YES;
    } else if (r == EscapeResult::RETURNED && result == EscapeResult::NO) {
      result = EscapeResult::RETURNED;
    }
  }

  bool isDerived(Value *v) {
    auto &dval = dfinder.derivedValues;
    auto &dvar = dfinder.derivedVars;

    auto it1 = dval.find(v->getId());

    // Case 1: Value itself is directly derived.
    // This is easy -- just return true.
    if (it1 != dval.end())
      return true;

    // Case 2: Value refers to var. This is trickier,
    // as we need to make sure the value that led to
    // this var being derived shares a reaching assignment
    // with the value we're checking. If not, the var
    // was definitely re-assigned before this point so
    // there is no escape of the original object.
    Var *var = nullptr;
    if (auto *x = cast<VarValue>(v)) {
      var = x->getVar();
    } else if (auto *x = cast<PointerValue>(v)) {
      var = x->getVar();
    }

    if (!var)
      return false;

    auto it2 = dvar.find(var->getId());
    if (it2 == dvar.end())
      return false;

    // 1st set: reaching definitions of the value in question
    // 2nd set: reaching definitions of the value leading to var being "derived"
    auto reachingSet1 = dfinder.rd->getReachingDefinitions(var, v);
    auto reachingSet2 = dfinder.rd->getReachingDefinitions(var, it2->second.second);

    // Finally, we are derived if the above two sets share at least one element.
    bool derived = false;
    for (auto &elem : reachingSet1) {
      if (reachingSet2.count(elem)) {
        derived = true;
        break;
      }
    }
    return derived;
  }

  void handle(AssignInstr *v) override {
    if (v->getLhs()->isGlobal() && isDerived(v->getRhs()))
      update(EscapeResult::YES);
  }

  void handle(ReturnInstr *v) override {
    if (isDerived(v->getValue()))
      update(EscapeResult::RETURNED);
  }

  void handle(ThrowInstr *v) override {
    if (isDerived(v->getValue()))
      update(EscapeResult::YES);
  }

  void handle(CallInstr *v) override {
    // TODO
  }
};

} // namespace

EscapeResult escapes(BodiedFunc *parent, Value *value,
                     analyze::dataflow::RDResult *reaching) {
  auto *c = reaching->cfgResult;
  auto it = reaching->results.find(parent->getId());
  seqassert(it != reaching->results.end(),
            "could not find parent function in reaching-definitions results");

  DerivedFinder dfinder(value, it->second.get());
  unsigned oldSize = 0;
  do {
    oldSize = dfinder.size();
    parent->accept(dfinder);
  } while (dfinder.size() != oldSize);

  return EscapeResult::YES;
}

} // namespace util
} // namespace ir
} // namespace codon
