// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "aarch64.h"

#include "llvm/TargetParser/AArch64TargetParser.h"

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
} // namespace

std::string Aarch64::getCPU(const llvm::Triple &triple) const {
  return llvm::sys::getHostCPUName().str();
}

std::string Aarch64::getFeatures(const llvm::Triple &triple) const {
  llvm::AArch64::ExtensionSet extensions;
  std::vector<llvm::StringRef> features;
  // Enable NEON by default.
  features.push_back("+neon");

  std::string cpu(llvm::sys::getHostCPUName());
  const std::optional<llvm::AArch64::CpuInfo> cpuInfo = llvm::AArch64::parseCpu(cpu);
  if (!cpuInfo)
    return "";

  extensions.addCPUDefaults(*cpuInfo);

  if (cpu == "cyclone" || llvm::StringRef(cpu).starts_with("apple")) {
    features.push_back("+zcm");
    features.push_back("+zcz");
  }

  auto *archInfo = &cpuInfo->Arch;
  features.push_back(archInfo->ArchFeature);
  // uint64_t extension = cpuInfo->getImpliedExtensions();
  // if (!llvm::AArch64::getExtensionFeatures(extension, features))
  //   return "";

  // Handle (arch-dependent) fp16fml/fullfp16 relationship.
  // FIXME: this fp16fml option handling will be reimplemented after the
  // TargetParser rewrite.
  const auto ItRNoFullFP16 = std::find(features.rbegin(), features.rend(), "-fullfp16");
  const auto ItRFP16FML = std::find(features.rbegin(), features.rend(), "+fp16fml");
  if (llvm::is_contained(features, "+v8.4a")) {
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

  // FIXME: this needs reimplementation too after the TargetParser rewrite
  //
  // Context sensitive meaning of Crypto:
  // 1) For Arch >= ARMv8.4a:  crypto = sm4 + sha3 + sha2 + aes
  // 2) For Arch <= ARMv8.3a:  crypto = sha2 + aes
  const auto ItBegin = features.begin();
  const auto ItEnd = features.end();
  const auto ItRBegin = features.rbegin();
  const auto ItREnd = features.rend();
  const auto ItRCrypto = std::find(ItRBegin, ItREnd, "+crypto");
  const auto ItRNoCrypto = std::find(ItRBegin, ItREnd, "-crypto");
  const auto HasCrypto = ItRCrypto != ItREnd;
  const auto HasNoCrypto = ItRNoCrypto != ItREnd;
  const ptrdiff_t PosCrypto = ItRCrypto - ItRBegin;
  const ptrdiff_t PosNoCrypto = ItRNoCrypto - ItRBegin;

  bool NoCrypto = false;
  if (HasCrypto && HasNoCrypto) {
    if (PosNoCrypto < PosCrypto)
      NoCrypto = true;
  }

  if (std::find(ItBegin, ItEnd, "+v8.4a") != ItEnd) {
    if (HasCrypto && !NoCrypto) {
      // Check if we have NOT disabled an algorithm with something like:
      //   +crypto, -algorithm
      // And if "-algorithm" does not occur, we enable that crypto algorithm.
      const bool HasSM4 = (std::find(ItBegin, ItEnd, "-sm4") == ItEnd);
      const bool HasSHA3 = (std::find(ItBegin, ItEnd, "-sha3") == ItEnd);
      const bool HasSHA2 = (std::find(ItBegin, ItEnd, "-sha2") == ItEnd);
      const bool HasAES = (std::find(ItBegin, ItEnd, "-aes") == ItEnd);
      if (HasSM4)
        features.push_back("+sm4");
      if (HasSHA3)
        features.push_back("+sha3");
      if (HasSHA2)
        features.push_back("+sha2");
      if (HasAES)
        features.push_back("+aes");
    } else if (HasNoCrypto) {
      // Check if we have NOT enabled a crypto algorithm with something like:
      //   -crypto, +algorithm
      // And if "+algorithm" does not occur, we disable that crypto algorithm.
      const bool HasSM4 = (std::find(ItBegin, ItEnd, "+sm4") != ItEnd);
      const bool HasSHA3 = (std::find(ItBegin, ItEnd, "+sha3") != ItEnd);
      const bool HasSHA2 = (std::find(ItBegin, ItEnd, "+sha2") != ItEnd);
      const bool HasAES = (std::find(ItBegin, ItEnd, "+aes") != ItEnd);
      if (!HasSM4)
        features.push_back("-sm4");
      if (!HasSHA3)
        features.push_back("-sha3");
      if (!HasSHA2)
        features.push_back("-sha2");
      if (!HasAES)
        features.push_back("-aes");
    }
  } else {
    if (HasCrypto && !NoCrypto) {
      const bool HasSHA2 = (std::find(ItBegin, ItEnd, "-sha2") == ItEnd);
      const bool HasAES = (std::find(ItBegin, ItEnd, "-aes") == ItEnd);
      if (HasSHA2)
        features.push_back("+sha2");
      if (HasAES)
        features.push_back("+aes");
    } else if (HasNoCrypto) {
      const bool HasSHA2 = (std::find(ItBegin, ItEnd, "+sha2") != ItEnd);
      const bool HasAES = (std::find(ItBegin, ItEnd, "+aes") != ItEnd);
      const bool HasV82a = (std::find(ItBegin, ItEnd, "+v8.2a") != ItEnd);
      const bool HasV83a = (std::find(ItBegin, ItEnd, "+v8.3a") != ItEnd);
      const bool HasV84a = (std::find(ItBegin, ItEnd, "+v8.4a") != ItEnd);
      if (!HasSHA2)
        features.push_back("-sha2");
      if (!HasAES)
        features.push_back("-aes");
      if (HasV82a || HasV83a || HasV84a) {
        features.push_back("-sm4");
        features.push_back("-sha3");
      }
    }
  }

  auto V8_6Pos = llvm::find(features, "+v8.6a");
  if (V8_6Pos != std::end(features))
    V8_6Pos = features.insert(std::next(V8_6Pos), {"+i8mm", "+bf16"});

  if (triple.isOSOpenBSD())
    features.push_back("+strict-align");

  return join(features);
}

} // namespace ir
} // namespace codon
