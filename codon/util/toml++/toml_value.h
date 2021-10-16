//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT

#pragma once
#include "toml_node.h"
#include "toml_print_to_stream.h"

#ifndef DOXYGEN
#if TOML_WINDOWS_COMPAT
#define TOML_SA_VALUE_MESSAGE_WSTRING TOML_SA_LIST_SEP "std::wstring"
#else
#define TOML_SA_VALUE_MESSAGE_WSTRING
#endif

#if TOML_HAS_CHAR8
#define TOML_SA_VALUE_MESSAGE_U8STRING_VIEW TOML_SA_LIST_SEP "std::u8string_view"
#define TOML_SA_VALUE_MESSAGE_CONST_CHAR8 TOML_SA_LIST_SEP "const char8_t*"
#else
#define TOML_SA_VALUE_MESSAGE_U8STRING_VIEW
#define TOML_SA_VALUE_MESSAGE_CONST_CHAR8
#endif

#define TOML_SA_VALUE_EXACT_FUNC_MESSAGE(type_arg)                                     \
  "The " type_arg " must be one of:" TOML_SA_LIST_NEW                                  \
  "A native TOML value type" TOML_SA_NATIVE_VALUE_TYPE_LIST                            \
                                                                                       \
      TOML_SA_LIST_NXT "A non-view type capable of losslessly representing a native "  \
  "TOML value type" TOML_SA_LIST_BEG                                                   \
  "std::string" TOML_SA_VALUE_MESSAGE_WSTRING TOML_SA_LIST_SEP                         \
  "any signed integer type >= 64 bits" TOML_SA_LIST_SEP                                \
  "any floating-point type >= 64 bits" TOML_SA_LIST_END                                \
                                                                                       \
      TOML_SA_LIST_NXT                                                                 \
  "An immutable view type not requiring additional temporary storage" TOML_SA_LIST_BEG \
  "std::string_view" TOML_SA_VALUE_MESSAGE_U8STRING_VIEW TOML_SA_LIST_SEP              \
  "const char*" TOML_SA_VALUE_MESSAGE_CONST_CHAR8 TOML_SA_LIST_END

#define TOML_SA_VALUE_FUNC_MESSAGE(type_arg)                                           \
  "The " type_arg " must be one of:" TOML_SA_LIST_NEW                                  \
  "A native TOML value type" TOML_SA_NATIVE_VALUE_TYPE_LIST                            \
                                                                                       \
      TOML_SA_LIST_NXT "A non-view type capable of losslessly representing a native "  \
  "TOML value type" TOML_SA_LIST_BEG                                                   \
  "std::string" TOML_SA_VALUE_MESSAGE_WSTRING TOML_SA_LIST_SEP                         \
  "any signed integer type >= 64 bits" TOML_SA_LIST_SEP                                \
  "any floating-point type >= 64 bits" TOML_SA_LIST_END                                \
                                                                                       \
      TOML_SA_LIST_NXT "A non-view type capable of (reasonably) representing a "       \
  "native TOML value type" TOML_SA_LIST_BEG "any other integer type" TOML_SA_LIST_SEP  \
  "any floating-point type >= 32 bits" TOML_SA_LIST_END                                \
                                                                                       \
      TOML_SA_LIST_NXT                                                                 \
  "An immutable view type not requiring additional temporary storage" TOML_SA_LIST_BEG \
  "std::string_view" TOML_SA_VALUE_MESSAGE_U8STRING_VIEW TOML_SA_LIST_SEP              \
  "const char*" TOML_SA_VALUE_MESSAGE_CONST_CHAR8 TOML_SA_LIST_END
#endif // !DOXYGEN

TOML_PUSH_WARNINGS;
TOML_DISABLE_ARITHMETIC_WARNINGS;

