// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "openmp.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <unordered_set>

#include "codon/cir/transform/parallel/schedule.h"
#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/outlining.h"

namespace codon {
namespace ir {
namespace transform {
namespace parallel {
namespace {
const std::string ompModule = "std.openmp";
const std::string gpuModule = "std.gpu";
const std::string builtinModule = "std.internal.builtin";

void warn(const std::string &msg, const Value *v) {
  auto src = v->getSrcInfo();
  compilationWarning(msg, src.file, src.line, src.col);
}

struct OMPTypes {
  types::Type *i64 = nullptr;
  types::Type *i32 = nullptr;
  types::Type *i8ptr = nullptr;
  types::Type *i32ptr = nullptr;

  explicit OMPTypes(Module *M) {
    i64 = M->getIntType();
    i32 = M->getIntNType(32, /*sign=*/true);
    i8ptr = M->getPointerType(M->getByteType());
    i32ptr = M->getPointerType(i32);
  }
};

Var *getVarFromOutlinedArg(Value *arg) {
  if (auto *val = cast<VarValue>(arg)) {
    return val->getVar();
  } else if (auto *val = cast<PointerValue>(arg)) {
    return val->getVar();
  } else {
    seqassertn(false, "unknown outline var");
  }
  return nullptr;
}

Value *ptrFromFunc(Func *func) {
  auto *M = func->getModule();
  auto *funcType = func->getType();
  auto *rawMethod = M->getOrRealizeMethod(funcType, "__raw__", {funcType});
  seqassertn(rawMethod, "cannot find function __raw__ method");
  return util::call(rawMethod, {M->Nr<VarValue>(func)});
}

// we create the locks lazily to avoid them when they're not needed
struct ReductionLocks {
  Var *mainLock =
      nullptr; // lock used in calls to _reduce_no_wait and _end_reduce_no_wait
  Var *critLock = nullptr; // lock used in reduction critical sections

  Var *createLock(Module *M) {
    auto *lockType = M->getOrRealizeType("Lock", {}, ompModule);
    seqassertn(lockType, "openmp.Lock type not found");
    auto *var = M->Nr<Var>(lockType, /*global=*/true);
    static int counter = 1;
    var->setName(".omp_lock." + std::to_string(counter++));

    // add it to main function so it doesn't get demoted by IR pass
    auto *series = cast<SeriesFlow>(cast<BodiedFunc>(M->getMainFunc())->getBody());
    auto *init = (*lockType)();
    seqassertn(init, "could not initialize openmp.Lock");
    series->insert(series->begin(), M->Nr<AssignInstr>(var, init));

    return var;
  }

  Var *getMainLock(Module *M) {
    if (!mainLock)
      mainLock = createLock(M);
    return mainLock;
  }

  Var *getCritLock(Module *M) {
    if (!critLock)
      critLock = createLock(M);
    return critLock;
  }
};

struct Reduction {
  enum Kind {
    NONE,
    ADD,
    MUL,
    AND,
    OR,
    XOR,
    MIN,
    MAX,
  };

  Kind kind = Kind::NONE;
  Var *shared = nullptr;

  types::Type *getType() {
    auto *ptrType = cast<types::PointerType>(shared->getType());
    seqassertn(ptrType, "expected shared var to be of pointer type");
    return ptrType->getBase();
  }

  Value *getInitial() {
    if (!*this)
      return nullptr;
    auto *M = shared->getModule();
    auto *type = getType();

    if (isA<types::IntType>(type)) {
      switch (kind) {
      case Kind::ADD:
        return M->getInt(0);
      case Kind::MUL:
        return M->getInt(1);
      case Kind::AND:
        return M->getInt(~0);
      case Kind::OR:
        return M->getInt(0);
      case Kind::XOR:
        return M->getInt(0);
      case Kind::MIN:
        return M->getInt(std::numeric_limits<int64_t>::max());
      case Kind::MAX:
        return M->getInt(std::numeric_limits<int64_t>::min());
      default:
        return nullptr;
      }
    } else if (isA<types::FloatType>(type)) {
      switch (kind) {
      case Kind::ADD:
        return M->getFloat(0.);
      case Kind::MUL:
        return M->getFloat(1.);
      case Kind::MIN:
        return M->getFloat(std::numeric_limits<double>::max());
      case Kind::MAX:
        return M->getFloat(std::numeric_limits<double>::min());
      default:
        return nullptr;
      }
    } else if (isA<types::Float32Type>(type)) {
      auto *f32 = M->getOrRealizeType("float32");
      float value = 0.0;

      switch (kind) {
      case Kind::ADD:
        value = 0.0;
        break;
      case Kind::MUL:
        value = 1.0;
        break;
      case Kind::MIN:
        value = std::numeric_limits<float>::max();
        break;
      case Kind::MAX:
        value = std::numeric_limits<float>::min();
        break;
      default:
        return nullptr;
      }

      return (*f32)(*M->getFloat(value));
    }

    auto *init = (*type)();
    if (!init || !init->getType()->is(type))
      return nullptr;
    return init;
  }

  Value *generateNonAtomicReduction(Value *ptr, Value *arg) {
    auto *M = ptr->getModule();
    Value *lhs = util::ptrLoad(ptr);
    Value *result = nullptr;
    switch (kind) {
    case Kind::ADD:
      result = *lhs + *arg;
      break;
    case Kind::MUL:
      result = *lhs * *arg;
      break;
    case Kind::AND:
      result = *lhs & *arg;
      break;
    case Kind::OR:
      result = *lhs | *arg;
      break;
    case Kind::XOR:
      result = *lhs ^ *arg;
      break;
    case Kind::MIN:
    case Kind::MAX: {
      // signature is (tuple of args, key, default)
      auto name = (kind == Kind::MIN ? "min" : "max");
      auto *tup = util::makeTuple({lhs, arg});
      auto *none = (*M->getNoneType())();
      auto *fn = M->getOrRealizeFunc(
          name, {tup->getType(), none->getType(), none->getType()}, {}, builtinModule);
      seqassertn(fn, "{} function not found", name);
      result = util::call(fn, {tup, none, none});
      break;
    }
    default:
      return nullptr;
    }
    return util::ptrStore(ptr, result);
  }

