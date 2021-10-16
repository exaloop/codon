//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT

#pragma once
#include "toml_array.h"

/// \cond
TOML_IMPL_NAMESPACE_START {
  template <bool IsConst> struct table_proxy_pair final {
    using value_type = std::conditional_t<IsConst, const node, node>;

    const std::string &first;
    value_type &second;
  };

  template <bool IsConst> class table_iterator final {
  private:
    template <bool C> friend class table_iterator;
    friend class TOML_NAMESPACE::table;

    using proxy_type = table_proxy_pair<IsConst>;
    using raw_mutable_iterator = string_map<std::unique_ptr<node>>::iterator;
    using raw_const_iterator = string_map<std::unique_ptr<node>>::const_iterator;
    using raw_iterator =
        std::conditional_t<IsConst, raw_const_iterator, raw_mutable_iterator>;

    mutable raw_iterator raw_;
    mutable std::aligned_storage_t<sizeof(proxy_type), alignof(proxy_type)> proxy;
    mutable bool proxy_instantiated = false;

    [[nodiscard]] proxy_type *get_proxy() const noexcept {
      if (!proxy_instantiated) {
        auto p = ::new (static_cast<void *>(&proxy))
            proxy_type{raw_->first, *raw_->second.get()};
        proxy_instantiated = true;
        return p;
      } else
        return TOML_LAUNDER(reinterpret_cast<proxy_type *>(&proxy));
    }

    TOML_NODISCARD_CTOR
    table_iterator(raw_mutable_iterator raw) noexcept : raw_{raw} {}

    template <bool C = IsConst, typename = std::enable_if_t<C>>
    TOML_NODISCARD_CTOR table_iterator(raw_const_iterator raw) noexcept : raw_{raw} {}

  public:
    TOML_NODISCARD_CTOR
    table_iterator() noexcept = default;

    TOML_NODISCARD_CTOR
    table_iterator(const table_iterator &other) noexcept : raw_{other.raw_} {}

    table_iterator &operator=(const table_iterator &rhs) noexcept {
      raw_ = rhs.raw_;
      proxy_instantiated = false;
      return *this;
    }

    using value_type = table_proxy_pair<IsConst>;
    using reference = value_type &;
    using pointer = value_type *;
    using difference_type =
        typename std::iterator_traits<raw_iterator>::difference_type;
    using iterator_category =
        typename std::iterator_traits<raw_iterator>::iterator_category;

    table_iterator &operator++() noexcept // ++pre
    {
      ++raw_;
      proxy_instantiated = false;
      return *this;
    }

    table_iterator operator++(int) noexcept // post++
    {
      table_iterator out{raw_};
      ++raw_;
      proxy_instantiated = false;
      return out;
    }

    table_iterator &operator--() noexcept // --pre
    {
      --raw_;
      proxy_instantiated = false;
      return *this;
    }

    table_iterator operator--(int) noexcept // post--
    {
      table_iterator out{raw_};
      --raw_;
      proxy_instantiated = false;
      return out;
    }

    [[nodiscard]] reference operator*() const noexcept { return *get_proxy(); }

    [[nodiscard]] pointer operator->() const noexcept { return get_proxy(); }

    [[nodiscard]] friend bool operator==(const table_iterator &lhs,
                                         const table_iterator &rhs) noexcept {
      return lhs.raw_ == rhs.raw_;
    }

    [[nodiscard]] friend bool operator!=(const table_iterator &lhs,
                                         const table_iterator &rhs) noexcept {
      return lhs.raw_ != rhs.raw_;
    }

    TOML_DISABLE_WARNINGS;

    template <bool C = IsConst, typename = std::enable_if_t<!C>>
    operator table_iterator<true>() const noexcept {
      return table_iterator<true>{raw_};
    }

    TOML_ENABLE_WARNINGS;
  };

  struct table_init_pair final {
    std::string key;
    std::unique_ptr<node> value;

    template <typename V>
    table_init_pair(std::string &&k, V &&v) noexcept
        : key{std::move(k)}, value{make_node(static_cast<V &&>(v))} {}

    template <typename V>
    table_init_pair(std::string_view k, V &&v) noexcept
        : key{k}, value{make_node(static_cast<V &&>(v))} {}

    template <typename V>
    table_init_pair(const char *k, V &&v) noexcept
        : key{k}, value{make_node(static_cast<V &&>(v))} {}

#if TOML_WINDOWS_COMPAT

    template <typename V>
    table_init_pair(std::wstring &&k, V &&v) noexcept
        : key{narrow(k)}, value{make_node(static_cast<V &&>(v))} {}

    template <typename V>
    table_init_pair(std::wstring_view k, V &&v) noexcept
        : key{narrow(k)}, value{make_node(static_cast<V &&>(v))} {}

    template <typename V>
    table_init_pair(const wchar_t *k, V &&v) noexcept
        : key{narrow(std::wstring_view{k})}, value{make_node(static_cast<V &&>(v))} {}

#endif
  };
}
TOML_IMPL_NAMESPACE_END;
/// \endcond