/// \cond
TOML_IMPL_NAMESPACE_START {
  template <typename T, typename...> struct native_value_maker {
    template <typename... Args>
    [[nodiscard]] static T
    make(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args &&...>) {
      return T(static_cast<Args &&>(args)...);
    }
  };

  template <typename T> struct native_value_maker<T, T> {
    template <typename U>
    [[nodiscard]] TOML_ALWAYS_INLINE static U &&make(U &&val) noexcept {
      return static_cast<U &&>(val);
    }
  };

#if TOML_HAS_CHAR8 || TOML_WINDOWS_COMPAT

  struct string_maker {
    template <typename T> [[nodiscard]] static std::string make(T &&arg) noexcept {
#if TOML_HAS_CHAR8
      if constexpr (is_one_of<std::decay_t<T>, char8_t *, const char8_t *>)
        return std::string(
            reinterpret_cast<const char *>(static_cast<const char8_t *>(arg)));
      else if constexpr (is_one_of<remove_cvref_t<T>, std::u8string,
                                   std::u8string_view>)
        return std::string(
            reinterpret_cast<const char *>(static_cast<const char8_t *>(arg.data())),
            arg.length());
#endif // TOML_HAS_CHAR8

#if TOML_WINDOWS_COMPAT
      if constexpr (is_wide_string<T>)
        return narrow(static_cast<T &&>(arg));
#endif // TOML_WINDOWS_COMPAT
    }
  };
#if TOML_HAS_CHAR8
  template <> struct native_value_maker<std::string, char8_t *> : string_maker {};
  template <> struct native_value_maker<std::string, const char8_t *> : string_maker {};
  template <> struct native_value_maker<std::string, std::u8string> : string_maker {};
  template <>
  struct native_value_maker<std::string, std::u8string_view> : string_maker {};
#endif // TOML_HAS_CHAR8
#if TOML_WINDOWS_COMPAT
  template <> struct native_value_maker<std::string, wchar_t *> : string_maker {};
  template <> struct native_value_maker<std::string, const wchar_t *> : string_maker {};
  template <> struct native_value_maker<std::string, std::wstring> : string_maker {};
  template <>
  struct native_value_maker<std::string, std::wstring_view> : string_maker {};
#endif // TOML_WINDOWS_COMPAT

#endif // TOML_HAS_CHAR8 || TOML_WINDOWS_COMPAT

  template <typename T>
  [[nodiscard]] TOML_ATTR(const) inline optional<T> node_integer_cast(
      int64_t val) noexcept {
    static_assert(node_type_of<T> == node_type::integer);
    static_assert(!is_cvref<T>);

    using traits = value_traits<T>;
    if constexpr (!traits::is_signed) {
      if constexpr ((sizeof(T) * CHAR_BIT) < 63) // 63 bits == int64_max
      {
        using common_t = decltype(int64_t{} + T{});
        if (val < int64_t{} ||
            static_cast<common_t>(val) > static_cast<common_t>(traits::max))
          return {};
      } else {
        if (val < int64_t{})
          return {};
      }
    } else {
      if (val < traits::min || val > traits::max)
        return {};
    }
    return {static_cast<T>(val)};
  }
}
TOML_IMPL_NAMESPACE_END;
/// \endcond

