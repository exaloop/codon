// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "capture.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "codon/cir/analyze/dataflow/reaching.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/side_effect.h"

namespace codon {
namespace ir {
namespace analyze {
namespace dataflow {
namespace {

template <typename S, typename T> bool contains(const S &x, T i) {
  for (auto a : x) {
    if (a == i)
      return true;
  }
  return false;
}

template <typename S, typename T> bool containsId(const S &x, T i) {
  for (auto a : x) {
    if (a->getId() == i->getId())
      return true;
  }
  return false;
}

template <typename T> bool shouldTrack(const T *x) {
  // We only care about things with pointers,
  // since you can't capture primitive types
  // like int, float, etc.
  return x && !x->getType()->isAtomic();
}

template <> bool shouldTrack(const types::Type *x) { return x && !x->isAtomic(); }

struct CaptureContext;

bool extractVars(CaptureContext &cc, const Value *v, std::vector<const Var *> &result);

bool reachable(CFBlock *start, CFBlock *end, std::unordered_set<CFBlock *> &seen) {
  if (start == end)
    return true;

  if (seen.count(start))
    return false;

  seen.insert(start);
  for (auto it = start->successors_begin(); it != start->successors_end(); ++it) {
    if (reachable(*it, end, seen))
      return true;
  }

  return false;
}

// Check if one value must always be encountered before another, if
// it is to be encountered at all. This is NOT the same as domination
// since we can have "(if _: B) ; A", where B does not dominate A yet
// must always occur before A is it does occur.
bool happensBefore(const Value *before, const Value *after, CFGraph *cfg,
                   DominatorInspector *dom) {
  auto *beforeBlock = cfg->getBlock(before);
  auto *afterBlock = cfg->getBlock(after);

  if (!beforeBlock || !afterBlock)
    return false;

  // If values are in the same block we just need to see
  // which one shows up first.
  if (beforeBlock == afterBlock) {
    for (auto *val : *beforeBlock) {
      if (val->getId() == before->getId())
        return true;
      else if (val->getId() == after->getId())
        return false;
    }
    seqassertn(false, "could not find values in CFG block");
    return false;
  }

  // If we have different blocks, then either 'before' dominates
  // 'after', in which case the answer is true, or there must be
  // no paths from 'afterBlock' to 'beforeBlock'.
  std::unordered_set<CFBlock *> seen;
  return dom->isDominated(after, before) || !reachable(afterBlock, beforeBlock, seen);
}

struct RDManager {
  struct IDPairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &pair) const {
      return (std::hash<T1>()(pair.first) << 32) ^ std::hash<T2>()(pair.second);
    }
  };

  RDInspector *rd;
  std::unordered_map<std::pair<id_t, id_t>, std::unordered_set<id_t>, IDPairHash> cache;

  explicit RDManager(RDInspector *rd) : rd(rd), cache() {}

  std::unordered_set<id_t> getReachingDefinitions(const Var *var, const Value *loc) {
    auto key = std::make_pair(var->getId(), loc->getId());
    auto it = cache.find(key);
    if (it == cache.end()) {
      auto result = rd->getReachingDefinitions(var, loc);
      cache.emplace(key, result);
      return result;
    } else {
      return it->second;
    }
  }

  bool isInvalid(const Var *v) { return rd->isInvalid(v); }
};

struct DerivedSet {
  const Func *func;
  const Var *root;
  std::vector<id_t> args;
  std::unordered_set<id_t> derivedVals;
  std::unordered_map<id_t, std::vector<const Value *>> derivedVars;
  CaptureInfo result;

  void setReturnCaptured() {
    if (shouldTrack(util::getReturnType(func)))
      result.returnCaptures = true;
  }

  void setExternCaptured() {
    setReturnCaptured();
    result.externCaptures = true;
  }

