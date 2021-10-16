//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT
/// \cond

#pragma once
#include "toml_print_to_stream.h"
#if TOML_PARSER && !TOML_EXCEPTIONS
#include "toml_parse_result.h"
#endif

TOML_PUSH_WARNINGS;
TOML_DISABLE_SWITCH_WARNINGS;

TOML_IMPL_NAMESPACE_START {
  template <typename Char = char> class TOML_API formatter {
  private:
    const toml::node *source_;
    std::basic_ostream<Char> *stream_ = {};
    format_flags flags_;
    int indent_;
    bool naked_newline_;
#if TOML_PARSER && !TOML_EXCEPTIONS
    const parse_result *result_ = {};
#endif

  protected:
    [[nodiscard]] const toml::node &source() const noexcept { return *source_; }
    [[nodiscard]] std::basic_ostream<Char> &stream() const noexcept { return *stream_; }

    static constexpr size_t indent_columns = 4;
    static constexpr std::string_view indent_string = "    "sv;
    [[nodiscard]] int indent() const noexcept { return indent_; }
    void indent(int level) noexcept { indent_ = level; }
    void increase_indent() noexcept { indent_++; }
    void decrease_indent() noexcept { indent_--; }

    [[nodiscard]] bool quote_dates_and_times() const noexcept {
      return (flags_ & format_flags::quote_dates_and_times) != format_flags::none;
    }

    [[nodiscard]] bool literal_strings_allowed() const noexcept {
      return (flags_ & format_flags::allow_literal_strings) != format_flags::none;
    }

    [[nodiscard]] bool multi_line_strings_allowed() const noexcept {
      return (flags_ & format_flags::allow_multi_line_strings) != format_flags::none;
    }

    [[nodiscard]] bool value_format_flags_allowed() const noexcept {
      return (flags_ & format_flags::allow_value_format_flags) != format_flags::none;
    }

    [[nodiscard]] bool naked_newline() const noexcept { return naked_newline_; }

    void clear_naked_newline() noexcept { naked_newline_ = false; }

    void attach(std::basic_ostream<Char> &stream) noexcept {
      indent_ = {};
      naked_newline_ = true;
      stream_ = &stream;
    }

    void detach() noexcept { stream_ = nullptr; }

    void print_newline(bool force = false) {
      if (!naked_newline_ || force) {
        print_to_stream('\n', *stream_);
        naked_newline_ = true;
      }
    }

    void print_indent() {
      for (int i = 0; i < indent_; i++) {
        print_to_stream(indent_string, *stream_);
        naked_newline_ = false;
      }
    }

    void print_quoted_string(std::string_view str, bool allow_multi_line = true) {
      auto literals = literal_strings_allowed();
      if (str.empty()) {
        print_to_stream(literals ? "''"sv : "\"\""sv, *stream_);
        clear_naked_newline();
        return;
      }

      auto multi_line = allow_multi_line && multi_line_strings_allowed();
      if (multi_line || literals) {
        utf8_decoder decoder;
        bool has_line_breaks = false;
        bool has_control_chars = false;
        bool has_single_quotes = false;
        for (size_t i = 0; i < str.length() &&
                           !(has_line_breaks && has_control_chars && has_single_quotes);
             i++) {
          decoder(static_cast<uint8_t>(str[i]));
          if (decoder.error()) {
            has_line_breaks = false;
            has_control_chars = true; // force ""
            has_single_quotes = true;
            break;
          } else if (decoder.has_code_point()) {
            if (is_line_break(decoder.codepoint))
              has_line_breaks = true;
            else if (is_nontab_control_character(decoder.codepoint))
              has_control_chars = true;
            else if (decoder.codepoint == U'\'')
              has_single_quotes = true;
          }
        }
        multi_line = multi_line && has_line_breaks;
        literals =
            literals && !has_control_chars && !(!multi_line && has_single_quotes);
      }

      if (literals) {
        const auto quot = multi_line ? "'''"sv : "'"sv;
        print_to_stream(quot, *stream_);
        print_to_stream(str, *stream_);
        print_to_stream(quot, *stream_);
      } else {
        const auto quot = multi_line ? R"(""")"sv : R"(")"sv;
        print_to_stream(quot, *stream_);
        print_to_stream_with_escapes(str, *stream_);
        print_to_stream(quot, *stream_);
      }
      clear_naked_newline();
    }

    template <typename T> void print(const value<T> &val) {
      if constexpr (std::is_same_v<T, std::string>) {
        print_quoted_string(val.get());
      } else {
        if constexpr (is_one_of<T, date, time, date_time>) {
          if (quote_dates_and_times()) {
            const auto quot = literal_strings_allowed() ? '\'' : '"';
            print_to_stream(quot, *stream_);
            print_to_stream(*val, *stream_);
            print_to_stream(quot, *stream_);
          } else
            print_to_stream(*val, *stream_);
        } else if constexpr (is_one_of<T, int64_t /*, double*/>) {
          if (value_format_flags_allowed() && *val >= 0) {
            const auto fmt = val.flags() & value_flags::format_as_hexadecimal;
            if (fmt != value_flags::none) {
              switch (fmt) {
              case value_flags::format_as_binary:
                print_to_stream("0b"sv, *stream_);
                break;
              case value_flags::format_as_octal:
                print_to_stream("0o"sv, *stream_);
                break;
              case value_flags::format_as_hexadecimal:
                print_to_stream("0x"sv, *stream_);
                break;
                TOML_NO_DEFAULT_CASE;
              }
              print_to_stream(*val, *stream_, fmt);
            } else
              print_to_stream(*val, *stream_);
          } else
            print_to_stream(*val, *stream_);
        } else
          print_to_stream(*val, *stream_);

        naked_newline_ = false;
      }
    }

    void print_value(const node &val_node, node_type type) {
      TOML_ASSUME(type > node_type::array);
      switch (type) {
      case node_type::string:
        print(*reinterpret_cast<const value<std::string> *>(&val_node));
        break;
      case node_type::integer:
        print(*reinterpret_cast<const value<int64_t> *>(&val_node));
        break;
      case node_type::floating_point:
        print(*reinterpret_cast<const value<double> *>(&val_node));
        break;
      case node_type::boolean:
        print(*reinterpret_cast<const value<bool> *>(&val_node));
        break;
      case node_type::date:
        print(*reinterpret_cast<const value<date> *>(&val_node));
        break;
      case node_type::time:
        print(*reinterpret_cast<const value<time> *>(&val_node));
        break;
      case node_type::date_time:
        print(*reinterpret_cast<const value<date_time> *>(&val_node));
        break;
        TOML_NO_DEFAULT_CASE;
      }
    }

    [[nodiscard]] bool dump_failed_parse_result() {
#if TOML_PARSER && !TOML_EXCEPTIONS
      if (result_ && !(*result_)) {
        stream() << result_->error();
        return true;
      }
#endif

      return false;
    }

    formatter(const toml::node &source, format_flags flags) noexcept
        : source_{&source}, flags_{flags} {}

#if TOML_PARSER && !TOML_EXCEPTIONS

    formatter(const parse_result &result, format_flags flags) noexcept
        : source_{result ? &result.table() : nullptr}, flags_{flags}, result_{&result} {
    }

#endif
  };

#if !defined(DOXYGEN) && !TOML_HEADER_ONLY
  extern template class TOML_API formatter<char>;
#endif
}
TOML_IMPL_NAMESPACE_END;

TOML_POP_WARNINGS; // TOML_DISABLE_SWITCH_WARNINGS

/// \endcond