  Value *generateAtomicReduction(Value *ptr, Value *arg, Var *loc, Var *gtid,
                                 ReductionLocks &locks) {
    auto *M = ptr->getModule();
    auto *type = getType();
    std::string func = "";

    if (isA<types::IntType>(type)) {
      switch (kind) {
      case Kind::ADD:
        func = "_atomic_int_add";
        break;
      case Kind::MUL:
        func = "_atomic_int_mul";
        break;
      case Kind::AND:
        func = "_atomic_int_and";
        break;
      case Kind::OR:
        func = "_atomic_int_or";
        break;
      case Kind::XOR:
        func = "_atomic_int_xor";
        break;
      case Kind::MIN:
        func = "_atomic_int_min";
        break;
      case Kind::MAX:
        func = "_atomic_int_max";
        break;
      default:
        break;
      }
    } else if (isA<types::FloatType>(type)) {
      switch (kind) {
      case Kind::ADD:
        func = "_atomic_float_add";
        break;
      case Kind::MUL:
        func = "_atomic_float_mul";
        break;
      case Kind::MIN:
        func = "_atomic_float_min";
        break;
      case Kind::MAX:
        func = "_atomic_float_max";
        break;
      default:
        break;
      }
    } else if (isA<types::Float32Type>(type)) {
      switch (kind) {
      case Kind::ADD:
        func = "_atomic_float32_add";
        break;
      case Kind::MUL:
        func = "_atomic_float32_mul";
        break;
      case Kind::MIN:
        func = "_atomic_float32_min";
        break;
      case Kind::MAX:
        func = "_atomic_float32_max";
        break;
      default:
        break;
      }
    }

    if (!func.empty()) {
      auto *atomicOp =
          M->getOrRealizeFunc(func, {ptr->getType(), arg->getType()}, {}, ompModule);
      seqassertn(atomicOp, "atomic op '{}' not found", func);
      return util::call(atomicOp, {ptr, arg});
    }

    switch (kind) {
    case Kind::ADD:
      func = "__atomic_add__";
      break;
    case Kind::MUL:
      func = "__atomic_mul__";
      break;
    case Kind::AND:
      func = "__atomic_and__";
      break;
    case Kind::OR:
      func = "__atomic_or__";
      break;
    case Kind::XOR:
      func = "__atomic_xor__";
      break;
    case Kind::MIN:
      func = "__atomic_min__";
      break;
    case Kind::MAX:
      func = "__atomic_max__";
      break;
    default:
      break;
    }

    if (!func.empty()) {
      auto *atomicOp =
          M->getOrRealizeMethod(arg->getType(), func, {ptr->getType(), arg->getType()});
      if (atomicOp)
        return util::call(atomicOp, {ptr, arg});
    }

    seqassertn(loc && gtid, "loc and/or gtid are null");
    auto *lck = locks.getCritLock(M);
    auto *lckPtrType = M->getPointerType(lck->getType());
    auto *critBegin = M->getOrRealizeFunc("_critical_begin",
                                          {loc->getType(), gtid->getType(), lckPtrType},
                                          {}, ompModule);
    seqassertn(critBegin, "critical begin function not found");
    auto *critEnd = M->getOrRealizeFunc(
        "_critical_end", {loc->getType(), gtid->getType(), lckPtrType}, {}, ompModule);
    seqassertn(critEnd, "critical end function not found");

    auto *critEnter =
        util::call(critBegin, {M->Nr<VarValue>(loc), M->Nr<VarValue>(gtid),
                               M->Nr<PointerValue>(lck)});
    auto *operation = generateNonAtomicReduction(ptr, arg);
    auto *critExit = util::call(critEnd, {M->Nr<VarValue>(loc), M->Nr<VarValue>(gtid),
                                          M->Nr<PointerValue>(lck)});
    // make sure the unlock is in a finally-block
    return util::series(critEnter, M->Nr<TryCatchFlow>(util::series(operation),
                                                       util::series(critExit)));
  }

  operator bool() const { return kind != Kind::NONE; }
};

struct ReductionFunction {
  std::string name;
  Reduction::Kind kind;
  bool method;
};

struct ReductionIdentifier : public util::Operator {
  std::vector<Var *> shareds;
  Var *loopVarArg;
  std::unordered_map<id_t, Reduction> reductions;

  ReductionIdentifier()
      : util::Operator(), shareds(), loopVarArg(nullptr), reductions() {}

  ReductionIdentifier(std::vector<Var *> shareds, Var *loopVarArg)
      : util::Operator(), shareds(std::move(shareds)), loopVarArg(loopVarArg),
        reductions() {}

  bool isShared(Var *shared) {
    if (loopVarArg && shared->getId() == loopVarArg->getId())
      return false;
    for (auto *v : shareds) {
      if (shared->getId() == v->getId())
        return true;
    }
    return false;
  }

  bool isSharedDeref(Var *shared, Value *v) {
    auto *M = v->getModule();
    auto *ptrType = cast<types::PointerType>(shared->getType());
    seqassertn(ptrType, "expected shared var to be of pointer type");
    auto *type = ptrType->getBase();

    if (util::isCallOf(v, Module::GETITEM_MAGIC_NAME, {ptrType, M->getIntType()}, type,
                       /*method=*/true)) {
      auto *call = cast<CallInstr>(v);
      auto *var = util::getVar(call->front());
      return util::isConst<int64_t>(call->back(), 0) && var &&
             var->getId() == shared->getId();
    }

    return false;
  }

  static void extractAssociativeOpChain(Value *v, const std::string &op,
                                        types::Type *type,
                                        std::vector<Value *> &result) {
    if (util::isCallOf(v, op, {type, type}, type, /*method=*/true)) {
      auto *call = cast<CallInstr>(v);
      extractAssociativeOpChain(call->front(), op, type, result);
      extractAssociativeOpChain(call->back(), op, type, result);
    } else {
      result.push_back(v);
    }
  }

  Reduction getReductionFromCall(CallInstr *v) {
    auto *M = v->getModule();
    auto *func = util::getFunc(v->getCallee());
    if (v->numArgs() != 3 || !func ||
        func->getUnmangledName() != Module::SETITEM_MAGIC_NAME)
      return {};

    std::vector<Value *> args(v->begin(), v->end());
    Value *self = args[0];
    Value *idx = args[1];
    Value *item = args[2];

    Var *shared = util::getVar(self);
    if (!shared || !isShared(shared) || !util::isConst<int64_t>(idx, 0))
      return {};

    auto *ptrType = cast<types::PointerType>(shared->getType());
    seqassertn(ptrType, "expected shared var to be of pointer type");
    auto *type = ptrType->getBase();
    auto *noneType = M->getOptionalType(M->getNoneType());

    // double-check the call
    if (!util::isCallOf(v, Module::SETITEM_MAGIC_NAME,
                        {self->getType(), idx->getType(), item->getType()},
                        M->getNoneType(), /*method=*/true))
      return {};

    const std::vector<ReductionFunction> reductionFunctions = {
        {Module::ADD_MAGIC_NAME, Reduction::Kind::ADD, true},
        {Module::MUL_MAGIC_NAME, Reduction::Kind::MUL, true},
        {Module::AND_MAGIC_NAME, Reduction::Kind::AND, true},
        {Module::OR_MAGIC_NAME, Reduction::Kind::OR, true},
        {Module::XOR_MAGIC_NAME, Reduction::Kind::XOR, true},
        {"min", Reduction::Kind::MIN, false},
        {"max", Reduction::Kind::MAX, false},
    };

    for (auto &rf : reductionFunctions) {
      if (rf.method) {
        if (!util::isCallOf(item, rf.name, {type, type}, type, /*method=*/true))
          continue;
      } else {
        if (!util::isCallOf(item, rf.name,
                            {M->getTupleType({type, type}), noneType, noneType}, type,
                            /*method=*/false))
          continue;
      }

      auto *callRHS = cast<CallInstr>(item);
      Value *deref = nullptr;

      if (rf.method) {
        std::vector<Value *> opChain;
        extractAssociativeOpChain(callRHS, rf.name, callRHS->front()->getType(),
                                  opChain);
        if (opChain.size() < 2)
          continue;

        for (auto *val : opChain) {
          if (isSharedDeref(shared, val)) {
            deref = val;
            break;
          }
        }
      } else {
        callRHS = cast<CallInstr>(callRHS->front()); // this will be Tuple.__new__
        if (!callRHS)
          continue;

        for (auto *val : *callRHS) {
          if (isSharedDeref(shared, val)) {
            deref = val;
            break;
          }
        }
      }

      if (!deref)
        return {};

      Reduction reduction = {rf.kind, shared};
      if (!reduction.getInitial())
        return {};

      return reduction;
    }

    return {};
  }

