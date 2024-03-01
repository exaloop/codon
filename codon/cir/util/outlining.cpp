// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "outlining.h"

#include <iterator>
#include <unordered_set>
#include <utility>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/operator.h"

namespace codon {
namespace ir {
namespace util {
namespace {
struct OutlineReplacer : public Operator {
  std::unordered_set<id_t> &modVars;
  std::vector<std::pair<Var *, Var *>> &remap;
  std::vector<Value *> &outFlows;
  CloneVisitor cv;

  OutlineReplacer(Module *M, std::unordered_set<id_t> &modVars,
                  std::vector<std::pair<Var *, Var *>> &remap,
                  std::vector<Value *> &outFlows)
      : Operator(), modVars(modVars), remap(remap), outFlows(outFlows), cv(M, false) {}

  // Replace all used vars based on remapping.
  void postHook(Node *node) override {
    for (auto &pair : remap) {
      node->replaceUsedVariable(std::get<0>(pair), std::get<1>(pair));
    }
  }

  Var *mappedVar(Var *v) {
    for (auto &pair : remap) {
      if (std::get<0>(pair)->getId() == v->getId())
        return std::get<1>(pair);
    }
    return nullptr;
  }

  // A return in the outlined func, or a break/continue that references a
  // non-outlined loop, will return a status code that tells the call site
  // what action to perform.
  template <typename InstrType> void replaceOutFlowWithReturn(InstrType *v) {
    auto *M = v->getModule();
    for (unsigned i = 0; i < outFlows.size(); i++) {
      if (outFlows[i]->getId() == v->getId()) {
        auto *copy = cv.clone(v);
        v->replaceAll(M->template Nr<ReturnInstr>(M->getInt(i + 1)));
        outFlows[i] = copy;
        break;
      }
    }
  }

  void handle(ReturnInstr *v) override { replaceOutFlowWithReturn(v); }

  void handle(BreakInstr *v) override { replaceOutFlowWithReturn(v); }

  void handle(ContinueInstr *v) override { replaceOutFlowWithReturn(v); }

  // If passed by pointer (i.e. a "mod var"), change variable reference to
  // a pointer dereference.
  void handle(VarValue *v) override {
    auto *M = v->getModule();
    if (modVars.count(v->getVar()->getId()) > 0) {
      // var -> pointer dereference
      auto *deref = util::ptrLoad(M->Nr<VarValue>(mappedVar(v->getVar())));
      saw(deref);
      v->replaceAll(deref);
    }
  }

  // If passed by pointer (i.e. a "mod var"), change pointer value to just
  // be the var itself.
  void handle(PointerValue *v) override {
    auto *M = v->getModule();
    if (modVars.count(v->getVar()->getId()) > 0) {
      // pointer -> var
      auto *ref = M->Nr<VarValue>(mappedVar(v->getVar()));
      saw(ref);
      v->replaceAll(ref);
    }
  }

  // If passed by pointer (i.e. a "mod var"), change assignment to store
  // in the pointer.
  void handle(AssignInstr *v) override {
    auto *M = v->getModule();
    if (modVars.count(v->getLhs()->getId()) > 0) {
      // store in pointer
      Var *newVar = mappedVar(v->getLhs());
      auto *setitem = util::ptrStore(M->Nr<VarValue>(newVar), v->getRhs());
      saw(setitem);
      v->replaceAll(setitem);
    }
  }
};

struct Outliner : public Operator {
  BodiedFunc *parent;
  SeriesFlow *flowRegion;
  decltype(flowRegion->begin()) begin, end;
  bool outlineGlobals;              // whether to outline globals that are modified
  bool allByValue;                  // outline all vars by value (can change semantics)
  bool inRegion;                    // are we in the outlined region?
  bool invalid;                     // if we can't outline for whatever reason
  std::unordered_set<id_t> inVars;  // vars used inside region
  std::unordered_set<id_t> outVars; // vars used outside region
  std::unordered_set<id_t>
      modifiedInVars; // vars modified (assigned or address'd) in region
  std::unordered_set<id_t> globalsToOutline; // modified global vars to outline
  std::unordered_set<id_t> inLoops;          // loops contained in region
  std::vector<Value *>
      outFlows; // control flows that need to be handled externally (e.g. return)

  Outliner(BodiedFunc *parent, SeriesFlow *flowRegion,
           decltype(flowRegion->begin()) begin, decltype(flowRegion->begin()) end,
           bool outlineGlobals, bool allByValue)
      : Operator(), parent(parent), flowRegion(flowRegion), begin(begin), end(end),
        outlineGlobals(outlineGlobals), allByValue(allByValue), inRegion(false),
        invalid(false), inVars(), outVars(), modifiedInVars(), globalsToOutline(),
        inLoops(), outFlows() {}

