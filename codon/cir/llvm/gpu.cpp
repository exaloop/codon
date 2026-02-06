// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "gpu.h"

#include <algorithm>
#include <memory>
#include <string>

#include "codon/cir/llvm/optimize.h"
#include "codon/util/common.h"

namespace codon {
namespace ir {
namespace {
const std::string GPU_TRIPLE = "nvptx64-nvidia-cuda";
const std::string GPU_DL =
    "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-"
    "f64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-n16:32:64";
llvm::cl::opt<std::string>
    libdevice("libdevice", llvm::cl::desc("libdevice path for GPU kernels"),
              llvm::cl::init("/usr/local/cuda/nvvm/libdevice/libdevice.10.bc"));
llvm::cl::opt<std::string> ptxOutput("ptx",
                                     llvm::cl::desc("Output PTX to specified file"));
llvm::cl::opt<std::string> gpuName(
    "gpu-name",
    llvm::cl::desc(
        "Target GPU architecture or compute capability (e.g. sm_70, sm_80, etc.)"),
    llvm::cl::init("sm_30"));
llvm::cl::opt<std::string> gpuFeatures(
    "gpu-features",
    llvm::cl::desc("GPU feature flags passed (e.g. +ptx42 to enable PTX 4.2 features)"),
    llvm::cl::init("+ptx42"));

// Adapted from LLVM's GVExtractorPass, which is not externally available
// as a pass for the new pass manager.
class GVExtractor : public llvm::PassInfoMixin<GVExtractor> {
  llvm::SetVector<llvm::GlobalValue *> named;
  bool deleteStuff;
  bool keepConstInit;

public:
  // If deleteS is true, this pass deletes the specified global values.
  // Otherwise, it deletes as much of the module as possible, except for the
  // global values specified.
  explicit GVExtractor(std::vector<llvm::GlobalValue *> &GVs, bool deleteS = true,
                       bool keepConstInit = false)
      : named(GVs.begin(), GVs.end()), deleteStuff(deleteS),
        keepConstInit(keepConstInit) {}

