#include "plugins.h"

#include <filesystem>

#include "codon/parser/common.h"
#include "codon/util/common.h"
#include "codon/util/semver/semver.h"
#include "codon/util/toml++/toml.h"

namespace codon {
namespace {
bool isVersionSupported(const std::string &versionRange, int major, int minor,
                        int patch) {
  try {
    return semver::range::satisfies(semver::version(major, minor, patch), versionRange);
  } catch (const std::invalid_argument &) {
    return false;
  }
}
} // namespace

typedef std::unique_ptr<DSL> LoadFunc();
namespace fs = std::filesystem;

PluginManager::Error PluginManager::load(const std::string &path) {
#if __APPLE__
  const std::string libExt = "dylib";
#else
  const std::string libExt = "so";
#endif

  fs::path tomlPath = path;
  auto tml = toml::parse_file(path);
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

  if (!isVersionSupported(info.supported, CODON_VERSION_MAJOR, CODON_VERSION_MINOR,
                          CODON_VERSION_PATCH))
    return Error::UNSUPPORTED_VERSION;

  if (!dylibPath.empty()) {
    std::string errMsg;
    auto handle =
        llvm::sys::DynamicLibrary::getPermanentLibrary(dylibPath.c_str(), &errMsg);
    if (!handle.isValid())
      return Error::NOT_FOUND;

    auto *entry = (LoadFunc *)handle.getAddressOfSymbol("load");
    if (!entry)
      return Error::NO_ENTRYPOINT;

    auto dsl = (*entry)();
    plugins.push_back(std::make_unique<Plugin>(std::move(dsl), info, handle));
    return load(plugins.back()->dsl.get());
  } else {
    plugins.push_back(std::make_unique<Plugin>(std::make_unique<DSL>(), info,
                                               llvm::sys::DynamicLibrary()));
    return Error::NONE;
  }
}

PluginManager::Error PluginManager::load(DSL *dsl) {
  dsl->addIRPasses(pm, debug);
  return Error::NONE;
}

} // namespace codon