  bool isEnclosingLoopInRegion(id_t loopId = -1) {
    int d = depth();
    for (int i = 0; i < d; i++) {
      Flow *v = getParent<WhileFlow>(i);
      if (!v)
        v = getParent<ForFlow>(i);
      if (!v)
        v = getParent<ImperativeForFlow>(i);

      if (v && (loopId == -1 || loopId == v->getId()))
        return inLoops.count(v->getId()) > 0;
    }
    return false;
  }

  void handle(WhileFlow *v) override {
    if (inRegion)
      inLoops.insert(v->getId());
  }

  void handle(ForFlow *v) override {
    if (inRegion)
      inLoops.insert(v->getId());
  }

  void handle(ImperativeForFlow *v) override {
    if (inRegion)
      inLoops.insert(v->getId());
  }

  void handle(ReturnInstr *v) override {
    if (inRegion)
      outFlows.push_back(v);
  }

  void handle(BreakInstr *v) override {
    auto *loop = v->getLoop();
    if (inRegion && !isEnclosingLoopInRegion(loop ? loop->getId() : -1))
      outFlows.push_back(v);
  }

  void handle(ContinueInstr *v) override {
    auto *loop = v->getLoop();
    if (inRegion && !isEnclosingLoopInRegion(loop ? loop->getId() : -1))
      outFlows.push_back(v);
  }

  void handle(YieldInstr *v) override {
    if (inRegion)
      invalid = true;
  }

  void handle(YieldInInstr *v) override {
    if (inRegion)
      invalid = true;
  }

  void handle(StackAllocInstr *v) override {
    if (inRegion)
      invalid = true;
  }

  void handle(AssignInstr *v) override {
    if (inRegion) {
      auto *var = v->getLhs();
      modifiedInVars.insert(var->getId());
      if (outlineGlobals && var->isGlobal())
        globalsToOutline.insert(var->getId());
    }
  }

  void handle(PointerValue *v) override {
    if (inRegion) {
      auto *var = v->getVar();
      modifiedInVars.insert(var->getId());
      if (outlineGlobals && var->isGlobal())
        globalsToOutline.insert(var->getId());
    }
  }

  void visit(SeriesFlow *v) override {
    if (v->getId() != flowRegion->getId())
      return Operator::visit(v);

    auto it = flowRegion->begin();
    for (; it != begin; ++it) {
      (*it)->accept(*this);
    }

    inRegion = true;

    for (; it != end; ++it) {
      (*it)->accept(*this);
    }

    inRegion = false;

    for (; it != flowRegion->end(); ++it) {
      (*it)->accept(*this);
    }
  }

  void visit(BodiedFunc *v) override {
    for (auto it = v->arg_begin(); it != v->arg_end(); ++it) {
      outVars.insert((*it)->getId());
    }
    Operator::visit(v);
  }

  void preHook(Node *node) override {
    auto vars = node->getUsedVariables();
    auto &set = (inRegion ? inVars : outVars);
    for (auto *var : vars) {
      if (!var->isGlobal())
        set.insert(var->getId());
      else if (inRegion && allByValue && !isA<Func>(var))
        globalsToOutline.insert(var->getId());
    }
  }

  // private = used in region AND NOT used outside region
  std::unordered_set<id_t> getPrivateVars() {
    std::unordered_set<id_t> privateVars;
    for (auto id : inVars) {
      if (outVars.count(id) == 0)
        privateVars.insert(id);
    }
    return privateVars;
  }

  // shared = used in region AND used outside region
  std::unordered_set<id_t> getSharedVars() {
    std::unordered_set<id_t> sharedVars;
    for (auto id : inVars) {
      if (outVars.count(id) > 0)
        sharedVars.insert(id);
    }
    return sharedVars;
  }

  // mod = shared AND modified in region
  std::unordered_set<id_t> getModVars() {
    if (allByValue)
      return {};

    std::unordered_set<id_t> modVars, shared = getSharedVars();
    for (auto id : modifiedInVars) {
      if (globalsToOutline.count(id) > 0 || shared.count(id) > 0)
        modVars.insert(id);
    }
    return modVars;
  }