  bool isDerived(const Var *v, const Value *loc, RDManager &rd) const {
    auto it = derivedVars.find(v->getId());
    if (it == derivedVars.end())
      return false;

    // We assume global references are always derived
    // if the var is derived, since they can change
    // at any point as far as we know. Same goes for
    // vars untracked by the reaching-def analysis.
    if (v->isGlobal() || rd.isInvalid(v))
      return true;

    // Make sure the var at this point is reached by
    // at least one definition that has led to a
    // derived value.
    auto mySet = rd.getReachingDefinitions(v, loc);
    for (auto *cause : it->second) {
      auto otherSet = rd.getReachingDefinitions(v, cause);
      bool derived = false;

      for (auto &elem : mySet) {
        if (otherSet.count(elem))
          return true;
      }
    }

    return false;
  }

  bool isDerived(const Value *v) const {
    return derivedVals.find(v->getId()) != derivedVals.end();
  }

  void setDerived(const Var *v, const Value *cause, bool shouldArgCapture = true) {
    if (!shouldTrack(v))
      return;

    if (v->isGlobal())
      setExternCaptured();

    auto id = v->getId();
    if (shouldArgCapture && root && id != root->getId()) {
      for (unsigned i = 0; i < args.size(); i++) {
        if (args[i] == id && !contains(result.argCaptures, i))
          result.argCaptures.push_back(i);
      }
    }

    auto it = derivedVars.find(id);
    if (it == derivedVars.end()) {
      std::vector<const Value *> info = {cause};
      derivedVars.emplace(id, info);
    } else {
      if (!containsId(it->second, cause))
        it->second.push_back(cause);
    }
  }

  void setDerived(const Value *v) {
    if (!shouldTrack(v))
      return;

    derivedVals.insert(v->getId());
  }

  unsigned size() const {
    unsigned total = derivedVals.size();
    for (auto &e : derivedVars) {
      total += e.second.size();
    }
    return total;
  }

  explicit DerivedSet(const Func *func, const Var *root = nullptr)
      : func(func), root(root), args(), derivedVals(), derivedVars(), result() {}

  // Set for function argument
  DerivedSet(const Func *func, const Var *root, const Value *cause)
      : DerivedSet(func, root) {
    // extract arguments
    for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
      args.push_back((*it)->getId());
    }

    setDerived(root, cause);
  }

  // Set for function argument
  DerivedSet(const Func *func, const Value *value, CaptureContext &cc)
      : DerivedSet(func) {
    std::vector<const Var *> vars;
    bool escapes = extractVars(cc, value, vars);
    if (escapes)
      setExternCaptured();

    setDerived(value);
    for (auto *var : vars) {
      setDerived(var, value);
    }
  }
};

bool noCaptureByAnnotation(const Func *func) {
  return util::hasAttribute(func, util::PURE_ATTR) ||
         util::hasAttribute(func, util::NO_SIDE_EFFECT_ATTR) ||
         util::hasAttribute(func, util::NO_CAPTURE_ATTR);
}

std::vector<CaptureInfo> makeAllCaptureInfo(const Func *func) {
  std::vector<CaptureInfo> result;
  for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
    result.push_back(CaptureInfo::unknown(func, (*it)->getType()));
  }
  return result;
}

std::vector<CaptureInfo> makeNoCaptureInfo(const Func *func, bool derives) {
  std::vector<CaptureInfo> result;
  for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
    auto info = CaptureInfo::nothing();
    if (derives && shouldTrack(*it))
      info.returnCaptures = true;
    result.push_back(info);
  }
  return result;
}

struct CaptureContext {
  RDResult *reaching;
  DominatorResult *dominating;
  std::unordered_map<id_t, std::vector<CaptureInfo>> results;

  CaptureContext(RDResult *reaching, DominatorResult *dominating)
      : reaching(reaching), dominating(dominating), results() {}

  std::vector<CaptureInfo> get(const Func *func);
  void set(const Func *func, const std::vector<CaptureInfo> &result);

  CFGraph *getCFGraph(const Func *func) {
    auto it = reaching->cfgResult->graphs.find(func->getId());
    seqassertn(it != reaching->cfgResult->graphs.end(),
               "could not find function in CFG results");
    return it->second.get();
  }

  RDInspector *getRDInspector(const Func *func) {
    auto it = reaching->results.find(func->getId());
    seqassertn(it != reaching->results.end(),
               "could not find function in reaching-definitions results");
    return it->second.get();
  }