  // Make sure GV is visible from both modules. Delete is true if it is
  // being deleted from this module.
  // This also makes sure GV cannot be dropped so that references from
  // the split module remain valid.
  static void makeVisible(llvm::GlobalValue &GV, bool del) {
    bool local = GV.hasLocalLinkage();
    if (local || del) {
      GV.setLinkage(llvm::GlobalValue::ExternalLinkage);
      if (local)
        GV.setVisibility(llvm::GlobalValue::HiddenVisibility);
      return;
    }

    if (!GV.hasLinkOnceLinkage()) {
      seqassertn(!GV.isDiscardableIfUnused(), "bad global in extractor");
      return;
    }

    // Map linkonce* to weak* so that llvm doesn't drop this GV.
    switch (GV.getLinkage()) {
    default:
      seqassertn(false, "unexpected linkage");
    case llvm::GlobalValue::LinkOnceAnyLinkage:
      GV.setLinkage(llvm::GlobalValue::WeakAnyLinkage);
      return;
    case llvm::GlobalValue::LinkOnceODRLinkage:
      GV.setLinkage(llvm::GlobalValue::WeakODRLinkage);
      return;
    }
  }

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &) {
    // Visit the global inline asm.
    if (!deleteStuff)
      M.setModuleInlineAsm("");

    // For simplicity, just give all GlobalValues ExternalLinkage. A trickier
    // implementation could figure out which GlobalValues are actually
    // referenced by the 'named' set, and which GlobalValues in the rest of
    // the module are referenced by the NamedSet, and get away with leaving
    // more internal and private things internal and private. But for now,
    // be conservative and simple.

    // Visit the GlobalVariables.
    for (auto &GV : M.globals()) {
      bool del = deleteStuff == (bool)named.count(&GV) && !GV.isDeclaration() &&
                 (!GV.isConstant() || !keepConstInit);
      if (!del) {
        if (GV.hasAvailableExternallyLinkage())
          continue;
        if (GV.getName() == "llvm.global_ctors")
          continue;
      }

      makeVisible(GV, del);

      if (del) {
        // Make this a declaration and drop it's comdat.
        GV.setInitializer(nullptr);
        GV.setComdat(nullptr);
      }
    }

    // Visit the Functions.
    for (auto &F : M) {
      bool del = deleteStuff == (bool)named.count(&F) && !F.isDeclaration();
      if (!del) {
        if (F.hasAvailableExternallyLinkage())
          continue;
      }

      makeVisible(F, del);

      if (del) {
        // Make this a declaration and drop it's comdat.
        F.deleteBody();
        F.setComdat(nullptr);
      }
    }

    // Visit the Aliases.
    for (auto &GA : llvm::make_early_inc_range(M.aliases())) {
      bool del = deleteStuff == (bool)named.count(&GA);
      makeVisible(GA, del);

      if (del) {
        auto *ty = GA.getValueType();
        GA.removeFromParent();
        llvm::Value *decl;
        if (auto *funcTy = llvm::dyn_cast<llvm::FunctionType>(ty)) {
          decl = llvm::Function::Create(funcTy, llvm::GlobalValue::ExternalLinkage,
                                        GA.getAddressSpace(), GA.getName(), &M);

        } else {
          decl = new llvm::GlobalVariable(
              M, ty, false, llvm::GlobalValue::ExternalLinkage, nullptr, GA.getName());
        }
        GA.replaceAllUsesWith(decl);
        delete &GA;
      }
    }

    return llvm::PreservedAnalyses::none();
  }
};

std::string cleanUpName(llvm::StringRef name) {
  std::string validName;
  llvm::raw_string_ostream validNameStream(validName);

  auto valid = [](char c, bool first) {
    bool ok = ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
    if (!first)
      ok = ok || ('0' <= c && c <= '9');
    return ok;
  };

  bool first = true;
  for (char c : name) {
    validNameStream << (valid(c, first) ? c : '_');
    first = false;
  }

  return validNameStream.str();
}

void linkLibdevice(llvm::Module *M, const std::string &path) {
  llvm::SMDiagnostic err;
  auto libdevice = llvm::parseIRFile(path, err, M->getContext());
  if (!libdevice)
    compilationError(err.getMessage().str(), err.getFilename().str(), err.getLineNo(),
                     err.getColumnNo());
  libdevice->setDataLayout(M->getDataLayout());
  libdevice->setTargetTriple(M->getTargetTriple());

  llvm::Linker L(*M);
  const bool fail = L.linkInModule(std::move(libdevice));
  seqassertn(!fail, "linking libdevice failed");
}

llvm::Function *copyPrototype(llvm::Function *F, const std::string &name,
                              bool external = false) {
  auto *M = F->getParent();
  return llvm::Function::Create(F->getFunctionType(),
                                external ? llvm::GlobalValue::ExternalLinkage
                                         : llvm::GlobalValue::PrivateLinkage,
                                name.empty() ? F->getName() : name, *M);
}

llvm::Function *makeNoOp(llvm::Function *F) {
  auto *M = F->getParent();
  auto &context = M->getContext();
  auto dummyName = (".codon.gpu.dummy." + F->getName()).str();
  auto *dummy = M->getFunction(dummyName);
  if (!dummy) {
    dummy = copyPrototype(F, dummyName);
    auto *entry = llvm::BasicBlock::Create(context, "entry", dummy);
    llvm::IRBuilder<> B(entry);

    auto *retType = F->getReturnType();
    if (retType->isVoidTy()) {
      B.CreateRetVoid();
    } else {
      B.CreateRet(llvm::UndefValue::get(retType));
    }
  }
  return dummy;
}

using Codegen =
    std::function<void(llvm::IRBuilder<> &, const std::vector<llvm::Value *> &)>;

void codegenVectorizedUnaryLoop(llvm::IRBuilder<> &B,
                                const std::vector<llvm::Value *> &args,
                                llvm::Function *func) {
  // Create IR to represent:
  //   p_in = in
  //   p_out = out
  //   for i in range(n):
  //       *p_out = func(*p_in)
  //       p_in += is
  //       p_out += os
  auto &context = B.getContext();
  auto *parent = B.GetInsertBlock()->getParent();
  auto *ty = func->getReturnType();
  auto *in = args[0];
  auto *is = args[1];
  auto *out = args[2];
  auto *os = args[3];
  auto *n = args[4];

  auto *loop = llvm::BasicBlock::Create(context, "loop", parent);
  auto *exit = llvm::BasicBlock::Create(context, "exit", parent);

  auto *pinStore = B.CreateAlloca(B.getPtrTy());
  auto *poutStore = B.CreateAlloca(B.getPtrTy());
  auto *idxStore = B.CreateAlloca(B.getInt64Ty());

  // p_in = in
  B.CreateStore(in, pinStore);
  // p_out = out
  B.CreateStore(out, poutStore);
  // i = 0
  B.CreateStore(B.getInt64(0), idxStore);
  // if n > 0: goto loop; else: goto exit
  B.CreateCondBr(B.CreateICmpSGT(n, B.getInt64(0)), loop, exit);

  // load pointers
  B.SetInsertPoint(loop);
  auto *pin = B.CreateLoad(B.getPtrTy(), pinStore);
  auto *pout = B.CreateLoad(B.getPtrTy(), poutStore);

  // y = func(x)
  auto *x = B.CreateLoad(ty, pin);
  auto *y = B.CreateCall(func, x);
  B.CreateStore(y, pout);

  auto *idx = B.CreateLoad(B.getInt64Ty(), idxStore);
  // i += 1
  B.CreateStore(B.CreateAdd(idx, B.getInt64(1)), idxStore);
  // p_in += is
  B.CreateStore(B.CreateGEP(B.getInt8Ty(), pin, is), pinStore);
  // p_out += os
  B.CreateStore(B.CreateGEP(B.getInt8Ty(), pout, os), poutStore);

  idx = B.CreateLoad(B.getInt64Ty(), idxStore);
  // if i < n: goto loop; else: goto exit
  B.CreateCondBr(B.CreateICmpSLT(idx, n), loop, exit);

  B.SetInsertPoint(exit);
  B.CreateRet(llvm::UndefValue::get(parent->getReturnType()));
}

void codegenVectorizedBinaryLoop(llvm::IRBuilder<> &B,
                                 const std::vector<llvm::Value *> &args,
                                 llvm::Function *func) {
  // Create IR to represent:
  //   p_in1 = in1
  //   p_in2 = in2
  //   p_out = out
  //   for i in range(n):
  //       *p_out = func(*p_in1, *p_in2)
  //       p_in1 += is1
  //       p_in2 += is2
  //       p_out += os
  auto &context = B.getContext();
  auto *parent = B.GetInsertBlock()->getParent();
  auto *ty = func->getReturnType();
  auto *in1 = args[0];
  auto *is1 = args[1];
  auto *in2 = args[2];
  auto *is2 = args[3];
  auto *out = args[4];
  auto *os = args[5];
  auto *n = args[6];

  auto *loop = llvm::BasicBlock::Create(context, "loop", parent);
  auto *exit = llvm::BasicBlock::Create(context, "exit", parent);

  auto *pin1Store = B.CreateAlloca(B.getPtrTy());
  auto *pin2Store = B.CreateAlloca(B.getPtrTy());
  auto *poutStore = B.CreateAlloca(B.getPtrTy());
  auto *idxStore = B.CreateAlloca(B.getInt64Ty());

  // p_in1 = in1
  B.CreateStore(in1, pin1Store);
  // p_in2 = in2
  B.CreateStore(in2, pin2Store);
  // p_out = out
  B.CreateStore(out, poutStore);
  // i = 0
  B.CreateStore(B.getInt64(0), idxStore);
  // if n > 0: goto loop; else: goto exit
  B.CreateCondBr(B.CreateICmpSGT(n, B.getInt64(0)), loop, exit);

  // load pointers
  B.SetInsertPoint(loop);
  auto *pin1 = B.CreateLoad(B.getPtrTy(), pin1Store);
  auto *pin2 = B.CreateLoad(B.getPtrTy(), pin2Store);
  auto *pout = B.CreateLoad(B.getPtrTy(), poutStore);

  // y = func(x1, x2)
  auto *x1 = B.CreateLoad(ty, pin1);
  auto *x2 = B.CreateLoad(ty, pin2);
  auto *y = B.CreateCall(func, {x1, x2});
  B.CreateStore(y, pout);

  auto *idx = B.CreateLoad(B.getInt64Ty(), idxStore);
  // i += 1
  B.CreateStore(B.CreateAdd(idx, B.getInt64(1)), idxStore);
  // p_in1 += is1
  B.CreateStore(B.CreateGEP(B.getInt8Ty(), pin1, is1), pin1Store);
  // p_in2 += is2
  B.CreateStore(B.CreateGEP(B.getInt8Ty(), pin2, is2), pin2Store);
  // p_out += os
  B.CreateStore(B.CreateGEP(B.getInt8Ty(), pout, os), poutStore);

  idx = B.CreateLoad(B.getInt64Ty(), idxStore);
  // if i < n: goto loop; else: goto exit
  B.CreateCondBr(B.CreateICmpSLT(idx, n), loop, exit);

  B.SetInsertPoint(exit);
  B.CreateRet(llvm::UndefValue::get(parent->getReturnType()));
}

llvm::Function *makeFillIn(llvm::Function *F, Codegen codegen) {
  auto *M = F->getParent();
  auto &context = M->getContext();
  auto fillInName = (".codon.gpu.fillin." + F->getName()).str();
  auto *fillIn = M->getFunction(fillInName);
  if (!fillIn) {
    fillIn = copyPrototype(F, fillInName);
    std::vector<llvm::Value *> args;
    for (auto it = fillIn->arg_begin(); it != fillIn->arg_end(); ++it) {
      args.push_back(it);
    }
    auto *entry = llvm::BasicBlock::Create(context, "entry", fillIn);
    llvm::IRBuilder<> B(entry);
    codegen(B, args);
  }
  return fillIn;
}

llvm::Function *makeMalloc(llvm::Module *M) {
  auto &context = M->getContext();
  llvm::IRBuilder<> B(context);
  auto F = M->getOrInsertFunction("malloc", B.getPtrTy(), B.getInt64Ty());
  auto *G = llvm::cast<llvm::Function>(F.getCallee());
  G->setLinkage(llvm::GlobalValue::ExternalLinkage);
  G->setDoesNotThrow();
  G->setReturnDoesNotAlias();
  G->setOnlyAccessesInaccessibleMemory();
  G->setWillReturn();
  return G;
}

void remapFunctions(llvm::Module *M) {
  // simple name-to-name remappings
  static const std::vector<std::pair<std::string, std::string>> remapping = {
      // 64-bit float intrinsics
      {"llvm.ceil.f64", "__nv_ceil"},
      {"llvm.floor.f64", "__nv_floor"},
      {"llvm.fabs.f64", "__nv_fabs"},
      {"llvm.exp.f64", "__nv_exp"},
      {"llvm.log.f64", "__nv_log"},
      {"llvm.log2.f64", "__nv_log2"},
      {"llvm.log10.f64", "__nv_log10"},
      {"llvm.sqrt.f64", "__nv_sqrt"},
      {"llvm.pow.f64", "__nv_pow"},
      {"llvm.sin.f64", "__nv_sin"},
      {"llvm.cos.f64", "__nv_cos"},
      {"llvm.copysign.f64", "__nv_copysign"},
      {"llvm.trunc.f64", "__nv_trunc"},
      {"llvm.rint.f64", "__nv_rint"},
      {"llvm.nearbyint.f64", "__nv_nearbyint"},
      {"llvm.round.f64", "__nv_round"},
      {"llvm.minnum.f64", "__nv_fmin"},
      {"llvm.maxnum.f64", "__nv_fmax"},
      {"llvm.copysign.f64", "__nv_copysign"},
      {"llvm.fma.f64", "__nv_fma"},

      // 64-bit float math functions
      {"expm1", "__nv_expm1"},
      {"ldexp", "__nv_ldexp"},
      {"acos", "__nv_acos"},
      {"asin", "__nv_asin"},
      {"atan", "__nv_atan"},
      {"atan2", "__nv_atan2"},
      {"hypot", "__nv_hypot"},
      {"tan", "__nv_tan"},
      {"cosh", "__nv_cosh"},
      {"sinh", "__nv_sinh"},
      {"tanh", "__nv_tanh"},
      {"acosh", "__nv_acosh"},
      {"asinh", "__nv_asinh"},
      {"atanh", "__nv_atanh"},
      {"erf", "__nv_erf"},
      {"erfc", "__nv_erfc"},
      {"tgamma", "__nv_tgamma"},
      {"lgamma", "__nv_lgamma"},
      {"remainder", "__nv_remainder"},
      {"frexp", "__nv_frexp"},
      {"modf", "__nv_modf"},

      // 32-bit float intrinsics
      {"llvm.ceil.f32", "__nv_ceilf"},
      {"llvm.floor.f32", "__nv_floorf"},
      {"llvm.fabs.f32", "__nv_fabsf"},
      {"llvm.exp.f32", "__nv_expf"},
      {"llvm.log.f32", "__nv_logf"},
      {"llvm.log2.f32", "__nv_log2f"},
      {"llvm.log10.f32", "__nv_log10f"},
      {"llvm.sqrt.f32", "__nv_sqrtf"},
      {"llvm.pow.f32", "__nv_powf"},
      {"llvm.sin.f32", "__nv_sinf"},
      {"llvm.cos.f32", "__nv_cosf"},
      {"llvm.copysign.f32", "__nv_copysignf"},
      {"llvm.trunc.f32", "__nv_truncf"},
      {"llvm.rint.f32", "__nv_rintf"},
      {"llvm.nearbyint.f32", "__nv_nearbyintf"},
      {"llvm.round.f32", "__nv_roundf"},
      {"llvm.minnum.f32", "__nv_fminf"},
      {"llvm.maxnum.f32", "__nv_fmaxf"},
      {"llvm.copysign.f32", "__nv_copysignf"},
      {"llvm.fma.f32", "__nv_fmaf"},

      // 32-bit float math functions
      {"expm1f", "__nv_expm1f"},
      {"ldexpf", "__nv_ldexpf"},
      {"acosf", "__nv_acosf"},
      {"asinf", "__nv_asinf"},
      {"atanf", "__nv_atanf"},
      {"atan2f", "__nv_atan2f"},
      {"hypotf", "__nv_hypotf"},
      {"tanf", "__nv_tanf"},
      {"coshf", "__nv_coshf"},
      {"sinhf", "__nv_sinhf"},
      {"tanhf", "__nv_tanhf"},
      {"acoshf", "__nv_acoshf"},
      {"asinhf", "__nv_asinhf"},
      {"atanhf", "__nv_atanhf"},
      {"erff", "__nv_erff"},
      {"erfcf", "__nv_erfcf"},
      {"tgammaf", "__nv_tgammaf"},
      {"lgammaf", "__nv_lgammaf"},
      {"remainderf", "__nv_remainderf"},
      {"frexpf", "__nv_frexpf"},
      {"modff", "__nv_modff"},

      // runtime library functions
      {"seq_free", "free"},
      {"seq_register_finalizer", ""},
      {"seq_gc_add_roots", ""},
      {"seq_gc_remove_roots", ""},
      {"seq_gc_clear_roots", ""},
      {"seq_gc_exclude_static_roots", ""},
  };

  // functions that need to be generated as they're not available on GPU
  static const std::vector<std::pair<std::string, Codegen>> fillins = {
      {"seq_alloc",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *mem = B.CreateCall(makeMalloc(M), args[0]);
         B.CreateRet(mem);
       }},

      {"seq_alloc_uncollectable",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *mem = B.CreateCall(makeMalloc(M), args[0]);
         B.CreateRet(mem);
       }},

