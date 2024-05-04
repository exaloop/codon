// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "jupyter.h"

#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <locale>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <xeus-zmq/xserver_zmq.hpp>
#include <xeus/xhelper.hpp>
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>

#include "codon/compiler/compiler.h"
#include "codon/compiler/error.h"
#include "codon/compiler/jit.h"
#include "codon/config/config.h"
#include "codon/parser/common.h"
#include "codon/util/common.h"

using std::move;
using std::string;

namespace nl = nlohmann;
namespace codon {

CodonJupyter::CodonJupyter(const std::string &argv0,
                           const std::vector<std::string> &plugins)
    : argv0(argv0), plugins(plugins) {}

nl::json CodonJupyter::execute_request_impl(int execution_counter, const string &code,
                                            bool silent, bool store_history,
                                            nl::json user_expressions,
                                            bool allow_stdin) {
  LOG("[codon-jupyter] execute_request_impl");
  auto result = jit->execute(code);
  string failed;
  llvm::handleAllErrors(
      result.takeError(),
      [&](const codon::error::ParserErrorInfo &e) {
        std::vector<string> backtrace;
        for (auto &msg : e)
          for (auto &s : msg)
            backtrace.push_back(s.getMessage());
        string err = backtrace[0];
        backtrace.erase(backtrace.begin());
        failed = fmt::format("Compile error: {}\nBacktrace:\n{}", err,
                             ast::join(backtrace, "  \n"));
      },
      [&](const codon::error::RuntimeErrorInfo &e) {
        auto backtrace = e.getBacktrace();
        failed = fmt::format("Runtime error: {}\nBacktrace:\n{}", e.getMessage(),
                             ast::join(backtrace, "  \n"));
      });
  if (failed.empty()) {
    std::string out = *result;
    nl::json pub_data;
    using std::string_literals::operator""s;
    std::string codonMimeMagic = "\x00\x00__codon/mime__\x00"s;
    if (ast::startswith(out, codonMimeMagic)) {
      std::string mime = "";
      int i = codonMimeMagic.size();
      for (; i < out.size() && out[i]; i++)
        mime += out[i];
      if (i < out.size() && !out[i]) {
        i += 1;
      } else {
        mime = "text/plain";
        i = 0;
      }
      pub_data[mime] = out.substr(i);
      LOG("> {}: {}", mime, out.substr(i));
    } else {
      pub_data["text/plain"] = out;
    }
    if (!out.empty())
      publish_execution_result(execution_counter, move(pub_data), nl::json::object());
    return nl::json{{"status", "ok"},
                    {"payload", nl::json::array()},
                    {"user_expressions", nl::json::object()}};
  } else {
    publish_stream("stderr", failed);
    return nl::json{{"status", "error"}};
  }
}

void CodonJupyter::configure_impl() {
  jit = std::make_unique<codon::jit::JIT>(argv0, "jupyter");
  jit->getCompiler()->getLLVMVisitor()->setCapture();

  for (const auto &plugin : plugins) {
    // TODO: error handling on plugin init
    bool failed = false;
    llvm::handleAllErrors(jit->getCompiler()->load(plugin),
                          [&failed](const codon::error::PluginErrorInfo &e) {
                            codon::compilationError(e.getMessage(), /*file=*/"",
                                                    /*line=*/0, /*col=*/0,
                                                    /*terminate=*/false);
                            failed = true;
                          });
  }
  llvm::cantFail(jit->init());
}

nl::json CodonJupyter::complete_request_impl(const string &code, int cursor_pos) {
  LOG("[codon-jupyter] complete_request_impl");
  return nl::json{{"status", "ok"}};
}

nl::json CodonJupyter::inspect_request_impl(const string &code, int cursor_pos,
                                            int detail_level) {
  LOG("[codon-jupyter] inspect_request_impl");
  return nl::json{{"status", "ok"}};
}

nl::json CodonJupyter::is_complete_request_impl(const string &code) {
  LOG("[codon-jupyter] is_complete_request_impl");
  return nl::json{{"status", "complete"}};
}

nl::json CodonJupyter::kernel_info_request_impl() {
  LOG("[codon-jupyter] kernel_info_request_impl");
  return xeus::create_info_reply("", "codon_kernel", CODON_VERSION, "python", "3.7",
                                 "text/x-python", ".codon", "python", "", "",
                                 "Codon Kernel");
}

void CodonJupyter::shutdown_request_impl() {
  LOG("[codon-jupyter] shutdown_request_impl");
}

int startJupyterKernel(const std::string &argv0,
                       const std::vector<std::string> &plugins,
                       const std::string &configPath) {
  xeus::xconfiguration config = xeus::load_configuration(configPath);

  auto context = xeus::make_context<zmq::context_t>();

  LOG("[codon-jupyter] startJupyterKernel");
  auto interpreter = std::make_unique<CodonJupyter>(argv0, plugins);
  xeus::xkernel kernel(config, xeus::get_user_name(), move(context), move(interpreter),
                       xeus::make_xserver_zmq);
  kernel.start();

  return 0;
}

} // namespace codon
