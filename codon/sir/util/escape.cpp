#include "escape.h"

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <utility>

#include "codon/sir/util/irtools.h"

namespace codon {
namespace ir {
namespace util {
namespace {

template <typename S, typename T> bool contains(const S &x, T i) {
  for (auto a : x) {
    if (a == i)
      return true;
  }
  return false;
}

template <typename T> bool shouldTrack(T *x) {
  // We only care about things with pointers,
  // since you can't capture primitive types
  // like int, float, etc.
  return x && !x->getType()->isAtomic();
}

enum DerivedKind {
  ORIGIN = 0,
  ASSIGN,
  MEMBER,
  INSERT,
  CALL,
  REFERENCE,
};

struct CaptureInfo {
  std::vector<unsigned> argCaptures; // what other arguments capture this?
  bool returnCaptures = false;       // does return capture this?
  bool externCaptures = false;       // is this externally captured by a global?

  operator bool() const {
    return !argCaptures.empty() || returnCaptures || externCaptures;
  }

  static CaptureInfo nothing() { return {}; }

  static CaptureInfo unknown(Func *func) {
    CaptureInfo c;
    unsigned i = 0;
    for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
      if (shouldTrack(*it))
        c.argCaptures.push_back(i);
      ++i;
    }
    c.returnCaptures = true;
    c.externCaptures = true;
    return c;
  }
};

struct DerivedValInfo {
  struct Element {
    DerivedKind kind;
    Var *var;
  };

  std::vector<Element> info;
};

struct DerivedVarInfo {
  struct Element {
    DerivedKind kind;
    Value *val;
  };

  std::vector<Element> info;
};

struct DerivedSet {
  unsigned argno;
  std::vector<id_t> args;
  std::unordered_map<id_t, DerivedValInfo> derivedVals;
  std::unordered_map<id_t, DerivedVarInfo> derivedVars;
  CaptureInfo result;

  bool isDerived(Var *v) const {
    return derivedVars.find(v->getId()) != derivedVars.end();
  }

  bool isDerived(Value *v) const {
    return derivedVals.find(v->getId()) != derivedVals.end();
  }

  void setDerived(Var *v, DerivedKind kind, Value *cause) {
    if (v->isGlobal())
      result.externCaptures = true;

    auto id = v->getId();
    if (contains(args, id) && !contains(result.argCaptures, id))
      result.argCaptures.push_back(id);

    auto it = derivedVars.find(id);
    if (it == derivedVars.end()) {
      DerivedVarInfo info = {{{kind, cause}}};
      derivedVars.emplace(id, info);
    } else {
      it->second.info.push_back({kind, cause});
    }
  }

  void setDerived(Value *v, DerivedKind kind, Var *cause = nullptr) {
    auto it = derivedVals.find(v->getId());
    if (it == derivedVals.end()) {
      DerivedValInfo info = {{{kind, cause}}};
      derivedVals.emplace(v->getId(), info);
    } else {
      it->second.info.push_back({kind, cause});
    }
  }

  unsigned size() const {
    unsigned total = 0;
    for (auto &e : derivedVals) {
      total += e.second.info.size();
    }
    for (auto &e : derivedVars) {
      total += e.second.info.size();
    }
    return total;
  }

  explicit DerivedSet(unsigned argno)
      : argno(argno), derivedVals(), derivedVars(), result() {}