      {"seq_alloc_atomic",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *mem = B.CreateCall(makeMalloc(M), args[0]);
         B.CreateRet(mem);
       }},

      {"seq_alloc_atomic_uncollectable",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *mem = B.CreateCall(makeMalloc(M), args[0]);
         B.CreateRet(mem);
       }},

      {"seq_realloc",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *mem = B.CreateCall(makeMalloc(M), args[1]);
         auto F = llvm::Intrinsic::getDeclaration(
             M, llvm::Intrinsic::memcpy, {B.getPtrTy(), B.getPtrTy(), B.getInt64Ty()});
         B.CreateCall(F, {mem, args[0], args[2], B.getFalse()});
         B.CreateRet(mem);
       }},

      {"seq_calloc",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *size = B.CreateMul(args[0], args[1]);
         llvm::Value *mem = B.CreateCall(makeMalloc(M), size);
         auto F = llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::memset,
                                                  {B.getPtrTy(), B.getInt64Ty()});
         B.CreateCall(F, {mem, B.getInt8(0), size, B.getFalse()});
         B.CreateRet(mem);
       }},

      {"seq_calloc_atomic",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *size = B.CreateMul(args[0], args[1]);
         llvm::Value *mem = B.CreateCall(makeMalloc(M), size);
         auto F = llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::memset,
                                                  {B.getPtrTy(), B.getInt64Ty()});
         B.CreateCall(F, {mem, B.getInt8(0), size, B.getFalse()});
         B.CreateRet(mem);
       }},

      {"seq_alloc_exc",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         // TODO: print error message and abort if in debug mode
         B.CreateUnreachable();
       }},

      {"seq_throw",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         B.CreateUnreachable();
       }},