  OutlineResult outline(bool allowOutflows = true) {
    if (invalid)
      return {};

    auto *M = flowRegion->getModule();
    std::vector<std::pair<Var *, Var *>> remap; // mapping of old vars to new func vars
    std::vector<types::Type *> argTypes;        // arg types of new func
    std::vector<std::string> argNames;          // arg names of new func
    std::vector<OutlineResult::ArgKind> argKinds; // arg information given back to user

    // Figure out arguments and outlined function type:
    //   - Private variables can be made local to the new function
    //   - Shared variables will be passed as arguments
    //   - Modified+shared variables will be passed as pointers
    unsigned idx = 0;
    auto shared = getSharedVars();
    shared.insert(globalsToOutline.begin(), globalsToOutline.end());
    auto mod = getModVars();
    for (auto id : shared) {
      Var *var = M->getVar(id);
      seqassertn(var, "unknown var id [{}]", var->getSrcInfo());
      remap.emplace_back(var, nullptr);
      const bool isMod = (mod.count(id) > 0);
      types::Type *type = isMod ? M->getPointerType(var->getType()) : var->getType();
      argTypes.push_back(type);
      argNames.push_back(var->getName());
      argKinds.push_back(isMod ? OutlineResult::ArgKind::MODIFIED
                               : OutlineResult::ArgKind::CONSTANT);
    }

    // Check if we need to handle control flow externally.
    // If so, function will return an int code indicating control.
    const bool callIndicatesControl = !outFlows.empty();
    if (callIndicatesControl && !allowOutflows)
      return {};
    auto *funcType = M->getFuncType(
        callIndicatesControl ? M->getIntType() : M->getNoneType(), argTypes);
    auto *outlinedFunc = M->Nr<BodiedFunc>("__outlined");
    outlinedFunc->realize(funcType, argNames);

    // Insert function arguments in variable remappings.
    idx = 0;
    for (auto it = outlinedFunc->arg_begin(); it != outlinedFunc->arg_end(); ++it) {
      remap[idx] = {std::get<0>(remap[idx]), *it};
      ++idx;
    }

    // Make private vars locals of the new function.
    for (auto id : getPrivateVars()) {
      Var *var = M->getVar(id);
      seqassertn(var, "unknown var id [{}]", var->getSrcInfo());
      Var *newVar = M->N<Var>(var->getSrcInfo(), var->getType(), /*global=*/false,
                              /*external=*/false, var->getName());
      remap.emplace_back(var, newVar);
      outlinedFunc->push_back(newVar);
    }

    // Delete outlined region from parent function and insert into outlined function.
    auto *body = M->N<SeriesFlow>((*begin)->getSrcInfo());
    auto it = begin;
    while (it != end) {
      body->push_back(*it);
      it = flowRegion->erase(it);
    }
    outlinedFunc->setBody(body);

    // Replace vars and externally-handled flows.
    OutlineReplacer outRep(M, mod, remap, outFlows);
    body->accept(outRep);

    // Determine arguments for call to outlined function.
    std::vector<Value *> args;
    for (unsigned i = 0; i < shared.size(); i++) {
      Var *var = std::get<0>(remap[i]);
      Value *arg = (mod.count(var->getId()) > 0)
                       ? static_cast<Value *>(M->Nr<PointerValue>(var))
                       : M->Nr<VarValue>(var);
      args.push_back(arg);
    }
    auto *outlinedCall = call(outlinedFunc, args);

    // Check if we need external control-flow handling.
    if (callIndicatesControl) {
      auto *codeVar = M->Nr<Var>(M->getIntType()); // result of outlined func call
      parent->push_back(codeVar);
      it = flowRegion->insert(it, M->Nr<AssignInstr>(codeVar, outlinedCall));
      // Check each return code of the function. 0 means normal return; do nothing.
      for (unsigned i = 0; i < outFlows.size(); i++) {
        // Generate "if (result == code) { action }".
        auto *codeVal = M->getInt(i + 1); // 1-based by convention
        auto *codeCheck = (*codeVal == *M->Nr<VarValue>(codeVar));
        auto *codeBody = series(outFlows[i]);
        auto *codeIf = M->Nr<IfFlow>(codeCheck, codeBody);
        ++it;
        it = flowRegion->insert(it, codeIf);
      }
    } else {
      it = flowRegion->insert(it, outlinedCall);
    }

    return {outlinedFunc, outlinedCall, argKinds, static_cast<int>(outFlows.size())};
  }
};

} // namespace

OutlineResult outlineRegion(BodiedFunc *parent, SeriesFlow *series,
                            decltype(series->begin()) begin,
                            decltype(series->end()) end, bool allowOutflows,
                            bool outlineGlobals, bool allByValue) {
  if (begin == end)
    return {};
  Outliner outliner(parent, series, begin, end, outlineGlobals, allByValue);
  parent->accept(outliner);
  return outliner.outline(allowOutflows);
}

OutlineResult outlineRegion(BodiedFunc *parent, SeriesFlow *series, bool allowOutflows,
                            bool outlineGlobals, bool allByValue) {
  return outlineRegion(parent, series, series->begin(), series->end(), allowOutflows,
                       outlineGlobals, allByValue);
}

} // namespace util
} // namespace ir
} // namespace codon