  DerivedSet(unsigned argno, std::vector<id_t> args, Var *var, Value *cause)
      : argno(argno), args(std::move(args)), derivedVals(), derivedVars(), result() {
    setDerived(var, DerivedKind::ORIGIN, cause);
  }
};

const std::string PURE_ATTR = "std.internal.attributes.pure";
const std::string NO_SIDE_EFFECT_ATTR = "std.internal.attributes.no_side_effect";
const std::string NO_CAPTURE_ATTR = "std.internal.attributes.nocapture";
const std::string DERIVES_ATTR = "std.internal.attributes.derives";

bool noCaptureByAnnotation(Func *func) {
  return util::hasAttribute(func, PURE_ATTR) ||
         util::hasAttribute(func, NO_SIDE_EFFECT_ATTR) ||
         util::hasAttribute(func, NO_CAPTURE_ATTR);
}

std::vector<CaptureInfo> makeAllCaptureInfo(Func *func) {
  std::vector<CaptureInfo> result;
  for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
    result.push_back(shouldTrack(*it) ? CaptureInfo::unknown(func)
                                      : CaptureInfo::nothing());
  }
  return result;
}

std::vector<CaptureInfo> makeNoCaptureInfo(Func *func, bool derives) {
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
  analyze::dataflow::RDResult *reaching;
  std::unordered_map<id_t, std::vector<CaptureInfo>> results;

  explicit CaptureContext(analyze::dataflow::RDResult *reaching)
      : reaching(reaching), results() {}

  std::vector<CaptureInfo> get(Func *func);
  void set(Func *func, const std::vector<CaptureInfo> &result);
};

// This visitor answers the questions of what vars are
// releavant to track in a capturing expression. For
// example, in "a[i] = x", the expression "a[i]" captures
// "x"; in this case we need to track "a" but the variable
// "i" (typically) we would not care about.
struct ExtractVars : public util::Visitor {
  CaptureContext &cc;
  std::unordered_set<id_t> vars;
  bool escapes;

  explicit ExtractVars(CaptureContext &cc)
      : util::Visitor(), cc(cc), vars(), escapes(false) {}

  template <typename Node> void process(Node *v) { v->accept(*this); }

  void add(Var *v) {
    if (shouldTrack(v))
      vars.insert(v->getId());
  }

  void defaultVisit(Node *) override {}

  void visit(VarValue *v) override { add(v->getVar()); }

  void visit(PointerValue *v) override { add(v->getVar()); }

  void visit(CallInstr *v) override {
    auto capInfo = cc.get(util::getFunc(v->getCallee()));
    unsigned i = 0;
    for (auto *arg : *v) {
      if (shouldTrack(arg) && capInfo[i].returnCaptures)
        process(arg);
      ++i;
    }
  }

  void visit(YieldInInstr *v) override {
    // We have no idea what the yield-in
    // value could be, so just assume we
    // escape in this case.
    escapes = true;
  }

  void visit(TernaryInstr *v) override {
    process(v->getTrueValue());
    process(v->getFalseValue());
  }

  void visit(ExtractInstr *v) override { process(v->getVal()); }

  void visit(FlowInstr *v) override { process(v->getValue()); }

  void visit(dsl::CustomInstr *v) override {
    // TODO
  }
};

bool extractVars(CaptureContext &cc, Value *v, std::vector<Var *> &result) {
  auto *M = v->getModule();
  ExtractVars ev(cc);
  v->accept(ev);
  for (auto id : ev.vars) {
    result.push_back(M->getVar(id));
  }
  return ev.escapes;
}

struct CaptureTracker : public Operator {
  CaptureContext &cc;
  BodiedFunc *func;
  analyze::dataflow::CFGraph *cfg;
  analyze::dataflow::RDInspector *rd;
  std::vector<DerivedSet> dsets;

  CaptureTracker(CaptureContext &cc, BodiedFunc *func, analyze::dataflow::CFGraph *cfg,
                 analyze::dataflow::RDInspector *rd)
      : Operator(), cc(cc), func(func), cfg(cfg), rd(rd), dsets() {
    using analyze::dataflow::SyntheticAssignInstr;

    // find synthetic assignments in CFG for argument vars
    auto *entry = cfg->getEntryBlock();
    std::unordered_map<id_t, SyntheticAssignInstr *> synthAssigns;

    for (auto *v : *entry) {
      if (auto *synth = cast<SyntheticAssignInstr>(v)) {
        if (shouldTrack(synth->getLhs()))
          synthAssigns[synth->getLhs()->getId()] =
              const_cast<SyntheticAssignInstr *>(synth);
      }
    }

    // extract arguments
    std::vector<id_t> args;
    for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
      args.push_back((*it)->getId());
    }

