#include "pipeline.h"
#include "sir/util/cloning.h"
#include "sir/util/irtools.h"
#include "sir/util/matching.h"
#include <iterator>

namespace seq {

using namespace ir;

namespace {
const std::string prefetchModule = "std.bio.prefetch";
const std::string builtinModule = "std.bio.builtin";
const std::string alignModule = "std.bio.align";
const std::string seqModule = "std.bio.seq";

bool isParallel(PipelineFlow *p) {
  for (const auto &stage : *p) {
    if (stage.isParallel())
      return true;
  }
  return false;
}

BodiedFunc *makeStageWrapperFunc(PipelineFlow::Stage *stage, Func *callee,
                                 types::Type *inputType) {
  auto *M = callee->getModule();
  std::vector<types::Type *> argTypes = {inputType};
  std::vector<std::string> argNames = {"0"};
  int i = 1;
  for (auto *arg : *stage) {
    if (arg) {
      argTypes.push_back(arg->getType());
      argNames.push_back(std::to_string(i++));
    }
  }
  auto *funcType = M->getFuncType(util::getReturnType(callee), argTypes);
  auto *wrapperFunc = M->Nr<BodiedFunc>("__stage_wrapper");
  wrapperFunc->realize(funcType, argNames);

  // reorder arguments
  std::vector<Value *> args;
  auto it = wrapperFunc->arg_begin();
  ++it;
  for (auto *arg : *stage) {
    if (arg) {
      args.push_back(M->Nr<VarValue>(*it++));
    } else {
      args.push_back(M->Nr<VarValue>(wrapperFunc->arg_front()));
    }
  }

  wrapperFunc->setBody(util::series(M->Nr<ReturnInstr>(util::call(callee, args))));
  return wrapperFunc;
}

// check for a regular func, or a flow-instr containing a func,
// which is caused by a partial function.
BodiedFunc *getStageFunc(PipelineFlow::Stage &stage) {
  auto *callee = stage.getCallee();
  if (auto *f = cast<BodiedFunc>(util::getFunc(callee))) {
    return f;
  } else if (auto *s = cast<FlowInstr>(callee)) {
    if (auto *f = cast<BodiedFunc>(util::getFunc(s->getValue()))) {
      return f;
    }
  }
  return nullptr;
}

Value *replaceStageFunc(PipelineFlow::Stage &stage, Func *schedFunc,
                        util::CloneVisitor &cv) {
  auto *callee = stage.getCallee();
  auto *M = callee->getModule();
  if (auto *f = cast<BodiedFunc>(util::getFunc(callee))) {
    return M->Nr<VarValue>(schedFunc);
  } else if (auto *s = cast<FlowInstr>(callee)) {
    if (auto *f = cast<BodiedFunc>(util::getFunc(s->getValue()))) {
      auto *clone = cast<FlowInstr>(cv.clone(s));
      clone->setValue(M->Nr<VarValue>(schedFunc));
      return clone;
    }
  }
  seqassert(0, "invalid stage func replacement");
  return nullptr;
}
} // namespace

const std::string PipelineSubstitutionOptimization::KEY = "seq-pipeline-subst-opt";
const std::string PipelinePrefetchOptimization::KEY = "seq-pipeline-prefetch-opt";
const std::string PipelineInterAlignOptimization::KEY = "seq-pipeline-inter-align-opt";

/*
 * Substitution optimizations
 */

void PipelineSubstitutionOptimization::handle(PipelineFlow *p) {
  auto *M = p->getModule();

  PipelineFlow::Stage *prev = nullptr;
  auto it = p->begin();
  while (it != p->end()) {
    if (prev) {
      {
        auto *f1 = util::getStdlibFunc(prev->getCallee(), "kmers", "bio");
        auto *f2 = util::getStdlibFunc(it->getCallee(), "revcomp", "bio");
        if (f1 && f2) {
          auto *funcType = cast<types::FuncType>(f1->getType());
          auto *genType = cast<types::GeneratorType>(funcType->getReturnType());
          auto *seqType = funcType->front();
          auto *kmerType = genType->getBase();
          auto *kmersRevcompFunc = M->getOrRealizeFunc(
              "_kmers_revcomp", {seqType, M->getIntType()}, {kmerType}, builtinModule);
          seqassert(kmersRevcompFunc &&
                        util::getReturnType(kmersRevcompFunc)->is(genType),
                    "invalid reverse complement function");
          cast<VarValue>(prev->getCallee())->setVar(kmersRevcompFunc);
          if (it->isParallel())
            prev->setParallel();
          it = p->erase(it);
          continue;
        }
      }

      {
        auto *f1 = util::getStdlibFunc(prev->getCallee(), "kmers_with_pos", "bio");
        auto *f2 = util::getStdlibFunc(it->getCallee(), "revcomp_with_pos", "bio");
        if (f1 && f2) {
          auto *funcType = cast<types::FuncType>(f1->getType());
          auto *genType = cast<types::GeneratorType>(funcType->getReturnType());
          auto *seqType = funcType->front();
          auto *kmerType =
              cast<types::MemberedType>(genType->getBase())->back().getType();
          auto *kmersRevcompWithPosFunc =
              M->getOrRealizeFunc("_kmers_revcomp_with_pos", {seqType, M->getIntType()},
                                  {kmerType}, builtinModule);
          seqassert(kmersRevcompWithPosFunc &&
                        util::getReturnType(kmersRevcompWithPosFunc)->is(genType),
                    "invalid pos reverse complement function");
          cast<VarValue>(prev->getCallee())->setVar(kmersRevcompWithPosFunc);
          if (it->isParallel())
            prev->setParallel();
          it = p->erase(it);
          continue;
        }
      }

      {
        auto *f1 = util::getStdlibFunc(prev->getCallee(), "kmers", "bio");
        auto *f2 = util::getStdlibFunc(it->getCallee(), "canonical", "bio");
        if (f1 && f2 && util::isConst<int64_t>(prev->back(), 1)) {
          auto *funcType = cast<types::FuncType>(f1->getType());
          auto *genType = cast<types::GeneratorType>(funcType->getReturnType());
          auto *seqType = funcType->front();
          auto *kmerType = genType->getBase();
          auto *kmersCanonicalFunc = M->getOrRealizeFunc("_kmers_canonical", {seqType},
                                                         {kmerType}, builtinModule);
          seqassert(kmersCanonicalFunc &&
                        util::getReturnType(kmersCanonicalFunc)->is(genType),
                    "invalid canonical kmers function");
          cast<VarValue>(prev->getCallee())->setVar(kmersCanonicalFunc);
          prev->erase(prev->end() - 1); // remove step argument
          if (it->isParallel())
            prev->setParallel();
          it = p->erase(it);
          continue;
        }
      }

      {
        auto *f1 = util::getStdlibFunc(prev->getCallee(), "kmers_with_pos", "bio");
        auto *f2 = util::getStdlibFunc(it->getCallee(), "canonical_with_pos", "bio");
        if (f1 && f2 && util::isConst<int64_t>(prev->back(), 1)) {
          auto *funcType = cast<types::FuncType>(f1->getType());
          auto *genType = cast<types::GeneratorType>(funcType->getReturnType());
          auto *seqType = funcType->front();
          auto *kmerType =
              cast<types::MemberedType>(genType->getBase())->back().getType();
          auto *kmersCanonicalWithPosFunc = M->getOrRealizeFunc(
              "_kmers_canonical_with_pos", {seqType}, {kmerType}, builtinModule);
          seqassert(kmersCanonicalWithPosFunc &&
                        util::getReturnType(kmersCanonicalWithPosFunc)->is(genType),
                    "invalid pos canonical kmers function");
          cast<VarValue>(prev->getCallee())->setVar(kmersCanonicalWithPosFunc);
          prev->erase(prev->end() - 1); // remove step argument
          if (it->isParallel())
            prev->setParallel();
          it = p->erase(it);
          continue;
        }
      }
    }
    prev = &*it;
    ++it;
  }
}

/*
 * Prefetch optimization
 */

struct PrefetchFunctionTransformer : public util::Operator {
  void handle(ReturnInstr *x) override {
    auto *M = x->getModule();
    x->replaceAll(M->Nr<YieldInstr>(x->getValue(), /*final=*/true));
  }