  Reduction getReduction(Var *shared) {
    auto it = reductions.find(shared->getId());
    return (it != reductions.end()) ? it->second : Reduction();
  }

  void handle(CallInstr *v) override {
    if (auto reduction = getReductionFromCall(v)) {
      auto it = reductions.find(reduction.shared->getId());
      // if we've seen the var before, make sure it's consistent
      // otherwise mark as invalid via an empty reduction
      if (it == reductions.end()) {
        reductions.emplace(reduction.shared->getId(), reduction);
      } else if (it->second && it->second.kind != reduction.kind) {
        it->second = {};
      }
    }
  }
};

struct SharedInfo {
  unsigned memb;       // member index in template's `extra` arg
  Var *local;          // the local var we create to store current value
  Reduction reduction; // the reduction we're performing, or empty if none
};

struct LoopTemplateReplacer : public util::Operator {
  BodiedFunc *parent;
  CallInstr *replacement;
  Var *loopVar;

  LoopTemplateReplacer(BodiedFunc *parent, CallInstr *replacement, Var *loopVar)
      : util::Operator(), parent(parent), replacement(replacement), loopVar(loopVar) {}
};

struct ParallelLoopTemplateReplacer : public LoopTemplateReplacer {
  ReductionIdentifier *reds;
  std::vector<SharedInfo> sharedInfo;
  ReductionLocks locks;
  Var *locRef;
  Var *reductionLocRef;
  Var *gtid;

  ParallelLoopTemplateReplacer(BodiedFunc *parent, CallInstr *replacement, Var *loopVar,
                               ReductionIdentifier *reds)
      : LoopTemplateReplacer(parent, replacement, loopVar), reds(reds), sharedInfo(),
        locks(), locRef(nullptr), reductionLocRef(nullptr), gtid(nullptr) {}

  unsigned numReductions() {
    unsigned num = 0;
    for (auto &info : sharedInfo) {
      if (info.reduction)
        num += 1;
    }
    return num;
  }

  Value *getReductionTuple() {
    auto *M = parent->getModule();
    std::vector<Value *> elements;
    for (auto &info : sharedInfo) {
      if (info.reduction)
        elements.push_back(M->Nr<PointerValue>(info.local));
    }
    return util::makeTuple(elements, M);
  }

  BodiedFunc *makeReductionFunc() {
    auto *M = parent->getModule();
    auto *tupleType = getReductionTuple()->getType();
    auto *argType = M->getPointerType(tupleType);
    auto *funcType = M->getFuncType(M->getNoneType(), {argType, argType});
    auto *reducer = M->Nr<BodiedFunc>("__omp_reducer");
    reducer->realize(funcType, {"lhs", "rhs"});

    auto *lhsVar = reducer->arg_front();
    auto *rhsVar = reducer->arg_back();
    auto *body = M->Nr<SeriesFlow>();
    unsigned next = 0;
    for (auto &info : sharedInfo) {
      if (info.reduction) {
        auto *lhs = util::ptrLoad(M->Nr<VarValue>(lhsVar));
        auto *rhs = util::ptrLoad(M->Nr<VarValue>(rhsVar));
        auto *lhsElem = util::tupleGet(lhs, next);
        auto *rhsElem = util::tupleGet(rhs, next);
        body->push_back(
            info.reduction.generateNonAtomicReduction(lhsElem, util::ptrLoad(rhsElem)));
        ++next;
      }
    }
    reducer->setBody(body);
    return reducer;
  }

  void handle(CallInstr *v) override {
    auto *M = v->getModule();
    auto *func = util::getFunc(v->getCallee());
    if (!func)
      return;
    auto name = func->getUnmangledName();

    if (name == "_loop_loc_and_gtid") {
      seqassertn(v->numArgs() == 3 &&
                     std::all_of(v->begin(), v->end(),
                                 [](auto x) { return isA<VarValue>(x); }),
                 "unexpected loop loc and gtid stub");
      std::vector<Value *> args(v->begin(), v->end());
      locRef = util::getVar(args[0]);
      reductionLocRef = util::getVar(args[1]);
      gtid = util::getVar(args[2]);
    }

    if (name == "_loop_reductions") {
      seqassertn(reductionLocRef && gtid, "bad visit order in template");
      seqassertn(v->numArgs() == 1 && isA<VarValue>(v->front()),
                 "unexpected shared updates stub");
      if (numReductions() == 0)
        return;

      auto *M = parent->getModule();
      auto *extras = util::getVar(v->front());
      auto *reductionTuple = getReductionTuple();
      auto *reducer = makeReductionFunc();
      auto *lck = locks.getMainLock(M);
      auto *rawReducer = ptrFromFunc(reducer);

      auto *lckPtrType = M->getPointerType(lck->getType());
      auto *reduceNoWait = M->getOrRealizeFunc(
          "_reduce_nowait",
          {reductionLocRef->getType(), gtid->getType(), reductionTuple->getType(),
           rawReducer->getType(), lckPtrType},
          {}, ompModule);
      seqassertn(reduceNoWait, "reduce nowait function not found");
      auto *reduceNoWaitEnd = M->getOrRealizeFunc(
          "_end_reduce_nowait",
          {reductionLocRef->getType(), gtid->getType(), lckPtrType}, {}, ompModule);
      seqassertn(reduceNoWaitEnd, "end reduce nowait function not found");

      auto *series = M->Nr<SeriesFlow>();
      auto *tupleVal = util::makeVar(reductionTuple, series, parent);
      auto *reduceCode = util::call(
          reduceNoWait, {M->Nr<VarValue>(reductionLocRef), M->Nr<VarValue>(gtid),
                         tupleVal, rawReducer, M->Nr<PointerValue>(lck)});
      auto *codeVar = util::makeVar(reduceCode, series, parent)->getVar();
      seqassertn(codeVar->getType()->is(M->getIntType()), "wrong reduce code type");

      auto *sectionNonAtomic = M->Nr<SeriesFlow>();
      auto *sectionAtomic = M->Nr<SeriesFlow>();

      for (auto &info : sharedInfo) {
        if (info.reduction) {
          Value *ptr = util::tupleGet(M->Nr<VarValue>(extras), info.memb);
          Value *arg = M->Nr<VarValue>(info.local);
          sectionNonAtomic->push_back(
              info.reduction.generateNonAtomicReduction(ptr, arg));
        }
      }
      sectionNonAtomic->push_back(util::call(
          reduceNoWaitEnd, {M->Nr<VarValue>(reductionLocRef), M->Nr<VarValue>(gtid),
                            M->Nr<PointerValue>(lck)}));

      for (auto &info : sharedInfo) {
        if (info.reduction) {
          Value *ptr = util::tupleGet(M->Nr<VarValue>(extras), info.memb);
          Value *arg = M->Nr<VarValue>(info.local);
          sectionAtomic->push_back(
              info.reduction.generateAtomicReduction(ptr, arg, locRef, gtid, locks));
        }
      }

      // make: if code == 1 { sectionNonAtomic } elif code == 2 { sectionAtomic }
      auto *theSwitch = M->Nr<IfFlow>(
          *M->Nr<VarValue>(codeVar) == *M->getInt(1), sectionNonAtomic,
          util::series(M->Nr<IfFlow>(*M->Nr<VarValue>(codeVar) == *M->getInt(2),
                                     sectionAtomic)));
      series->push_back(theSwitch);
      v->replaceAll(series);
    }
  }
};

struct ImperativeLoopTemplateReplacer : public ParallelLoopTemplateReplacer {
  OMPSched *sched;
  int64_t step;