#define FILLIN_VECLOOP_UNARY32(loop, func)                                             \
  {                                                                                    \
    loop, [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {           \
      auto *M = B.GetInsertBlock()->getModule();                                       \
      auto f = llvm::cast<llvm::Function>(                                             \
          M->getOrInsertFunction(func, B.getFloatTy(), B.getFloatTy()).getCallee());   \
      f->setWillReturn();                                                              \
      codegenVectorizedUnaryLoop(B, args, f);                                          \
    }                                                                                  \
  }

#define FILLIN_VECLOOP_UNARY64(loop, func)                                             \
  {                                                                                    \
    loop, [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {           \
      auto *M = B.GetInsertBlock()->getModule();                                       \
      auto f = llvm::cast<llvm::Function>(                                             \
          M->getOrInsertFunction(func, B.getDoubleTy(), B.getDoubleTy()).getCallee()); \
      f->setWillReturn();                                                              \
      codegenVectorizedUnaryLoop(B, args, f);                                          \
    }                                                                                  \
  }

#define FILLIN_VECLOOP_BINARY32(loop, func)                                            \
  {                                                                                    \
    loop, [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {           \
      auto *M = B.GetInsertBlock()->getModule();                                       \
      auto f = llvm::cast<llvm::Function>(                                             \
          M->getOrInsertFunction(func, B.getFloatTy(), B.getFloatTy(), B.getFloatTy()) \
              .getCallee());                                                           \
      f->setWillReturn();                                                              \
      codegenVectorizedBinaryLoop(B, args, f);                                         \
    }                                                                                  \
  }

#define FILLIN_VECLOOP_BINARY64(loop, func)                                            \
  {                                                                                    \
    loop, [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {           \
      auto *M = B.GetInsertBlock()->getModule();                                       \
      auto f = llvm::cast<llvm::Function>(                                             \
          M->getOrInsertFunction(func, B.getDoubleTy(), B.getDoubleTy(),               \
                                 B.getDoubleTy())                                      \
              .getCallee());                                                           \
      f->setWillReturn();                                                              \
      codegenVectorizedBinaryLoop(B, args, f);                                         \
    }                                                                                  \
  }

      FILLIN_VECLOOP_UNARY64("cnp_acos_float64", "__nv_acos"),
      FILLIN_VECLOOP_UNARY64("cnp_acosh_float64", "__nv_acosh"),
      FILLIN_VECLOOP_UNARY64("cnp_asin_float64", "__nv_asin"),
      FILLIN_VECLOOP_UNARY64("cnp_asinh_float64", "__nv_asinh"),
      FILLIN_VECLOOP_UNARY64("cnp_atan_float64", "__nv_atan"),
      FILLIN_VECLOOP_UNARY64("cnp_atanh_float64", "__nv_atanh"),
      FILLIN_VECLOOP_BINARY64("cnp_atan2_float64", "__nv_atan2"),
      FILLIN_VECLOOP_UNARY64("cnp_exp_float64", "__nv_exp"),
      FILLIN_VECLOOP_UNARY64("cnp_exp2_float64", "__nv_exp2"),
      FILLIN_VECLOOP_UNARY64("cnp_expm1_float64", "__nv_expm1"),
      FILLIN_VECLOOP_UNARY64("cnp_log_float64", "__nv_log"),
      FILLIN_VECLOOP_UNARY64("cnp_log10_float64", "__nv_log10"),
      FILLIN_VECLOOP_UNARY64("cnp_log1p_float64", "__nv_log1p"),
      FILLIN_VECLOOP_UNARY64("cnp_log2_float64", "__nv_log2"),
      FILLIN_VECLOOP_UNARY64("cnp_sin_float64", "__nv_sin"),
      FILLIN_VECLOOP_UNARY64("cnp_sinh_float64", "__nv_sinh"),
      FILLIN_VECLOOP_UNARY64("cnp_tan_float64", "__nv_tan"),
      FILLIN_VECLOOP_UNARY64("cnp_tanh_float64", "__nv_tanh"),
      FILLIN_VECLOOP_BINARY64("cnp_hypot_float64", "__nv_hypot"),

      FILLIN_VECLOOP_UNARY32("cnp_acos_float32", "__nv_acosf"),
      FILLIN_VECLOOP_UNARY32("cnp_acosh_float32", "__nv_acoshf"),
      FILLIN_VECLOOP_UNARY32("cnp_asin_float32", "__nv_asinf"),
      FILLIN_VECLOOP_UNARY32("cnp_asinh_float32", "__nv_asinhf"),
      FILLIN_VECLOOP_UNARY32("cnp_atan_float32", "__nv_atanf"),
      FILLIN_VECLOOP_UNARY32("cnp_atanh_float32", "__nv_atanhf"),
      FILLIN_VECLOOP_BINARY32("cnp_atan2_float32", "__nv_atan2f"),
      FILLIN_VECLOOP_UNARY32("cnp_exp_float32", "__nv_expf"),
      FILLIN_VECLOOP_UNARY32("cnp_exp2_float32", "__nv_exp2f"),
      FILLIN_VECLOOP_UNARY32("cnp_expm1_float32", "__nv_expm1f"),
      FILLIN_VECLOOP_UNARY32("cnp_log_float32", "__nv_logf"),
      FILLIN_VECLOOP_UNARY32("cnp_log10_float32", "__nv_log10f"),
      FILLIN_VECLOOP_UNARY32("cnp_log1p_float32", "__nv_log1pf"),
      FILLIN_VECLOOP_UNARY32("cnp_log2_float32", "__nv_log2f"),
      FILLIN_VECLOOP_UNARY32("cnp_sin_float32", "__nv_sinf"),
      FILLIN_VECLOOP_UNARY32("cnp_sinh_float32", "__nv_sinhf"),
      FILLIN_VECLOOP_UNARY32("cnp_tan_float32", "__nv_tanf"),
      FILLIN_VECLOOP_UNARY32("cnp_tanh_float32", "__nv_tanhf"),
      FILLIN_VECLOOP_BINARY32("cnp_hypot_float32", "__nv_hypotf"),
  };

  for (auto &pair : remapping) {
    if (auto *F = M->getFunction(pair.first)) {
      llvm::Function *G = nullptr;
      if (pair.second.empty()) {
        G = makeNoOp(F);
      } else {
        G = M->getFunction(pair.second);
        if (!G)
          G = copyPrototype(F, pair.second, /*external=*/true);
      }

      G->setWillReturn();
      F->replaceAllUsesWith(G);
      F->dropAllReferences();
      F->eraseFromParent();
    }
  }

  for (auto &pair : fillins) {
    if (auto *F = M->getFunction(pair.first)) {
      llvm::Function *G = makeFillIn(F, pair.second);
      F->replaceAllUsesWith(G);
      F->dropAllReferences();
      F->eraseFromParent();
    }
  }
}

void exploreGV(llvm::GlobalValue *G, llvm::SmallPtrSetImpl<llvm::GlobalValue *> &keep) {
  if (keep.contains(G))
    return;

  keep.insert(G);
  if (auto *F = llvm::dyn_cast<llvm::Function>(G)) {
    for (auto I = llvm::inst_begin(F), E = inst_end(F); I != E; ++I) {
      for (auto &U : I->operands()) {
        if (auto *G2 = llvm::dyn_cast<llvm::GlobalValue>(U.get()))
          exploreGV(G2, keep);
      }
    }
  }
}

std::vector<llvm::GlobalValue *>
getRequiredGVs(const std::vector<llvm::GlobalValue *> &kernels) {
  llvm::SmallPtrSet<llvm::GlobalValue *, 32> keep;
  for (auto *G : kernels) {
    exploreGV(G, keep);
  }
  return std::vector<llvm::GlobalValue *>(keep.begin(), keep.end());
}

std::string moduleToPTX(llvm::Module *M, std::vector<llvm::GlobalValue *> &kernels) {
  llvm::Triple triple(llvm::Triple::normalize(GPU_TRIPLE));
  llvm::TargetLibraryInfoImpl tlii(triple);

  std::string err;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget("nvptx64", triple, err);
  seqassertn(target, "couldn't lookup target: {}", err);

  const llvm::TargetOptions options =
      llvm::codegen::InitTargetOptionsFromCodeGenFlags(triple);

  std::unique_ptr<llvm::TargetMachine> machine(target->createTargetMachine(
      triple.getTriple(), gpuName, gpuFeatures, options,
      llvm::codegen::getExplicitRelocModel(), llvm::codegen::getExplicitCodeModel(),
      llvm::CodeGenOptLevel::Aggressive));

  // Remove personality functions
  for (auto &F : *M)
    F.setPersonalityFn(nullptr);

  M->setDataLayout(machine->createDataLayout());
  auto keep = getRequiredGVs(kernels);

  auto prune = [&](std::vector<llvm::GlobalValue *> keep) {
    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cgam;
    llvm::ModuleAnalysisManager mam;
    llvm::ModulePassManager mpm;
    llvm::PassBuilder pb;

    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    mpm.addPass(GVExtractor(keep, false));
    mpm.addPass(llvm::GlobalDCEPass());
    mpm.addPass(llvm::StripDeadDebugInfoPass());
    mpm.addPass(llvm::StripDeadPrototypesPass());
    mpm.run(*M, mam);
  };

  // Remove non-kernel functions.
  prune(keep);

  // Link libdevice and other cleanup.
  linkLibdevice(M, libdevice);
  remapFunctions(M);

  // Strip debug info and remove noinline from functions (added in debug mode).
  // Also, tell LLVM that all functions will return.
  for (auto &F : *M) {
    F.removeFnAttr(llvm::Attribute::AttrKind::NoInline);
    F.setWillReturn();
  }
  llvm::StripDebugInfo(*M);

  // Run NVPTX passes and general opt pipeline.
  {
    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cgam;
    llvm::ModuleAnalysisManager mam;
    llvm::PassBuilder pb(machine.get());

    llvm::TargetLibraryInfoImpl tlii(triple);
    fam.registerPass([&] { return llvm::TargetLibraryAnalysis(tlii); });

    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    pb.registerPipelineStartEPCallback(
        [&](llvm::ModulePassManager &pm, llvm::OptimizationLevel opt) {
          pm.addPass(llvm::InternalizePass([&](const llvm::GlobalValue &gv) {
            return std::find(keep.begin(), keep.end(), &gv) != keep.end();
          }));
        });

    llvm::ModulePassManager mpm =
        pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
    mpm.run(*M, mam);
  }

  // Prune again after optimizations.
  keep = getRequiredGVs(kernels);
  prune(keep);

  // Clean up names.
  {
    for (auto &G : M->globals()) {
      G.setName(cleanUpName(G.getName()));
    }

    for (auto &F : M->functions()) {
      if (F.getInstructionCount() > 0)
        F.setName(cleanUpName(F.getName()));
    }

    for (auto *S : M->getIdentifiedStructTypes()) {
      S->setName(cleanUpName(S->getName()));
    }
  }

  // Generate PTX code.
  {
    llvm::SmallVector<char, 1024> ptx;
    llvm::raw_svector_ostream os(ptx);

    auto *mmiwp = new llvm::MachineModuleInfoWrapperPass(machine.get());
    llvm::legacy::PassManager pm;

    pm.add(new llvm::TargetLibraryInfoWrapperPass(tlii));
    bool fail = machine->addPassesToEmitFile(pm, os, nullptr,
                                             llvm::CodeGenFileType::AssemblyFile,
                                             /*DisableVerify=*/false, mmiwp);
    seqassertn(!fail, "could not add passes");

    const_cast<llvm::TargetLoweringObjectFile *>(machine->getObjFileLowering())
        ->Initialize(mmiwp->getMMI().getContext(), *machine);

    pm.run(*M);
    return std::string(ptx.data(), ptx.size());
  }
}

void cleanUpIntrinsics(llvm::Module *M) {
  llvm::LLVMContext &context = M->getContext();
  llvm::SmallVector<llvm::Function *, 16> remove;
  for (auto &F : *M) {
    if (F.getIntrinsicID() != llvm::Intrinsic::not_intrinsic &&
        F.getName().starts_with("llvm.nvvm"))
      remove.push_back(&F);
  }

  for (auto *F : remove) {
    F->replaceAllUsesWith(makeNoOp(F));
    F->dropAllReferences();
    F->eraseFromParent();
  }
}

void patchPTXVar(llvm::Module *M, llvm::GlobalValue *ptxVar,
                 const std::string &ptxTarget = "__codon_ptx__") {
  // Find and patch direct calls to cuModuleLoadData()
  llvm::SmallVector<llvm::Instruction *, 1> callsToReplace;
  for (auto &F : *M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *call = llvm::dyn_cast<llvm::CallBase>(&I);
        if (!call)
          continue;

        auto *callee = call->getCalledFunction();
        if (!callee)
          continue;

        if (callee->getName() == ptxTarget && call->arg_size() == 0)
          callsToReplace.push_back(call);
      }
    }
  }

  for (auto *call : callsToReplace) {
    if (ptxVar) {
      call->replaceAllUsesWith(ptxVar);
    } else {
      call->replaceAllUsesWith(
          llvm::ConstantPointerNull::get(llvm::PointerType::get(M->getContext(), 0)));
    }
    call->dropAllReferences();
    call->eraseFromParent();
  }

  // Delete __codon_ptx__() stub
  if (auto *F = M->getFunction(ptxTarget)) {
    seqassertn(F->use_empty(), "some __codon_ptx__() calls not replaced in module");
    F->eraseFromParent();
  }
}
} // namespace