  DominatorInspector *getDomInspector(const Func *func) {
    auto it = dominating->results.find(func->getId());
    seqassertn(it != dominating->results.end(),
               "could not find function in dominator results");
    return it->second.get();
  }
};

// This visitor answers the questions of what vars are
// relevant to track in a capturing expression. For
// example, in "a[i] = x", the expression "a[i]" captures
// "x"; in this case we need to track "a" but the variable
// "i" (typically) we would not care about.
struct ExtractVars : public util::ConstVisitor {
  CaptureContext &cc;
  std::unordered_set<id_t> vars;
  bool escapes;

  explicit ExtractVars(CaptureContext &cc)
      : util::ConstVisitor(), cc(cc), vars(), escapes(false) {}

  template <typename Node> void process(const Node *v) { v->accept(*this); }

  void add(const Var *v) {
    if (shouldTrack(v))
      vars.insert(v->getId());
  }

  void defaultVisit(const Node *) override {}

  void visit(const VarValue *v) override { add(v->getVar()); }

  void visit(const PointerValue *v) override { add(v->getVar()); }

  void visit(const CallInstr *v) override {
    if (auto *func = util::getFunc(v->getCallee())) {
      auto capInfo = cc.get(util::getFunc(v->getCallee()));
      unsigned i = 0;
      for (auto *arg : *v) {
        // note possibly capInfo.size() != v->numArgs() if calling vararg C function
        auto info = (i < capInfo.size()) ? capInfo[i]
                                         : CaptureInfo::unknown(func, arg->getType());
        if (shouldTrack(arg) && capInfo[i].returnCaptures)
          process(arg);
        ++i;
      }
    } else {
      for (auto *arg : *v) {
        if (shouldTrack(arg))
          process(arg);
      }
    }
  }

  void visit(const YieldInInstr *v) override {
    // We have no idea what the yield-in
    // value could be, so just assume we
    // escape in this case.
    escapes = true;
  }

  void visit(const TernaryInstr *v) override {
    process(v->getTrueValue());
    process(v->getFalseValue());
  }

  void visit(const ExtractInstr *v) override { process(v->getVal()); }

  void visit(const FlowInstr *v) override { process(v->getValue()); }

  void visit(const dsl::CustomInstr *v) override {
    // TODO
  }
};

bool extractVars(CaptureContext &cc, const Value *v, std::vector<const Var *> &result) {
  auto *M = v->getModule();
  ExtractVars ev(cc);
  v->accept(ev);
  for (auto id : ev.vars) {
    result.push_back(M->getVar(id));
  }
  return ev.escapes;
}

struct CaptureTracker : public util::Operator {
  CaptureContext &cc;
  CFGraph *cfg;
  RDManager rd;
  DominatorInspector *dom;
  std::vector<DerivedSet> dsets;

  CaptureTracker(CaptureContext &cc, const Func *func, bool isArg)
      : Operator(), cc(cc), cfg(cc.getCFGraph(func)), rd(cc.getRDInspector(func)),
        dom(cc.getDomInspector(func)), dsets() {}

  CaptureTracker(CaptureContext &cc, const BodiedFunc *func)
      : CaptureTracker(cc, func, /*isArg=*/true) {
    // find synthetic assignments in CFG for argument vars
    auto *entry = cfg->getEntryBlock();
    std::unordered_map<id_t, const SyntheticAssignInstr *> synthAssigns;

    for (auto *v : *entry) {
      if (auto *synth = cast<SyntheticAssignInstr>(v)) {
        if (shouldTrack(synth->getLhs()))
          synthAssigns[synth->getLhs()->getId()] = synth;
      }
    }

    // extract arguments
    std::vector<id_t> args;
    for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
      args.push_back((*it)->getId());
    }

    // make a derived set for each function argument
    for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
      if (!shouldTrack(*it))
        continue;