  ImperativeLoopTemplateReplacer(BodiedFunc *parent, CallInstr *replacement,
                                 Var *loopVar, ReductionIdentifier *reds,
                                 OMPSched *sched, int64_t step)
      : ParallelLoopTemplateReplacer(parent, replacement, loopVar, reds), sched(sched),
        step(step) {}

  void handle(CallInstr *v) override {
    ParallelLoopTemplateReplacer::handle(v);
    auto *M = v->getModule();
    auto *func = util::getFunc(v->getCallee());
    if (!func)
      return;
    auto name = func->getUnmangledName();

    if (name == "_loop_step") {
      v->replaceAll(M->getInt(step));
    }

    if (name == "_loop_body_stub") {
      seqassertn(replacement, "unexpected double replacement");
      seqassertn(v->numArgs() == 2 && isA<VarValue>(v->front()) &&
                     isA<VarValue>(v->back()),
                 "unexpected loop body stub");

      auto *outlinedFunc = util::getFunc(replacement->getCallee());

      // the template passes the new loop var and extra args
      // to the body stub for convenience
      auto *newLoopVar = util::getVar(v->front());
      auto *extras = util::getVar(v->back());

      std::vector<Value *> newArgs;
      auto outlinedArgs = outlinedFunc->arg_begin(); // arg vars of *outlined func*
      unsigned next = 0; // next index in "extra" args tuple, passed to template
      // `arg` is an argument of the original outlined func call
      for (auto *arg : *replacement) {
        if (getVarFromOutlinedArg(arg)->getId() != loopVar->getId()) {
          Value *newArg = nullptr;

          // shared vars will be stored in a new var
          if (isA<PointerValue>(arg)) {
            types::Type *base = cast<types::PointerType>(arg->getType())->getBase();

            // get extras again since we'll be inserting the new var before extras local
            Var *lastArg = parent->arg_back(); // ptr to {chunk, start, stop, extras}
            Value *val = util::tupleGet(util::ptrLoad(M->Nr<VarValue>(lastArg)), 3);
            Value *initVal = util::ptrLoad(util::tupleGet(val, next));

            Reduction reduction = reds->getReduction(*outlinedArgs);
            if (reduction) {
              initVal = reduction.getInitial();
              seqassertn(initVal && initVal->getType()->is(base),
                         "unknown reduction init value");
            }

            VarValue *newVar = util::makeVar(
                initVal, cast<SeriesFlow>(parent->getBody()), parent, /*prepend=*/true);
            sharedInfo.push_back({next, newVar->getVar(), reduction});

            newArg = M->Nr<PointerValue>(newVar->getVar());
            ++next;
          } else {
            newArg = util::tupleGet(M->Nr<VarValue>(extras), next++);
          }

          newArgs.push_back(newArg);
        } else {
          if (isA<VarValue>(arg)) {
            newArgs.push_back(M->Nr<VarValue>(newLoopVar));
          } else if (isA<PointerValue>(arg)) {
            newArgs.push_back(M->Nr<PointerValue>(newLoopVar));
          } else {
            seqassertn(false, "unknown outline var");
          }
        }

        ++outlinedArgs;
      }

      v->replaceAll(util::call(outlinedFunc, newArgs));
      replacement = nullptr;
    }

    if (name == "_loop_shared_updates") {
      // for all non-reduction shareds, set the final values
      // this will be similar to OpenMP's "lastprivate"
      seqassertn(v->numArgs() == 1 && isA<VarValue>(v->front()),
                 "unexpected shared updates stub");
      auto *extras = util::getVar(v->front());
      auto *series = M->Nr<SeriesFlow>();

      for (auto &info : sharedInfo) {
        if (info.reduction)
          continue;

        auto *finalValue = M->Nr<VarValue>(info.local);
        auto *val = M->Nr<VarValue>(extras);
        auto *origPtr = util::tupleGet(val, info.memb);
        series->push_back(util::ptrStore(origPtr, finalValue));
      }

      v->replaceAll(series);
    }

    if (name == "_loop_schedule") {
      v->replaceAll(M->getInt(sched->code));
    }

    if (name == "_loop_ordered") {
      v->replaceAll(M->getBool(sched->ordered));
    }
  }
};

struct TaskLoopReductionVarReplacer : public util::Operator {
  std::vector<Var *> reductionArgs;
  std::vector<std::pair<Var *, Var *>> reductionRemap;
  BodiedFunc *parent;

  void setupReductionRemap() {
    auto *M = parent->getModule();

    for (auto *var : reductionArgs) {
      auto *newVar = M->Nr<Var>(var->getType(), /*global=*/false);
      reductionRemap.emplace_back(var, newVar);
    }
  }

  TaskLoopReductionVarReplacer(std::vector<Var *> reductionArgs, BodiedFunc *parent)
      : util::Operator(), reductionArgs(std::move(reductionArgs)), reductionRemap(),
        parent(parent) {
    setupReductionRemap();
  }

  void preHook(Node *v) override {
    for (auto &p : reductionRemap) {
      v->replaceUsedVariable(p.first->getId(), p.second);
    }
  }

  // need to do this as a separate step since otherwise the old variable
  // in the assignment will be replaced, which we don't want
  void finalize() {
    auto *M = parent->getModule();
    auto *body = cast<SeriesFlow>(parent->getBody());
    auto *gtid = parent->arg_back();

    for (auto &p : reductionRemap) {
      auto *taskRedData = M->getOrRealizeFunc(
          "_taskred_data", {M->getIntType(), p.first->getType()}, {}, ompModule);
      seqassertn(taskRedData, "could not find '_taskred_data'");

      auto *assign = M->Nr<AssignInstr>(
          p.second,
          util::call(taskRedData, {M->Nr<VarValue>(gtid), M->Nr<VarValue>(p.first)}));
      body->insert(body->begin(), assign);
      parent->push_back(p.second);
    }
  }
};

struct TaskLoopBodyStubReplacer : public util::Operator {
  CallInstr *replacement;
  std::vector<bool> reduceArgs;

  TaskLoopBodyStubReplacer(CallInstr *replacement, std::vector<bool> reduceArgs)
      : util::Operator(), replacement(replacement), reduceArgs(std::move(reduceArgs)) {}

