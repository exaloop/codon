//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT

#pragma once
#include "toml_common.h"

#if defined(DOXYGEN) || TOML_SIMPLE_STATIC_ASSERT_MESSAGES

#define TOML_SA_NEWLINE " "
#define TOML_SA_LIST_SEP ", "
#define TOML_SA_LIST_BEG " ("
#define TOML_SA_LIST_END ")"
#define TOML_SA_LIST_NEW " "
#define TOML_SA_LIST_NXT ", "

#else

#define TOML_SA_NEWLINE "\n| "
#define TOML_SA_LIST_SEP TOML_SA_NEWLINE "  - "
#define TOML_SA_LIST_BEG TOML_SA_LIST_SEP
#define TOML_SA_LIST_END
#define TOML_SA_LIST_NEW TOML_SA_NEWLINE TOML_SA_NEWLINE
#define TOML_SA_LIST_NXT TOML_SA_LIST_NEW

#endif

#define TOML_SA_NATIVE_VALUE_TYPE_LIST                                                 \
  TOML_SA_LIST_BEG "std::string" TOML_SA_LIST_SEP "int64_t" TOML_SA_LIST_SEP           \
                   "double" TOML_SA_LIST_SEP "bool" TOML_SA_LIST_SEP                   \
                   "toml::date" TOML_SA_LIST_SEP "toml::time" TOML_SA_LIST_SEP         \
                   "toml::date_time" TOML_SA_LIST_END

#define TOML_SA_NODE_TYPE_LIST                                                         \
  TOML_SA_LIST_BEG                                                                     \
  "toml::table" TOML_SA_LIST_SEP "toml::array" TOML_SA_LIST_SEP                        \
  "toml::value<std::string>" TOML_SA_LIST_SEP "toml::value<int64_t>" TOML_SA_LIST_SEP  \
  "toml::value<double>" TOML_SA_LIST_SEP "toml::value<bool>" TOML_SA_LIST_SEP          \
  "toml::value<toml::date>" TOML_SA_LIST_SEP                                           \
  "toml::value<toml::time>" TOML_SA_LIST_SEP                                           \
  "toml::value<toml::date_time>" TOML_SA_LIST_END

#define TOML_SA_UNWRAPPED_NODE_TYPE_LIST                                               \
  TOML_SA_LIST_NEW "A native TOML value type" TOML_SA_NATIVE_VALUE_TYPE_LIST           \
                                                                                       \
      TOML_SA_LIST_NXT "A TOML node type" TOML_SA_NODE_TYPE_LIST