      auto it2 = synthAssigns.find((*it)->getId());
      seqassertn(it2 != synthAssigns.end(),
                 "could not find synthetic assignment for arg var");
      dsets.push_back(DerivedSet(func, *it, it2->second));
    }
  }

  CaptureTracker(CaptureContext &cc, const BodiedFunc *func, const Value *value)
      : CaptureTracker(cc, func, /*isArg=*/false) {
    dsets.push_back(DerivedSet(func, value, cc));
  }

  unsigned size() const {
    unsigned total = 0;
    for (auto &dset : dsets) {
      total += dset.size();
    }
    return total;
  }

  void forEachDSetOf(Value *v, std::function<void(DerivedSet &)> func) {
    if (!v)
      return;

    for (auto &dset : dsets) {
      if (dset.isDerived(v))
        func(dset);
    }
  }

  void forEachDSetOf(Var *v, Value *loc, std::function<void(DerivedSet &)> func) {
    if (!v)
      return;

    for (auto &dset : dsets) {
      if (dset.isDerived(v, loc, rd))
        func(dset);
    }
  }

  void forwardLink(Value *from, Value *cause, const std::vector<const Var *> &toVars,
                   bool toEscapes, bool shouldArgCapture) {
    forEachDSetOf(from, [&](DerivedSet &dset) {
      if (toEscapes)
        dset.setExternCaptured();

      for (auto *toVar : toVars) {
        dset.setDerived(toVar, cause, shouldArgCapture);
      }
    });
  }

  void backwardLinkFunc(DerivedSet &dset, Value *cause,
                        const std::vector<const Var *> &toVars,
                        const std::vector<const Var *> &fromVars, bool fromEscapes) {
    if (fromEscapes)
      dset.setExternCaptured();

    for (auto *toVar : toVars) {
      auto it = dset.derivedVars.find(toVar->getId());
      if (it == dset.derivedVars.end())
        continue;
      auto &toCauses = it->second;

      for (auto *toCause : toCauses) {
        if (isA<AssignInstr>(toCause) || isA<SyntheticAssignInstr>(toCause) ||
            happensBefore(toCause, cause, cfg, dom))
          continue;

        bool derived = false;
        if (toVar->isGlobal() || rd.isInvalid(toVar)) {
          derived = true;
        } else {
          auto mySet = rd.getReachingDefinitions(toVar, cause);
          auto otherSet = rd.getReachingDefinitions(toVar, toCause);
          for (auto &elem : mySet) {
            if (otherSet.count(elem)) {
              derived = true;
              break;
            }
          }
        }

        if (derived) {
          for (auto *fromVar : fromVars) {
            dset.setDerived(fromVar, toCause);
          }
        }
      }
    }
  }

  void link(Value *from, Value *to, Value *cause) {
    std::vector<const Var *> fromVars, toVars;
    bool fromEscapes = extractVars(cc, from, fromVars);
    bool toEscapes = extractVars(cc, to, toVars);

    forwardLink(from, cause, toVars, toEscapes, /*shouldArgCapture=*/true);
    forEachDSetOf(to, [&](DerivedSet &dset) {
      backwardLinkFunc(dset, cause, toVars, fromVars, fromEscapes);
    });
  }

  void link(Value *from, Var *to, Value *cause) {
    std::vector<const Var *> fromVars, toVars = {to};
    bool fromEscapes = extractVars(cc, from, fromVars);
    bool toEscapes = false;

    forwardLink(from, cause, toVars, toEscapes, /*shouldArgCapture=*/false);
    forEachDSetOf(to, cause, [&](DerivedSet &dset) {
      backwardLinkFunc(dset, cause, toVars, fromVars, fromEscapes);
    });
  }

  void handle(VarValue *v) override {
    forEachDSetOf(v->getVar(), v, [&](DerivedSet &dset) { dset.setDerived(v); });
  }

  void handle(PointerValue *v) override {
    forEachDSetOf(v->getVar(), v, [&](DerivedSet &dset) { dset.setDerived(v); });
  }

  void handle(AssignInstr *v) override { link(v->getRhs(), v->getLhs(), v); }

  void handle(ExtractInstr *v) override {
    if (!shouldTrack(v))
      return;

    forEachDSetOf(v->getVal(), [&](DerivedSet &dset) { dset.setDerived(v); });
  }

  void handle(InsertInstr *v) override {
    link(v->getRhs(), v->getLhs(), v);
    forEachDSetOf(v->getLhs(), [&](DerivedSet &dset) { dset.result.modified = true; });
  }

  void handle(CallInstr *v) override {
    std::vector<Value *> args(v->begin(), v->end());
    std::vector<CaptureInfo> capInfo;
    auto *func = util::getFunc(v->getCallee());

    if (func) {
      capInfo = cc.get(func);
    } else {
      std::vector<unsigned> argCaptures;
      unsigned i = 0;
      for (auto *arg : args) {
        if (shouldTrack(arg))
          argCaptures.push_back(i);
        ++i;
      }

      const bool returnCaptures = shouldTrack(v);
      for (auto *arg : args) {
        CaptureInfo info = CaptureInfo::nothing();
        if (shouldTrack(arg)) {
          info.argCaptures = argCaptures;
          info.returnCaptures = returnCaptures;
          info.externCaptures = true;
          info.modified = true;
        }
        capInfo.push_back(info);
      }
    }

    unsigned i = 0;
    for (auto *arg : args) {
      // note possibly capInfo.size() != v->numArgs() if calling vararg C function
      auto info = (i < capInfo.size()) ? capInfo[i]
                                       : CaptureInfo::unknown(func, arg->getType());
      for (auto argno : info.argCaptures) {
        Value *other = args[argno];
        link(arg, other, v);
      }

      forEachDSetOf(arg, [&](DerivedSet &dset) {
        // Check if the return value captures.
        if (info.returnCaptures)
          dset.setDerived(v);

        // Check if we're externally captured.
        if (info.externCaptures)
          dset.setExternCaptured();

        if (info.modified)
          dset.result.modified = true;
      });
      ++i;
    }
  }

  void handle(ForFlow *v) override {
    auto *var = v->getVar();
    if (!shouldTrack(var))
      return;

    forEachDSetOf(v->getIter(), [&](DerivedSet &dset) {
      bool found = false;
      for (auto it = cfg->synth_begin(); it != cfg->synth_end(); ++it) {
        if (auto *synth = cast<SyntheticAssignInstr>(*it)) {
          if (synth->getKind() == SyntheticAssignInstr::Kind::NEXT_VALUE &&
              synth->getLhs()->getId() == var->getId()) {
            seqassertn(!found, "found multiple synthetic assignments for loop var");
            dset.setDerived(var, synth);
            found = true;
          }
        }
      }
    });
  }

  void handle(TernaryInstr *v) override {
    forEachDSetOf(v->getTrueValue(), [&](DerivedSet &dset) { dset.setDerived(v); });
    forEachDSetOf(v->getFalseValue(), [&](DerivedSet &dset) { dset.setDerived(v); });
  }

  void handle(FlowInstr *v) override {
    forEachDSetOf(v->getValue(), [&](DerivedSet &dset) { dset.setDerived(v); });
  }

  void handle(dsl::CustomInstr *v) override {
    // TODO
  }

  // Actual capture points:

  void handle(ReturnInstr *v) override {
    forEachDSetOf(v->getValue(),
                  [&](DerivedSet &dset) { dset.result.returnCaptures = true; });
  }

  void handle(YieldInstr *v) override {
    forEachDSetOf(v->getValue(),
                  [&](DerivedSet &dset) { dset.result.returnCaptures = true; });
  }

  void handle(ThrowInstr *v) override {
    forEachDSetOf(v->getValue(), [&](DerivedSet &dset) { dset.setExternCaptured(); });
  }

  // Helper to run to completion

  void runToCompletion(const Func *func) {
    unsigned oldSize = 0;
    do {
      oldSize = size();
      const_cast<Func *>(func)->accept(*this);
      reset();
    } while (size() != oldSize);
  }
};

