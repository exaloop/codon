#include "plugins.h"

#include <filesystem>

#include "codon/parser/common.h"
#include "codon/util/common.h"
#include "codon/util/semver/semver.h"
#include "codon/util/toml++/toml.h"

namespace codon {
namespace {
bool error(const std::string &msg, std::string *errMsg) {
  if (!msg.empty() && errMsg)
    *errMsg = msg;
  return false;
}

typedef std::unique_ptr<DSL> LoadFunc();
} // namespace

namespace fs = std::filesystem;

bool PluginManager::load(const std::string &path, std::string *errMsg) {
#if __APPLE__
  const std::string libExt = "dylib";
#else
  const std::string libExt = "so";
#endif

  fs::path tomlPath = fs::path(path) / "plugin.toml";
  toml::parse_result tml;
  try {
    tml = toml::parse_file(tomlPath.string());
  } catch (const toml::parse_error &e) {
    return error(
        fmt::format("[toml::parse_file(\"{}\")] {}", tomlPath.string(), e.what()),
        errMsg);
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
    return error(fmt::format("[semver::range::satisfies(..., \"{}\")] {}",
                             info.supported, e.what()),
                 errMsg);
  }
  if (!versionOk)
    return error(fmt::format("unsupported version {} (supported: {})", CODON_VERSION,
                             info.supported),
                 errMsg);

  if (!dylibPath.empty()) {
    std::string libLoadErrorMsg;
    auto handle = llvm::sys::DynamicLibrary::getPermanentLibrary(dylibPath.c_str(),
                                                                 &libLoadErrorMsg);
    if (!handle.isValid())
      return error(
          fmt::format(
              "[llvm::sys::DynamicLibrary::getPermanentLibrary(\"{}\", ...)] {}",
              dylibPath, libLoadErrorMsg),
          errMsg);

    auto *entry = (LoadFunc *)handle.getAddressOfSymbol("load");
    if (!entry)
      return error(
          fmt::format("could not find 'load' in plugin shared library: {}", dylibPath),
          errMsg);

    auto dsl = (*entry)();
    plugins.push_back(std::make_unique<Plugin>(std::move(dsl), info, handle));
  } else {
    plugins.push_back(std::make_unique<Plugin>(std::make_unique<DSL>(), info,
                                               llvm::sys::DynamicLibrary()));
  }
  return true;
}

} // namespace codon
