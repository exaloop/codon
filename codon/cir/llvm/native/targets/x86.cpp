// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "x86.h"

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

std::string X86::getCPU(const llvm::Triple &triple) const {
  auto CPU = llvm::sys::getHostCPUName();
  if (!CPU.empty() && CPU != "generic")
    return std::string(CPU);

  // Select the default CPU if none was given (or detection failed).

  if (!triple.isX86())
    return ""; // This routine is only handling x86 targets.

  bool is64Bit = triple.getArch() == llvm::Triple::x86_64;

  // FIXME: Need target hooks.
  if (triple.isOSDarwin()) {
    if (triple.getArchName() == "x86_64h")
      return "core-avx2";
    // macosx10.12 drops support for all pre-Penryn Macs.
    // Simulators can still run on 10.11 though, like Xcode.
    if (triple.isMacOSX() && !triple.isOSVersionLT(10, 12))
      return "penryn";

    if (triple.isDriverKit())
      return "nehalem";

    // The oldest x86_64 Macs have core2/Merom; the oldest x86 Macs have Yonah.
    return is64Bit ? "core2" : "yonah";
  }

  // Set up default CPU name for PS4/PS5 compilers.
  if (triple.isPS4())
    return "btver2";
  if (triple.isPS5())
    return "znver2";

  // On Android use targets compatible with gcc
  if (triple.isAndroid())
    return is64Bit ? "x86-64" : "i686";

  // Everything else goes to x86-64 in 64-bit mode.
  if (is64Bit)
    return "x86-64";

  switch (triple.getOS()) {
  case llvm::Triple::NetBSD:
    return "i486";
  case llvm::Triple::Haiku:
  case llvm::Triple::OpenBSD:
    return "i586";
  case llvm::Triple::FreeBSD:
    return "i686";
  default:
    // Fallback to p4.
    return "pentium4";
  }
}

std::string X86::getFeatures(const llvm::Triple &triple) const {
  std::vector<std::string> features;
  llvm::StringMap<bool> hostFeatures;
  if (llvm::sys::getHostCPUFeatures(hostFeatures)) {
    for (auto &f : hostFeatures) {
      features.push_back((f.second ? "+" : "-") + f.first().str());
    }
  }

  if (triple.getArchName() == "x86_64h") {
    // x86_64h implies quite a few of the more modern subtarget features
    // for Haswell class CPUs, but not all of them. Opt-out of a few.
    features.push_back("-rdrnd");
    features.push_back("-aes");
    features.push_back("-pclmul");
    features.push_back("-rtm");
    features.push_back("-fsgsbase");
  }

  const llvm::Triple::ArchType ArchType = triple.getArch();
  // Add features to be compatible with gcc for Android.
  if (triple.isAndroid()) {
    if (ArchType == llvm::Triple::x86_64) {
      features.push_back("+sse4.2");
      features.push_back("+popcnt");
      features.push_back("+cx16");
    } else
      features.push_back("+ssse3");
  }
  return join(features);
}

} // namespace ir
} // namespace codon