  void handle(CallInstr *v) override {
    auto *func = util::getFunc(v->getCallee());
    if (func && func->getUnmangledName() == "_task_loop_body_stub") {
      seqassertn(replacement, "unexpected double replacement");
      seqassertn(v->numArgs() == 3 && isA<VarValue>(v->front()) &&
                     isA<VarValue>(v->back()),
                 "unexpected loop body stub");

      // the template passes gtid, privs and shareds to the body stub for convenience
      std::vector<Value *> args(v->begin(), v->end());
      auto *gtid = args[0];
      auto *privatesTuple = args[1];
      auto *sharedsTuple = args[2];
      unsigned privatesNext = 0;
      unsigned sharedsNext = 0;
      std::vector<Value *> newArgs;
      bool hasReductions =
          std::any_of(reduceArgs.begin(), reduceArgs.end(), [](bool b) { return b; });

      for (auto *arg : *replacement) {
        if (isA<VarValue>(arg)) {
          newArgs.push_back(util::tupleGet(privatesTuple, privatesNext++));
        } else if (isA<PointerValue>(arg)) {
          newArgs.push_back(util::tupleGet(sharedsTuple, sharedsNext++));
        } else {
          // make sure we're on the last arg, which should be gtid
          // in case of reductions
          seqassertn(hasReductions && arg == replacement->back(),
                     "unknown outline var");
        }
      }

      auto *outlinedFunc = cast<BodiedFunc>(util::getFunc(replacement->getCallee()));

      if (hasReductions) {
        newArgs.push_back(gtid);

        std::vector<Var *> reductionArgs;
        unsigned i = 0;
        for (auto it = outlinedFunc->arg_begin(); it != outlinedFunc->arg_end(); ++it) {
          if (reduceArgs[i++])
            reductionArgs.push_back(*it);
        }
        TaskLoopReductionVarReplacer redrep(reductionArgs, outlinedFunc);
        outlinedFunc->accept(redrep);
        redrep.finalize();
      }

      v->replaceAll(util::call(outlinedFunc, newArgs));
      replacement = nullptr;
    }
  }
};

struct TaskLoopRoutineStubReplacer : public ParallelLoopTemplateReplacer {
  std::vector<Value *> privates;
  std::vector<Value *> shareds;
  Var *array;  // task reduction input array
  Var *tskgrp; // task group identifier

  void setupSharedInfo(std::vector<Reduction> &sharedRedux) {
    unsigned sharedsNext = 0;
    for (auto *val : shareds) {
      if (getVarFromOutlinedArg(val)->getId() != loopVar->getId()) {
        if (auto &reduction = sharedRedux[sharedsNext]) {
          Var *newVar = util::getVar(util::makeVar(
              reduction.getInitial(), cast<SeriesFlow>(parent->getBody()), parent,
              /*prepend=*/true));
          sharedInfo.push_back({sharedsNext, newVar, reduction});
        }
      }
      ++sharedsNext;
    }
  }

  TaskLoopRoutineStubReplacer(BodiedFunc *parent, CallInstr *replacement, Var *loopVar,
                              ReductionIdentifier *reds, std::vector<Value *> privates,
                              std::vector<Value *> shareds,
                              std::vector<Reduction> sharedRedux)
      : ParallelLoopTemplateReplacer(parent, replacement, loopVar, reds),
        privates(std::move(privates)), shareds(std::move(shareds)), array(nullptr),
        tskgrp(nullptr) {
    setupSharedInfo(sharedRedux);
  }

  BodiedFunc *makeTaskRedInitFunc(Reduction *reduction) {
    auto *M = parent->getModule();
    auto *argType = M->getPointerType(reduction->getType());
    auto *funcType = M->getFuncType(M->getNoneType(), {argType, argType});
    auto *initializer = M->Nr<BodiedFunc>("__red_init");
    initializer->realize(funcType, {"lhs", "rhs"});

    auto *lhsVar = initializer->arg_front();
    auto *body = M->Nr<SeriesFlow>();
    auto *lhsPtr = M->Nr<VarValue>(lhsVar);
    body->push_back(util::ptrStore(lhsPtr, reduction->getInitial()));
    initializer->setBody(body);
    return initializer;
  }

  BodiedFunc *makeTaskRedCombFunc(Reduction *reduction) {
    auto *M = parent->getModule();
    auto *argType = M->getPointerType(reduction->getType());
    auto *funcType = M->getFuncType(M->getNoneType(), {argType, argType});
    auto *reducer = M->Nr<BodiedFunc>("__red_comb");
    reducer->realize(funcType, {"lhs", "rhs"});

    auto *lhsVar = reducer->arg_front();
    auto *rhsVar = reducer->arg_back();
    auto *body = M->Nr<SeriesFlow>();
    auto *lhsPtr = M->Nr<VarValue>(lhsVar);
    auto *rhsPtr = M->Nr<VarValue>(rhsVar);
    body->push_back(
        reduction->generateNonAtomicReduction(lhsPtr, util::ptrLoad(rhsPtr)));
    reducer->setBody(body);
    return reducer;
  }

  Value *makeTaskRedInput(Reduction *reduction, Value *shar, Value *orig) {
    auto *M = shar->getModule();
    auto *size = M->Nr<TypePropertyInstr>(reduction->getType(),
                                          TypePropertyInstr::Property::SIZEOF);
    auto *init = ptrFromFunc(makeTaskRedInitFunc(reduction));
    auto *comb = ptrFromFunc(makeTaskRedCombFunc(reduction));

    auto *taskRedInputType = M->getOrRealizeType("TaskReductionInput", {}, ompModule);
    seqassertn(taskRedInputType, "could not find 'TaskReductionInput' type");
    auto *result = taskRedInputType->construct({shar, orig, size, init, comb});
    seqassertn(result, "bad construction of 'TaskReductionInput' type");
    return result;
  }

  void handle(VarValue *v) override {
    auto *M = v->getModule();
    auto *func = util::getFunc(v);
    if (func && func->getUnmangledName() == "_routine_stub") {
      std::vector<bool> reduceArgs;
      unsigned sharedsNext = 0;
      unsigned infoNext = 0;

      for (auto *arg : *replacement) {
        if (isA<VarValue>(arg)) {
          reduceArgs.push_back(false);
        } else if (isA<PointerValue>(arg)) {
          if (infoNext < sharedInfo.size() &&
              sharedInfo[infoNext].memb == sharedsNext &&
              sharedInfo[infoNext].reduction) {
            reduceArgs.push_back(true);
            ++infoNext;
          } else {
            reduceArgs.push_back(false);
          }
          ++sharedsNext;
        } else {
          // make sure we're on the last arg, which should be gtid
          // in case of reductions
          seqassertn(numReductions() > 0 && arg == replacement->back(),
                     "unknown outline var");
          reduceArgs.push_back(false);
        }
      }

      util::CloneVisitor cv(M);
      auto *newRoutine = cv.forceClone(func);
      TaskLoopBodyStubReplacer rep(replacement, reduceArgs);
      newRoutine->accept(rep);
      v->setVar(newRoutine);
    }
  }

