// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "plugins.h"

#include <cstdlib>
#include <semver.hpp>
#include <toml++/toml.h>

#include "codon/parser/common.h"
#include "codon/util/common.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

namespace codon {
namespace {
llvm::Expected<Plugin *> pluginError(const std::string &msg) {
  return llvm::make_error<error::PluginErrorInfo>(msg);
}

typedef std::unique_ptr<DSL> LoadFunc();
} // namespace

llvm::Expected<Plugin *> PluginManager::load(const std::string &path) {
#if __APPLE__
  const std::string libExt = "dylib";
#else
  const std::string libExt = "so";
#endif

  const std::string config = "plugin.toml";

  llvm::SmallString<128> tomlPath(path);
  llvm::sys::path::append(tomlPath, config);
  if (!llvm::sys::fs::exists(tomlPath)) {
    // try default install path
    std::string s = ast::Filesystem::executable_path(argv0.c_str()).string();
    tomlPath = llvm::SmallString<128>(llvm::sys::path::parent_path(s));
    llvm::sys::path::append(tomlPath, "../lib/codon/plugins", path, config);
  }

  toml::parse_result tml;
  try {
    tml = toml::parse_file(tomlPath.str());
  } catch (const toml::parse_error &e) {
    return pluginError(
        fmt::format("[toml::parse_file(\"{}\")] {}", tomlPath.str(), e.what()));
  }
  auto about = tml["about"];
  auto library = tml["library"];

  auto libraryPath = [&](const std::string &name) -> std::string {
    if (name.empty())
      return {};
    llvm::SmallString<128> p = llvm::sys::path::parent_path(tomlPath);
    llvm::sys::path::append(p, name + "." + libExt);
    return std::string(p.str());
  };
  for (const char *key : {"compiler", "runtime"}) {
    if (library[key] && !library[key].is_string())
      return pluginError(fmt::format("library.{} must be a string", key));
  }
  auto compilerLib = library["compiler"].value<std::string>();
  auto runtimeLib = library["runtime"].value<std::string>();

  auto link = library["link"];
  if (!runtimeLib && link.is_boolean() && !link.value_or(true))
    runtimeLib = "";
  std::vector<std::string> linkArgs;
  if (auto arr = link.as_array()) {
    arr->for_each([&linkArgs](auto &&el) {
      std::string l = el.value_or("");
      if (!l.empty())
        linkArgs.push_back(l);
    });
  } else {
    std::string l = link.value_or("");
    if (!l.empty())
      linkArgs.push_back(l);
  }
  for (auto &l : linkArgs)
    l = fmt::format(fmt::runtime(l),
                    fmt::arg("root", llvm::sys::path::parent_path(tomlPath)));

  std::string codonLib = library["codon"].value_or("");
  std::string stdlibPath;
  if (!codonLib.empty()) {
    llvm::SmallString<128> p = llvm::sys::path::parent_path(tomlPath);
    llvm::sys::path::append(p, codonLib);
    stdlibPath = p.str();
  }

  DSL::Info info = {
      about["name"].value_or(""),
      about["description"].value_or(""),
      about["version"].value_or(""),
      about["url"].value_or(""),
      about["supported"].value_or(""),
      stdlibPath,
      libraryPath(library["cpp"].value_or("")),
      linkArgs,
      compilerLib ? std::make_optional(libraryPath(*compilerLib)) : std::nullopt,
      runtimeLib ? std::make_optional(libraryPath(*runtimeLib)) : std::nullopt};

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

  const auto &dylibPath = info.getCompilerDylibPath();
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

llvm::Error PluginManager::loadRuntimeLibraries() const {
  for (const auto &plugin : plugins) {
    const auto &runtimePath = plugin->info.getRuntimeDylibPath();
    if (runtimePath.empty() || runtimePath == plugin->info.getCompilerDylibPath() ||
        loadedRuntimeLibraries.count(runtimePath))
      continue;
    std::string message;
    if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(runtimePath.c_str(),
                                                          &message))
      return llvm::make_error<error::PluginErrorInfo>(
          fmt::format("could not load runtime library '{}': {}", runtimePath, message));
    loadedRuntimeLibraries.insert(runtimePath);
  }
  return llvm::Error::success();
}

} // namespace codon