    // make a derived set for each function argument
    unsigned argno = 0;
    for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
      if (!shouldTrack(*it))
        continue;

      auto it2 = synthAssigns.find((*it)->getId());
      seqassert(it2 != synthAssigns.end(),
                "could not find synthetic assignment for arg var");
      dsets.push_back(DerivedSet(argno++, args, *it, it2->second));
    }
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

  void forEachDSetOf(Var *v, std::function<void(DerivedSet &)> func) {
    if (!v)
      return;

    for (auto &dset : dsets) {
      if (dset.isDerived(v))
        func(dset);
    }
  }

  void handleVarReference(Value *v, Var *var) {
    forEachDSetOf(var, [&](DerivedSet &dset) {
      // Make sure the var at this point is reached by
      // at least one definition that has led to a
      // derived value.
      auto mySet = rd->getReachingDefinitions(var, v);
      auto it = dset.derivedVars.find(var->getId());
      if (it == dset.derivedVars.end())
        return;

      for (auto &e : it->second.info) {
        auto otherSet = rd->getReachingDefinitions(var, e.val);
        bool derived = false;
        for (auto &elem : mySet) {
          if (otherSet.count(elem)) {
            derived = true;
            break;
          }
        }
        if (derived) {
          dset.setDerived(v, DerivedKind::REFERENCE, var);
          return;
        }
      }
    });
  }

  void handle(VarValue *v) override { handleVarReference(v, v->getVar()); }

  void handle(PointerValue *v) override { handleVarReference(v, v->getVar()); }

  void handle(AssignInstr *v) override {
    forEachDSetOf(v->getRhs(), [&](DerivedSet &dset) {
      dset.setDerived(v->getLhs(), DerivedKind::ASSIGN, v);
    });
  }

  void handle(ExtractInstr *v) override {
    if (!shouldTrack(v))
      return;

    forEachDSetOf(v->getVal(),
                  [&](DerivedSet &dset) { dset.setDerived(v, DerivedKind::MEMBER); });
  }

  void handle(InsertInstr *v) override {
    std::vector<Var *> vars;
    bool escapes = extractVars(cc, v->getLhs(), vars);

    forEachDSetOf(v->getRhs(), [&](DerivedSet &dset) {
      if (escapes)
        dset.result.externCaptures = true;

      for (auto *var : vars) {
        dset.setDerived(var, DerivedKind::INSERT, v);
      }
    });
  }

  void handle(CallInstr *v) override {
    std::vector<Value *> args(v->begin(), v->end());
    std::vector<CaptureInfo> capInfo;

    if (auto *func = util::getFunc(v->getCallee())) {
      capInfo = cc.get(func);
    } else {
      std::vector<unsigned> argCaptures;
      unsigned i = 0;
      for (auto *arg : args) {
        if (shouldTrack(arg))
          argCaptures.push_back(i);
        ++i;
      }

      for (auto *arg : args) {
        CaptureInfo info = CaptureInfo::nothing();
        if (shouldTrack(arg)) {
          info.argCaptures = argCaptures;
          info.returnCaptures = true;
          info.externCaptures = true;
        }
        capInfo.push_back(info);
      }
    }

    unsigned i = 0;
    for (auto *arg : args) {
      forEachDSetOf(arg, [&](DerivedSet &dset) {
        auto &info = capInfo[i];

        // Process all other arguments that capture us.
        for (auto argno : info.argCaptures) {
          Value *arg = args[argno];
          std::vector<Var *> vars;
          bool escapes = extractVars(cc, arg, vars);
          if (escapes)
            dset.result.externCaptures = true;

          for (auto *var : vars) {
            dset.setDerived(var, DerivedKind::CALL, v);
          }
        }

        // Check if the return value captures.
        if (info.returnCaptures)
          dset.setDerived(v, DerivedKind::CALL);

        // Check if we're externally captured.
        if (info.externCaptures)
          dset.result.externCaptures = true;
      });
      ++i;
    }
  }

