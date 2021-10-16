//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT

#pragma once
#include "toml_array.h"
#include "toml_formatter.h"
#include "toml_table.h"

TOML_PUSH_WARNINGS;
TOML_DISABLE_SWITCH_WARNINGS;

TOML_NAMESPACE_START {
  /// \brief	A wrapper for printing TOML objects out to a stream as formatted JSON.
  ///
  /// \detail \cpp
  /// auto some_toml = toml::parse(R"(
  ///		[fruit]
  ///		apple.color = "red"
  ///		apple.taste.sweet = true
  ///
  ///		[fruit.apple.texture]
  ///		smooth = true
  /// )"sv);
  ///	std::cout << toml::json_formatter{ some_toml } << "\n";
  ///
  /// \ecpp
  ///
  /// \out
  /// {
  ///     "fruit" : {
  ///         "apple" : {
  ///             "color" : "red",
  ///             "taste" : {
  ///                 "sweet" : true
  ///             },
  ///             "texture" : {
  ///                 "smooth" : true
  ///             }
  ///         }
  ///     }
  /// }
  /// \eout
  ///
  /// \tparam	Char	The underlying character type of the output stream. Must be 1
  /// byte in size.
  template <typename Char = char>
  class TOML_API json_formatter final : impl::formatter<Char> {
  private:
    /// \cond

    using base = impl::formatter<Char>;

    void print(const toml::table &tbl);

    void print(const array &arr) {
      if (arr.empty())
        impl::print_to_stream("[]"sv, base::stream());
      else {
        impl::print_to_stream('[', base::stream());
        base::increase_indent();
        for (size_t i = 0; i < arr.size(); i++) {
          if (i > 0_sz)
            impl::print_to_stream(',', base::stream());
          base::print_newline(true);
          base::print_indent();

          auto &v = arr[i];
          const auto type = v.type();
          TOML_ASSUME(type != node_type::none);
          switch (type) {
          case node_type::table:
            print(*reinterpret_cast<const table *>(&v));
            break;
          case node_type::array:
            print(*reinterpret_cast<const array *>(&v));
            break;
          default:
            base::print_value(v, type);
          }
        }
        base::decrease_indent();
        base::print_newline(true);
        base::print_indent();
        impl::print_to_stream(']', base::stream());
      }
      base::clear_naked_newline();
    }

    void print() {
      if (base::dump_failed_parse_result())
        return;

      switch (auto source_type = base::source().type()) {
      case node_type::table:
        print(*reinterpret_cast<const table *>(&base::source()));
        break;

      case node_type::array:
        print(*reinterpret_cast<const array *>(&base::source()));
        break;

      default:
        base::print_value(base::source(), source_type);
      }
    }

    /// \endcond

  public:
    /// \brief	The default flags for a json_formatter.
    static constexpr format_flags default_flags = format_flags::quote_dates_and_times;

    /// \brief	Constructs a JSON formatter and binds it to a TOML object.
    ///
    /// \param 	source	The source TOML object.
    /// \param 	flags 	Format option flags.
    TOML_NODISCARD_CTOR
    explicit json_formatter(const toml::node &source,
                            format_flags flags = default_flags) noexcept
        : base{source, flags} {}

#if defined(DOXYGEN) || (TOML_PARSER && !TOML_EXCEPTIONS)

    /// \brief	Constructs a JSON formatter and binds it to a toml::parse_result.
    ///
    /// \availability This constructor is only available when exceptions are disabled.
    ///
    /// \attention Formatting a failed parse result will simply dump the error message
    /// out as-is.
    ///		This will not be valid JSON, but at least gives you something to log or
    /// show up in diagnostics:
    /// \cpp
    /// std::cout << toml::json_formatter{ toml::parse("a = 'b'"sv) } // ok
    ///           << "\n\n"
    ///           << toml::json_formatter{ toml::parse("a = "sv) } // malformed
    ///           << "\n";
    /// \ecpp
    /// \out
    /// {
    ///     "a" : "b"
    /// }
    ///
    /// Error while parsing key-value pair: encountered end-of-file
    ///         (error occurred at line 1, column 5)
    /// \eout
    /// Use the library with exceptions if you want to avoid this scenario.
    ///
    /// \param 	result	The parse result.
    /// \param 	flags 	Format option flags.
    TOML_NODISCARD_CTOR
    explicit json_formatter(const toml::parse_result &result,
                            format_flags flags = default_flags) noexcept
        : base{result, flags} {}

#endif

    template <typename T, typename U>
    friend std::basic_ostream<T> &operator<<(std::basic_ostream<T> &,
                                             json_formatter<U> &);
    template <typename T, typename U>
    friend std::basic_ostream<T> &operator<<(std::basic_ostream<T> &,
                                             json_formatter<U> &&);
  };

#if !defined(DOXYGEN) && !TOML_HEADER_ONLY
  extern template class TOML_API json_formatter<char>;
#endif

  json_formatter(const table &)->json_formatter<char>;
  json_formatter(const array &)->json_formatter<char>;
  template <typename T> json_formatter(const value<T> &) -> json_formatter<char>;

  /// \brief	Prints the bound TOML object out to the stream as JSON.
  template <typename T, typename U>
  inline std::basic_ostream<T> &operator<<(std::basic_ostream<T> &lhs,
                                           json_formatter<U> &rhs) {
    rhs.attach(lhs);
    rhs.print();
    rhs.detach();
    return lhs;
  }

  /// \brief	Prints the bound TOML object out to the stream as JSON (rvalue
  /// overload).
  template <typename T, typename U>
  inline std::basic_ostream<T> &operator<<(std::basic_ostream<T> &lhs,
                                           json_formatter<U> &&rhs) {
    return lhs << rhs; // as lvalue
  }

#if !defined(DOXYGEN) && !TOML_HEADER_ONLY
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    json_formatter<char> &);
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    json_formatter<char> &&);
#endif
}
TOML_NAMESPACE_END;

TOML_POP_WARNINGS; // TOML_DISABLE_SWITCH_WARNINGS
