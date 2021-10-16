//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT

#pragma once
#include "toml_array.h"
#include "toml_formatter.h"
#include "toml_table.h"
#include "toml_utf8.h"

TOML_PUSH_WARNINGS;
TOML_DISABLE_SWITCH_WARNINGS;

/// \cond
TOML_IMPL_NAMESPACE_START {
  [[nodiscard]] TOML_API std::string default_formatter_make_key_segment(
      const std::string &) noexcept;
  [[nodiscard]] TOML_API size_t default_formatter_inline_columns(const node &) noexcept;
  [[nodiscard]] TOML_API bool default_formatter_forces_multiline(const node &,
                                                                 size_t = 0) noexcept;
}
TOML_IMPL_NAMESPACE_END;
/// \endcond

TOML_NAMESPACE_START {
  /// \brief	A wrapper for printing TOML objects out to a stream as formatted TOML.
  ///
  /// \remarks You generally don't need to create an instance of this class explicitly;
  /// the stream 		 operators of the TOML node types already print
  /// themselves out using this formatter.
  ///
  /// \detail \cpp
  /// auto tbl = toml::table{{
  ///		{ "description", "This is some TOML, yo." },
  ///		{ "fruit", toml::array{ "apple", "orange", "pear" } },
  ///		{ "numbers", toml::array{ 1, 2, 3, 4, 5 } },
  ///		{ "table", toml::table{{ { "foo", "bar" } }} }
  /// }};
  ///
  /// // these two lines are equivalent:
  ///	std::cout << toml::default_formatter{ tbl } << "\n";
  ///	std::cout << tbl << "\n";
  ///
  /// \ecpp
  ///
  /// \out
  /// description = "This is some TOML, yo."
  /// fruit = ["apple", "orange", "pear"]
  /// numbers = [1, 2, 3, 4, 5]
  ///
  /// [table]
  /// foo = "bar"
  /// \eout
  ///
  /// \tparam	Char	The underlying character type of the output stream. Must be 1
  /// byte in size.
  template <typename Char = char>
  class TOML_API default_formatter final : impl::formatter<Char> {
  private:
    /// \cond

    using base = impl::formatter<Char>;
    std::vector<std::string> key_path;
    bool pending_table_separator_ = false;

    void print_pending_table_separator() {
      if (pending_table_separator_) {
        base::print_newline(true);
        base::print_newline(true);
        pending_table_separator_ = false;
      }
    }

    void print_key_segment(const std::string &str) {
      if (str.empty())
        impl::print_to_stream("''"sv, base::stream());
      else {
        bool requiresQuotes = false;
        {
          impl::utf8_decoder decoder;
          for (size_t i = 0; i < str.length() && !requiresQuotes; i++) {
            decoder(static_cast<uint8_t>(str[i]));
            if (decoder.error())
              requiresQuotes = true;
            else if (decoder.has_code_point())
              requiresQuotes = !impl::is_bare_key_character(decoder.codepoint);
          }
        }

        if (requiresQuotes) {
          impl::print_to_stream('"', base::stream());
          impl::print_to_stream_with_escapes(str, base::stream());
          impl::print_to_stream('"', base::stream());
        } else
          impl::print_to_stream(str, base::stream());
      }
      base::clear_naked_newline();
    }

    void print_key_path() {
      for (const auto &segment : key_path) {
        if (std::addressof(segment) > key_path.data())
          impl::print_to_stream('.', base::stream());
        impl::print_to_stream(segment, base::stream());
      }
      base::clear_naked_newline();
    }

    void print_inline(const table & /*tbl*/);

    void print(const array &arr) {
      if (arr.empty())
        impl::print_to_stream("[]"sv, base::stream());
      else {
        const auto original_indent = base::indent();
        const auto multiline = impl::default_formatter_forces_multiline(
            arr, base::indent_columns *
                     static_cast<size_t>(original_indent < 0 ? 0 : original_indent));
        impl::print_to_stream("["sv, base::stream());
        if (multiline) {
          if (original_indent < 0)
            base::indent(0);
          base::increase_indent();
        } else
          impl::print_to_stream(' ', base::stream());

        for (size_t i = 0; i < arr.size(); i++) {
          if (i > 0_sz) {
            impl::print_to_stream(',', base::stream());
            if (!multiline)
              impl::print_to_stream(' ', base::stream());
          }

          if (multiline) {
            base::print_newline(true);
            base::print_indent();
          }

          auto &v = arr[i];
          const auto type = v.type();
          TOML_ASSUME(type != node_type::none);
          switch (type) {
          case node_type::table:
            print_inline(*reinterpret_cast<const table *>(&v));
            break;
          case node_type::array:
            print(*reinterpret_cast<const array *>(&v));
            break;
          default:
            base::print_value(v, type);
          }
        }
        if (multiline) {
          base::indent(original_indent);
          base::print_newline(true);
          base::print_indent();
        } else
          impl::print_to_stream(' ', base::stream());
        impl::print_to_stream("]"sv, base::stream());
      }
      base::clear_naked_newline();
    }

    void print(const table &tbl) {
      static constexpr auto is_non_inline_array_of_tables = [](auto &&nde) noexcept {
        auto arr = nde.as_array();
        return arr && arr->is_array_of_tables() &&
               !arr->template get_as<table>(0_sz)->is_inline();
      };

      // values, arrays, and inline tables/table arrays
      for (auto &&[k, v] : tbl) {
        const auto type = v.type();
        if ((type == node_type::table &&
             !reinterpret_cast<const table *>(&v)->is_inline()) ||
            (type == node_type::array && is_non_inline_array_of_tables(v)))
          continue;

        pending_table_separator_ = true;
        base::print_newline();
        base::print_indent();
        print_key_segment(k);
        impl::print_to_stream(" = "sv, base::stream());
        TOML_ASSUME(type != node_type::none);
        switch (type) {
        case node_type::table:
          print_inline(*reinterpret_cast<const table *>(&v));
          break;
        case node_type::array:
          print(*reinterpret_cast<const array *>(&v));
          break;
        default:
          base::print_value(v, type);
        }
      }

      // non-inline tables
      for (auto &&[k, v] : tbl) {
        const auto type = v.type();
        if (type != node_type::table ||
            reinterpret_cast<const table *>(&v)->is_inline())
          continue;
        auto &child_tbl = *reinterpret_cast<const table *>(&v);

        // we can skip indenting and emitting the headers for tables that only contain
        // other tables (so we don't over-nest)
        size_t child_value_count{}; // includes inline tables and non-table arrays
        size_t child_table_count{};
        size_t child_table_array_count{};
        for (auto &&[child_k, child_v] : child_tbl) {
          (void)child_k;
          const auto child_type = child_v.type();
          TOML_ASSUME(child_type != node_type::none);
          switch (child_type) {
          case node_type::table:
            if (reinterpret_cast<const table *>(&child_v)->is_inline())
              child_value_count++;
            else
              child_table_count++;
            break;

          case node_type::array:
            if (is_non_inline_array_of_tables(child_v))
              child_table_array_count++;
            else
              child_value_count++;
            break;

          default:
            child_value_count++;
          }
        }
        bool skip_self = false;
        if (child_value_count == 0_sz &&
            (child_table_count > 0_sz || child_table_array_count > 0_sz))
          skip_self = true;

        key_path.push_back(impl::default_formatter_make_key_segment(k));

        if (!skip_self) {
          print_pending_table_separator();
          base::increase_indent();
          base::print_indent();
          impl::print_to_stream("["sv, base::stream());
          print_key_path();
          impl::print_to_stream("]"sv, base::stream());
          pending_table_separator_ = true;
        }

        print(child_tbl);

        key_path.pop_back();
        if (!skip_self)
          base::decrease_indent();
      }

      // table arrays
      for (auto &&[k, v] : tbl) {
        if (!is_non_inline_array_of_tables(v))
          continue;
        auto &arr = *reinterpret_cast<const array *>(&v);

        base::increase_indent();
        key_path.push_back(impl::default_formatter_make_key_segment(k));

        for (size_t i = 0; i < arr.size(); i++) {
          print_pending_table_separator();
          base::print_indent();
          impl::print_to_stream("[["sv, base::stream());
          print_key_path();
          impl::print_to_stream("]]"sv, base::stream());
          pending_table_separator_ = true;
          print(*reinterpret_cast<const table *>(&arr[i]));
        }

        key_path.pop_back();
        base::decrease_indent();
      }
    }

    void print() {
      if (base::dump_failed_parse_result())
        return;

      switch (auto source_type = base::source().type()) {
      case node_type::table: {
        auto &tbl = *reinterpret_cast<const table *>(&base::source());
        if (tbl.is_inline())
          print_inline(tbl);
        else {
          base::decrease_indent(); // so root kvps and tables have the same indent
          print(tbl);
        }
        break;
      }

      case node_type::array:
        print(*reinterpret_cast<const array *>(&base::source()));
        break;

      default:
        base::print_value(base::source(), source_type);
      }
    }

    /// \endcond

  public:
    /// \brief	The default flags for a default_formatter.
    static constexpr format_flags default_flags =
        format_flags::allow_literal_strings | format_flags::allow_multi_line_strings |
        format_flags::allow_value_format_flags;

    /// \brief	Constructs a default formatter and binds it to a TOML object.
    ///
    /// \param 	source	The source TOML object.
    /// \param 	flags 	Format option flags.
    TOML_NODISCARD_CTOR
    explicit default_formatter(const toml::node &source,
                               format_flags flags = default_flags) noexcept
        : base{source, flags} {}

#if defined(DOXYGEN) || (TOML_PARSER && !TOML_EXCEPTIONS)

    /// \brief	Constructs a default TOML formatter and binds it to a
    /// toml::parse_result.
    ///
    /// \availability This constructor is only available when exceptions are disabled.
    ///
    /// \attention Formatting a failed parse result will simply dump the error message
    /// out as-is.
    ///		This will not be valid TOML, but at least gives you something to log or
    /// show up in diagnostics:
    /// \cpp
    /// std::cout << toml::default_formatter{ toml::parse("a = 'b'"sv) } // ok
    ///           << "\n\n"
    ///           << toml::default_formatter{ toml::parse("a = "sv) } // malformed
    ///           << "\n";
    /// \ecpp
    /// \out
    /// a = 'b'
    ///
    /// Error while parsing key-value pair: encountered end-of-file
    ///         (error occurred at line 1, column 5)
    /// \eout
    /// Use the library with exceptions if you want to avoid this scenario.
    ///
    /// \param 	result	The parse result.
    /// \param 	flags 	Format option flags.
    TOML_NODISCARD_CTOR
    explicit default_formatter(const toml::parse_result &result,
                               format_flags flags = default_flags) noexcept
        : base{result, flags} {}

#endif

    template <typename T, typename U>
    friend std::basic_ostream<T> &operator<<(std::basic_ostream<T> &,
                                             default_formatter<U> &);
    template <typename T, typename U>
    friend std::basic_ostream<T> &operator<<(std::basic_ostream<T> &,
                                             default_formatter<U> &&);
  };

#if !defined(DOXYGEN) && !TOML_HEADER_ONLY
  extern template class TOML_API default_formatter<char>;
#endif

  default_formatter(const table &)->default_formatter<char>;
  default_formatter(const array &)->default_formatter<char>;
  template <typename T> default_formatter(const value<T> &) -> default_formatter<char>;

  /// \brief	Prints the bound TOML object out to the stream as formatted TOML.
  template <typename T, typename U>
  inline std::basic_ostream<T> &operator<<(std::basic_ostream<T> &lhs,
                                           default_formatter<U> &rhs) {
    rhs.attach(lhs);
    rhs.key_path.clear();
    rhs.print();
    rhs.detach();
    return lhs;
  }

  /// \brief	Prints the bound TOML object out to the stream as formatted TOML (rvalue
  /// overload).
  template <typename T, typename U>
  inline std::basic_ostream<T> &operator<<(std::basic_ostream<T> &lhs,
                                           default_formatter<U> &&rhs) {
    return lhs << rhs; // as lvalue
  }

#ifndef DOXYGEN

#if !TOML_HEADER_ONLY
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    default_formatter<char> &);
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    default_formatter<char> &&);
  extern template TOML_API std::ostream &operator<<(std::ostream &, const table &);
  extern template TOML_API std::ostream &operator<<(std::ostream &, const array &);
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    const value<std::string> &);
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    const value<int64_t> &);
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    const value<double> &);
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    const value<bool> &);
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    const value<toml::date> &);
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    const value<toml::time> &);
  extern template TOML_API std::ostream &operator<<(std::ostream &,
                                                    const value<toml::date_time> &);
#endif

  template <typename Char>
  inline std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &lhs,
                                              const table &rhs) {
    return lhs << default_formatter<Char>{rhs};
  }

  template <typename Char>
  inline std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &lhs,
                                              const array &rhs) {
    return lhs << default_formatter<Char>{rhs};
  }

  template <typename Char, typename T>
  inline std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &lhs,
                                              const value<T> &rhs) {
    return lhs << default_formatter<Char>{rhs};
  }

#endif // !DOXYGEN
}
TOML_NAMESPACE_END;

TOML_POP_WARNINGS; // TOML_DISABLE_SWITCH_WARNINGS
