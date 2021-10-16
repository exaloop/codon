//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT

#pragma once
//# {{
#include "toml_preprocessor.h"
#if !TOML_PARSER
#error This header cannot not be included when TOML_PARSER is disabled.
#endif
//# }}
TOML_DISABLE_WARNINGS;
#if TOML_EXCEPTIONS
#include <stdexcept>
#endif
TOML_ENABLE_WARNINGS;

TOML_PUSH_WARNINGS;
TOML_DISABLE_INIT_WARNINGS;

TOML_NAMESPACE_START {
  TOML_ABI_NAMESPACE_BOOL(TOML_EXCEPTIONS, ex, noex);

#if defined(DOXYGEN) || !TOML_EXCEPTIONS

  /// \brief	An error generated when parsing fails.
  ///
  /// \remarks This class inherits from std::runtime_error when exceptions are enabled.
  /// 		 The public interface is the same regardless of exception mode.
  class parse_error final {
  private:
    std::string description_;
    source_region source_;

  public:
    TOML_NODISCARD_CTOR
    parse_error(std::string &&desc, source_region &&src) noexcept
        : description_{std::move(desc)}, source_{std::move(src)} {}

    TOML_NODISCARD_CTOR
    parse_error(std::string &&desc, const source_region &src) noexcept
        : parse_error{std::move(desc), source_region{src}} {}

    TOML_NODISCARD_CTOR
    parse_error(std::string &&desc, const source_position &position,
                const source_path_ptr &path = {}) noexcept
        : parse_error{std::move(desc), source_region{position, position, path}} {}

    /// \brief	Returns a textual description of the error.
    /// \remark The backing string is guaranteed to be null-terminated.
    [[nodiscard]] std::string_view description() const noexcept { return description_; }

    /// \brief	Returns the region of the source document responsible for the error.
    [[nodiscard]] const source_region &source() const noexcept { return source_; }
  };

#else

  class parse_error final : public std::runtime_error {
  private:
    source_region source_;

  public:
    TOML_NODISCARD_CTOR
    TOML_ATTR(nonnull)
    parse_error(const char *desc, source_region &&src) noexcept
        : std::runtime_error{desc}, source_{std::move(src)} {}

    TOML_NODISCARD_CTOR
    TOML_ATTR(nonnull)
    parse_error(const char *desc, const source_region &src) noexcept
        : parse_error{desc, source_region{src}} {}

    TOML_NODISCARD_CTOR
    TOML_ATTR(nonnull)
    parse_error(const char *desc, const source_position &position,
                const source_path_ptr &path = {}) noexcept
        : parse_error{desc, source_region{position, position, path}} {}

    [[nodiscard]] std::string_view description() const noexcept {
      return std::string_view{what()};
    }

    [[nodiscard]] const source_region &source() const noexcept { return source_; }
  };

#endif

  TOML_ABI_NAMESPACE_END; // TOML_EXCEPTIONS

  /// \brief	Prints a parse_error to a stream.
  ///
  /// \detail \cpp
  /// try
  /// {
  /// 	auto tbl = toml::parse("enabled = trUe"sv);
  /// }
  /// catch (const toml::parse_error & err)
  /// {
  /// 	std::cerr << "Parsing failed:\n"sv << err << "\n";
  /// }
  /// \ecpp
  ///
  /// \out
  /// Parsing failed:
  /// Encountered unexpected character while parsing boolean; expected 'true', saw 'trU'
  ///		(error occurred at line 1, column 13)
  /// \eout
  ///
  /// \tparam Char The output stream's underlying character type. Must be 1 byte in
  /// size. \param 	lhs	The stream. \param 	rhs	The parse_error.
  ///
  /// \returns	The input stream.
  template <typename Char>
  inline std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &lhs,
                                              const parse_error &rhs) {
    lhs << rhs.description();
    lhs << "\n\t(error occurred at "sv;
    lhs << rhs.source();
    lhs << ")"sv;
    return lhs;
  }

#if !defined(DOXYGEN) && !TOML_HEADER_ONLY
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    const parse_error &);
#endif
}
TOML_NAMESPACE_END;

TOML_POP_WARNINGS; // TOML_DISABLE_INIT_WARNINGS
