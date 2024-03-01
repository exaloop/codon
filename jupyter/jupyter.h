// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once
#include <codon/compiler/jit.h>
#include <nlohmann/json.hpp>
#include <xeus/xinterpreter.hpp>

using xeus::xinterpreter;
namespace nl = nlohmann;

namespace codon {
class CodonJupyter : public xinterpreter {
  std::unique_ptr<codon::jit::JIT> jit;
  std::string argv0;
  std::vector<std::string> plugins;

public:
  CodonJupyter(const std::string &argv0, const std::vector<std::string> &plugins);

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

int startJupyterKernel(const std::string &argv0,
                       const std::vector<std::string> &plugins,
                       const std::string &configPath);

} // namespace codon