TOML_NAMESPACE_START {
  /// \brief A BidirectionalIterator for iterating over key-value pairs in a
  /// toml::table.
  using table_iterator = impl::table_iterator<false>;

  /// \brief A BidirectionalIterator for iterating over const key-value pairs in a
  /// toml::table.
  using const_table_iterator = impl::table_iterator<true>;

  /// \brief	A TOML table.
  ///
  /// \remarks The interface of this type is modeled after std::map, with some
  /// 		additional considerations made for the heterogeneous nature of a
  /// 		TOML table, and for the removal of some cruft (the public interface of
  /// 		std::map is, simply, _a hot mess_).
  class TOML_API table final : public node {
  private:
    friend class TOML_PARSER_TYPENAME;

    /// \cond

    impl::string_map<std::unique_ptr<node>> map;
    bool inline_ = false;

#if TOML_LIFETIME_HOOKS
    void lh_ctor() noexcept;
    void lh_dtor() noexcept;
#endif

    table(impl::table_init_pair *, size_t) noexcept;

    /// \endcond

  public:
    /// \brief A BidirectionalIterator for iterating over key-value pairs in a
    /// toml::table.
    using iterator = table_iterator;
    /// \brief A BidirectionalIterator for iterating over const key-value pairs in a
    /// toml::table.
    using const_iterator = const_table_iterator;

    /// \brief	Default constructor.
    TOML_NODISCARD_CTOR
    table() noexcept;

    /// \brief	Copy constructor.
    TOML_NODISCARD_CTOR
    table(const table &) noexcept;

    /// \brief	Move constructor.
    TOML_NODISCARD_CTOR
    table(table &&other) noexcept;

    /// \brief	Copy-assignment operator.
    table &operator=(const table &) noexcept;

    /// \brief	Move-assignment operator.
    table &operator=(table &&rhs) noexcept;

    /// \brief	Destructor.
    ~table() noexcept override;

    /// \brief	Constructs a table with one or more initial key-value pairs.
    ///
    /// \detail \cpp
    /// auto tbl = toml::table{{ // double braces required :( - see remark
    ///		{ "foo", 1 },
    ///		{ "bar", 2.0 },
    ///		{ "kek", "three" }
    ///	}};
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// { foo = 1, bar = 2.0, kek = "three" }
    /// \eout
    ///
    /// \tparam	N	Number of key-value pairs used to initialize the table.
    /// \param 	arr	An array of key-value pairs used to initialize the table.
    ///
    /// \remarks C++ std::initializer_lists represent their member elements as
    /// 		 const even if the list's value type is non-const. This is great
    /// for 		 compiler writers, I guess, but pretty annoying for me since
    /// TOML key-value
    /// pairs are polymorphic (and thus move-only). This means 		 that for the
    /// human-friendly braced init list syntax to work I can't use
    /// std::initializer_list
    /// and must instead invent an annoying proxy type, 		 which means an
    /// extra level of nesting. 		 <br><br> 		 See
    /// https://en.cppreference.com/w/cpp/utility/initializer_list 		 if
    /// you'd like to learn more about this.
    template <size_t N>
    TOML_NODISCARD_CTOR explicit table(impl::table_init_pair(&&arr)[N]) noexcept
        : table{arr, N} {
#if TOML_LIFETIME_HOOKS
      lh_ctor();
#endif
    }

    /// \name Type checks
    /// @{

    [[nodiscard]] node_type type() const noexcept override;
    [[nodiscard]] bool is_table() const noexcept override;
    [[nodiscard]] bool is_array() const noexcept override;
    [[nodiscard]] bool is_value() const noexcept override;
    [[nodiscard]] bool is_homogeneous(node_type ntype) const noexcept override;
    [[nodiscard]] bool is_homogeneous(node_type ntype,
                                      node *&first_nonmatch) noexcept override;
    [[nodiscard]] bool
    is_homogeneous(node_type ntype,
                   const node *&first_nonmatch) const noexcept override;
    template <typename ElemType = void>
    [[nodiscard]] bool is_homogeneous() const noexcept {
      using type = impl::unwrap_node<ElemType>;
      static_assert(std::is_void_v<type> ||
                        ((impl::is_native<type> ||
                          impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>),
                    "The template type argument of table::is_homogeneous() must be "
                    "void or one of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);
      return is_homogeneous(impl::node_type_of<type>);
    }

    /// @}

    /// \name Type casts
    /// @{

    [[nodiscard]] table *as_table() noexcept override;
    [[nodiscard]] const table *as_table() const noexcept override;

    /// @}

    /// \name Metadata
    /// @{

    /// \brief	Returns true if this table is an inline table.
    ///
    /// \remarks Runtime-constructed tables (i.e. those not created during
    /// 		 parsing) are not inline by default.
    [[nodiscard]] bool is_inline() const noexcept;

    /// \brief	Sets whether this table is a TOML inline table.
    ///
    /// \detail \godbolt{an9xdj}
    ///
    /// \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 1 },
    ///		{ "b", 2 },
    ///		{ "c", 3 },
    ///		{ "d", toml::table{{ { "e", 4 } }} }
    ///	}};
    /// std::cout << "is inline? "sv << tbl.is_inline() << "\n";
    /// std::cout << tbl << "\n\n";
    ///
    /// tbl.is_inline(!tbl.is_inline());
    /// std::cout << "is inline? "sv << tbl.is_inline() << "\n";
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// is inline? false
    /// a = 1
    /// b = 2
    /// c = 3
    ///
    /// [d]
    /// e = 4
    ///
    ///
    /// is inline? true
    /// { a = 1, b = 2, c = 3, d = { e = 4 } }
    /// \eout
    ///
    /// \remarks A table being 'inline' is only relevent during printing;
    /// 		 it has no effect on the general functionality of the table
    /// 		 object.
    ///
    /// \param 	val	The new value for 'inline'.
    void is_inline(bool val) noexcept;

    /// @}

    /// \name Node views
    /// @{

    /// \brief	Gets a node_view for the selected key-value pair.
    ///
    /// \param 	key The key used for the lookup.
    ///
    /// \returns	A view of the value at the given key if one existed, or an empty
    /// node view.
    ///
    /// \remarks std::map::operator[]'s behaviour of default-constructing a value at a
    /// key if it 		 didn't exist is a crazy bug factory so I've
    /// deliberately chosen not to emulate it. 		 <strong>This is not an
    /// error.</strong>
    ///
    /// \see toml::node_view
    [[nodiscard]] node_view<node> operator[](std::string_view key) noexcept;

    /// \brief	Gets a node_view for the selected key-value pair (const overload).
    ///
    /// \param 	key The key used for the lookup.
    ///
    /// \returns	A view of the value at the given key if one existed, or an empty
    /// node view.
    ///
    /// \remarks std::map::operator[]'s behaviour of default-constructing a value at a
    /// key if it 		 didn't exist is a crazy bug factory so I've
    /// deliberately chosen not to emulate it. 		 <strong>This is not an
    /// error.</strong>
    ///
    /// \see toml::node_view
    [[nodiscard]] node_view<const node> operator[](std::string_view key) const noexcept;

#if TOML_WINDOWS_COMPAT

    /// \brief	Gets a node_view for the selected key-value pair.
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \param 	key The key used for the lookup.
    ///
    /// \returns	A view of the value at the given key if one existed, or an empty
    /// node view.
    ///
    /// \remarks std::map::operator[]'s behaviour of default-constructing a value at a
    /// key if it 		 didn't exist is a crazy bug factory so I've
    /// deliberately chosen not to emulate it. 		 <strong>This is not an
    /// error.</strong>
    ///
    /// \see toml::node_view
    [[nodiscard]] node_view<node> operator[](std::wstring_view key) noexcept;

    /// \brief	Gets a node_view for the selected key-value pair (const overload).
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \param 	key The key used for the lookup.
    ///
    /// \returns	A view of the value at the given key if one existed, or an empty
    /// node view.
    ///
    /// \remarks std::map::operator[]'s behaviour of default-constructing a value at a
    /// key if it 		 didn't exist is a crazy bug factory so I've
    /// deliberately chosen not to emulate it. 		 <strong>This is not an
    /// error.</strong>
    ///
    /// \see toml::node_view
    [[nodiscard]] node_view<const node>
    operator[](std::wstring_view key) const noexcept;

#endif // TOML_WINDOWS_COMPAT

    /// @}

    /// \name Table operations
    /// @{

    /// \brief	Returns an iterator to the first key-value pair.
    [[nodiscard]] iterator begin() noexcept;
    /// \brief	Returns an iterator to the first key-value pair.
    [[nodiscard]] const_iterator begin() const noexcept;
    /// \brief	Returns an iterator to the first key-value pair.
    [[nodiscard]] const_iterator cbegin() const noexcept;

    /// \brief	Returns an iterator to one-past-the-last key-value pair.
    [[nodiscard]] iterator end() noexcept;
    /// \brief	Returns an iterator to one-past-the-last key-value pair.
    [[nodiscard]] const_iterator end() const noexcept;
    /// \brief	Returns an iterator to one-past-the-last key-value pair.
    [[nodiscard]] const_iterator cend() const noexcept;

    /// \brief	Returns true if the table is empty.
    [[nodiscard]] bool empty() const noexcept;
    /// \brief	Returns the number of key-value pairs in the table.
    [[nodiscard]] size_t size() const noexcept;
    /// \brief	Removes all key-value pairs from the table.
    void clear() noexcept;

    /// \brief	Inserts a new value at a specific key if one did not already exist.
    ///
    /// \detail \godbolt{bMnW5r}
    ///
    /// \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 1 },
    ///		{ "b", 2 },
    ///		{ "c", 3 }
    ///	}};
    /// std::cout << tbl << "\n";
    ///
    /// for (auto k : { "a", "d" })
    /// {
    ///		auto result = tbl.insert(k, 42);
    ///		std::cout << "inserted with key '"sv << k << "': "sv << result.second <<
    ///"\n";
    /// }
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// a = 1
    /// b = 2
    /// c = 3
    ///
    /// inserted with key 'a': false
    /// inserted with key 'd': true
    /// a = 1
    /// b = 2
    /// c = 3
    /// d = 42
    /// \eout
    ///
    /// \tparam KeyType		std::string (or a type convertible to it).
    /// \tparam ValueType	toml::node, toml::node_view, toml::table, toml::array,
    /// or a native TOML value type 					(or a type
    /// promotable to one). \param 	key The key at which to insert the new value.
    /// \param 	val			The new value to insert.
    ///
    /// \returns	\conditional_return{Valid input}
    /// 			<ul>
    /// 				<li>An iterator to the insertion position (or
    /// the position of the value that prevented insertion)
    /// <li>A boolean indicating if the insertion was successful.
    /// 			</ul>
    ///				\conditional_return{Input is an empty toml::node_view}
    /// 			`{ end(), false }`
    ///
    /// \attention The return value will always be `{ end(), false }` if the input value
    /// was an 		   empty toml::node_view, because no insertion can take place.
    /// This is the only circumstance 		   in which this can occur.
    template <typename KeyType, typename ValueType,
              typename = std::enable_if_t<
                  std::is_convertible_v<KeyType &&, std::string_view> ||
                  impl::is_wide_string<KeyType>>>
    std::pair<iterator, bool> insert(KeyType &&key, ValueType &&val) noexcept {
      static_assert(!impl::is_wide_string<KeyType> || TOML_WINDOWS_COMPAT,
                    "Insertion using wide-character keys is only supported on Windows "
                    "with TOML_WINDOWS_COMPAT enabled.");

      if constexpr (is_node_view<ValueType>) {
        if (!val)
          return {end(), false};
      }

      if constexpr (impl::is_wide_string<KeyType>) {
#if TOML_WINDOWS_COMPAT
        return insert(impl::narrow(std::forward<KeyType>(key)),
                      std::forward<ValueType>(val));
#else
        static_assert(impl::dependent_false<KeyType>, "Evaluated unreachable branch!");
#endif
      } else {
        auto ipos = map.lower_bound(key);
        if (ipos == map.end() || ipos->first != key) {
          ipos = map.emplace_hint(ipos, std::forward<KeyType>(key),
                                  impl::make_node(std::forward<ValueType>(val)));
          return {iterator{ipos}, true};
        }
        return {iterator{ipos}, false};
      }
    }

    /// \brief	Inserts a series of key-value pairs into the table.
    ///
    /// \detail \godbolt{bzYcce}
    ///
    /// \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 1 },
    ///		{ "b", 2 },
    ///		{ "c", 3 }
    ///	}};
    /// std::cout << tbl << "\n";
    ///
    /// auto kvps = std::array<std::pair<std::string, int>, 2>{{
    ///		{ "d", 42 },
    ///		{ "a", 43 } // won't be inserted, 'a' already exists
    ///	}};
    ///	tbl.insert(kvps.begin(), kvps.end());
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// a = 1
    /// b = 2
    /// c = 3
    ///
    /// a = 1
    /// b = 2
    /// c = 3
    /// d = 42
    /// \eout
    ///
    /// \tparam Iter	An InputIterator to a collection of key-value pairs.
    /// \param 	first	An iterator to the first value in the input collection.
    /// \param 	last	An iterator to one-past-the-last value in the input collection.
    ///
    /// \remarks This function is morally equivalent to calling `insert(key, value)` for
    /// each 		 key-value pair covered by the iterator range, so any values
    /// with keys already found in the 		 table will not be replaced.
    template <typename Iter, typename = std::enable_if_t<
                                 !std::is_convertible_v<Iter, std::string_view> &&
                                 !impl::is_wide_string<Iter>>>
    void insert(Iter first, Iter last) noexcept {
      if (first == last)
        return;
      for (auto it = first; it != last; it++) {
        if constexpr (std::is_rvalue_reference_v<decltype(*it)>)
          insert(std::move((*it).first), std::move((*it).second));
        else
          insert((*it).first, (*it).second);
      }
    }

    /// \brief	Inserts or assigns a value at a specific key.
    ///
    /// \detail \godbolt{ddK563}
    ///
    /// \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 1 },
    ///		{ "b", 2 },
    ///		{ "c", 3 }
    ///	}};
    /// std::cout << tbl << "\n";
    ///
    /// for (auto k : { "a", "d" })
    /// {
    ///		auto result = tbl.insert_or_assign(k, 42);
    ///		std::cout << "value at key '"sv << k
    ///			<< "' was "sv << (result.second ? "inserted"sv : "assigned"sv)
    ///<<
    ///"\n";
    /// }
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// a = 1
    /// b = 2
    /// c = 3
    ///
    /// value at key 'a' was assigned
    /// value at key 'd' was inserted
    /// a = 42
    /// b = 2
    /// c = 3
    /// d = 42
    /// \eout
    ///
    /// \tparam KeyType		std::string (or a type convertible to it).
    /// \tparam ValueType	toml::node, toml::node_view, toml::table, toml::array,
    /// or a native TOML value type 					(or a type
    /// promotable to one). \param 	key The key at which to insert or assign the
    /// value. \param 	val The value to insert/assign.
    ///
    /// \returns	\conditional_return{Valid input}
    /// 			<ul>
    /// 				<li>An iterator to the value's position
    /// 				<li>`true` if the value was inserted, `false` if
    /// it was assigned.
    /// 			</ul>
    /// 			\conditional_return{Input is an empty toml::node_view}
    /// 			 `{ end(), false }`
    ///
    /// \attention The return value will always be `{ end(), false }` if the input value
    /// was 		   an empty toml::node_view, because no insertion or assignment
    /// can take place. 		   This is the only circumstance in which this
    /// can occur.
    template <typename KeyType, typename ValueType>
    std::pair<iterator, bool> insert_or_assign(KeyType &&key,
                                               ValueType &&val) noexcept {
      static_assert(!impl::is_wide_string<KeyType> || TOML_WINDOWS_COMPAT,
                    "Insertion using wide-character keys is only supported on Windows "
                    "with TOML_WINDOWS_COMPAT enabled.");

      if constexpr (is_node_view<ValueType>) {
        if (!val)
          return {end(), false};
      }

      if constexpr (impl::is_wide_string<KeyType>) {
#if TOML_WINDOWS_COMPAT
        return insert_or_assign(impl::narrow(std::forward<KeyType>(key)),
                                std::forward<ValueType>(val));
#else
        static_assert(impl::dependent_false<KeyType>, "Evaluated unreachable branch!");
#endif
      } else {
        auto ipos = map.lower_bound(key);
        if (ipos == map.end() || ipos->first != key) {
          ipos = map.emplace_hint(ipos, std::forward<KeyType>(key),
                                  impl::make_node(std::forward<ValueType>(val)));
          return {iterator{ipos}, true};
        } else {
          (*ipos).second.reset(impl::make_node(std::forward<ValueType>(val)));
          return {iterator{ipos}, false};
        }
      }
    }

    /// \brief	Emplaces a new value at a specific key if one did not already exist.
    ///
    /// \detail \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 1 },
    ///		{ "b", 2 },
    ///		{ "c", 3 }
    ///	}};
    /// std::cout << tbl << "\n";
    ///
    /// for (auto k : { "a", "d" })
    /// {
    ///		// add a string using std::string's substring constructor
    ///		auto result = tbl.emplace<std::string>(k, "this is not a drill"sv, 14,
    /// 5); 		std::cout << "emplaced with key '"sv << k << "': "sv <<
    /// result.second <<
    ///"\n";
    /// }
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// { a = 1, b = 2, c = 3 }
    /// emplaced with key 'a': false
    /// emplaced with key 'd': true
    /// { a = 1, b = 2, c = 3, d = "drill" }
    /// \eout
    ///
    /// \tparam ValueType	toml::table, toml::array, or any native TOML value type.
    /// \tparam KeyType		std::string (or a type convertible to it).
    /// \tparam ValueArgs	Value constructor argument types.
    /// \param 	key			The key at which to emplace the new value.
    /// \param 	args		Arguments to forward to the value's constructor.
    ///
    /// \returns A std::pair containing: <br>
    /// 		- An iterator to the emplacement position (or the position of
    /// the value that prevented emplacement)
    /// 		- A boolean indicating if the emplacement was successful.
    ///
    /// \remark There is no difference between insert() and emplace() for trivial value
    /// types (floats, ints, bools).
    template <typename ValueType, typename KeyType, typename... ValueArgs>
    std::pair<iterator, bool> emplace(KeyType &&key, ValueArgs &&...args) noexcept {
      static_assert(!impl::is_wide_string<KeyType> || TOML_WINDOWS_COMPAT,
                    "Emplacement using wide-character keys is only supported on "
                    "Windows with TOML_WINDOWS_COMPAT enabled.");

      if constexpr (impl::is_wide_string<KeyType>) {
#if TOML_WINDOWS_COMPAT
        return emplace<ValueType>(impl::narrow(std::forward<KeyType>(key)),
                                  std::forward<ValueArgs>(args)...);
#else
        static_assert(impl::dependent_false<KeyType>, "Evaluated unreachable branch!");
#endif
      } else {

        using type = impl::unwrap_node<ValueType>;
        static_assert((impl::is_native<type> ||
                       impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>,
                      "The emplacement type argument of table::emplace() must be one "
                      "of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);

        auto ipos = map.lower_bound(key);
        if (ipos == map.end() || ipos->first != key) {
          ipos = map.emplace_hint(
              ipos, std::forward<KeyType>(key),
              new impl::wrap_node<type>{std::forward<ValueArgs>(args)...});
          return {iterator{ipos}, true};
        }
        return {iterator{ipos}, false};
      }
    }

    /// \brief	Removes the specified key-value pair from the table.
    ///
    /// \detail \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 1 },
    ///		{ "b", 2 },
    ///		{ "c", 3 }
    ///	}};
    /// std::cout << tbl << "\n";
    ///
    /// tbl.erase(tbl.begin() + 1);
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// { a = 1, b = 2, c = 3 }
    /// { a = 1, c = 3 }
    /// \eout
    ///
    /// \param 	pos		Iterator to the key-value pair being erased.
    ///
    /// \returns Iterator to the first key-value pair immediately following the removed
    /// key-value pair.
    iterator erase(iterator pos) noexcept;

    /// \brief	Removes the specified key-value pair from the table (const iterator
    /// overload).
    ///
    /// \detail \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 1 },
    ///		{ "b", 2 },
    ///		{ "c", 3 }
    ///	}};
    /// std::cout << tbl << "\n";
    ///
    /// tbl.erase(tbl.cbegin() + 1);
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// { a = 1, b = 2, c = 3 }
    /// { a = 1, c = 3 }
    /// \eout
    ///
    /// \param 	pos		Iterator to the key-value pair being erased.
    ///
    /// \returns Iterator to the first key-value pair immediately following the removed
    /// key-value pair.
    iterator erase(const_iterator pos) noexcept;

    /// \brief	Removes the key-value pairs in the range [first, last) from the table.
    ///
    /// \detail \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 1 },
    ///		{ "b", "bad" },
    ///		{ "c", "karma" },
    ///		{ "d", 2 }
    ///	}};
    /// std::cout << tbl << "\n";
    ///
    /// tbl.erase(tbl.cbegin() + 1, tbl.cbegin() + 3);
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// { a = 1, b = "bad", c = "karma", d = 2 }
    /// { a = 1, d = 2 }
    /// \eout
    ///
    /// \param 	first	Iterator to the first key-value pair being erased.
    /// \param 	last	Iterator to the one-past-the-last key-value pair being erased.
    ///
    /// \returns Iterator to the first key-value pair immediately following the last
    /// removed key-value pair.
    iterator erase(const_iterator first, const_iterator last) noexcept;

    /// \brief	Removes the value with the given key from the table.
    ///
    /// \detail \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 1 },
    ///		{ "b", 2 },
    ///		{ "c", 3 }
    ///	}};
    /// std::cout << tbl << "\n";
    ///
    /// std::cout << tbl.erase("b") << "\n";
    /// std::cout << tbl.erase("not an existing key") << "\n";
    /// std::cout << tbl << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// { a = 1, b = 2, c = 3 }
    /// true
    /// false
    /// { a = 1, c = 3 }
    /// \eout
    ///
    /// \param 	key		Key to erase.
    ///
    /// \returns True if any values with matching keys were found and erased.
    bool erase(std::string_view key) noexcept;

#if TOML_WINDOWS_COMPAT

    /// \brief	Removes the value with the given key from the table.
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \param 	key		Key to erase.
    ///
    /// \returns True if any values with matching keys were found and erased.
    bool erase(std::wstring_view key) noexcept;

#endif // TOML_WINDOWS_COMPAT

    /// \brief	Gets an iterator to the node at a specific key.
    ///
    /// \param 	key	The node's key.
    ///
    /// \returns	An iterator to the node at the specified key, or end().
    [[nodiscard]] iterator find(std::string_view key) noexcept;

    /// \brief	Gets an iterator to the node at a specific key (const overload)
    ///
    /// \param 	key	The node's key.
    ///
    /// \returns	A const iterator to the node at the specified key, or cend().
    [[nodiscard]] const_iterator find(std::string_view key) const noexcept;

    /// \brief	Returns true if the table contains a node at the given key.
    [[nodiscard]] bool contains(std::string_view key) const noexcept;

#if TOML_WINDOWS_COMPAT

    /// \brief	Gets an iterator to the node at a specific key.
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \param 	key	The node's key.
    ///
    /// \returns	An iterator to the node at the specified key, or end().
    [[nodiscard]] iterator find(std::wstring_view key) noexcept;

    /// \brief	Gets an iterator to the node at a specific key (const overload).
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \param 	key	The node's key.
    ///
    /// \returns	A const iterator to the node at the specified key, or cend().
    [[nodiscard]] const_iterator find(std::wstring_view key) const noexcept;

    /// \brief	Returns true if the table contains a node at the given key.
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    [[nodiscard]] bool contains(std::wstring_view key) const noexcept;

#endif // TOML_WINDOWS_COMPAT

    /// @}

  private:
    /// \cond

    template <typename Map, typename Key>
    [[nodiscard]] static auto do_get(Map &vals, const Key &key) noexcept
        -> std::conditional_t<std::is_const_v<Map>, const node *, node *> {
      static_assert(!impl::is_wide_string<Key> || TOML_WINDOWS_COMPAT,
                    "Retrieval using wide-character keys is only supported on Windows "
                    "with TOML_WINDOWS_COMPAT enabled.");

      if constexpr (impl::is_wide_string<Key>) {
#if TOML_WINDOWS_COMPAT
        return do_get(vals, impl::narrow(key));
#else
        static_assert(impl::dependent_false<Key>, "Evaluated unreachable branch!");
#endif
      } else {
        if (auto it = vals.find(key); it != vals.end())
          return {it->second.get()};
        return {};
      }
    }

    template <typename T, typename Map, typename Key>
    [[nodiscard]] static auto do_get_as(Map &vals, const Key &key) noexcept {
      const auto node = do_get(vals, key);
      return node ? node->template as<T>() : nullptr;
    }

    template <typename Map, typename Key>
    [[nodiscard]] TOML_ALWAYS_INLINE static bool do_contains(Map &vals,
                                                             const Key &key) noexcept {
      return do_get(vals, key) != nullptr;
    }

    /// \endcond

  public:
    /// \name Value retrieval
    /// @{

    /// \brief	Gets the node at a specific key.
    ///
    /// \detail \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 42, },
    ///		{ "b", "is the meaning of life, apparently." }
    ///	}};
    ///	std::cout << R"(node ["a"] exists: )"sv << !!arr.get("a") << "\n";
    ///	std::cout << R"(node ["b"] exists: )"sv << !!arr.get("b") << "\n";
    ///	std::cout << R"(node ["c"] exists: )"sv << !!arr.get("c") << "\n";
    /// if (auto val = arr.get("a"))
    ///		std::cout << R"(node ["a"] was an )"sv << val->type() << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// node ["a"] exists: true
    /// node ["b"] exists: true
    /// node ["c"] exists: false
    /// node ["a"] was an integer
    /// \eout
    ///
    /// \param 	key	The node's key.
    ///
    /// \returns	A pointer to the node at the specified key, or nullptr.
    [[nodiscard]] node *get(std::string_view key) noexcept;

    /// \brief	Gets the node at a specific key (const overload).
    ///
    /// \param 	key	The node's key.
    ///
    /// \returns	A pointer to the node at the specified key, or nullptr.
    [[nodiscard]] const node *get(std::string_view key) const noexcept;

#if TOML_WINDOWS_COMPAT

    /// \brief	Gets the node at a specific key.
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \param 	key	The node's key.
    ///
    /// \returns	A pointer to the node at the specified key, or nullptr.
    [[nodiscard]] node *get(std::wstring_view key) noexcept;

    /// \brief	Gets the node at a specific key (const overload).
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \param 	key	The node's key.
    ///
    /// \returns	A pointer to the node at the specified key, or nullptr.
    [[nodiscard]] const node *get(std::wstring_view key) const noexcept;

#endif // TOML_WINDOWS_COMPAT

    /// \brief	Gets the node at a specific key if it is a particular type.
    ///
    /// \detail \cpp
    /// auto tbl = toml::table{{
    ///		{ "a", 42, },
    ///		{ "b", "is the meaning of life, apparently." }
    ///	}};
    /// if (auto val = arr.get_as<int64_t>("a"))
    ///		std::cout << R"(node ["a"] was an integer with value )"sv << **val <<
    ///"\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// node ["a"] was an integer with value 42
    /// \eout
    ///
    /// \tparam	ValueType	One of the TOML node or value types.
    /// \param 	key			The node's key.
    ///
    /// \returns	A pointer to the node at the specified key if it was of the
    /// given type, or nullptr.
    template <typename ValueType>
    [[nodiscard]] impl::wrap_node<ValueType> *get_as(std::string_view key) noexcept {
      return do_get_as<ValueType>(map, key);
    }

    /// \brief	Gets the node at a specific key if it is a particular type (const
    /// overload).
    ///
    /// \tparam	ValueType	One of the TOML node or value types.
    /// \param 	key			The node's key.
    ///
    /// \returns	A pointer to the node at the specified key if it was of the
    /// given type, or nullptr.
    template <typename ValueType>
    [[nodiscard]] const impl::wrap_node<ValueType> *
    get_as(std::string_view key) const noexcept {
      return do_get_as<ValueType>(map, key);
    }

#if TOML_WINDOWS_COMPAT

    /// \brief	Gets the node at a specific key if it is a particular type.
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \tparam	ValueType	One of the TOML node or value types.
    /// \param 	key			The node's key.
    ///
    /// \returns	A pointer to the node at the specified key if it was of the
    /// given type, or nullptr.
    template <typename ValueType>
    [[nodiscard]] impl::wrap_node<ValueType> *get_as(std::wstring_view key) noexcept {
      return get_as<ValueType>(impl::narrow(key));
    }

    /// \brief	Gets the node at a specific key if it is a particular type (const
    /// overload).
    ///
    /// \availability This overload is only available when #TOML_WINDOWS_COMPAT is
    /// enabled.
    ///
    /// \tparam	ValueType	One of the TOML node or value types.
    /// \param 	key			The node's key.
    ///
    /// \returns	A pointer to the node at the specified key if it was of the
    /// given type, or nullptr.
    template <typename ValueType>
    [[nodiscard]] const impl::wrap_node<ValueType> *
    get_as(std::wstring_view key) const noexcept {
      return get_as<ValueType>(impl::narrow(key));
    }

#endif // TOML_WINDOWS_COMPAT

    /// @}

    /// \name Equality
    /// @{

    /// \brief	Equality operator.
    ///
    /// \param 	lhs	The LHS table.
    /// \param 	rhs	The RHS table.
    ///
    /// \returns	True if the tables contained the same keys and map.
    friend bool operator==(const table &lhs, const table &rhs) noexcept;

    /// \brief	Inequality operator.
    ///
    /// \param 	lhs	The LHS table.
    /// \param 	rhs	The RHS table.
    ///
    /// \returns	True if the tables did not contain the same keys and map.
    friend bool operator!=(const table &lhs, const table &rhs) noexcept;

    /// \brief	Prints the table out to a stream as formatted TOML.
    template <typename Char>
    friend std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &,
                                                const table &);
    // implemented in toml_default_formatter.h

    /// @}
  };

#ifndef DOXYGEN

  // template <typename T>
  // inline std::vector<T> node::select_exact() const noexcept
  //{
  //	using namespace impl;

  //	static_assert(
  //		!is_wide_string<T> || TOML_WINDOWS_COMPAT,
  //		"Retrieving values as wide-character strings with node::select_exact()
  // is only " 		"supported on Windows with TOML_WINDOWS_COMPAT enabled."
  //	);

  //	static_assert(
  //		(is_native<T> || can_represent_native<T>) && !is_cvref<T>,
  //		TOML_SA_VALUE_EXACT_FUNC_MESSAGE("return type of node::select_exact()")
  //	);
  //}

  // template <typename T>
  // inline std::vector<T> node::select() const noexcept
  //{
  //	using namespace impl;

  //	static_assert(
  //		!is_wide_string<T> || TOML_WINDOWS_COMPAT,
  //		"Retrieving values as wide-character strings with node::select() is only
  //" 		"supported on Windows with TOML_WINDOWS_COMPAT enabled."
  //	);
  //	static_assert(
  //		(is_native<T> || can_represent_native<T> ||
  // can_partially_represent_native<T>) && !is_cvref<T>,
  //		TOML_SA_VALUE_FUNC_MESSAGE("return type of node::select()")
  //	);
  //}

#endif // !DOXYGEN
}
TOML_NAMESPACE_END;