  void handle(CallInstr *x) override {
    auto *func = cast<BodiedFunc>(util::getFunc(x->getCallee()));
    if (!func || func->getUnmangledName() != Module::GETITEM_MAGIC_NAME ||
        x->numArgs() != 2)
      return;

    auto *M = x->getModule();
    Value *self = x->front();
    Value *key = x->back();
    types::Type *selfType = self->getType();
    types::Type *keyType = key->getType();
    Func *prefetchFunc =
        M->getOrRealizeMethod(selfType, "__prefetch__", {selfType, keyType});
    if (!prefetchFunc)
      return;

    Value *prefetch = util::call(prefetchFunc, {self, key});
    auto *yield = M->Nr<YieldInstr>();
    auto *replacement = util::series(prefetch, yield);

    util::CloneVisitor cv(M);
    auto *clone = cv.clone(x);
    see(clone); // avoid infinite loop on clone
    x->replaceAll(M->Nr<FlowInstr>(replacement, clone));
  }
};

void PipelinePrefetchOptimization::handle(PipelineFlow *p) {
  if (isParallel(p))
    return;
  auto *M = p->getModule();
  PrefetchFunctionTransformer pft;
  PipelineFlow::Stage *prev = nullptr;
  util::CloneVisitor cv(M);
  for (auto it = p->begin(); it != p->end(); ++it) {
    if (auto *func = getStageFunc(*it)) {
      if (!it->isGenerator() && util::hasAttribute(func, "std.bio.builtin.prefetch")) {
        // transform prefetch'ing function
        auto *clone = cast<BodiedFunc>(cv.forceClone(func));
        util::setReturnType(clone, M->getGeneratorType(util::getReturnType(clone)));
        clone->setGenerator();
        clone->getBody()->accept(pft);

        // make sure the arguments are in the correct order
        auto *inputType = prev->getOutputElementType();
        clone = makeStageWrapperFunc(&*it, clone, inputType);
        auto *coroType = cast<types::FuncType>(clone->getType());

        // vars
        auto *statesType = M->getArrayType(coroType->getReturnType());
        seqassert((SCHED_WIDTH_PREFETCH & (SCHED_WIDTH_PREFETCH - 1)) == 0,
                  "not a power of 2"); // power of 2
        auto *width = M->getInt(SCHED_WIDTH_PREFETCH);

        auto *init = M->Nr<SeriesFlow>();
        auto *parent = cast<BodiedFunc>(getParentFunc());
        seqassert(parent, "not in a function");
        auto *filled = util::makeVar(M->getInt(0), init, parent);
        auto *next = util::makeVar(M->getInt(0), init, parent);
        auto *states = util::makeVar(
            M->Nr<StackAllocInstr>(statesType, SCHED_WIDTH_PREFETCH), init, parent);
        insertBefore(init);

        // scheduler
        auto *intType = M->getIntType();
        auto *intPtrType = M->getPointerType(intType);

        std::vector<types::Type *> stageArgTypes;
        std::vector<Value *> stageArgs;
        for (auto *arg : *it) {
          if (arg) {
            stageArgs.push_back(arg);
            stageArgTypes.push_back(arg->getType());
          }
        }
        auto *extraArgs = util::makeTuple(stageArgs, M);
        std::vector<types::Type *> argTypes = {
            inputType,  coroType, statesType,          intPtrType,
            intPtrType, intType,  extraArgs->getType()};

        Func *schedFunc = M->getOrRealizeFunc("_dynamic_coroutine_scheduler", argTypes,
                                              {}, prefetchModule);
        seqassert(schedFunc, "could not realize scheduler function");
        PipelineFlow::Stage stage(replaceStageFunc(*it, schedFunc, cv),
                                  {nullptr, M->Nr<VarValue>(clone), states,
                                   M->Nr<PointerValue>(next->getVar()),
                                   M->Nr<PointerValue>(filled->getVar()), width,
                                   extraArgs},
                                  /*generator=*/true, /*parallel=*/false);

        // drain
        Func *drainFunc =
            M->getOrRealizeFunc("_dynamic_coroutine_scheduler_drain",
                                {statesType, intType}, {}, prefetchModule);
        std::vector<Value *> args = {states, filled};

        std::vector<PipelineFlow::Stage> drainStages = {
            {util::call(drainFunc, args), {}, /*generator=*/true, /*parallel=*/false}};
        *it = stage;

        if (std::distance(it, p->end()) == 1 &&
            !util::getReturnType(func)->is(M->getVoidType())) {
          Func *dummyFunc =
              M->getOrRealizeFunc("_dummy_prefetch_terminal_stage",
                                  {stage.getOutputElementType()}, {}, prefetchModule);
          seqassert(dummyFunc, "could not realize dummy prefetch");
          p->push_back({M->Nr<VarValue>(dummyFunc),
                        {nullptr},
                        /*generator=*/false,
                        /*parallel=*/false});
        }

        for (++it; it != p->end(); ++it) {
          drainStages.push_back(cv.clone(*it));
        }

        auto *drain = util::series(M->Nr<AssignInstr>(next->getVar(), M->getInt(0)),
                                   M->Nr<PipelineFlow>(drainStages));
        insertAfter(drain);

        LOG_REALIZE("[prefetch] {}", *p);
        break; // at most one prefetch transformation per pipeline
      }
    }
    prev = &*it;
  }
}

/*
 * Inter-sequence alignment optimization
 */

struct InterAlignTypes {
  types::Type *seq;    // plain sequence type ('seq')
  types::Type *cigar;  // CIGAR string type ('CIGAR')
  types::Type *align;  // alignment result type ('Alignment')
  types::Type *params; // alignment parameters type ('InterAlignParams')
  types::Type *pair;   // sequence pair type ('SeqPair')
  types::Type *yield;  // inter-align yield type ('InterAlignYield')

