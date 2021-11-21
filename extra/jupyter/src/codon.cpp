#include "codon.h"

#ifdef CODON_JUPYTER
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <xeus/xhelper.hpp>
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <xeus/xserver_zmq.hpp>

#include "codon/compiler/compiler.h"
#include "codon/compiler/error.h"
#include "codon/compiler/jit.h"
#include "codon/parser/common.h"
#include "codon/util/common.h"

using std::move;
using std::string;

namespace nl = nlohmann;
namespace codon {

CodonJupyter::CodonJupyter(const std::string &argv0) : argv0(argv0) {}

nl::json CodonJupyter::execute_request_impl(int execution_counter, const string &code,
                                            bool silent, bool store_history,
                                            nl::json user_expressions,
                                            bool allow_stdin) {
  auto result = jit->exec(code);
  string failed;
  llvm::handleAllErrors(
      result.takeError(),
      [&](const codon::error::ParserErrorInfo &e) {
        std::vector<string> backtrace;
        for (auto &msg : e)
          backtrace.push_back(msg.getMessage());
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
    nl::json pub_data;
    pub_data["text/plain"] = *result;
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
  jit = std::make_unique<codon::jit::JIT>(argv0);
  llvm::cantFail(jit->init());
}

nl::json CodonJupyter::complete_request_impl(const string &code, int cursor_pos) {
  return nl::json{{"status", "ok"}};
}

nl::json CodonJupyter::inspect_request_impl(const string &code, int cursor_pos,
                                            int detail_level) {
  return nl::json{{"status", "ok"}};
}

nl::json CodonJupyter::is_complete_request_impl(const string &code) {
  return nl::json{{"status", "complete"}};
}

nl::json CodonJupyter::kernel_info_request_impl() {
  return xeus::create_info_reply("1.0", "codon_kernel", "0.1.0", "python", "3.7",
                                 "text/x-python", ".seq", "python", "", "",
                                 "Codon Kernel");
}

void CodonJupyter::shutdown_request_impl() {}

int startJupyterKernel(const std::string &argv0, const std::string &configPath) {
  xeus::xconfiguration config = xeus::load_configuration(configPath);

  auto context = xeus::make_context<zmq::context_t>();
  auto interpreter = std::make_unique<CodonJupyter>(argv0);
  xeus::xkernel kernel(config, xeus::get_user_name(), move(context), move(interpreter),
                       xeus::make_xserver_zmq);
  kernel.start();

  return 0;
}

} // namespace codon
#endif