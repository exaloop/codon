#include "codon.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <xeus/xhelper.hpp>

using std::move;
using std::string;

namespace nl = nlohmann;

namespace codon {

nl::json CodonJupyter::execute_request_impl(int execution_counter, const string &code,
                                            bool silent, bool store_history,
                                            nl::json user_expressions,
                                            bool allow_stdin) {
  nl::json pub_data;
  pub_data["text/plain"] = "Hello World !!";
  publish_execution_result(execution_counter, move(pub_data), nl::json::object());
  //   publish_execution_error("TypeError", "123", {"!@#$", "*(*"});
  return xeus::create_successful_reply();
}

void CodonJupyter::configure_impl() {}

nl::json CodonJupyter::complete_request_impl(const string &code, int cursor_pos) {
  return xeus::create_complete_reply({}, cursor_pos, cursor_pos);
}

nl::json CodonJupyter::inspect_request_impl(const string &code, int cursor_pos,
                                            int detail_level) {
  return xeus::create_inspect_reply();
}

nl::json CodonJupyter::is_complete_request_impl(const string &code) {
  return xeus::create_is_complete_reply("complete");
}

nl::json CodonJupyter::kernel_info_request_impl() {
  return xeus::create_info_reply("", "codon_kernel", "0.1.0", "python", "3.7",
                                 "text/x-python", ".seq");
}

void CodonJupyter::shutdown_request_impl() {}

} // namespace codon