  operator bool() const { return seq && cigar && align && params && pair && yield; }
};

InterAlignTypes gatherInterAlignTypes(Module *M) {
  return {M->getOrRealizeType("seq", {}, seqModule),
          M->getOrRealizeType("CIGAR", {}, alignModule),
          M->getOrRealizeType("Alignment", {}, alignModule),
          M->getOrRealizeType("InterAlignParams", {}, alignModule),
          M->getOrRealizeType("SeqPair", {}, alignModule),
          M->getOrRealizeType("InterAlignYield", {}, alignModule)};
}

bool isConstOrGlobal(const Value *x) {
  if (!x) {
    return false;
  } else if (auto *v = cast<VarValue>(x)) {
    return v->getVar()->isGlobal();
  } else {
    return util::isConst<int64_t>(x) || util::isConst<bool>(x);
  }
}

bool isGlobalVar(Value *x) {
  if (auto *v = cast<VarValue>(x)) {
    return v->getVar()->isGlobal();
  }
  return false;
}

template <typename T> bool verifyAlignParams(T begin, T end) {
  enum ParamKind {
    SI, // supported int
    SB, // supported bool
    UI, // unsupported int
    UB, // unsupported bool
  };

  /*
    a: int = 2,
    b: int = 4,
    ambig: int = 0,
    gapo: int = 4,
    gape: int = 2,
    gapo2: int = -1,
    gape2: int = -1,
    bandwidth: int = -1,
    zdrop: int = -1,
    end_bonus: int = 0,
    score_only: bool = False,
    right: bool = False,
    generic_sc: bool = False,
    approx_max: bool = False,
    approx_drop: bool = False,
    ext_only: bool = False,
    rev_cigar: bool = False,
    splice: bool = False,
    splice_fwd: bool = False,
    splice_rev: bool = False,
    splice_flank: bool = False
  */

  ParamKind kinds[] = {
      SI, SI, SI, SI, SI, UI, UI, SI, SI, SI, SB,
      UB, UB, UB, UB, SB, SB, UB, UB, UB, UB,
  };

  int i = 0;
  for (auto it = begin; it != end; ++it) {
    Value *v = *it;
    switch (kinds[i]) {
    case SI:
      if (!(isGlobalVar(v) || util::isConst<int64_t>(v)))
        return false;
      break;
    case SB:
      if (!(isGlobalVar(v) || util::isConst<bool>(v)))
        return false;
      break;
    case UI:
      if (!util::isConst<int64_t>(v, -1))
        return false;
      break;
    case UB:
      if (!util::isConst<bool>(v, false))
        return false;
      break;
    default:
      seqassert(0, "invalid parameters");
    }
    i += 1;
  }

  return true;
}

struct InterAlignFunctionTransformer : public util::Operator {
  InterAlignTypes *types;
  std::vector<Value *> params;