void applyGPUTransformations(llvm::Module *M, const std::string &ptxFilename) {
  llvm::LLVMContext &context = M->getContext();
  std::unique_ptr<llvm::Module> clone = llvm::CloneModule(*M);
  clone->setTargetTriple(llvm::Triple::normalize(GPU_TRIPLE));
  clone->setDataLayout(GPU_DL);

  if (isFastMathOn()) {
    clone->addModuleFlag(llvm::Module::ModFlagBehavior::Override, "nvvm-reflect-ftz",
                         1);
  }

  llvm::NamedMDNode *nvvmAnno = clone->getOrInsertNamedMetadata("nvvm.annotations");
  std::vector<llvm::GlobalValue *> kernels;

  for (auto &F : *clone) {
    if (!F.hasFnAttribute("kernel"))
      continue;

    llvm::Metadata *nvvmElem[] = {
        llvm::ConstantAsMetadata::get(&F),
        llvm::MDString::get(context, "kernel"),
        llvm::ConstantAsMetadata::get(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1)),
    };

    nvvmAnno->addOperand(llvm::MDNode::get(context, nvvmElem));
    kernels.push_back(&F);
  }

  if (kernels.empty()) {
    patchPTXVar(M, nullptr);
    return;
  }

  auto ptx = moduleToPTX(clone.get(), kernels);
  cleanUpIntrinsics(M);

  if (ptxOutput.getNumOccurrences() > 0) {
    std::error_code err;
    llvm::ToolOutputFile out(ptxOutput, err, llvm::sys::fs::OF_Text);
    seqassertn(!err, "Could not open file: {}", err.message());
    llvm::raw_ostream &os = out.os();
    os << ptx;
    os.flush();
    out.keep();
  }

  // Add ptx code as a global var
  auto *ptxVar = new llvm::GlobalVariable(
      *M, llvm::ArrayType::get(llvm::Type::getInt8Ty(context), ptx.length() + 1),
      /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage,
      llvm::ConstantDataArray::getString(context, ptx), ".ptx");

  ptxVar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  patchPTXVar(M, ptxVar);
}

} // namespace ir
} // namespace codon