TOML_NAMESPACE_START {
  /// \brief	A TOML node.
  ///
  /// \detail A parsed TOML document forms a tree made up of tables, arrays and values.
  /// 		This type is the base of each of those, providing a lot of the
  /// polymorphic plumbing.
  class TOML_ABSTRACT_BASE TOML_API node {
  private:
    friend class TOML_PARSER_TYPENAME;
    source_region source_{};

    /// \cond

    template <typename T> [[nodiscard]] decltype(auto) get_value_exact() const noexcept;

    template <typename T, typename N>
    [[nodiscard]] TOML_ATTR(pure) static decltype(auto) do_ref(N &&n) noexcept {
      using type = impl::unwrap_node<T>;
      static_assert((impl::is_native<type> ||
                     impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>,
                    "The template type argument of node::ref() must be one "
                    "of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);
      TOML_ASSERT(n.template is<T>() &&
                  "template type argument T provided to toml::node::ref() didn't match "
                  "the node's actual type");
      if constexpr (impl::is_native<type>)
        return static_cast<N &&>(n).template ref_cast<type>().get();
      else
        return static_cast<N &&>(n).template ref_cast<type>();
    }

    /// \endcond

  protected:
    node() noexcept = default;
    node(const node &) noexcept;
    node(node &&) noexcept;
    node &operator=(const node &) noexcept;
    node &operator=(node &&) noexcept;

    template <typename T>
    [[nodiscard]] TOML_ALWAYS_INLINE
    TOML_ATTR(pure) impl::wrap_node<T> &ref_cast() &noexcept {
      return *reinterpret_cast<impl::wrap_node<T> *>(this);
    }

    template <typename T>
    [[nodiscard]] TOML_ALWAYS_INLINE
    TOML_ATTR(pure) impl::wrap_node<T> &&ref_cast() &&noexcept {
      return std::move(*reinterpret_cast<impl::wrap_node<T> *>(this));
    }

    template <typename T>
    [[nodiscard]] TOML_ALWAYS_INLINE
    TOML_ATTR(pure) const impl::wrap_node<T> &ref_cast() const &noexcept {
      return *reinterpret_cast<const impl::wrap_node<T> *>(this);
    }

    template <typename N, typename T>
    using ref_cast_type = decltype(std::declval<N>().template ref_cast<T>());

  public:
    virtual ~node() noexcept = default;

    /// \name Type checks
    /// @{

#if defined(DOXYGEN) || !TOML_ICC || TOML_ICC_CL

    /// \brief	Returns the node's type identifier.
    [[nodiscard]] virtual node_type type() const noexcept = 0;

#else

    [[nodiscard]] virtual node_type type() const noexcept {
      // Q: "what the fuck?"
      // A: https://github.com/marzer/tomlplusplus/issues/83
      //    tl,dr: go home ICC, you're drunk.

      return type();
    }

#endif

    /// \brief	Returns true if this node is a table.
    [[nodiscard]] virtual bool is_table() const noexcept = 0;
    /// \brief	Returns true if this node is an array.
    [[nodiscard]] virtual bool is_array() const noexcept = 0;
    /// \brief	Returns true if this node is a value.
    [[nodiscard]] virtual bool is_value() const noexcept = 0;

    /// \brief	Returns true if this node is a string value.
    [[nodiscard]] virtual bool is_string() const noexcept;
    /// \brief	Returns true if this node is an integer value.
    [[nodiscard]] virtual bool is_integer() const noexcept;
    /// \brief	Returns true if this node is an floating-point value.
    [[nodiscard]] virtual bool is_floating_point() const noexcept;
    /// \brief	Returns true if this node is an integer or floating-point value.
    [[nodiscard]] virtual bool is_number() const noexcept;
    /// \brief	Returns true if this node is a boolean value.
    [[nodiscard]] virtual bool is_boolean() const noexcept;
    /// \brief	Returns true if this node is a local date value.
    [[nodiscard]] virtual bool is_date() const noexcept;
    /// \brief	Returns true if this node is a local time value.
    [[nodiscard]] virtual bool is_time() const noexcept;
    /// \brief	Returns true if this node is a date-time value.
    [[nodiscard]] virtual bool is_date_time() const noexcept;
    /// \brief	Returns true if this node is an array containing only tables.
    [[nodiscard]] virtual bool is_array_of_tables() const noexcept;

    /// \brief	Checks if a node is a specific type.
    ///
    /// \tparam	T	A TOML node or value type.
    ///
    /// \returns	Returns true if this node is an instance of the specified type.
    template <typename T> [[nodiscard]] TOML_ATTR(pure) bool is() const noexcept {
      using type = impl::unwrap_node<T>;
      static_assert((impl::is_native<type> ||
                     impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>,
                    "The template type argument of node::is() must be one "
                    "of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);

      if constexpr (std::is_same_v<type, table>)
        return is_table();
      else if constexpr (std::is_same_v<type, array>)
        return is_array();
      else if constexpr (std::is_same_v<type, std::string>)
        return is_string();
      else if constexpr (std::is_same_v<type, int64_t>)
        return is_integer();
      else if constexpr (std::is_same_v<type, double>)
        return is_floating_point();
      else if constexpr (std::is_same_v<type, bool>)
        return is_boolean();
      else if constexpr (std::is_same_v<type, date>)
        return is_date();
      else if constexpr (std::is_same_v<type, time>)
        return is_time();
      else if constexpr (std::is_same_v<type, date_time>)
        return is_date_time();
    }

    /// \brief	Checks if a node contains values/elements of only one type.
    ///
    /// \detail \cpp
    /// auto cfg = toml::parse("arr = [ 1, 2, 3, 4.0 ]");
    /// toml::array& arr = *cfg["arr"].as_array();
    ///
    /// toml::node* nonmatch{};
    /// if (arr.is_homogeneous(toml::node_type::integer, nonmatch))
    /// 	std::cout << "array was homogeneous"sv << "\n";
    /// else
    /// 	std::cout << "array was not homogeneous!\n"
    /// 	<< "first non-match was a "sv << nonmatch->type() << " at " <<
    /// nonmatch->source() << "\n"; \ecpp
    ///
    /// \out
    /// array was not homogeneous!
    ///	first non-match was a floating-point at line 1, column 18
    /// \eout
    ///
    /// \param	ntype	A TOML node type. <br>
    /// 				\conditional_return{toml::node_type::none}
    ///					"is every element the same type?"
    /// 				\conditional_return{Anything else}
    ///					"is every element one of these?"
    ///
    /// \param first_nonmatch	Reference to a pointer in which the address of the first
    /// non-matching element 						will be stored
    /// if the return value is false.
    ///
    /// \returns	True if the node was homogeneous.
    ///
    /// \remarks	Always returns `false` for empty tables and arrays.
    [[nodiscard]] virtual bool is_homogeneous(node_type ntype,
                                              node *&first_nonmatch) noexcept = 0;

    /// \brief	Checks if a node contains values/elements of only one type (const
    /// overload).
    [[nodiscard]] virtual bool
    is_homogeneous(node_type ntype, const node *&first_nonmatch) const noexcept = 0;

    /// \brief	Checks if the node contains values/elements of only one type.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 1, 2, 3 };
    /// std::cout << "homogenous: "sv << arr.is_homogeneous(toml::node_type::none) <<
    /// "\n"; std::cout << "all floats: "sv <<
    /// arr.is_homogeneous(toml::node_type::floating_point) << "\n"; std::cout << "all
    /// arrays: "sv << arr.is_homogeneous(toml::node_type::array) << "\n"; std::cout <<
    /// "all ints:   "sv << arr.is_homogeneous(toml::node_type::integer) << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// homogeneous: true
    /// all floats:  false
    /// all arrays:  false
    /// all ints:    true
    /// \eout
    ///
    /// \param	ntype	A TOML node type. <br>
    /// 				\conditional_return{toml::node_type::none}
    ///					"is every element the same type?"
    /// 				\conditional_return{Anything else}
    ///					"is every element one of these?"
    ///
    /// \returns	True if the node was homogeneous.
    ///
    /// \remarks	Always returns `false` for empty tables and arrays.
    [[nodiscard]] virtual bool is_homogeneous(node_type ntype) const noexcept = 0;

    /// \brief	Checks if the node contains values/elements of only one type.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 1, 2, 3 };
    /// std::cout << "homogenous:   "sv << arr.is_homogeneous() << "\n";
    /// std::cout << "all doubles:  "sv << arr.is_homogeneous<double>() << "\n";
    /// std::cout << "all arrays:   "sv << arr.is_homogeneous<toml::array>() << "\n";
    /// std::cout << "all integers: "sv << arr.is_homogeneous<int64_t>() << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// homogeneous: true
    /// all floats:  false
    /// all arrays:  false
    /// all ints:    true
    /// \eout
    ///
    /// \tparam	ElemType	A TOML node or value type. <br>
    /// 					\conditional_return{Left as `void`}
    ///						"is every element the same type?" <br>
    /// 					\conditional_return{Explicitly
    /// specified}
    ///						"is every element a T?"
    ///
    /// \returns	True if the node was homogeneous.
    ///
    /// \remarks	Always returns `false` for empty tables and arrays.
    template <typename ElemType = void>
    [[nodiscard]] TOML_ATTR(pure) bool is_homogeneous() const noexcept {
      using type = impl::unwrap_node<ElemType>;
      static_assert(std::is_void_v<type> ||
                        ((impl::is_native<type> ||
                          impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>),
                    "The template type argument of node::is_homogeneous() must be void "
                    "or one of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);
      return is_homogeneous(impl::node_type_of<type>);
    }

    /// @}

    /// \name Type casts
    /// @{

    /// \brief	Returns a pointer to the node as a toml::table, if it is one.
    [[nodiscard]] virtual table *as_table() noexcept;
    /// \brief	Returns a pointer to the node as a toml::array, if it is one.
    [[nodiscard]] virtual array *as_array() noexcept;
    /// \brief	Returns a pointer to the node as a toml::value<string>, if it is one.
    [[nodiscard]] virtual toml::value<std::string> *as_string() noexcept;
    /// \brief	Returns a pointer to the node as a toml::value<int64_t>, if it is one.
    [[nodiscard]] virtual toml::value<int64_t> *as_integer() noexcept;
    /// \brief	Returns a pointer to the node as a toml::value<double>, if it is one.
    [[nodiscard]] virtual toml::value<double> *as_floating_point() noexcept;
    /// \brief	Returns a pointer to the node as a toml::value<bool>, if it is one.
    [[nodiscard]] virtual toml::value<bool> *as_boolean() noexcept;
    /// \brief	Returns a pointer to the node as a toml::value<date>, if it is one.
    [[nodiscard]] virtual toml::value<date> *as_date() noexcept;
    /// \brief	Returns a pointer to the node as a toml::value<time>, if it is one.
    [[nodiscard]] virtual toml::value<time> *as_time() noexcept;
    /// \brief	Returns a pointer to the node as a toml::value<date_time>, if it is one.
    [[nodiscard]] virtual toml::value<date_time> *as_date_time() noexcept;

    [[nodiscard]] virtual const table *as_table() const noexcept;
    [[nodiscard]] virtual const array *as_array() const noexcept;
    [[nodiscard]] virtual const toml::value<std::string> *as_string() const noexcept;
    [[nodiscard]] virtual const toml::value<int64_t> *as_integer() const noexcept;
    [[nodiscard]] virtual const toml::value<double> *as_floating_point() const noexcept;
    [[nodiscard]] virtual const toml::value<bool> *as_boolean() const noexcept;
    [[nodiscard]] virtual const toml::value<date> *as_date() const noexcept;
    [[nodiscard]] virtual const toml::value<time> *as_time() const noexcept;
    [[nodiscard]] virtual const toml::value<date_time> *as_date_time() const noexcept;

    /// \brief	Gets a pointer to the node as a more specific node type.
    ///
    /// \details \cpp
    ///
    /// toml::value<int64_t>* int_value = node->as<int64_t>();
    /// toml::table* tbl = node->as<toml::table>();
    /// if (int_value)
    ///		std::cout << "Node is a value<int64_t>\n";
    /// else if (tbl)
    ///		std::cout << "Node is a table\n";
    ///
    ///	// fully-qualified value node types also work (useful for template code):
    ///	toml::value<int64_t>* int_value2 = node->as<toml::value<int64_t>>();
    /// if (int_value2)
    ///		std::cout << "Node is a value<int64_t>\n";
    ///
    /// \ecpp
    ///
    /// \tparam	T	The node type or TOML value type to cast to.
    ///
    /// \returns	A pointer to the node as the given type, or nullptr if it was a
    /// different type.
    template <typename T>
    [[nodiscard]] TOML_ATTR(pure) impl::wrap_node<T> *as() noexcept {
      using type = impl::unwrap_node<T>;
      static_assert((impl::is_native<type> ||
                     impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>,
                    "The template type argument of node::as() must be one "
                    "of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);

      if constexpr (std::is_same_v<type, table>)
        return as_table();
      else if constexpr (std::is_same_v<type, array>)
        return as_array();
      else if constexpr (std::is_same_v<type, std::string>)
        return as_string();
      else if constexpr (std::is_same_v<type, int64_t>)
        return as_integer();
      else if constexpr (std::is_same_v<type, double>)
        return as_floating_point();
      else if constexpr (std::is_same_v<type, bool>)
        return as_boolean();
      else if constexpr (std::is_same_v<type, date>)
        return as_date();
      else if constexpr (std::is_same_v<type, time>)
        return as_time();
      else if constexpr (std::is_same_v<type, date_time>)
        return as_date_time();
    }

    /// \brief	Gets a pointer to the node as a more specific node type (const
    /// overload).
    template <typename T>
    [[nodiscard]] TOML_ATTR(pure) const impl::wrap_node<T> *as() const noexcept {
      using type = impl::unwrap_node<T>;
      static_assert((impl::is_native<type> ||
                     impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>,
                    "The template type argument of node::as() must be one "
                    "of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);

      if constexpr (std::is_same_v<type, table>)
        return as_table();
      else if constexpr (std::is_same_v<type, array>)
        return as_array();
      else if constexpr (std::is_same_v<type, std::string>)
        return as_string();
      else if constexpr (std::is_same_v<type, int64_t>)
        return as_integer();
      else if constexpr (std::is_same_v<type, double>)
        return as_floating_point();
      else if constexpr (std::is_same_v<type, bool>)
        return as_boolean();
      else if constexpr (std::is_same_v<type, date>)
        return as_date();
      else if constexpr (std::is_same_v<type, time>)
        return as_time();
      else if constexpr (std::is_same_v<type, date_time>)
        return as_date_time();
    }

    /// @}

    /// \name Value retrieval
    /// @{

    /// \brief	Gets the value contained by this node.
    ///
    /// \detail This function has 'exact' retrieval semantics; the only return value
    /// types allowed are the 		TOML native value types, or types that can
    /// losslessly represent a native value type (e.g. 		std::wstring on
    /// Windows).
    ///
    /// \tparam	T	One of the native TOML value types, or a type capable of
    /// losslessly representing one.
    ///
    /// \returns	The underlying value if the node was a value of the
    /// 			matching type (or losslessly convertible to it), or an
    /// empty optional.
    ///
    /// \see node::value()
    template <typename T> [[nodiscard]] optional<T> value_exact() const noexcept;

    /// \brief	Gets the value contained by this node.
    ///
    /// \detail This function has 'permissive' retrieval semantics; some value types are
    /// allowed 		to convert to others (e.g. retrieving a boolean as an
    /// integer), and the
    /// specified return value 		type can be any type where a reasonable
    /// conversion from a native TOML value exists 		(e.g. std::wstring on
    /// Windows). If the source value
    /// cannot be represented by 		the destination type, an empty optional
    /// is returned.
    ///
    /// \godbolt{zzG81K}
    ///
    /// \cpp
    /// auto tbl = toml::parse(R"(
    /// 	int	= -10
    /// 	flt	= 25.0
    /// 	pi	= 3.14159
    /// 	bool = false
    /// 	huge = 9223372036854775807
    /// 	str	= "foo"
    /// )"sv);
    ///
    /// const auto print_value_with_typename =
    /// 	[&](std::string_view key, std::string_view type_name, auto* dummy)
    /// 	{
    /// 		std::cout << "- " << std::setw(18) << std::left << type_name;
    /// 		using type = std::remove_pointer_t<decltype(dummy)>;
    /// 		if (std::optional<type> val = tbl.get(key)->value<type>())
    /// 			std::cout << *val << "\n";
    /// 		else
    /// 			std::cout << "n/a\n";
    /// 	};
    ///
    /// #define print_value(key, T) print_value_with_typename(key, #T, (T*)nullptr)
    ///
    /// for (auto key : { "int", "flt", "pi", "bool", "huge", "str" })
    /// {
    /// 	std::cout << tbl[key].type() << " value '" << key << "' as:\n";
    /// 	print_value(key, bool);
    /// 	print_value(key, int);
    /// 	print_value(key, unsigned int);
    /// 	print_value(key, long long);
    /// 	print_value(key, float);
    /// 	print_value(key, double);
    /// 	print_value(key, std::string);
    /// 	print_value(key, std::string_view);
    /// 	print_value(key, const char*);
    /// 	std::cout << "\n";
    /// }
    /// \ecpp
    ///
    /// \out
    /// integer value 'int' as:
    /// - bool              true
    /// - int               -10
    /// - unsigned int      n/a
    /// - long long         -10
    /// - float             -10
    /// - double            -10
    /// - std::string       n/a
    /// - std::string_view  n/a
    /// - const char*       n/a
    ///
    /// floating-point value 'flt' as:
    /// - bool              n/a
    /// - int               25
    /// - unsigned int      25
    /// - long long         25
    /// - float             25
    /// - double            25
    /// - std::string       n/a
    /// - std::string_view  n/a
    /// - const char*       n/a
    ///
    /// floating-point value 'pi' as:
    /// - bool              n/a
    /// - int               n/a
    /// - unsigned int      n/a
    /// - long long         n/a
    /// - float             3.14159
    /// - double            3.14159
    /// - std::string       n/a
    /// - std::string_view  n/a
    /// - const char*       n/a
    ///
    /// boolean value 'bool' as:
    /// - bool              false
    /// - int               0
    /// - unsigned int      0
    /// - long long         0
    /// - float             n/a
    /// - double            n/a
    /// - std::string       n/a
    /// - std::string_view  n/a
    /// - const char*       n/a
    ///
    /// integer value 'huge' as:
    /// - bool              true
    /// - int               n/a
    /// - unsigned int      n/a
    /// - long long         9223372036854775807
    /// - float             n/a
    /// - double            n/a
    /// - std::string       n/a
    /// - std::string_view  n/a
    /// - const char*       n/a
    ///
    /// string value 'str' as:
    /// - bool              n/a
    /// - int               n/a
    /// - unsigned int      n/a
    /// - long long         n/a
    /// - float             n/a
    /// - double            n/a
    /// - std::string       foo
    /// - std::string_view  foo
    /// - const char*       foo
    /// \eout
    ///
    /// \tparam	T	One of the native TOML value types, or a type capable of
    /// converting to one.
    ///
    /// \returns	The underlying value if the node was a value of the matching
    /// type (or convertible to it) 			and within the range of the
    /// output type, or an empty optional.
    ///
    /// \note		If you want strict value retrieval semantics that do not allow
    /// for any type conversions, 			use node::value_exact() instead.
    ///
    /// \see node::value_exact()
    template <typename T> [[nodiscard]] optional<T> value() const noexcept;

    /// \brief	Gets the raw value contained by this node, or a default.
    ///
    /// \tparam	T				Default value type. Must be one of the
    /// native TOML value types, 						or
    /// convertible to it. \param 	default_value	The default value to return if
    /// the node wasn't a value, wasn't the
    /// correct type, or no conversion was possible.
    ///
    /// \returns	The underlying value if the node was a value of the matching
    /// type (or convertible to it) 			and within the range of the
    /// output type, or the provided default.
    ///
    /// \note	This function has the same permissive retrieval semantics as
    /// node::value(). If you want strict 		value retrieval semantics that
    /// do not allow for any type conversions, use node::value_exact() 		instead.
    ///
    /// \see
    /// 	- node::value()
    ///		- node::value_exact()
    template <typename T> [[nodiscard]] auto value_or(T &&default_value) const noexcept;

    // template <typename T>
    //[[nodiscard]]
    // std::vector<T> select_exact() const noexcept;

    // template <typename T>
    //[[nodiscard]]
    // std::vector<T> select() const noexcept;

    /// \brief	Gets a raw reference to a value node's underlying data.
    ///
    /// \warning This function is dangerous if used carelessly and **WILL** break your
    /// code if the
    ///			 chosen value type doesn't match the node's actual type. In
    /// debug builds an assertion 			 will fire when invalid accesses
    /// are
    /// attempted:
    /// \cpp
    ///
    /// auto tbl = toml::parse(R"(
    ///		min = 32
    ///		max = 45
    /// )"sv);
    ///
    /// int64_t& min_ref = tbl.get("min")->ref<int64_t>(); // matching type
    /// double& max_ref = tbl.get("max")->ref<double>();  // mismatched type, hits
    /// assert()
    ///
    /// \ecpp
    ///
    /// \tparam	T	One of the TOML value types.
    ///
    /// \returns	A reference to the underlying data.
    template <typename T>
    [[nodiscard]] TOML_ATTR(pure) impl::unwrap_node<T> &ref() &noexcept {
      return do_ref<T>(*this);
    }

    /// \brief	Gets a raw reference to a value node's underlying data (rvalue
    /// overload).
    template <typename T>
    [[nodiscard]] TOML_ATTR(pure) impl::unwrap_node<T> &&ref() &&noexcept {
      return do_ref<T>(std::move(*this));
    }

    /// \brief	Gets a raw reference to a value node's underlying data (const lvalue
    /// overload).
    template <typename T>
    [[nodiscard]] TOML_ATTR(pure) const impl::unwrap_node<T> &ref() const &noexcept {
      return do_ref<T>(*this);
    }

    /// @}

    /// \name Metadata
    /// @{

    /// \brief	Returns the source region responsible for generating this node during
    /// parsing.
    [[nodiscard]] const source_region &source() const noexcept;

    /// @}

  private:
    /// \cond

    template <typename Func, typename N, typename T>
    static constexpr bool can_visit = std::is_invocable_v<Func, ref_cast_type<N, T>>;

    template <typename Func, typename N>
    static constexpr bool can_visit_any =
        can_visit<Func, N, table> || can_visit<Func, N, array> ||
        can_visit<Func, N, std::string> || can_visit<Func, N, int64_t> ||
        can_visit<Func, N, double> || can_visit<Func, N, bool> ||
        can_visit<Func, N, date> || can_visit<Func, N, time> ||
        can_visit<Func, N, date_time>;

    template <typename Func, typename N>
    static constexpr bool can_visit_all =
        can_visit<Func, N, table> &&can_visit<Func, N, array>
            &&can_visit<Func, N, std::string> &&can_visit<Func, N, int64_t> &&can_visit<
                Func, N, double> &&can_visit<Func, N, bool> &&can_visit<Func, N, date>
                &&can_visit<Func, N, time> &&can_visit<Func, N, date_time>;

    template <typename Func, typename N, typename T>
    static constexpr bool visit_is_nothrow_one =
        !can_visit<Func, N, T> ||
        std::is_nothrow_invocable_v<Func, ref_cast_type<N, T>>;

    template <typename Func, typename N>
    static constexpr bool visit_is_nothrow = visit_is_nothrow_one<Func, N, table>
        &&visit_is_nothrow_one<Func, N, array> &&visit_is_nothrow_one<
            Func, N, std::string> &&visit_is_nothrow_one<Func, N, int64_t> &&
            visit_is_nothrow_one<Func, N, double> &&visit_is_nothrow_one<Func, N, bool>
                &&visit_is_nothrow_one<Func, N, date> &&visit_is_nothrow_one<
                    Func, N, time> &&visit_is_nothrow_one<Func, N, date_time>;

    template <typename Func, typename N, typename T, bool = can_visit<Func, N, T>>
    struct visit_return_type final {
      using type = decltype(std::declval<Func>()(std::declval<ref_cast_type<N, T>>()));
    };
    template <typename Func, typename N, typename T>
    struct visit_return_type<Func, N, T, false> final {
      using type = void;
    };

    template <typename A, typename B>
    using nonvoid = std::conditional_t<std::is_void_v<A>, B, A>;

    //# these functions are static helpers to preserve const and ref categories
    //# (otherwise I'd have to implement them thrice)
    //# ((propagation in C++: a modern horror story))

    template <typename N, typename Func>
    static decltype(auto)
    do_visit(N &&n, Func &&visitor) noexcept(visit_is_nothrow<Func &&, N &&>) {
      static_assert(can_visit_any<Func &&, N &&>,
                    "TOML node visitors must be invocable for at least one of the "
                    "toml::node specializations:" TOML_SA_NODE_TYPE_LIST);

      switch (n.type()) {
      case node_type::table:
        if constexpr (can_visit<Func &&, N &&, table>)
          return static_cast<Func &&>(visitor)(
              static_cast<N &&>(n).template ref_cast<table>());
        break;

      case node_type::array:
        if constexpr (can_visit<Func &&, N &&, array>)
          return static_cast<Func &&>(visitor)(
              static_cast<N &&>(n).template ref_cast<array>());
        break;

      case node_type::string:
        if constexpr (can_visit<Func &&, N &&, std::string>)
          return static_cast<Func &&>(visitor)(
              static_cast<N &&>(n).template ref_cast<std::string>());
        break;

      case node_type::integer:
        if constexpr (can_visit<Func &&, N &&, int64_t>)
          return static_cast<Func &&>(visitor)(
              static_cast<N &&>(n).template ref_cast<int64_t>());
        break;

      case node_type::floating_point:
        if constexpr (can_visit<Func &&, N &&, double>)
          return static_cast<Func &&>(visitor)(
              static_cast<N &&>(n).template ref_cast<double>());
        break;

      case node_type::boolean:
        if constexpr (can_visit<Func &&, N &&, bool>)
          return static_cast<Func &&>(visitor)(
              static_cast<N &&>(n).template ref_cast<bool>());
        break;

      case node_type::date:
        if constexpr (can_visit<Func &&, N &&, date>)
          return static_cast<Func &&>(visitor)(
              static_cast<N &&>(n).template ref_cast<date>());
        break;

      case node_type::time:
        if constexpr (can_visit<Func &&, N &&, time>)
          return static_cast<Func &&>(visitor)(
              static_cast<N &&>(n).template ref_cast<time>());
        break;

      case node_type::date_time:
        if constexpr (can_visit<Func &&, N &&, date_time>)
          return static_cast<Func &&>(visitor)(
              static_cast<N &&>(n).template ref_cast<date_time>());
        break;

      case node_type::none:
        TOML_UNREACHABLE;
        TOML_NO_DEFAULT_CASE;
      }

      if constexpr (can_visit_all<Func &&, N &&>)
        TOML_UNREACHABLE;
      else {
        using return_type = nonvoid<
            typename visit_return_type<Func &&, N &&, table>::type,
            nonvoid<
                typename visit_return_type<Func &&, N &&, array>::type,
                nonvoid<
                    typename visit_return_type<Func &&, N &&, std::string>::type,
                    nonvoid<
                        typename visit_return_type<Func &&, N &&, int64_t>::type,
                        nonvoid<typename visit_return_type<Func &&, N &&, double>::type,
                                nonvoid<typename visit_return_type<Func &&, N &&,
                                                                   bool>::type,
                                        nonvoid<typename visit_return_type<
                                                    Func &&, N &&, date>::type,
                                                nonvoid<typename visit_return_type<
                                                            Func &&, N &&, time>::type,
                                                        typename visit_return_type<
                                                            Func &&, N &&,
                                                            date_time>::type>>>>>>>>;

        if constexpr (!std::is_void_v<return_type>) {
          static_assert(std::is_default_constructible_v<return_type>,
                        "Non-exhaustive visitors must return a default-constructible "
                        "type, or void");
          return return_type{};
        }
      }
    }

    /// \endcond

  public:
    /// \name Visitation
    /// @{

    /// \brief	Invokes a visitor on the node based on the node's concrete type.
    ///
    /// \details Visitation is useful when you expect
    /// 		 a node to be one of a set number of types and need
    /// 		 to handle these types differently. Using `visit()` allows
    /// 		 you to eliminate some of the casting/conversion boilerplate:
    /// \cpp
    ///
    /// node.visit([](auto&& n)
    /// {
    ///		if constexpr (toml::is_string<decltype(n)>)
    ///			do_something_with_a_string(*n)); //n is a
    /// toml::value<std::string> 		else if constexpr
    /// (toml::is_integer<decltype(n)>)
    /// do_something_with_an_int(*n); //n is a
    /// toml::value<int64_t> 		else 			throw std::exception{
    /// "Expected string or integer" };
    /// });
    ///
    /// \ecpp
    ///
    /// \tparam	Func	A callable type invocable with one or more of the
    /// 				toml++ node types.
    ///
    /// \param 	visitor	The visitor object.
    ///
    /// \returns The return value of the visitor.
    /// 		 Can be void. Non-exhaustive visitors must return a
    /// default-constructible type.
    ///
    /// \see https://en.wikipedia.org/wiki/Visitor_pattern
    template <typename Func>
    decltype(auto) visit(Func &&visitor) &noexcept(visit_is_nothrow<Func &&, node &>) {
      return do_visit(*this, static_cast<Func &&>(visitor));
    }

    /// \brief	Invokes a visitor on the node based on the node's concrete type (rvalue
    /// overload).
    template <typename Func>
    decltype(auto)
    visit(Func &&visitor) &&noexcept(visit_is_nothrow<Func &&, node &&>) {
      return do_visit(static_cast<node &&>(*this), static_cast<Func &&>(visitor));
    }

    /// \brief	Invokes a visitor on the node based on the node's concrete type (const
    /// lvalue overload).
    template <typename Func>
    decltype(auto)
    visit(Func &&visitor) const &noexcept(visit_is_nothrow<Func &&, const node &>) {
      return do_visit(*this, static_cast<Func &&>(visitor));
    }

    /// @}

    /// \name Node views
    /// @{

    /// \brief	Creates a node_view pointing to this node.
    [[nodiscard]] explicit operator node_view<node>() noexcept;

    /// \brief	Creates a node_view pointing to this node (const overload).
    [[nodiscard]] explicit operator node_view<const node>() const noexcept;

    /// @}
  };
}
TOML_NAMESPACE_END;
