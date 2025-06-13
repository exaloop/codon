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

  std::string cpu(llvm::sys::getHostCPUName());
  const std::optional<llvm::AArch64::CpuInfo> cpuInfo = llvm::AArch64::parseCpu(cpu);
  if (!cpuInfo)
    return "";

  auto *archInfo = llvm::AArch64::getArchForCpu(cpu);
  extensions.addCPUDefaults(*cpuInfo);
  extensions.addArchDefaults(*archInfo);
  extensions.toLLVMFeatureList(features);

  for (auto &f : llvm::sys::getHostCPUFeatures()) {
    features.push_back((f.second ? "+" : "-") + f.first().str());
  }

  if (cpu == "cyclone" || llvm::StringRef(cpu).starts_with("apple")) {
    features.push_back("+zcm");
    features.push_back("+zcz");
  }

  if (triple.isAndroid() || triple.isOHOSFamily()) {
    // Enabled A53 errata (835769) workaround by default on android
    features.push_back("+fix-cortex-a53-835769");
  } else if (triple.isOSFuchsia()) {
    if (cpu.empty() || cpu == "generic" || cpu == "cortex-a53")
      features.push_back("+fix-cortex-a53-835769");
  }

  if (triple.isOSOpenBSD())
    features.push_back("+strict-align");

  return join(features);
}

} // namespace ir
} // namespace codon
