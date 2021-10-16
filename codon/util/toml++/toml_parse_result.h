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
#include "toml_parse_error.h"
#include "toml_table.h"

#if defined(DOXYGEN) || !TOML_EXCEPTIONS
TOML_NAMESPACE_START {
  TOML_ABI_NAMESPACE_START(noex);

  /// \brief	The result of a parsing operation.
  ///
  /// \detail A parse_result is effectively a discriminated union containing either a
  /// toml::table 		or a toml::parse_error. Most member functions assume a
  /// particular one of these two states, 		and calling them when in the
  /// wrong state will cause errors
  /// (e.g. attempting to access the 		error object when parsing was
  /// successful). \cpp parse_result result = toml::parse_file("config.toml"); if
  /// (result)
  ///		do_stuff_with_a_table(result); //implicitly converts to table&
  ///	else
  ///		std::cerr << "Parse failed:\n"sv << result.error() << "\n";
  ///
  /// \ecpp
  ///
  /// \out
  /// example output:
  ///
  /// Parse failed:
  /// Encountered unexpected character while parsing boolean; expected 'true', saw 'trU'
  ///		(error occurred at line 1, column 13 of 'config.toml')
  /// \eout
  ///
  /// Getting node_views (`operator[]`) and using the iterator accessor functions
  /// (`begin(), end()` etc.) are unconditionally safe; when parsing fails these just
  /// return 'empty' values. A ranged-for loop on a failed parse_result is also safe
  /// since `begin()` and `end()` return the same iterator and will not lead to any
  /// dereferences and iterations.
  ///
  /// \availability <strong>This type only exists when exceptions are disabled.</strong>
  /// 		 Otherwise parse_result is just an alias for toml::table: \cpp
  /// #if TOML_EXCEPTIONS
  ///		using parse_result = table;
  /// #else
  ///		class parse_result final { // ...
  ///	#endif
  /// \ecpp
  class parse_result {
  private:
    struct storage_t {
      static constexpr size_t size_ =
          (sizeof(toml::table) < sizeof(parse_error) ? sizeof(parse_error)
                                                     : sizeof(toml::table));
      static constexpr size_t align_ =
          (alignof(toml::table) < alignof(parse_error) ? alignof(parse_error)
                                                       : alignof(toml::table));

      alignas(align_) unsigned char bytes[size_];
    };

    mutable storage_t storage_;
    bool err_;

    template <typename Type>
    [[nodiscard]] TOML_ALWAYS_INLINE static Type *get_as(storage_t &s) noexcept {
      return TOML_LAUNDER(reinterpret_cast<Type *>(s.bytes));
    }

    void destroy() noexcept {
      if (err_)
        get_as<parse_error>(storage_)->~parse_error();
      else
        get_as<toml::table>(storage_)->~table();
    }

  public:
    /// \brief A BidirectionalIterator for iterating over key-value pairs in a wrapped
    /// toml::table.
    using iterator = table_iterator;

    /// \brief A BidirectionalIterator for iterating over const key-value pairs in a
    /// wrapped toml::table.
    using const_iterator = const_table_iterator;

    /// \brief	Returns true if parsing succeeeded.
    [[nodiscard]] bool succeeded() const noexcept { return !err_; }
    /// \brief	Returns true if parsing failed.
    [[nodiscard]] bool failed() const noexcept { return err_; }
    /// \brief	Returns true if parsing succeeded.
    [[nodiscard]] explicit operator bool() const noexcept { return !err_; }

    /// \brief	Returns the internal toml::table.
    [[nodiscard]] toml::table &table() &noexcept {
      TOML_ASSERT(!err_);
      return *get_as<toml::table>(storage_);
    }

    /// \brief	Returns the internal toml::table (rvalue overload).
    [[nodiscard]] toml::table &&table() &&noexcept {
      TOML_ASSERT(!err_);
      return static_cast<toml::table &&>(*get_as<toml::table>(storage_));
    }

    /// \brief	Returns the internal toml::table (const lvalue overload).
    [[nodiscard]] const toml::table &table() const &noexcept {
      TOML_ASSERT(!err_);
      return *get_as<const toml::table>(storage_);
    }

    /// \brief	Returns the internal toml::parse_error.
    [[nodiscard]] parse_error &error() &noexcept {
      TOML_ASSERT(err_);
      return *get_as<parse_error>(storage_);
    }

    /// \brief	Returns the internal toml::parse_error (rvalue overload).
    [[nodiscard]] parse_error &&error() &&noexcept {
      TOML_ASSERT(err_);
      return static_cast<parse_error &&>(*get_as<parse_error>(storage_));
    }

    /// \brief	Returns the internal toml::parse_error (const lvalue overload).
    [[nodiscard]] const parse_error &error() const &noexcept {
      TOML_ASSERT(err_);
      return *get_as<const parse_error>(storage_);
    }

    /// \brief	Returns the internal toml::table.
    [[nodiscard]] operator toml::table &() noexcept { return table(); }
    /// \brief	Returns the internal toml::table (rvalue overload).
    [[nodiscard]] operator toml::table &&() noexcept { return std::move(table()); }
    /// \brief	Returns the internal toml::table (const lvalue overload).
    [[nodiscard]] operator const toml::table &() const noexcept { return table(); }

    /// \brief	Returns the internal toml::parse_error.
    [[nodiscard]] explicit operator parse_error &() noexcept { return error(); }
    /// \brief	Returns the internal toml::parse_error (rvalue overload).
    [[nodiscard]] explicit operator parse_error &&() noexcept {
      return std::move(error());
    }
    /// \brief	Returns the internal toml::parse_error (const lvalue overload).
    [[nodiscard]] explicit operator const parse_error &() const noexcept {
      return error();
    }

    TOML_NODISCARD_CTOR
    parse_result() noexcept : err_{true} {
      ::new (static_cast<void *>(storage_.bytes))
          parse_error{std::string{}, source_region{}};
    }

    TOML_NODISCARD_CTOR
    explicit parse_result(toml::table &&tbl) noexcept : err_{false} {
      ::new (static_cast<void *>(storage_.bytes)) toml::table{std::move(tbl)};
    }

    TOML_NODISCARD_CTOR
    explicit parse_result(parse_error &&err) noexcept : err_{true} {
      ::new (static_cast<void *>(storage_.bytes)) parse_error{std::move(err)};
    }

    /// \brief	Move constructor.
    TOML_NODISCARD_CTOR
    parse_result(parse_result &&res) noexcept : err_{res.err_} {
      if (err_)
        ::new (static_cast<void *>(storage_.bytes)) parse_error{std::move(res).error()};
      else
        ::new (static_cast<void *>(storage_.bytes)) toml::table{std::move(res).table()};
    }

    /// \brief	Move-assignment operator.
    parse_result &operator=(parse_result &&rhs) noexcept {
      if (err_ != rhs.err_) {
        destroy();
        err_ = rhs.err_;
        if (err_)
          ::new (static_cast<void *>(storage_.bytes))
              parse_error{std::move(rhs).error()};
        else
          ::new (static_cast<void *>(storage_.bytes))
              toml::table{std::move(rhs).table()};
      } else {
        if (err_)
          error() = std::move(rhs).error();
        else
          table() = std::move(rhs).table();
      }
      return *this;
    }

    /// \brief	Destructor.
    ~parse_result() noexcept { destroy(); }

    /// \brief	Gets a node_view for the selected key-value pair in the wrapped table.
    ///
    /// \param 	key The key used for the lookup.
    ///
    /// \returns	A view of the value at the given key if parsing was successful
    /// and a matching key existed, 			or an empty node view.
    ///
    /// \see toml::node_view
    [[nodiscard]] node_view<node> operator[](string_view key) noexcept {
      return err_ ? node_view<node>{} : table()[key];
    }

    /// \brief	Gets a node_view for the selected key-value pair in the wrapped table
    /// (const overload).
    ///
    /// \param 	key The key used for the lookup.
    ///
    /// \returns	A view of the value at the given key if parsing was successful
    /// and a matching key existed, 			or an empty node view.
    ///
    /// \see toml::node_view
    [[nodiscard]] node_view<const node> operator[](string_view key) const noexcept {
      return err_ ? node_view<const node>{} : table()[key];
    }

#if TOML_WINDOWS_COMPAT

    /// \brief	Gets a node_view for the selected key-value pair in the wrapped table.
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \param 	key The key used for the lookup.
    ///
    /// \returns	A view of the value at the given key if parsing was successful
    /// and a matching key existed, 			or an empty node view.
    ///
    /// \see toml::node_view
    [[nodiscard]] node_view<node> operator[](std::wstring_view key) noexcept {
      return err_ ? node_view<node>{} : table()[key];
    }

    /// \brief	Gets a node_view for the selected key-value pair in the wrapped table
    /// (const overload).
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \param 	key The key used for the lookup.
    ///
    /// \returns	A view of the value at the given key if parsing was successful
    /// and a matching key existed, 			or an empty node view.
    ///
    /// \see toml::node_view
    [[nodiscard]] node_view<const node>
    operator[](std::wstring_view key) const noexcept {
      return err_ ? node_view<const node>{} : table()[key];
    }

#endif // TOML_WINDOWS_COMPAT

    /// \brief	Returns an iterator to the first key-value pair in the wrapped table.
    /// \remarks Returns a default-constructed 'nothing' iterator if the parsing failed.
    [[nodiscard]] table_iterator begin() noexcept {
      return err_ ? table_iterator{} : table().begin();
    }

    /// \brief	Returns an iterator to the first key-value pair in the wrapped table.
    /// \remarks Returns a default-constructed 'nothing' iterator if the parsing failed.
    [[nodiscard]] const_table_iterator begin() const noexcept {
      return err_ ? const_table_iterator{} : table().begin();
    }

    /// \brief	Returns an iterator to the first key-value pair in the wrapped table.
    /// \remarks Returns a default-constructed 'nothing' iterator if the parsing failed.
    [[nodiscard]] const_table_iterator cbegin() const noexcept {
      return err_ ? const_table_iterator{} : table().cbegin();
    }

    /// \brief	Returns an iterator to one-past-the-last key-value pair in the wrapped
    /// table. \remarks Returns a default-constructed 'nothing' iterator if the parsing
    /// failed.
    [[nodiscard]] table_iterator end() noexcept {
      return err_ ? table_iterator{} : table().end();
    }

    /// \brief	Returns an iterator to one-past-the-last key-value pair in the wrapped
    /// table. \remarks Returns a default-constructed 'nothing' iterator if the parsing
    /// failed.
    [[nodiscard]] const_table_iterator end() const noexcept {
      return err_ ? const_table_iterator{} : table().end();
    }

    /// \brief	Returns an iterator to one-past-the-last key-value pair in the wrapped
    /// table. \remarks Returns a default-constructed 'nothing' iterator if the parsing
    /// failed.
    [[nodiscard]] const_table_iterator cend() const noexcept {
      return err_ ? const_table_iterator{} : table().cend();
    }

    /// \brief Prints the held error or table object out to a text stream.
    template <typename Char>
    friend std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &os,
                                                const parse_result &result) {
      return result.err_ ? (os << result.error()) : (os << result.table());
    }
  };

  TOML_ABI_NAMESPACE_END;
}
TOML_NAMESPACE_END;
#endif // !TOML_EXCEPTIONS