  void handle(ReturnInstr *x) override {
    seqassert(!x->getValue(), "function returns");
    auto *M = x->getModule();
    x->replaceAll(M->Nr<YieldInstr>(nullptr, /*final=*/true));
  }

  void handle(CallInstr *x) override {
    if (!params.empty())
      return;

    auto *M = x->getModule();
    auto *I = M->getIntType();
    auto *B = M->getBoolType();
    auto *alignFunc = M->getOrRealizeMethod(
        types->seq, "align", {types->seq, types->seq, I, I, I, I, I, I, I, I, I, I,
                              B,          B,          B, B, B, B, B, B, B, B, B});

    auto *func = cast<BodiedFunc>(util::getFunc(x->getCallee()));
    if (!(func && alignFunc && util::match(func, alignFunc) &&
          verifyAlignParams(x->begin() + 2, x->end())))
      return;

    params = std::vector<Value *>(x->begin(), x->end());
    Value *self = x->front();
    Value *other = *(x->begin() + 1);
    Value *extzOnly = params[17];
    Value *revCigar = params[18];
    Value *yieldValue = (*types->yield)(*self, *other, *extzOnly, *revCigar);

    auto *yieldOut = M->Nr<YieldInstr>(yieldValue);
    auto *yieldIn = M->Nr<YieldInInstr>(types->yield, /*suspend=*/false);
    auto *alnResult = M->Nr<ExtractInstr>(yieldIn, "aln");
    x->replaceAll(M->Nr<FlowInstr>(util::series(yieldOut), alnResult));
  }