  void handle(CallInstr *v) override {
    ParallelLoopTemplateReplacer::handle(v);
    auto *M = v->getModule();
    auto *func = util::getFunc(v->getCallee());
    if (!func)
      return;
    auto name = func->getUnmangledName();

    if (name == "_taskred_setup") {
      seqassertn(reductionLocRef && gtid, "bad visit order in template");
      seqassertn(v->numArgs() == 1 && isA<VarValue>(v->front()),
                 "unexpected shared updates stub");
      unsigned numRed = numReductions();
      if (numRed == 0)
        return;

      auto *M = parent->getModule();
      auto *extras = util::getVar(v->front());

      // add task reduction inputs
      auto *taskRedInitSeries = M->Nr<SeriesFlow>();
      auto *taskRedInputType = M->getOrRealizeType("TaskReductionInput", {}, ompModule);
      seqassertn(taskRedInputType, "could not find 'TaskReductionInput' type");
      auto *irArrayType = M->getOrRealizeType("TaskReductionInputArray", {}, ompModule);
      seqassertn(irArrayType, "could not find 'TaskReductionInputArray' type");
      auto *taskRedInputsArray = util::makeVar(
          M->Nr<StackAllocInstr>(irArrayType, numRed), taskRedInitSeries, parent);
      array = util::getVar(taskRedInputsArray);
      auto *taskRedInputsArrayType = taskRedInputsArray->getType();

      auto *taskRedSetItem = M->getOrRealizeMethod(
          taskRedInputsArrayType, Module::SETITEM_MAGIC_NAME,
          {taskRedInputsArrayType, M->getIntType(), taskRedInputType});
      seqassertn(taskRedSetItem,
                 "could not find 'TaskReductionInputArray.__setitem__' method");
      int i = 0;
      for (auto &info : sharedInfo) {
        if (info.reduction) {
          Value *shar = M->Nr<PointerValue>(info.local);
          Value *orig = util::tupleGet(M->Nr<VarValue>(extras), info.memb);
          auto *taskRedInput = makeTaskRedInput(&info.reduction, shar, orig);
          taskRedInitSeries->push_back(util::call(
              taskRedSetItem, {M->Nr<VarValue>(array), M->getInt(i++), taskRedInput}));
        }
      }

      auto *arrayPtr = M->Nr<ExtractInstr>(M->Nr<VarValue>(array), "ptr");
      auto *taskRedInitFunc =
          M->getOrRealizeFunc("_taskred_init",
                              {reductionLocRef->getType(), gtid->getType(),
                               M->getIntType(), arrayPtr->getType()},
                              {}, ompModule);
      seqassertn(taskRedInitFunc, "task red init function not found");
      auto *taskRedInitResult =
          util::makeVar(util::call(taskRedInitFunc, {M->Nr<VarValue>(reductionLocRef),
                                                     M->Nr<VarValue>(gtid),
                                                     M->getInt(numRed), arrayPtr}),
                        taskRedInitSeries, parent);
      tskgrp = util::getVar(taskRedInitResult);
      v->replaceAll(taskRedInitSeries);
    }

    if (name == "_fix_privates_and_shareds") {
      std::vector<Value *> args(v->begin(), v->end());
      seqassertn(args.size() == 3, "invalid _fix_privates_and_shareds call found");
      unsigned numRed = numReductions();
      auto *newLoopVar = args[0];
      auto *privatesTuple = args[1];
      auto *sharedsTuple = args[2];

      unsigned privatesNext = 0;
      unsigned sharedsNext = 0;
      unsigned infoNext = 0;

      bool needNewPrivates = false;
      bool needNewShareds = false;

      std::vector<Value *> newPrivates;
      std::vector<Value *> newShareds;

      for (auto *val : privates) {
        if (numRed > 0 && val == privates.back()) { // i.e. task group identifier
          seqassertn(tskgrp, "tskgrp var not set");
          newPrivates.push_back(M->Nr<VarValue>(tskgrp));
          needNewPrivates = true;
        } else if (getVarFromOutlinedArg(val)->getId() != loopVar->getId()) {
          newPrivates.push_back(util::tupleGet(privatesTuple, privatesNext));
        } else {
          newPrivates.push_back(newLoopVar);
          needNewPrivates = true;
        }
        ++privatesNext;
      }

      for (auto *val : shareds) {
        if (getVarFromOutlinedArg(val)->getId() != loopVar->getId()) {
          if (infoNext < sharedInfo.size() &&
              sharedInfo[infoNext].memb == sharedsNext &&
              sharedInfo[infoNext].reduction) {
            newShareds.push_back(M->Nr<PointerValue>(sharedInfo[infoNext].local));
            needNewShareds = true;
            ++infoNext;
          } else {
            newShareds.push_back(util::tupleGet(sharedsTuple, sharedsNext));
          }
        } else {
          newShareds.push_back(M->Nr<PointerValue>(util::getVar(newLoopVar)));
          needNewShareds = true;
        }
        ++sharedsNext;
      }

      privatesTuple = needNewPrivates ? util::makeTuple(newPrivates, M) : privatesTuple;
      sharedsTuple = needNewShareds ? util::makeTuple(newShareds, M) : sharedsTuple;

      Value *result = util::makeTuple({privatesTuple, sharedsTuple}, M);
      v->replaceAll(result);
    }

    if (name == "_taskred_finish") {
      seqassertn(reductionLocRef && gtid, "bad visit order in template");
      if (numReductions() == 0)
        return;

      auto *taskRedFini = M->getOrRealizeFunc(
          "_taskred_fini", {reductionLocRef->getType(), gtid->getType()}, {},
          ompModule);
      seqassertn(taskRedFini, "taskred finish function not found not found");
      v->replaceAll(util::call(
          taskRedFini, {M->Nr<VarValue>(reductionLocRef), M->Nr<VarValue>(gtid)}));
    }
  }
};

struct GPULoopBodyStubReplacer : public util::Operator {
  CallInstr *replacement;
  Var *loopVar;
  int64_t step;

  GPULoopBodyStubReplacer(CallInstr *replacement, Var *loopVar, int64_t step)
      : util::Operator(), replacement(replacement), loopVar(loopVar), step(step) {}

  void handle(CallInstr *v) override {
    auto *M = v->getModule();
    auto *func = util::getFunc(v->getCallee());
    if (!func)
      return;
    auto name = func->getUnmangledName();

    if (name == "_gpu_loop_body_stub") {
      seqassertn(replacement, "unexpected double replacement");
      seqassertn(v->numArgs() == 2, "unexpected loop body stub");

      // the template passes gtid, privs and shareds to the body stub for convenience
      auto *idx = v->front();
      auto *args = v->back();
      unsigned next = 0;

      std::vector<Value *> newArgs;
      for (auto *arg : *replacement) {
        if (getVarFromOutlinedArg(arg)->getId() == loopVar->getId()) {
          newArgs.push_back(idx);
        } else {
          newArgs.push_back(util::tupleGet(args, next++));
        }
      }

      auto *outlinedFunc = cast<BodiedFunc>(util::getFunc(replacement->getCallee()));
      v->replaceAll(util::call(outlinedFunc, newArgs));
      replacement = nullptr;
    }

    if (name == "_loop_step") {
      v->replaceAll(M->getInt(step));
    }
  }
};

struct GPULoopTemplateReplacer : public LoopTemplateReplacer {
  int64_t step;

  GPULoopTemplateReplacer(BodiedFunc *parent, CallInstr *replacement, Var *loopVar,
                          int64_t step)
      : LoopTemplateReplacer(parent, replacement, loopVar), step(step) {}

