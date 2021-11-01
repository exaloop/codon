#pragma once

#include <nlohmann/json.hpp>
#include <xeus/xinterpreter.hpp>

using xeus::xinterpreter;
namespace nl = nlohmann;

namespace codon {
class CodonJupyter : public xinterpreter {
private:
  void configure_impl() override;

  nl::json execute_request_impl(int execution_counter, const std::string &code,
                                bool silent, bool store_history,
                                nl::json user_expressions, bool allow_stdin) override;

  nl::json complete_request_impl(const std::string &code, int cursor_pos) override;

  nl::json inspect_request_impl(const std::string &code, int cursor_pos,
                                int detail_level) override;

  nl::json is_complete_request_impl(const std::string &code) override;

  nl::json kernel_info_request_impl() override;

  void shutdown_request_impl() override;
};
} // namespace codon
