#include "plugins.h"

#include <cstdlib>
#include <experimental/filesystem>

#include "codon/parser/common.h"
#include "codon/util/common.h"
#include "codon/util/semver/semver.h"
#include "codon/util/toml++/toml.h"

namespace codon {
namespace {
llvm::Expected<Plugin *> pluginError(const std::string &msg) {
  return llvm::make_error<error::PluginErrorInfo>(msg);
}

typedef std::unique_ptr<DSL> LoadFunc();
} // namespace

namespace fs = std::experimental::filesystem;

llvm::Expected<Plugin *> PluginManager::load(const std::string &path) {
#if __APPLE__
  const std::string libExt = "dylib";
#else
  const std::string libExt = "so";
#endif

  const std::string config = "plugin.toml";
  fs::path tomlPath = fs::path(path) / config;
  if (!fs::exists(tomlPath)) {
    // try default install path
    if (auto *homeDir = std::getenv("HOME"))
      tomlPath = fs::path(homeDir) / ".codon/plugins" / path / config;
  }

  toml::parse_result tml;
  try {
    tml = toml::parse_file(tomlPath.string());
  } catch (const toml::parse_error &e) {
    return pluginError(
        fmt::format("[toml::parse_file(\"{}\")] {}", tomlPath.string(), e.what()));
  }
  auto about = tml["about"];
  auto library = tml["library"];

  std::string cppLib = library["cpp"].value_or("");
  std::string dylibPath;
  if (!cppLib.empty())
    dylibPath = fs::path(tomlPath)
                    .replace_filename(library["cpp"].value_or("lib"))
                    .replace_extension(libExt)
                    .string();

  std::string codonLib = library["codon"].value_or("");
  std::string stdlibPath;
  if (!codonLib.empty())
    stdlibPath = fs::path(tomlPath).replace_filename(codonLib).string();

  DSL::Info info = {about["name"].value_or(""),      about["description"].value_or(""),
                    about["version"].value_or(""),   about["url"].value_or(""),
                    about["supported"].value_or(""), stdlibPath};

  bool versionOk = false;
  try {
    versionOk = semver::range::satisfies(
        semver::version(CODON_VERSION_MAJOR, CODON_VERSION_MINOR, CODON_VERSION_PATCH),
        info.supported);
  } catch (const std::invalid_argument &e) {
    return pluginError(fmt::format("[semver::range::satisfies(..., \"{}\")] {}",
                                   info.supported, e.what()));
  }
  if (!versionOk)
    return pluginError(fmt::format("unsupported version {} (supported: {})",
                                   CODON_VERSION, info.supported));

  if (!dylibPath.empty()) {
    std::string libLoadErrorMsg;
    auto handle = llvm::sys::DynamicLibrary::getPermanentLibrary(dylibPath.c_str(),
                                                                 &libLoadErrorMsg);
    if (!handle.isValid())
      return pluginError(fmt::format(
          "[llvm::sys::DynamicLibrary::getPermanentLibrary(\"{}\", ...)] {}", dylibPath,
          libLoadErrorMsg));

    auto *entry = (LoadFunc *)handle.getAddressOfSymbol("load");
    if (!entry)
      return pluginError(
          fmt::format("could not find 'load' in plugin shared library: {}", dylibPath));

    auto dsl = (*entry)();
    plugins.push_back(std::make_unique<Plugin>(std::move(dsl), info, handle));
  } else {
    plugins.push_back(std::make_unique<Plugin>(std::make_unique<DSL>(), info,
                                               llvm::sys::DynamicLibrary()));
  }
  return plugins.back().get();
}

} // namespace codon
