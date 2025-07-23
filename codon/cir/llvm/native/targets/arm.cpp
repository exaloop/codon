// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "arm.h"

#include "llvm/TargetParser/ARMTargetParser.h"

namespace codon {
namespace ir {
namespace {
template <typename T> std::string join(const T &v, const std::string &delim = ",") {
  std::ostringstream s;
  for (const auto &i : v) {
    if (&i != &v[0])
      s << delim;
    s << std::string(i);
  }
  return s.str();
}

int getARMSubArchVersionNumber(const llvm::Triple &triple) {
  auto arch = triple.getArchName();
  return llvm::ARM::parseArchVersion(arch);
}

bool isARMMProfile(const llvm::Triple &triple) {
  auto arch = triple.getArchName();
  return llvm::ARM::parseArchProfile(arch) == llvm::ARM::ProfileKind::M;
}

bool isARMBigEndian(const llvm::Triple &triple) {
  return triple.getArch() == llvm::Triple::armeb ||
         triple.getArch() == llvm::Triple::thumbeb;
}

bool isARMAProfile(const llvm::Triple &triple) {
  auto arch = triple.getArchName();
  return llvm::ARM::parseArchProfile(arch) == llvm::ARM::ProfileKind::A;
}

bool isARMEABIBareMetal(const llvm::Triple &triple) {
  auto arch = triple.getArch();
  if (arch != llvm::Triple::arm && arch != llvm::Triple::thumb &&
      arch != llvm::Triple::armeb && arch != llvm::Triple::thumbeb)
    return false;

  if (triple.getVendor() != llvm::Triple::UnknownVendor)
    return false;

  if (triple.getOS() != llvm::Triple::UnknownOS)
    return false;

  if (triple.getEnvironment() != llvm::Triple::EABI &&
      triple.getEnvironment() != llvm::Triple::EABIHF)
    return false;

  return true;
}

bool useAAPCSForMachO(const llvm::Triple &triple) {
  return triple.getEnvironment() == llvm::Triple::EABI ||
         triple.getEnvironment() == llvm::Triple::EABIHF ||
         triple.getOS() == llvm::Triple::UnknownOS || isARMMProfile(triple);
}

enum FloatABI { Invalid, Hard, Soft, SoftFP };

FloatABI getDefaultFloatABI(const llvm::Triple &triple) {
  auto sub = getARMSubArchVersionNumber(triple);
  switch (triple.getOS()) {
  case llvm::Triple::Darwin:
  case llvm::Triple::MacOSX:
  case llvm::Triple::IOS:
  case llvm::Triple::TvOS:
  case llvm::Triple::DriverKit:
  case llvm::Triple::XROS:
    // Darwin defaults to "softfp" for v6 and v7.
    if (triple.isWatchABI())
      return FloatABI::Hard;
    else
      return (sub == 6 || sub == 7) ? FloatABI::SoftFP : FloatABI::Soft;

  case llvm::Triple::WatchOS:
    return FloatABI::Hard;

  // FIXME: this is invalid for WindowsCE
  case llvm::Triple::Win32:
    // It is incorrect to select hard float ABI on MachO platforms if the ABI is
    // "apcs-gnu".
    if (triple.isOSBinFormatMachO() && !useAAPCSForMachO(triple))
      return FloatABI::Soft;
    return FloatABI::Hard;

  case llvm::Triple::NetBSD:
    switch (triple.getEnvironment()) {
    case llvm::Triple::EABIHF:
    case llvm::Triple::GNUEABIHF:
      return FloatABI::Hard;
    default:
      return FloatABI::Soft;
    }
    break;

  case llvm::Triple::FreeBSD:
    switch (triple.getEnvironment()) {
    case llvm::Triple::GNUEABIHF:
      return FloatABI::Hard;
    default:
      // FreeBSD defaults to soft float
      return FloatABI::Soft;
    }
    break;

  case llvm::Triple::Haiku:
  case llvm::Triple::OpenBSD:
    return FloatABI::SoftFP;

  default:
    if (triple.isOHOSFamily())
      return FloatABI::Soft;
    switch (triple.getEnvironment()) {
    case llvm::Triple::GNUEABIHF:
    case llvm::Triple::GNUEABIHFT64:
    case llvm::Triple::MuslEABIHF:
    case llvm::Triple::EABIHF:
      return FloatABI::Hard;
    case llvm::Triple::Android:
    case llvm::Triple::GNUEABI:
    case llvm::Triple::GNUEABIT64:
    case llvm::Triple::MuslEABI:
    case llvm::Triple::EABI:
      // EABI is always AAPCS, and if it was not marked 'hard', it's softfp
      return FloatABI::SoftFP;
    default:
      return FloatABI::Invalid;
    }
  }
  return FloatABI::Invalid;
}

FloatABI getARMFloatABI(const llvm::Triple &triple) {
  FloatABI abi = getDefaultFloatABI(triple);
  if (abi == FloatABI::Invalid) {
    // Assume "soft", but warn the user we are guessing.
    if (triple.isOSBinFormatMachO() &&
        triple.getSubArch() == llvm::Triple::ARMSubArch_v7em)
      abi = FloatABI::Hard;
    else
      abi = FloatABI::Soft;
  }
  return abi;
}

llvm::ARM::ArchKind getLLVMArchKindForARM(llvm::StringRef cpu, llvm::StringRef arch,
                                          const llvm::Triple &triple) {
  return (arch == "armv7k" || arch == "thumbv7k") ? llvm::ARM::ArchKind::ARMV7K
                                                  : llvm::ARM::parseCPUArch(cpu);
}

llvm::StringRef getLLVMArchSuffixForARM(llvm::StringRef cpu, llvm::StringRef arch,
                                        const llvm::Triple &triple) {
  llvm::ARM::ArchKind archKind = getLLVMArchKindForARM(cpu, arch, triple);
  if (archKind == llvm::ARM::ArchKind::INVALID)
    return "";
  return llvm::ARM::getSubArch(archKind);
}

bool hasIntegerMVE(const std::vector<llvm::StringRef> &F) {
  auto MVE = llvm::find(llvm::reverse(F), "+mve");
  auto NoMVE = llvm::find(llvm::reverse(F), "-mve");
  return MVE != F.rend() && (NoMVE == F.rend() || std::distance(MVE, NoMVE) > 0);
}
} // namespace

std::string ARM::getCPU(const llvm::Triple &triple) const {
  return llvm::sys::getHostCPUName().str();
}

std::string ARM::getFeatures(const llvm::Triple &triple) const {
  std::vector<llvm::StringRef> features;

  auto abi = getARMFloatABI(triple);
  // uint64_t HWDivID = llvm::ARM::parseHWDiv(HWDiv);

  // Use software floating point operations?
  if (abi == FloatABI::Soft)
    features.push_back("+soft-float");

  // Use software floating point argument passing?
  if (abi != FloatABI::Hard)
    features.push_back("+soft-float-abi");

  std::vector<std::string> tmp; // make sure we don't delete string data
  for (auto &f : llvm::sys::getHostCPUFeatures()) {
    tmp.push_back((f.second ? "+" : "-") + f.first().str());
    features.push_back(tmp.back());
  }

  llvm::ARM::FPUKind fpu = llvm::ARM::FK_INVALID;
  auto arch = triple.getArchName();

  // Honor -mfpu=. ClangAs gives preference to -Wa,-mfpu=.
  if (triple.isAndroid() && getARMSubArchVersionNumber(triple) == 7) {
    const char *androidFPU = "neon";
    fpu = llvm::ARM::parseFPU(androidFPU);
  } else {
    std::string cpu = getCPU(triple);
    llvm::ARM::ArchKind archkind = getLLVMArchKindForARM(cpu, arch, triple);
    fpu = llvm::ARM::getDefaultFPU(cpu, archkind);
  }
  (void)llvm::ARM::getFPUFeatures(fpu, features);

  // Handle (arch-dependent) fp16fml/fullfp16 relationship.
  // Must happen before any features are disabled due to soft-float.
  // FIXME: this fp16fml option handling will be reimplemented after the
  // TargetParser rewrite.
  const auto ItRNoFullFP16 = std::find(features.rbegin(), features.rend(), "-fullfp16");
  const auto ItRFP16FML = std::find(features.rbegin(), features.rend(), "+fp16fml");
  if (triple.getSubArch() == llvm::Triple::SubArchType::ARMSubArch_v8_4a) {
    const auto ItRFullFP16 = std::find(features.rbegin(), features.rend(), "+fullfp16");
    if (ItRFullFP16 < ItRNoFullFP16 && ItRFullFP16 < ItRFP16FML) {
      // Only entangled feature that can be to the right of this +fullfp16 is -fp16fml.
      // Only append the +fp16fml if there is no -fp16fml after the +fullfp16.
      if (std::find(features.rbegin(), ItRFullFP16, "-fp16fml") == ItRFullFP16)
        features.push_back("+fp16fml");
    } else
      goto fp16_fml_fallthrough;
  } else {
  fp16_fml_fallthrough:
    // In both of these cases, putting the 'other' feature on the end of the vector will
    // result in the same effect as placing it immediately after the current feature.
    if (ItRNoFullFP16 < ItRFP16FML)
      features.push_back("-fp16fml");
    else if (ItRNoFullFP16 > ItRFP16FML)
      features.push_back("+fullfp16");
  }

  // Setting -msoft-float/-mfloat-abi=soft, -mfpu=none, or adding +nofp to
  // -march/-mcpu effectively disables the FPU (GCC ignores the -mfpu options in
  // this case). Note that the ABI can also be set implicitly by the target
  // selected.
  bool HasFPRegs = true;
  if (abi == FloatABI::Soft) {
    llvm::ARM::getFPUFeatures(llvm::ARM::FK_NONE, features);

    // Disable all features relating to hardware FP, not already disabled by the
    // above call.
    features.insert(features.end(),
                    {"-dotprod", "-fp16fml", "-bf16", "-mve", "-mve.fp"});
    HasFPRegs = false;
    fpu = llvm::ARM::FK_NONE;
  } else if (fpu == llvm::ARM::FK_NONE) {
    // -mfpu=none, -march=armvX+nofp or -mcpu=X+nofp is *very* similar to
    // -mfloat-abi=soft, only that it should not disable MVE-I. They disable the
    // FPU, but not the FPU registers, thus MVE-I, which depends only on the
    // latter, is still supported.
    features.insert(features.end(), {"-dotprod", "-fp16fml", "-bf16", "-mve.fp"});
    HasFPRegs = hasIntegerMVE(features);
  }
  if (!HasFPRegs)
    features.emplace_back("-fpregs");

  // Invalid value of the __ARM_FEATURE_MVE macro when an explicit -mfpu= option
  // disables MVE-FP -mfpu=fpv5-d16 or -mfpu=fpv5-sp-d16 disables the scalar
  // half-precision floating-point operations feature. Therefore, because the
  // M-profile Vector Extension (MVE) floating-point feature requires the scalar
  // half-precision floating-point operations, this option also disables the MVE
  // floating-point feature: -mve.fp
  if (fpu == llvm::ARM::FK_FPV5_D16 || fpu == llvm::ARM::FK_FPV5_SP_D16)
    features.push_back("-mve.fp");

  // For Arch >= ARMv8.0 && A or R profile:  crypto = sha2 + aes
  // Rather than replace within the feature vector, determine whether each
  // algorithm is enabled and append this to the end of the vector.
  // The algorithms can be controlled by their specific feature or the crypto
  // feature, so their status can be determined by the last occurance of
  // either in the vector. This allows one to supercede the other.
  // e.g. +crypto+noaes in -march/-mcpu should enable sha2, but not aes
  // FIXME: this needs reimplementation after the TargetParser rewrite
  bool HasSHA2 = false;
  bool HasAES = false;
  const auto ItCrypto =
      llvm::find_if(llvm::reverse(features),
                    [](const llvm::StringRef F) { return F.contains("crypto"); });
  const auto ItSHA2 =
      llvm::find_if(llvm::reverse(features), [](const llvm::StringRef F) {
        return F.contains("crypto") || F.contains("sha2");
      });
  const auto ItAES =
      llvm::find_if(llvm::reverse(features), [](const llvm::StringRef F) {
        return F.contains("crypto") || F.contains("aes");
      });
  const bool FoundSHA2 = ItSHA2 != features.rend();
  const bool FoundAES = ItAES != features.rend();
  if (FoundSHA2)
    HasSHA2 = ItSHA2->take_front() == "+";
  if (FoundAES)
    HasAES = ItAES->take_front() == "+";
  if (ItCrypto != features.rend()) {
    if (HasSHA2 && HasAES)
      features.push_back("+crypto");
    else
      features.push_back("-crypto");
    if (HasSHA2)
      features.push_back("+sha2");
    else
      features.push_back("-sha2");
    if (HasAES)
      features.push_back("+aes");
    else
      features.push_back("-aes");
  }

  if (HasSHA2 || HasAES) {
    auto ArchSuffix = getLLVMArchSuffixForARM(getCPU(triple), arch, triple);
    llvm::ARM::ProfileKind ArchProfile = llvm::ARM::parseArchProfile(ArchSuffix);
    if (!((llvm::ARM::parseArchVersion(ArchSuffix) >= 8) &&
          (ArchProfile == llvm::ARM::ProfileKind::A ||
           ArchProfile == llvm::ARM::ProfileKind::R))) {
      features.push_back("-sha2");
      features.push_back("-aes");
    }
  }

  // Assume pre-ARMv6 doesn't support unaligned accesses.
  //
  // ARMv6 may or may not support unaligned accesses depending on the
  // SCTLR.U bit, which is architecture-specific. We assume ARMv6
  // Darwin and NetBSD targets support unaligned accesses, and others don't.
  //
  // ARMv7 always has SCTLR.U set to 1, but it has a new SCTLR.A bit which
  // raises an alignment fault on unaligned accesses. Assume ARMv7+ supports
  // unaligned accesses, except ARMv6-M, and ARMv8-M without the Main
  // Extension. This aligns with the default behavior of ARM's downstream
  // versions of GCC and Clang.
  //
  // Users can change the default behavior via -m[no-]unaliged-access.
  int versionNum = getARMSubArchVersionNumber(triple);
  if (triple.isOSDarwin() || triple.isOSNetBSD()) {
    if (versionNum < 6 ||
        triple.getSubArch() == llvm::Triple::SubArchType::ARMSubArch_v6m)
      features.push_back("+strict-align");
  } else if (triple.getVendor() == llvm::Triple::Apple && triple.isOSBinFormatMachO()) {
    // Firmwares on Apple platforms are strict-align by default.
    features.push_back("+strict-align");
  } else if (versionNum < 7 ||
             triple.getSubArch() == llvm::Triple::SubArchType::ARMSubArch_v6m ||
             triple.getSubArch() ==
                 llvm::Triple::SubArchType::ARMSubArch_v8m_baseline) {
    features.push_back("+strict-align");
  }

  return join(features);
}

} // namespace ir
} // namespace codon