  void handle(CallInstr *v) override {
    auto *M = v->getModule();
    auto *func = util::getFunc(v->getCallee());
    if (!func)
      return;
    auto name = func->getUnmangledName();

    if (name == "_loop_step") {
      v->replaceAll(M->getInt(step));
    }
  }
};

struct OpenMPTransformData {
  util::OutlineResult outline;
  std::vector<Var *> sharedVars;
  ReductionIdentifier reds;
};

template <typename T> OpenMPTransformData unpar(T *v) {
  v->setParallel(false);
  return {{}, {}, {}};
}

template <typename T>
OpenMPTransformData setupOpenMPTransform(T *v, BodiedFunc *parent, bool gpu) {
  if (!v->isParallel())
    return unpar(v);
  auto *M = v->getModule();
  auto *body = cast<SeriesFlow>(v->getBody());
  if (!parent || !body)
    return unpar(v);
  auto outline = util::outlineRegion(parent, body, /*allowOutflows=*/false,
                                     /*outlineGlobals=*/true, /*allByValue=*/gpu);
  if (!outline)
    return unpar(v);

  // set up args to pass fork_call
  Var *loopVar = v->getVar();
  std::vector<Value *> outlineCallArgs(outline.call->begin(), outline.call->end());

  // shared argument vars
  std::vector<Var *> sharedVars;
  Var *loopVarArg = nullptr;
  unsigned i = 0;
  for (auto it = outline.func->arg_begin(); it != outline.func->arg_end(); ++it) {
    // pick out loop variable to pass to reduction identifier, which will
    // ensure we don't reduce over it
    if (getVarFromOutlinedArg(outlineCallArgs[i])->getId() == loopVar->getId())
      loopVarArg = *it;
    if (outline.argKinds[i] == util::OutlineResult::ArgKind::MODIFIED)
      sharedVars.push_back(*it);
    ++i;
  }
  ReductionIdentifier reds(sharedVars, loopVarArg);
  outline.func->accept(reds);

  return {outline, sharedVars, reds};
}

struct ForkCallData {
  CallInstr *fork = nullptr;
  CallInstr *pushNumThreads = nullptr;
};

ForkCallData createForkCall(Module *M, OMPTypes &types, Value *rawTemplateFunc,
                            const std::vector<Value *> &forkExtraArgs,
                            transform::parallel::OMPSched *sched) {
  ForkCallData result;
  auto *forkExtra = util::makeTuple(forkExtraArgs, M);
  std::vector<types::Type *> forkArgTypes = {types.i8ptr, forkExtra->getType()};
  auto *forkFunc = M->getOrRealizeFunc("_fork_call", forkArgTypes, {}, ompModule);
  seqassertn(forkFunc, "fork call function not found");
  result.fork = util::call(forkFunc, {rawTemplateFunc, forkExtra});

  if (sched->threads && sched->threads->getType()->is(types.i64)) {
    auto *pushNumThreadsFunc =
        M->getOrRealizeFunc("_push_num_threads", {types.i64}, {}, ompModule);
    seqassertn(pushNumThreadsFunc, "push num threads func not found");
    result.pushNumThreads = util::call(pushNumThreadsFunc, {sched->threads});
  }
  return result;
}

struct CollapseResult {
  ImperativeForFlow *collapsed = nullptr;
  SeriesFlow *setup = nullptr;
  std::string error;