  void handle(ForFlow *v) override {
    auto *var = v->getVar();
    if (!shouldTrack(var))
      return;

    forEachDSetOf(v->getIter(), [&](DerivedSet &dset) {
      using analyze::dataflow::SyntheticAssignInstr;
      bool found = false;
      for (auto it = cfg->synth_begin(); it != cfg->synth_end(); ++it) {
        if (auto *synth = cast<SyntheticAssignInstr>(*it)) {
          if (synth->getKind() == SyntheticAssignInstr::Kind::NEXT_VALUE &&
              synth->getLhs()->getId() == var->getId()) {
            seqassert(!found, "found multiple synthetic assignments for loop var");
            dset.setDerived(var, DerivedKind::ASSIGN, synth);
            found = true;
          }
        }
      }
    });
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
    forEachDSetOf(v->getValue(),
                  [&](DerivedSet &dset) { dset.result.externCaptures = true; });
  }
};

std::vector<CaptureInfo> CaptureContext::get(Func *func) {
  // Don't know anything about external/LLVM funcs so use annotations.
  if (isA<ExternalFunc>(func) || isA<LLVMFunc>(func)) {
    bool derives = util::hasAttribute(func, DERIVES_ATTR);
    auto info = noCaptureByAnnotation(func) ? makeNoCaptureInfo(func, derives)
                                            : makeAllCaptureInfo(func);
  }

  // Only Tuple.__new__(...) and Generator.__promise__(self) capture.
  if (isA<InternalFunc>(func)) {
    bool isTupleNew = func->getUnmangledName() == "__new__" &&
                      std::distance(func->arg_begin(), func->arg_end()) == 1 &&
                      isA<types::RecordType>(func->arg_front()->getType());

    bool isPromise = func->getUnmangledName() == "__promise__" &&
                     std::distance(func->arg_begin(), func->arg_end()) == 2 &&
                     isA<types::GeneratorType>(func->arg_front()->getType()) &&
                     isA<types::GeneratorType>(func->arg_back()->getType());

    return (isTupleNew || isPromise) ? makeAllCaptureInfo(func)
                                     : makeNoCaptureInfo(func, /*derives=*/false);
  }

  // Bodied function
  if (isA<BodiedFunc>(func)) {
    auto it = results.find(func->getId());
    if (it != results.end())
      return it->second;

    set(func, makeAllCaptureInfo(func));

    auto it1 = reaching->cfgResult->graphs.find(func->getId());
    seqassert(it1 != reaching->cfgResult->graphs.end(),
              "could not find function in CFG results");

    auto it2 = reaching->results.find(func->getId());
    seqassert(it2 != reaching->results.end(),
              "could not find function in reaching-definitions results");

    CaptureTracker ct(*this, cast<BodiedFunc>(func), it1->second.get(),
                      it2->second.get());
    unsigned oldSize = 0;
    do {
      oldSize = ct.size();
      func->accept(ct);
    } while (ct.size() != oldSize);

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

  seqassert(false, "unknown function type");
  return {};
}

void CaptureContext::set(Func *func, const std::vector<CaptureInfo> &result) {
  results[func->getId()] = result;
}

} // namespace

EscapeResult escapes(BodiedFunc *parent, Value *value,
                     analyze::dataflow::RDResult *reaching) {
  auto it1 = reaching->cfgResult->graphs.find(parent->getId());
  seqassert(it1 != reaching->cfgResult->graphs.end(),
            "could not find parent function in CFG results");

  auto it2 = reaching->results.find(parent->getId());
  seqassert(it2 != reaching->results.end(),
            "could not find parent function in reaching-definitions results");

  CaptureContext cc(reaching);
  CaptureTracker ct(cc, parent, it1->second.get(), it2->second.get());
  unsigned oldSize = 0;
  do {
    oldSize = ct.size();
    parent->accept(ct);
  } while (ct.size() != oldSize);

  return EscapeResult::YES;
}

} // namespace util
} // namespace ir
} // namespace codon