  InterAlignFunctionTransformer(InterAlignTypes *types)
      : util::Operator(), types(types), params() {}

  Value *getParams() {
    // order of 'args': a, b, ambig, gapo, gape, score_only, bandwidth, zdrop, end_bonus
    std::vector<Value *> args = {params[2], params[3],  params[4],
                                 params[5], params[6],  params[12],
                                 params[9], params[10], params[11]};
    return types->params->construct(args);
  }
};

void PipelineInterAlignOptimization::handle(PipelineFlow *p) {
  if (isParallel(p))
    return;
  auto *M = p->getModule();
  auto types = gatherInterAlignTypes(M);
  if (!types) // bio module not loaded; nothing to do
    return;
  PipelineFlow::Stage *prev = nullptr;
  util::CloneVisitor cv(M);
  for (auto it = p->begin(); it != p->end(); ++it) {
    if (auto *func = getStageFunc(*it)) {
      if (!it->isGenerator() &&
          util::hasAttribute(func, "std.bio.builtin.inter_align") &&
          util::getReturnType(func)->is(M->getVoidType())) {
        // transform aligning function
        InterAlignFunctionTransformer aft(&types);
        auto *clone = cast<BodiedFunc>(cv.forceClone(func));
        util::setReturnType(clone, M->getGeneratorType(types.yield));
        clone->setGenerator();
        clone->getBody()->accept(aft);
        if (aft.params.empty())
          continue;

        // make sure the arguments are in the correct order
        auto *inputType = prev->getOutputElementType();
        clone = makeStageWrapperFunc(&*it, clone, inputType);
        auto *coroType = cast<types::FuncType>(clone->getType());

        // vars
        // following defs are from bio/align.seq
        const int LEN_LIMIT = 512;
        const int MAX_SEQ_LEN8 = 128;
        const int MAX_SEQ_LEN16 = 32768;
        const unsigned W = SCHED_WIDTH_INTERALIGN;

        auto *intType = M->getIntType();
        auto *intPtrType = M->getPointerType(intType);
        auto *i32 = M->getIntNType(32, true);

        auto *parent = cast<BodiedFunc>(getParentFunc());
        seqassert(parent, "not in a function");

        auto *init = M->Nr<SeriesFlow>();
        auto *states =
            util::makeVar(util::alloc(coroType->getReturnType(), W), init, parent);
        auto *statesTemp =
            util::makeVar(util::alloc(coroType->getReturnType(), W), init, parent);
        auto *pairs = util::makeVar(util::alloc(types.pair, W), init, parent);
        auto *pairsTemp = util::makeVar(util::alloc(types.pair, W), init, parent);
        auto *bufRef =
            util::makeVar(util::alloc(M->getByteType(), LEN_LIMIT * W), init, parent);
        auto *bufQer =
            util::makeVar(util::alloc(M->getByteType(), LEN_LIMIT * W), init, parent);
        auto *hist = util::makeVar(util::alloc(i32, MAX_SEQ_LEN8 + MAX_SEQ_LEN16 + 32),
                                   init, parent);
        auto *filled = util::makeVar(M->getInt(0), init, parent);
        insertBefore(init);

        auto *width = M->getInt(W);
        auto *params = aft.getParams();

        std::vector<types::Type *> stageArgTypes;
        std::vector<Value *> stageArgs;
        for (auto *arg : *it) {
          if (arg) {
            stageArgs.push_back(arg);
            stageArgTypes.push_back(arg->getType());
          }
        }
        auto *extraArgs = util::makeTuple(stageArgs, M);

        auto *schedFunc = M->getOrRealizeFunc(
            "_interaln_scheduler",
            {inputType, coroType, pairs->getType(), bufRef->getType(),
             bufQer->getType(), states->getType(), types.params, hist->getType(),
             pairsTemp->getType(), statesTemp->getType(), intPtrType, intType,
             extraArgs->getType()},
            {}, alignModule);
        auto *flushFunc = M->getOrRealizeFunc(
            "_interaln_flush",
            {pairs->getType(), bufRef->getType(), bufQer->getType(), states->getType(),
             M->getIntType(), types.params, hist->getType(), pairsTemp->getType(),
             statesTemp->getType()},
            {}, alignModule);
        seqassert(schedFunc, "could not realize scheduler");
        seqassert(flushFunc, "could not realize flush");

        PipelineFlow::Stage stage(replaceStageFunc(*it, schedFunc, cv),
                                  {nullptr, M->Nr<VarValue>(clone), pairs, bufRef,
                                   bufQer, states, params, hist, pairsTemp, statesTemp,
                                   M->Nr<PointerValue>(filled->getVar()), width,
                                   extraArgs},
                                  /*generator=*/false, /*parallel=*/false);
        *it = stage;

        auto *drain = util::call(flushFunc, {pairs, bufRef, bufQer, states, filled,
                                             params, hist, pairsTemp, statesTemp});
        insertAfter(drain);

        break; // at most one inter-sequence alignment transformation per pipeline
      }
    }
    prev = &*it;
  }
}

} // namespace seq