  operator bool() const { return collapsed != nullptr; }
};

struct LoopRange {
  ImperativeForFlow *loop;
  Var *start;
  Var *stop;
  int64_t step;
  Var *len;
};

CollapseResult collapseLoop(BodiedFunc *parent, ImperativeForFlow *v, int64_t levels) {
  auto fail = [](const std::string &error) {
    CollapseResult bad;
    bad.error = error;
    return bad;
  };

  auto *M = v->getModule();
  CollapseResult res;
  if (levels < 1)
    return fail("'collapse' must be at least 1");

  std::vector<ImperativeForFlow *> loopNests = {v};
  ImperativeForFlow *curr = v;

  for (auto i = 0; i < levels - 1; i++) {
    auto *body = cast<SeriesFlow>(curr->getBody());
    seqassertn(body, "unexpected loop body");
    if (std::distance(body->begin(), body->end()) != 1 ||
        !isA<ImperativeForFlow>(body->front()))
      return fail("loop nest not collapsible");

    curr = cast<ImperativeForFlow>(body->front());
    loopNests.push_back(curr);
  }

  std::vector<LoopRange> ranges;
  auto *setup = M->Nr<SeriesFlow>();

  auto *intType = M->getIntType();
  auto *lenCalc =
      M->getOrRealizeFunc("_range_len", {intType, intType, intType}, {}, ompModule);
  seqassertn(lenCalc, "range length calculation function not found");

  for (auto *loop : loopNests) {
    LoopRange range;
    range.loop = loop;
    range.start = util::makeVar(loop->getStart(), setup, parent)->getVar();
    range.stop = util::makeVar(loop->getEnd(), setup, parent)->getVar();
    range.step = loop->getStep();
    range.len = util::makeVar(util::call(lenCalc, {M->Nr<VarValue>(range.start),
                                                   M->Nr<VarValue>(range.stop),
                                                   M->getInt(range.step)}),
                              setup, parent)
                    ->getVar();
    ranges.push_back(range);
  }

  auto *numIters = M->getInt(1);
  for (auto &range : ranges) {
    numIters = (*numIters) * (*M->Nr<VarValue>(range.len));
  }

  auto *collapsedVar = M->Nr<Var>(M->getIntType(), /*global=*/false);
  parent->push_back(collapsedVar);
  auto *body = M->Nr<SeriesFlow>();
  auto sched = std::make_unique<OMPSched>(*v->getSchedule());
  sched->collapse = 0;
  auto *collapsed = M->Nr<ImperativeForFlow>(M->getInt(0), 1, numIters, body,
                                             collapsedVar, std::move(sched));

  // reconstruct indices by successive divmods
  Var *lastDiv = nullptr;
  for (auto it = ranges.rbegin(); it != ranges.rend(); ++it) {
    auto *k = lastDiv ? lastDiv : collapsedVar;
    auto *div =
        util::makeVar(*M->Nr<VarValue>(k) / *M->Nr<VarValue>(it->len), body, parent)
            ->getVar();
    auto *mod =
        util::makeVar(*M->Nr<VarValue>(k) % *M->Nr<VarValue>(it->len), body, parent)
            ->getVar();
    auto *i =
        *M->Nr<VarValue>(it->start) + *(*M->Nr<VarValue>(mod) * *M->getInt(it->step));
    body->push_back(M->Nr<AssignInstr>(it->loop->getVar(), i));
    lastDiv = div;
  }

  auto *oldBody = cast<SeriesFlow>(loopNests.back()->getBody());
  for (auto *x : *oldBody) {
    body->push_back(x);
  }

  res.collapsed = collapsed;
  res.setup = setup;

  return res;
}
} // namespace

const std::string OpenMPPass::KEY = "core-parallel-openmp";

void OpenMPPass::handle(ForFlow *v) {
  auto data = setupOpenMPTransform(v, cast<BodiedFunc>(getParentFunc()), /*gpu=*/false);
  if (!v->isParallel())
    return;

  auto &outline = data.outline;
  auto &sharedVars = data.sharedVars;
  auto &reds = data.reds;

  auto *M = v->getModule();
  auto *loopVar = v->getVar();
  auto *sched = v->getSchedule();
  OMPTypes types(M);

  // separate arguments into 'private' and 'shared'
  std::vector<Reduction> sharedRedux; // reductions corresponding to shared vars
  std::vector<Value *> privates, shareds;
  unsigned i = 0;
  for (auto *arg : *outline.call) {
    if (isA<VarValue>(arg)) {
      privates.push_back(arg);
    } else {
      shareds.push_back(arg);
      sharedRedux.push_back(reds.getReduction(sharedVars[i++]));
    }
  }

  util::CloneVisitor cv(M);

  // We need to pass the task group identifier returned from
  // __kmpc_taskred_modifier_init to the task entry, so append
  // it to private data (initially as null void pointer). Also
  // we add an argument to the end of the outlined function for
  // the gtid.
  if (reds.reductions.size() > 0) {
    auto *nullPtr = types.i8ptr->construct({});
    privates.push_back(nullPtr);

    auto *outlinedFuncType = cast<types::FuncType>(outline.func->getType());
    std::vector<types::Type *> argTypes(outlinedFuncType->begin(),
                                        outlinedFuncType->end());
    argTypes.push_back(M->getIntType());
    auto *retType = outlinedFuncType->getReturnType();

    std::vector<Var *> oldArgVars(outline.func->arg_begin(), outline.func->arg_end());
    std::vector<std::string> argNames;

    for (auto *var : oldArgVars) {
      argNames.push_back(var->getName());
    }
    argNames.push_back("gtid");

    auto *newOutlinedFunc = M->Nr<BodiedFunc>("__outlined_new");
    newOutlinedFunc->realize(M->getFuncType(retType, argTypes), argNames);

    std::vector<Var *> newArgVars(newOutlinedFunc->arg_begin(),
                                  newOutlinedFunc->arg_end());

    std::unordered_map<id_t, Var *> remaps;
    for (unsigned i = 0; i < oldArgVars.size(); i++) {
      remaps.emplace(oldArgVars[i]->getId(), newArgVars[i]);
    }
    auto *newBody =
        cast<SeriesFlow>(cv.clone(outline.func->getBody(), newOutlinedFunc, remaps));
    newOutlinedFunc->setBody(newBody);

    // update outline struct
    outline.func = newOutlinedFunc;
    outline.call->setCallee(M->Nr<VarValue>(newOutlinedFunc));
    outline.call->insert(outline.call->end(), M->getInt(0));
    outline.argKinds.push_back(util::OutlineResult::ArgKind::CONSTANT);
  }

  auto *privatesTuple = util::makeTuple(privates, M);
  auto *sharedsTuple = util::makeTuple(shareds, M);

  // template call
  std::vector<types::Type *> templateFuncArgs = {
      types.i32ptr, types.i32ptr,
      M->getPointerType(
          M->getTupleType({v->getIter()->getType(), privatesTuple->getType(),
                           sharedsTuple->getType()}))};
  auto *templateFunc = M->getOrRealizeFunc("_task_loop_outline_template",
                                           templateFuncArgs, {}, ompModule);
  seqassertn(templateFunc, "task loop outline template not found");

  templateFunc = cv.forceClone(templateFunc);
  TaskLoopRoutineStubReplacer rep(cast<BodiedFunc>(templateFunc), outline.call, loopVar,
                                  &reds, privates, shareds, sharedRedux);
  templateFunc->accept(rep);
  auto *rawTemplateFunc = ptrFromFunc(templateFunc);

  std::vector<Value *> forkExtraArgs = {v->getIter(), privatesTuple, sharedsTuple};

  // fork call
  auto forkData = createForkCall(M, types, rawTemplateFunc, forkExtraArgs, sched);
  if (forkData.pushNumThreads)
    insertBefore(forkData.pushNumThreads);
  v->replaceAll(forkData.fork);
}

void OpenMPPass::handle(ImperativeForFlow *v) {
  auto *parent = cast<BodiedFunc>(getParentFunc());

  if (v->isParallel() && v->getSchedule()->collapse != 0) {
    auto levels = v->getSchedule()->collapse;
    auto collapse = collapseLoop(parent, v, levels);

    if (collapse) {
      v->replaceAll(collapse.collapsed);
      v = collapse.collapsed;
      insertBefore(collapse.setup);
    } else if (!collapse.error.empty()) {
      warn("could not collapse loop: " + collapse.error, v);
    }
  }

  auto data =
      setupOpenMPTransform(v, parent, (v->isParallel() && v->getSchedule()->gpu));
  if (!v->isParallel())
    return;

  auto &outline = data.outline;
  auto &sharedVars = data.sharedVars;
  auto &reds = data.reds;

  auto *M = v->getModule();
  auto *loopVar = v->getVar();
  auto *sched = v->getSchedule();
  OMPTypes types(M);

  // we disable shared vars for GPU loops
  seqassertn(!(sched->gpu && !sharedVars.empty()), "GPU-parallel loop had shared vars");

  // gather extra arguments
  std::vector<Value *> extraArgs;
  std::vector<types::Type *> extraArgTypes;
  for (auto *arg : *outline.call) {
    if (getVarFromOutlinedArg(arg)->getId() != loopVar->getId()) {
      extraArgs.push_back(arg);
      extraArgTypes.push_back(arg->getType());
    }
  }

  // template call
  std::string templateFuncName;
  if (sched->gpu) {
    templateFuncName = "_gpu_loop_outline_template";
  } else if (sched->dynamic) {
    templateFuncName = "_dynamic_loop_outline_template";
  } else if (sched->chunk) {
    templateFuncName = "_static_chunked_loop_outline_template";
  } else {
    templateFuncName = "_static_loop_outline_template";
  }

  if (sched->gpu) {
    std::unordered_set<id_t> kernels;
    const std::string gpuAttr = "std.gpu.kernel";
    for (auto *var : *M) {
      if (auto *func = cast<BodiedFunc>(var)) {
        if (util::hasAttribute(func, gpuAttr))
          kernels.insert(func->getId());
      }
    }

    std::vector<types::Type *> templateFuncArgs = {types.i64, types.i64,
                                                   M->getTupleType(extraArgTypes)};
    static int64_t instance = 0;
    auto *templateFunc = M->getOrRealizeFunc(templateFuncName, templateFuncArgs,
                                             {instance++}, gpuModule);

    if (!templateFunc) {
      warn("loop not compilable for GPU; ignoring", v);
      v->setParallel(false);
      return;
    }

    BodiedFunc *kernel = nullptr;
    for (auto *var : *M) {
      if (auto *func = cast<BodiedFunc>(var)) {
        if (util::hasAttribute(func, gpuAttr) && kernels.count(func->getId()) == 0) {
          seqassertn(!kernel, "multiple new kernels found after instantiation");
          kernel = func;
        }
      }
    }
    seqassertn(kernel, "no new kernel found");
    GPULoopBodyStubReplacer brep(outline.call, loopVar, v->getStep());
    kernel->accept(brep);

    util::CloneVisitor cv(M);
    templateFunc = cast<Func>(cv.forceClone(templateFunc));
    GPULoopTemplateReplacer rep(cast<BodiedFunc>(templateFunc), outline.call, loopVar,
                                v->getStep());
    templateFunc->accept(rep);
    v->replaceAll(util::call(
        templateFunc, {v->getStart(), v->getEnd(), util::makeTuple(extraArgs, M)}));
  } else {
    std::vector<types::Type *> templateFuncArgs = {
        types.i32ptr, types.i32ptr,
        M->getPointerType(M->getTupleType(
            {types.i64, types.i64, types.i64, M->getTupleType(extraArgTypes)}))};
    auto *templateFunc =
        M->getOrRealizeFunc(templateFuncName, templateFuncArgs, {}, ompModule);
    seqassertn(templateFunc, "imperative loop outline template not found");

    util::CloneVisitor cv(M);
    templateFunc = cast<Func>(cv.forceClone(templateFunc));
    ImperativeLoopTemplateReplacer rep(cast<BodiedFunc>(templateFunc), outline.call,
                                       loopVar, &reds, sched, v->getStep());
    templateFunc->accept(rep);
    auto *rawTemplateFunc = ptrFromFunc(templateFunc);

    auto *chunk = (sched->chunk && sched->chunk->getType()->is(types.i64))
                      ? sched->chunk
                      : M->getInt(1);
    std::vector<Value *> forkExtraArgs = {chunk, v->getStart(), v->getEnd()};
    for (auto *arg : extraArgs) {
      forkExtraArgs.push_back(arg);
    }

    // fork call
    auto forkData = createForkCall(M, types, rawTemplateFunc, forkExtraArgs, sched);
    if (forkData.pushNumThreads)
      insertBefore(forkData.pushNumThreads);
    v->replaceAll(forkData.fork);
  }
}

} // namespace parallel
} // namespace transform
} // namespace ir
} // namespace codon