TOML_NAMESPACE_START {
  /// \brief	A TOML value.
  ///
  /// \tparam	ValueType	The value's native TOML data type. Can be one of:
  /// 						- std::string
  /// 						- toml::date
  /// 						- toml::time
  /// 						- toml::date_time
  /// 						- int64_t
  /// 						- double
  /// 						- bool
  template <typename ValueType> class TOML_API value final : public node {
    static_assert(impl::is_native<ValueType> && !impl::is_cvref<ValueType>,
                  "A toml::value<> must model one of the native TOML value "
                  "types:" TOML_SA_NATIVE_VALUE_TYPE_LIST);

  private:
    friend class TOML_PARSER_TYPENAME;

    /// \cond

    template <typename T, typename U>
    [[nodiscard]] TOML_ALWAYS_INLINE
    TOML_ATTR(const) static auto as_value([[maybe_unused]] U *ptr) noexcept {
      if constexpr (std::is_same_v<value_type, T>)
        return ptr;
      else
        return nullptr;
    }

    ValueType val_;
    value_flags flags_ = value_flags::none;

#if TOML_LIFETIME_HOOKS
    void lh_ctor() noexcept { TOML_VALUE_CREATED; }

    void lh_dtor() noexcept { TOML_VALUE_DESTROYED; }
#endif

    /// \endcond

  public:
    /// \brief	The value's underlying data type.
    using value_type = ValueType;

    /// \brief	A type alias for 'value arguments'.
    /// \details This differs according to the value's type argument:
    /// 		 - ints, floats, booleans: `value_type`
    /// 		 - strings: `string_view`
    /// 		 - everything else: `const value_type&`
    using value_arg = std::conditional_t<
        std::is_same_v<value_type, std::string>, std::string_view,
        std::conditional_t<impl::is_one_of<value_type, double, int64_t, bool>,
                           value_type, const value_type &>>;

    /// \brief	Constructs a toml value.
    ///
    /// \tparam	Args	Constructor argument types.
    /// \param 	args	Arguments to forward to the internal value's constructor.
    template <typename... Args>
    TOML_NODISCARD_CTOR explicit value(Args &&...args) noexcept(noexcept(
        value_type(impl::native_value_maker<value_type, std::decay_t<Args>...>::make(
            static_cast<Args &&>(args)...))))
        : val_(impl::native_value_maker<value_type, std::decay_t<Args>...>::make(
              static_cast<Args &&>(args)...)) {
#if TOML_LIFETIME_HOOKS
      lh_ctor();
#endif
    }

    /// \brief	Copy constructor.
    TOML_NODISCARD_CTOR
    value(const value &other) noexcept
        : node(other), val_{other.val_}, flags_{other.flags_} {
#if TOML_LIFETIME_HOOKS
      lh_ctor();
#endif
    }

    /// \brief	Move constructor.
    TOML_NODISCARD_CTOR
    value(value &&other) noexcept
        : node(std::move(other)), val_{std::move(other.val_)}, flags_{other.flags_} {
#if TOML_LIFETIME_HOOKS
      lh_ctor();
#endif
    }

    /// \brief	Copy-assignment operator.
    value &operator=(const value &rhs) noexcept {
      node::operator=(rhs);
      val_ = rhs.val_;
      flags_ = rhs.flags_;
      return *this;
    }

    /// \brief	Move-assignment operator.
    value &operator=(value &&rhs) noexcept {
      if (&rhs != this) {
        node::operator=(std::move(rhs));
        val_ = std::move(rhs.val_);
        flags_ = rhs.flags_;
      }
      return *this;
    }

#if TOML_LIFETIME_HOOKS
    ~value() noexcept override { lh_dtor(); }
#endif

    /// \name Type checks
    /// @{

    /// \brief	Returns the value's node type identifier.
    ///
    /// \returns	One of:
    /// 			- node_type::string
    /// 			- node_type::integer
    /// 			- node_type::floating_point
    /// 			- node_type::boolean
    /// 			- node_type::date
    /// 			- node_type::time
    /// 			- node_type::date_time
    [[nodiscard]] node_type type() const noexcept override {
      return impl::node_type_of<value_type>;
    }

    [[nodiscard]] bool is_table() const noexcept override { return false; }
    [[nodiscard]] bool is_array() const noexcept override { return false; }
    [[nodiscard]] bool is_value() const noexcept override { return true; }

    [[nodiscard]] bool is_string() const noexcept override {
      return std::is_same_v<value_type, std::string>;
    }
    [[nodiscard]] bool is_integer() const noexcept override {
      return std::is_same_v<value_type, int64_t>;
    }
    [[nodiscard]] bool is_floating_point() const noexcept override {
      return std::is_same_v<value_type, double>;
    }
    [[nodiscard]] bool is_number() const noexcept override {
      return impl::is_one_of<value_type, int64_t, double>;
    }
    [[nodiscard]] bool is_boolean() const noexcept override {
      return std::is_same_v<value_type, bool>;
    }
    [[nodiscard]] bool is_date() const noexcept override {
      return std::is_same_v<value_type, date>;
    }
    [[nodiscard]] bool is_time() const noexcept override {
      return std::is_same_v<value_type, time>;
    }
    [[nodiscard]] bool is_date_time() const noexcept override {
      return std::is_same_v<value_type, date_time>;
    }

    [[nodiscard]] bool is_homogeneous(node_type ntype) const noexcept override {
      return ntype == node_type::none || ntype == impl::node_type_of<value_type>;
    }
    [[nodiscard]] bool is_homogeneous(node_type ntype,
                                      toml::node *&first_nonmatch) noexcept override {
      if (ntype != node_type::none && ntype != impl::node_type_of<value_type>) {
        first_nonmatch = this;
        return false;
      }
      return true;
    }
    [[nodiscard]] bool
    is_homogeneous(node_type ntype,
                   const toml::node *&first_nonmatch) const noexcept override {
      if (ntype != node_type::none && ntype != impl::node_type_of<value_type>) {
        first_nonmatch = this;
        return false;
      }
      return true;
    }
    template <typename ElemType = void>
    [[nodiscard]] bool is_homogeneous() const noexcept {
      using type = impl::unwrap_node<ElemType>;
      static_assert(std::is_void_v<type> ||
                        ((impl::is_native<type> ||
                          impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>),
                    "The template type argument of value::is_homogeneous() must be "
                    "void or one of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);

      using type = impl::unwrap_node<ElemType>;
      if constexpr (std::is_void_v<type>)
        return true;
      else
        return impl::node_type_of<type> == impl::node_type_of<value_type>;
    }

    /// @}

    /// \name Type casts
    /// @{

    [[nodiscard]] value<std::string> *as_string() noexcept override {
      return as_value<std::string>(this);
    }
    [[nodiscard]] value<int64_t> *as_integer() noexcept override {
      return as_value<int64_t>(this);
    }
    [[nodiscard]] value<double> *as_floating_point() noexcept override {
      return as_value<double>(this);
    }
    [[nodiscard]] value<bool> *as_boolean() noexcept override {
      return as_value<bool>(this);
    }
    [[nodiscard]] value<date> *as_date() noexcept override {
      return as_value<date>(this);
    }
    [[nodiscard]] value<time> *as_time() noexcept override {
      return as_value<time>(this);
    }
    [[nodiscard]] value<date_time> *as_date_time() noexcept override {
      return as_value<date_time>(this);
    }

    [[nodiscard]] const value<std::string> *as_string() const noexcept override {
      return as_value<std::string>(this);
    }
    [[nodiscard]] const value<int64_t> *as_integer() const noexcept override {
      return as_value<int64_t>(this);
    }
    [[nodiscard]] const value<double> *as_floating_point() const noexcept override {
      return as_value<double>(this);
    }
    [[nodiscard]] const value<bool> *as_boolean() const noexcept override {
      return as_value<bool>(this);
    }
    [[nodiscard]] const value<date> *as_date() const noexcept override {
      return as_value<date>(this);
    }
    [[nodiscard]] const value<time> *as_time() const noexcept override {
      return as_value<time>(this);
    }
    [[nodiscard]] const value<date_time> *as_date_time() const noexcept override {
      return as_value<date_time>(this);
    }

    /// @}

    /// \name Value retrieval
    /// @{

    /// \brief	Returns a reference to the underlying value.
    [[nodiscard]] value_type &get() &noexcept { return val_; }
    /// \brief	Returns a reference to the underlying value (rvalue overload).
    [[nodiscard]] value_type &&get() &&noexcept {
      return static_cast<value_type &&>(val_);
    }
    /// \brief	Returns a reference to the underlying value (const overload).
    [[nodiscard]] const value_type &get() const &noexcept { return val_; }

    /// \brief	Returns a reference to the underlying value.
    [[nodiscard]] value_type &operator*() &noexcept { return val_; }
    /// \brief	Returns a reference to the underlying value (rvalue overload).
    [[nodiscard]] value_type &&operator*() &&noexcept {
      return static_cast<value_type &&>(val_);
    }
    /// \brief	Returns a reference to the underlying value (const overload).
    [[nodiscard]] const value_type &operator*() const &noexcept { return val_; }

    /// \brief	Returns a reference to the underlying value.
    [[nodiscard]] explicit operator value_type &() &noexcept { return val_; }
    /// \brief	Returns a reference to the underlying value (rvalue overload).
    [[nodiscard]] explicit operator value_type &&() &&noexcept {
      return static_cast<value_type &&>(val_);
    }
    /// \brief	Returns a reference to the underlying value (const overload).
    [[nodiscard]] explicit operator const value_type &() const &noexcept {
      return val_;
    }

    /// @}

    /// \name Metadata
    /// @{

    /// \brief	Returns the metadata flags associated with this value.
    [[nodiscard]] value_flags flags() const noexcept { return flags_; }

    /// \brief	Sets the metadata flags associated with this value.
    /// \returns A reference to the value object.
    value &flags(value_flags new_flags) noexcept {
      flags_ = new_flags;
      return *this;
    }

    /// @}

    /// \brief	Prints the value out to a stream as formatted TOML.
    template <typename Char, typename T>
    friend std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &lhs,
                                                const value<T> &rhs);
    // implemented in toml_default_formatter.h

    /// \brief	Value-assignment operator.
    value &operator=(value_arg rhs) noexcept {
      if constexpr (std::is_same_v<value_type, std::string>)
        val_.assign(rhs);
      else
        val_ = rhs;
      return *this;
    }

    template <typename T = value_type,
              typename = std::enable_if_t<std::is_same_v<T, std::string>>>
    value &operator=(std::string &&rhs) noexcept {
      val_ = std::move(rhs);
      return *this;
    }

    /// \name Equality
    /// @{

    /// \brief	Value equality operator.
    [[nodiscard]] friend bool operator==(const value &lhs, value_arg rhs) noexcept {
      if constexpr (std::is_same_v<value_type, double>) {
        const auto lhs_class = impl::fpclassify(lhs.val_);
        const auto rhs_class = impl::fpclassify(rhs);
        if (lhs_class == impl::fp_class::nan && rhs_class == impl::fp_class::nan)
          return true;
        if ((lhs_class == impl::fp_class::nan) != (rhs_class == impl::fp_class::nan))
          return false;
      }
      return lhs.val_ == rhs;
    }
    TOML_ASYMMETRICAL_EQUALITY_OPS(const value &, value_arg, );

    /// \brief	Value less-than operator.
    [[nodiscard]] friend bool operator<(const value &lhs, value_arg rhs) noexcept {
      return lhs.val_ < rhs;
    }
    /// \brief	Value less-than operator.
    [[nodiscard]] friend bool operator<(value_arg lhs, const value &rhs) noexcept {
      return lhs < rhs.val_;
    }
    /// \brief	Value less-than-or-equal-to operator.
    [[nodiscard]] friend bool operator<=(const value &lhs, value_arg rhs) noexcept {
      return lhs.val_ <= rhs;
    }
    /// \brief	Value less-than-or-equal-to operator.
    [[nodiscard]] friend bool operator<=(value_arg lhs, const value &rhs) noexcept {
      return lhs <= rhs.val_;
    }

    /// \brief	Value greater-than operator.
    [[nodiscard]] friend bool operator>(const value &lhs, value_arg rhs) noexcept {
      return lhs.val_ > rhs;
    }
    /// \brief	Value greater-than operator.
    [[nodiscard]] friend bool operator>(value_arg lhs, const value &rhs) noexcept {
      return lhs > rhs.val_;
    }
    /// \brief	Value greater-than-or-equal-to operator.
    [[nodiscard]] friend bool operator>=(const value &lhs, value_arg rhs) noexcept {
      return lhs.val_ >= rhs;
    }
    /// \brief	Value greater-than-or-equal-to operator.
    [[nodiscard]] friend bool operator>=(value_arg lhs, const value &rhs) noexcept {
      return lhs >= rhs.val_;
    }

    /// \brief	Equality operator.
    ///
    /// \param 	lhs	The LHS value.
    /// \param 	rhs	The RHS value.
    ///
    /// \returns	True if the values were of the same type and contained the same
    /// value.
    template <typename T>
    [[nodiscard]] friend bool operator==(const value &lhs,
                                         const value<T> &rhs) noexcept {
      if constexpr (std::is_same_v<value_type, T>)
        return lhs ==
               rhs.val_; // calls asymmetrical value-equality operator defined above
      else
        return false;
    }

    /// \brief	Inequality operator.
    ///
    /// \param 	lhs	The LHS value.
    /// \param 	rhs	The RHS value.
    ///
    /// \returns	True if the values were not of the same type, or did not contain
    /// the same value.
    template <typename T>
    [[nodiscard]] friend bool operator!=(const value &lhs,
                                         const value<T> &rhs) noexcept {
      return !(lhs == rhs);
    }

    /// \brief	Less-than operator.
    ///
    /// \param 	lhs	The LHS toml::value.
    /// \param 	rhs	The RHS toml::value.
    ///
    /// \returns	\conditional_return{Same value types}
    ///				`lhs.get() < rhs.get()`
    /// 			\conditional_return{Different value types}
    ///				`lhs.type() < rhs.type()`
    template <typename T>
    [[nodiscard]] friend bool operator<(const value &lhs,
                                        const value<T> &rhs) noexcept {
      if constexpr (std::is_same_v<value_type, T>)
        return lhs.val_ < rhs.val_;
      else
        return impl::node_type_of<value_type> < impl::node_type_of<T>;
    }

    /// \brief	Less-than-or-equal-to operator.
    ///
    /// \param 	lhs	The LHS toml::value.
    /// \param 	rhs	The RHS toml::value.
    ///
    /// \returns	\conditional_return{Same value types}
    ///				`lhs.get() <= rhs.get()`
    /// 			\conditional_return{Different value types}
    ///				`lhs.type() <= rhs.type()`
    template <typename T>
    [[nodiscard]] friend bool operator<=(const value &lhs,
                                         const value<T> &rhs) noexcept {
      if constexpr (std::is_same_v<value_type, T>)
        return lhs.val_ <= rhs.val_;
      else
        return impl::node_type_of<value_type> <= impl::node_type_of<T>;
    }

    /// \brief	Greater-than operator.
    ///
    /// \param 	lhs	The LHS toml::value.
    /// \param 	rhs	The RHS toml::value.
    ///
    /// \returns	\conditional_return{Same value types}
    ///				`lhs.get() > rhs.get()`
    /// 			\conditional_return{Different value types}
    ///				`lhs.type() > rhs.type()`
    template <typename T>
    [[nodiscard]] friend bool operator>(const value &lhs,
                                        const value<T> &rhs) noexcept {
      if constexpr (std::is_same_v<value_type, T>)
        return lhs.val_ > rhs.val_;
      else
        return impl::node_type_of<value_type> > impl::node_type_of<T>;
    }

    /// \brief	Greater-than-or-equal-to operator.
    ///
    /// \param 	lhs	The LHS toml::value.
    /// \param 	rhs	The RHS toml::value.
    ///
    /// \returns	\conditional_return{Same value types}
    ///				`lhs.get() >= rhs.get()`
    /// 			\conditional_return{Different value types}
    ///				`lhs.type() >= rhs.type()`
    template <typename T>
    [[nodiscard]] friend bool operator>=(const value &lhs,
                                         const value<T> &rhs) noexcept {
      if constexpr (std::is_same_v<value_type, T>)
        return lhs.val_ >= rhs.val_;
      else
        return impl::node_type_of<value_type> >= impl::node_type_of<T>;
    }

    /// @}
  };

  /// \cond
  template <typename T>
  value(T) -> value<impl::native_type_of<impl::remove_cvref_t<T>>>;

  TOML_PUSH_WARNINGS;
  TOML_DISABLE_INIT_WARNINGS;
  TOML_DISABLE_SWITCH_WARNINGS;

#if !TOML_HEADER_ONLY
  extern template class TOML_API value<std::string>;
  extern template class TOML_API value<int64_t>;
  extern template class TOML_API value<double>;
  extern template class TOML_API value<bool>;
  extern template class TOML_API value<date>;
  extern template class TOML_API value<time>;
  extern template class TOML_API value<date_time>;
#endif

  template <typename T>
  [[nodiscard]] inline decltype(auto) node::get_value_exact() const noexcept {
    using namespace impl;

    static_assert(node_type_of<T> != node_type::none);
    static_assert(node_type_of<T> != node_type::table);
    static_assert(node_type_of<T> != node_type::array);
    static_assert(is_native<T> || can_represent_native<T>);
    static_assert(!is_cvref<T>);
    TOML_ASSERT(this->type() == node_type_of<T>);

    if constexpr (node_type_of<T> == node_type::string) {
      const auto &str = *ref_cast<std::string>();
      if constexpr (std::is_same_v<T, std::string>)
        return str;
      else if constexpr (std::is_same_v<T, std::string_view>)
        return T{str};
      else if constexpr (std::is_same_v<T, const char *>)
        return str.c_str();

      else if constexpr (std::is_same_v<T, std::wstring>) {
#if TOML_WINDOWS_COMPAT
        return widen(str);
#else
        static_assert(dependent_false<T>, "Evaluated unreachable branch!");
#endif
      }

#if TOML_HAS_CHAR8

      // char -> char8_t (potentially unsafe - the feature is 'experimental'!)
      else if constexpr (is_one_of<T, std::u8string, std::u8string_view>)
        return T(reinterpret_cast<const char8_t *>(str.c_str()), str.length());
      else if constexpr (std::is_same_v<T, const char8_t *>)
        return reinterpret_cast<const char8_t *>(str.c_str());
      else
        static_assert(dependent_false<T>, "Evaluated unreachable branch!");

#endif
    } else
      return static_cast<T>(*ref_cast<native_type_of<T>>());
  }

  template <typename T> inline optional<T> node::value_exact() const noexcept {
    using namespace impl;

    static_assert(
        !is_wide_string<T> || TOML_WINDOWS_COMPAT,
        "Retrieving values as wide-character strings with node::value_exact() is only "
        "supported on Windows with TOML_WINDOWS_COMPAT enabled.");

    static_assert(
        (is_native<T> || can_represent_native<T>)&&!is_cvref<T>,
        TOML_SA_VALUE_EXACT_FUNC_MESSAGE("return type of node::value_exact()"));

    // prevent additional compiler error spam when the static_assert fails by gating
    // behind if constexpr
    if constexpr ((is_native<T> || can_represent_native<T>)&&!is_cvref<T>) {
      if (type() == node_type_of<T>)
        return {this->get_value_exact<T>()};
      else
        return {};
    }
  }

  template <typename T> inline optional<T> node::value() const noexcept {
    using namespace impl;

    static_assert(
        !is_wide_string<T> || TOML_WINDOWS_COMPAT,
        "Retrieving values as wide-character strings with node::value() is only "
        "supported on Windows with TOML_WINDOWS_COMPAT enabled.");
    static_assert((is_native<T> || can_represent_native<T> ||
                   can_partially_represent_native<T>)&&!is_cvref<T>,
                  TOML_SA_VALUE_FUNC_MESSAGE("return type of node::value()"));

    // when asking for strings, dates, times and date_times there's no 'fuzzy'
    // conversion semantics to be mindful of so the exact retrieval is enough.
    if constexpr (is_natively_one_of<T, std::string, time, date, date_time>) {
      if (type() == node_type_of<T>)
        return {this->get_value_exact<T>()};
      else
        return {};
    }

    // everything else requires a bit of logicking.
    else {
      switch (type()) {
      // int -> *
      case node_type::integer: {
        // int -> int
        if constexpr (is_natively_one_of<T, int64_t>) {
          if constexpr (is_native<T> || can_represent_native<T>)
            return static_cast<T>(*ref_cast<int64_t>());
          else
            return node_integer_cast<T>(*ref_cast<int64_t>());
        }

        // int -> float
        else if constexpr (is_natively_one_of<T, double>) {
          const int64_t val = *ref_cast<int64_t>();
          if constexpr (std::numeric_limits<T>::digits < 64) {
            constexpr auto largest_whole_float =
                (int64_t{1} << std::numeric_limits<T>::digits);
            if (val < -largest_whole_float || val > largest_whole_float)
              return {};
          }
          return static_cast<T>(val);
        }

        // int -> bool
        else if constexpr (is_natively_one_of<T, bool>)
          return static_cast<bool>(*ref_cast<int64_t>());

        // int -> anything else
        else
          return {};
      }

      // float -> *
      case node_type::floating_point: {
        // float -> float
        if constexpr (is_natively_one_of<T, double>) {
          if constexpr (is_native<T> || can_represent_native<T>)
            return {static_cast<T>(*ref_cast<double>())};
          else {
            const double val = *ref_cast<double>();
            if (impl::fpclassify(val) == fp_class::ok &&
                (val < (std::numeric_limits<T>::lowest)() ||
                 val > (std::numeric_limits<T>::max)()))
              return {};
            return {static_cast<T>(val)};
          }
        }

        // float -> int
        else if constexpr (is_natively_one_of<T, int64_t>) {
          const double val = *ref_cast<double>();
          if (impl::fpclassify(val) == fp_class::ok &&
              static_cast<double>(static_cast<int64_t>(val)) == val)
            return node_integer_cast<T>(static_cast<int64_t>(val));
          else
            return {};
        }

        // float -> anything else
        else
          return {};
      }

      // bool -> *
      case node_type::boolean: {
        // bool -> bool
        if constexpr (is_natively_one_of<T, bool>)
          return {*ref_cast<bool>()};

        // bool -> int
        else if constexpr (is_natively_one_of<T, int64_t>)
          return {static_cast<T>(*ref_cast<bool>())};

        // bool -> anything else
        else
          return {};
      }
      }

      // non-values, or 'exact' types covered above
      return {};
    }
  }

  template <typename T> inline auto node::value_or(T && default_value) const noexcept {
    using namespace impl;

    static_assert(
        !is_wide_string<T> || TOML_WINDOWS_COMPAT,
        "Retrieving values as wide-character strings with node::value_or() is only "
        "supported on Windows with TOML_WINDOWS_COMPAT enabled.");

    if constexpr (is_wide_string<T>) {
#if TOML_WINDOWS_COMPAT

      if (type() == node_type::string)
        return widen(*ref_cast<std::string>());
      return std::wstring{static_cast<T &&>(default_value)};

#else

      static_assert(dependent_false<T>, "Evaluated unreachable branch!");

#endif
    } else {
      using value_type = std::conditional_t<
          std::is_pointer_v<std::decay_t<T>>,
          std::add_pointer_t<std::add_const_t<std::remove_pointer_t<std::decay_t<T>>>>,
          std::decay_t<T>>;
      using traits = value_traits<value_type>;

      static_assert(
          traits::is_native || traits::can_represent_native ||
              traits::can_partially_represent_native,
          "The default value type of node::value_or() must be one of:" TOML_SA_LIST_NEW
          "A native TOML value type" TOML_SA_NATIVE_VALUE_TYPE_LIST

              TOML_SA_LIST_NXT "A non-view type capable of losslessly representing a "
          "native TOML value type" TOML_SA_LIST_BEG "std::string"
#if TOML_WINDOWS_COMPAT
          TOML_SA_LIST_SEP "std::wstring"
#endif
          TOML_SA_LIST_SEP "any signed integer type >= 64 bits" TOML_SA_LIST_SEP
          "any floating-point type >= 64 bits" TOML_SA_LIST_END

              TOML_SA_LIST_NXT "A non-view type capable of (reasonably) representing a "
          "native TOML value type" TOML_SA_LIST_BEG
          "any other integer type" TOML_SA_LIST_SEP
          "any floating-point type >= 32 bits" TOML_SA_LIST_END

              TOML_SA_LIST_NXT "A compatible view type" TOML_SA_LIST_BEG
          "std::string_view"
#if TOML_HAS_CHAR8
          TOML_SA_LIST_SEP "std::u8string_view"
#endif
#if TOML_WINDOWS_COMPAT
          TOML_SA_LIST_SEP "std::wstring_view"
#endif
          TOML_SA_LIST_SEP "const char*"
#if TOML_HAS_CHAR8
          TOML_SA_LIST_SEP "const char8_t*"
#endif
#if TOML_WINDOWS_COMPAT
          TOML_SA_LIST_SEP "const wchar_t*"
#endif
          TOML_SA_LIST_END);

      // prevent additional compiler error spam when the static_assert fails by gating
      // behind if constexpr
      if constexpr (traits::is_native || traits::can_represent_native ||
                    traits::can_partially_represent_native) {
        if constexpr (traits::is_native) {
          if (type() == node_type_of<value_type>)
            return *ref_cast<typename traits::native_type>();
        }
        if (auto val = this->value<value_type>())
          return *val;
        if constexpr (std::is_pointer_v<value_type>)
          return value_type{default_value};
        else
          return static_cast<T &&>(default_value);
      }
    }
  }

#if !TOML_HEADER_ONLY

#define TOML_EXTERN(name, T)                                                           \
  extern template TOML_API optional<T> node::name<T>() const noexcept
  TOML_EXTERN(value_exact, std::string_view);
  TOML_EXTERN(value_exact, std::string);
  TOML_EXTERN(value_exact, const char *);
  TOML_EXTERN(value_exact, int64_t);
  TOML_EXTERN(value_exact, double);
  TOML_EXTERN(value_exact, date);
  TOML_EXTERN(value_exact, time);
  TOML_EXTERN(value_exact, date_time);
  TOML_EXTERN(value_exact, bool);
  TOML_EXTERN(value, std::string_view);
  TOML_EXTERN(value, std::string);
  TOML_EXTERN(value, const char *);
  TOML_EXTERN(value, signed char);
  TOML_EXTERN(value, signed short);
  TOML_EXTERN(value, signed int);
  TOML_EXTERN(value, signed long);
  TOML_EXTERN(value, signed long long);
  TOML_EXTERN(value, unsigned char);
  TOML_EXTERN(value, unsigned short);
  TOML_EXTERN(value, unsigned int);
  TOML_EXTERN(value, unsigned long);
  TOML_EXTERN(value, unsigned long long);
  TOML_EXTERN(value, double);
  TOML_EXTERN(value, float);
  TOML_EXTERN(value, date);
  TOML_EXTERN(value, time);
  TOML_EXTERN(value, date_time);
  TOML_EXTERN(value, bool);
#if TOML_HAS_CHAR8
  TOML_EXTERN(value_exact, std::u8string_view);
  TOML_EXTERN(value_exact, std::u8string);
  TOML_EXTERN(value_exact, const char8_t *);
  TOML_EXTERN(value, std::u8string_view);
  TOML_EXTERN(value, std::u8string);
  TOML_EXTERN(value, const char8_t *);
#endif
#if TOML_WINDOWS_COMPAT
  TOML_EXTERN(value_exact, std::wstring);
  TOML_EXTERN(value, std::wstring);
#endif
#undef TOML_EXTERN

#endif // !TOML_HEADER_ONLY

  TOML_POP_WARNINGS; // TOML_DISABLE_INIT_WARNINGS, TOML_DISABLE_SWITCH_WARNINGS
                     /// \endcond
}
TOML_NAMESPACE_END;

TOML_POP_WARNINGS; // TOML_DISABLE_ARITHMETIC_WARNINGS