std::vector<CaptureInfo> CaptureContext::get(const Func *func) {
  // Don't know anything about external/LLVM funcs so use annotations.
  if (isA<ExternalFunc>(func) || isA<LLVMFunc>(func)) {
    bool derives = util::hasAttribute(func, util::DERIVES_ATTR);

    if (util::hasAttribute(func, util::SELF_CAPTURES_ATTR)) {
      auto ans = makeNoCaptureInfo(func, derives);
      if (!ans.empty())
        ans[0].modified = true;

      std::vector<Var *> argVars(func->arg_begin(), func->arg_end());
      for (unsigned i = 1; i < ans.size(); i++) {
        if (shouldTrack(argVars[i]))
          ans[i].argCaptures.push_back(0);
      }
      return ans;
    }

    return noCaptureByAnnotation(func) ? makeNoCaptureInfo(func, derives)
                                       : makeAllCaptureInfo(func);
  }

  // Only Tuple.__new__(...) and Generator.__promise__(self) capture.
  if (isA<InternalFunc>(func)) {
    bool isTupleNew = func->getUnmangledName() == "__new__" &&
                      isA<types::RecordType>(util::getReturnType(func));

    bool isPromise = func->getUnmangledName() == "__promise__" &&
                     std::distance(func->arg_begin(), func->arg_end()) == 1 &&
                     isA<types::GeneratorType>(func->arg_front()->getType());

    bool derives = (isTupleNew || isPromise);
    return makeNoCaptureInfo(func, derives);
  }

  // Bodied function
  if (isA<BodiedFunc>(func)) {
    auto it = results.find(func->getId());
    if (it != results.end())
      return it->second;

    set(func, makeAllCaptureInfo(func));

    CaptureTracker ct(*this, cast<BodiedFunc>(func));
    ct.runToCompletion(func);

    std::vector<CaptureInfo> answer;
    unsigned i = 0;
    for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
      if (shouldTrack(*it)) {
        answer.push_back(ct.dsets[i++].result);
      } else {
        answer.push_back(CaptureInfo::nothing());
      }
    }

    set(func, answer);
    return answer;
  }

  seqassertn(false, "unknown function type");
  return {};
}

void CaptureContext::set(const Func *func, const std::vector<CaptureInfo> &result) {
  results[func->getId()] = result;
}

} // namespace

CaptureInfo CaptureInfo::unknown(const Func *func, types::Type *type) {
  if (!shouldTrack(type))
    return CaptureInfo::nothing();

  CaptureInfo c;
  unsigned i = 0;
  for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
    if (shouldTrack(*it))
      c.argCaptures.push_back(i);
    ++i;
  }
  c.returnCaptures = shouldTrack(util::getReturnType(func));
  c.externCaptures = true;
  c.modified = true;
  return c;
}

const std::string CaptureAnalysis::KEY = "core-analyses-capture";

std::unique_ptr<Result> CaptureAnalysis::run(const Module *m) {
  auto res = std::make_unique<CaptureResult>();
  auto *rdResult = getAnalysisResult<RDResult>(rdAnalysisKey);
  auto *domResult = getAnalysisResult<DominatorResult>(domAnalysisKey);
  res->rdResult = rdResult;
  res->domResult = domResult;
  CaptureContext cc(rdResult, domResult);

  if (const auto *main = cast<BodiedFunc>(m->getMainFunc())) {
    auto ans = cc.get(main);
    res->results.emplace(main->getId(), ans);
  }

  for (const auto *var : *m) {
    if (const auto *f = cast<Func>(var)) {
      auto ans = cc.get(f);
      res->results.emplace(f->getId(), ans);
    }
  }

  return res;
}

CaptureInfo escapes(const BodiedFunc *func, const Value *value, CaptureResult *cr) {
  if (!shouldTrack(value))
    return CaptureInfo::nothing();

  CaptureContext cc(cr->rdResult, cr->domResult);
  cc.results = cr->results;
  CaptureTracker ct(cc, cast<BodiedFunc>(func), value);
  ct.runToCompletion(func);
  seqassertn(ct.dsets.size() == 1, "unexpected dsets size");
  return ct.dsets[0].result;
}

} // namespace dataflow
} // namespace analyze
} // namespace ir
} // namespace codon
