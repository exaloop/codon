//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT

#pragma once
#include "toml_value.h"

/// \cond
TOML_IMPL_NAMESPACE_START {
  template <bool IsConst> class TOML_TRIVIAL_ABI array_iterator final {
  private:
    template <bool C> friend class array_iterator;
    friend class TOML_NAMESPACE::array;

    using raw_mutable_iterator = std::vector<std::unique_ptr<node>>::iterator;
    using raw_const_iterator = std::vector<std::unique_ptr<node>>::const_iterator;
    using raw_iterator =
        std::conditional_t<IsConst, raw_const_iterator, raw_mutable_iterator>;

    mutable raw_iterator raw_;

    TOML_NODISCARD_CTOR
    array_iterator(raw_mutable_iterator raw) noexcept : raw_{raw} {}

    template <bool C = IsConst, typename = std::enable_if_t<C>>
    TOML_NODISCARD_CTOR array_iterator(raw_const_iterator raw) noexcept : raw_{raw} {}

  public:
    using value_type = std::conditional_t<IsConst, const node, node>;
    using reference = value_type &;
    using pointer = value_type *;
    using difference_type = ptrdiff_t;
    using iterator_category =
        typename std::iterator_traits<raw_iterator>::iterator_category;

    TOML_NODISCARD_CTOR
    array_iterator() noexcept = default;

    TOML_NODISCARD_CTOR
    array_iterator(const array_iterator &) noexcept = default;

    array_iterator &operator=(const array_iterator &) noexcept = default;

    array_iterator &operator++() noexcept // ++pre
    {
      ++raw_;
      return *this;
    }

    array_iterator operator++(int) noexcept // post++
    {
      array_iterator out{raw_};
      ++raw_;
      return out;
    }

    array_iterator &operator--() noexcept // --pre
    {
      --raw_;
      return *this;
    }

    array_iterator operator--(int) noexcept // post--
    {
      array_iterator out{raw_};
      --raw_;
      return out;
    }

    [[nodiscard]] reference operator*() const noexcept { return *raw_->get(); }

    [[nodiscard]] pointer operator->() const noexcept { return raw_->get(); }

    array_iterator &operator+=(ptrdiff_t rhs) noexcept {
      raw_ += rhs;
      return *this;
    }

    array_iterator &operator-=(ptrdiff_t rhs) noexcept {
      raw_ -= rhs;
      return *this;
    }

    [[nodiscard]] friend array_iterator operator+(const array_iterator &lhs,
                                                  ptrdiff_t rhs) noexcept {
      return {lhs.raw_ + rhs};
    }

    [[nodiscard]] friend array_iterator operator+(ptrdiff_t lhs,
                                                  const array_iterator &rhs) noexcept {
      return {rhs.raw_ + lhs};
    }

    [[nodiscard]] friend array_iterator operator-(const array_iterator &lhs,
                                                  ptrdiff_t rhs) noexcept {
      return {lhs.raw_ - rhs};
    }

    [[nodiscard]] friend ptrdiff_t operator-(const array_iterator &lhs,
                                             const array_iterator &rhs) noexcept {
      return lhs.raw_ - rhs.raw_;
    }

    [[nodiscard]] friend bool operator==(const array_iterator &lhs,
                                         const array_iterator &rhs) noexcept {
      return lhs.raw_ == rhs.raw_;
    }

    [[nodiscard]] friend bool operator!=(const array_iterator &lhs,
                                         const array_iterator &rhs) noexcept {
      return lhs.raw_ != rhs.raw_;
    }

    [[nodiscard]] friend bool operator<(const array_iterator &lhs,
                                        const array_iterator &rhs) noexcept {
      return lhs.raw_ < rhs.raw_;
    }

    [[nodiscard]] friend bool operator<=(const array_iterator &lhs,
                                         const array_iterator &rhs) noexcept {
      return lhs.raw_ <= rhs.raw_;
    }

    [[nodiscard]] friend bool operator>(const array_iterator &lhs,
                                        const array_iterator &rhs) noexcept {
      return lhs.raw_ > rhs.raw_;
    }

    [[nodiscard]] friend bool operator>=(const array_iterator &lhs,
                                         const array_iterator &rhs) noexcept {
      return lhs.raw_ >= rhs.raw_;
    }

    [[nodiscard]] reference operator[](ptrdiff_t idx) const noexcept {
      return *(raw_ + idx)->get();
    }

    TOML_DISABLE_WARNINGS;

    template <bool C = IsConst, typename = std::enable_if_t<!C>>
    operator array_iterator<true>() const noexcept {
      return array_iterator<true>{raw_};
    }

    TOML_ENABLE_WARNINGS;
  };
}
TOML_IMPL_NAMESPACE_END;
/// \endcond

TOML_NAMESPACE_START {
  /// \brief A RandomAccessIterator for iterating over elements in a toml::array.
  using array_iterator = impl::array_iterator<false>;

  /// \brief A RandomAccessIterator for iterating over const elements in a toml::array.
  using const_array_iterator = impl::array_iterator<true>;

  /// \brief	A TOML array.
  ///
  /// \detail The interface of this type is modeled after std::vector, with some
  /// 		additional considerations made for the heterogeneous nature of a
  /// 		TOML array.
  ///
  /// \godbolt{sjK4da}
  ///
  /// \cpp
  ///
  /// toml::table tbl = toml::parse(R"(
  ///     arr = [1, 2, 3, 4, 'five']
  /// )"sv);
  ///
  /// // get the element as an array
  /// toml::array& arr = *tbl.get_as<toml::array>("arr");
  /// std::cout << arr << "\n";
  ///
  /// // increment each element with visit()
  /// for (auto&& elem : arr)
  /// {
  /// 	elem.visit([](auto&& el) noexcept
  /// 	{
  /// 		if constexpr (toml::is_number<decltype(el)>)
  /// 			(*el)++;
  /// 		else if constexpr (toml::is_string<decltype(el)>)
  /// 			el = "six"sv;
  /// 	});
  /// }
  /// std::cout << arr << "\n";
  ///
  /// // add and remove elements
  /// arr.push_back(7);
  /// arr.push_back(8.0f);
  /// arr.push_back("nine"sv);
  /// arr.erase(arr.cbegin());
  /// std::cout << arr << "\n";
  ///
  /// // emplace elements
  /// arr.emplace_back<std::string>("ten");
  /// arr.emplace_back<toml::array>(11, 12.0);
  /// std::cout << arr << "\n";
  ///
  /// \ecpp
  ///
  /// \out
  /// [ 1, 2, 3, 4, 'five' ]
  /// [ 2, 3, 4, 5, 'six' ]
  /// [ 3, 4, 5, 'six', 7, 8.0, 'nine' ]
  /// [ 3, 4, 5, 'six', 7, 8.0, 'nine', 'ten', [ 11, 12.0 ] ]
  /// \eout
  class TOML_API array final : public node {
  private:
    /// \cond

    friend class TOML_PARSER_TYPENAME;
    std::vector<std::unique_ptr<node>> elements;

    void preinsertion_resize(size_t idx, size_t count) noexcept;

    template <typename T> void emplace_back_if_not_empty_view(T &&val) noexcept {
      if constexpr (is_node_view<T>) {
        if (!val)
          return;
      }
      elements.emplace_back(impl::make_node(static_cast<T &&>(val)));
    }

#if TOML_LIFETIME_HOOKS
    void lh_ctor() noexcept;
    void lh_dtor() noexcept;
#endif

    [[nodiscard]] size_t total_leaf_count() const noexcept;

    void flatten_child(array &&child, size_t &dest_index) noexcept;
    /// \endcond

  public:
    using value_type = node;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = node &;
    using const_reference = const node &;

    /// \brief A RandomAccessIterator for iterating over elements in a toml::array.
    using iterator = array_iterator;
    /// \brief A RandomAccessIterator for iterating over const elements in a
    /// toml::array.
    using const_iterator = const_array_iterator;

    /// \brief	Default constructor.
    TOML_NODISCARD_CTOR
    array() noexcept;

    /// \brief	Copy constructor.
    TOML_NODISCARD_CTOR
    array(const array &) noexcept;

    /// \brief	Move constructor.
    TOML_NODISCARD_CTOR
    array(array &&other) noexcept;

    /// \brief	Copy-assignment operator.
    array &operator=(const array &) noexcept;

    /// \brief	Move-assignment operator.
    array &operator=(array &&rhs) noexcept;

    /// \brief	Destructor.
    ~array() noexcept override;

    /// \brief	Constructs an array with one or more initial elements.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 1, 2.0, "three"sv, toml::array{ 4, 5 } };
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 2.0, 'three', [ 4, 5 ] ]
    /// \eout
    ///
    /// \remark	\parblock If you need to construct an array with one child array
    /// element, the array's move constructor 		will take precedence and perform
    /// a move-construction instead. You can use toml::inserter to 		suppress
    /// this behaviour: \cpp
    /// // desired result: [ [ 42 ] ]
    /// auto bad = toml::array{ toml::array{ 42 } }
    /// auto good = toml::array{ toml::inserter{ toml::array{ 42 } } }
    /// std::cout << "bad: " << bad << "\n";
    /// std::cout << "good:" << good << "\n";
    /// \ecpp
    ///
    /// \out
    /// bad:  [ 42 ]
    /// good: [ [ 42 ] ]
    /// \eout
    ///
    /// \endparblock
    ///
    /// \tparam	ElemType	One of the TOML node or value types (or a type
    /// promotable to one).
    /// \tparam	ElemTypes	One of the TOML node or value types (or a type
    /// promotable to one). \param 	val 	The node or value used to initialize
    /// element 0. \param 	vals	The nodes or values used to initialize
    /// elements 1...N.
    template <typename ElemType, typename... ElemTypes,
              typename = std::enable_if_t<
                  (sizeof...(ElemTypes) > 0_sz) ||
                  !std::is_same_v<impl::remove_cvref_t<ElemType>, array>>>
    TOML_NODISCARD_CTOR explicit array(ElemType &&val, ElemTypes &&...vals) {
      elements.reserve(sizeof...(ElemTypes) + 1_sz);
      emplace_back_if_not_empty_view(static_cast<ElemType &&>(val));
      if constexpr (sizeof...(ElemTypes) > 0) {
        (emplace_back_if_not_empty_view(static_cast<ElemTypes &&>(vals)), ...);
      }

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
    [[nodiscard]] array *as_array() noexcept override;
    [[nodiscard]] const array *as_array() const noexcept override;
    [[nodiscard]] bool is_array_of_tables() const noexcept override;

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
                    "The template type argument of array::is_homogeneous() must be "
                    "void or one of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);
      return is_homogeneous(impl::node_type_of<type>);
    }

    /// @}

    /// \name Array operations
    /// @{

    /// \brief	Gets a reference to the element at a specific index.
    [[nodiscard]] node &operator[](size_t index) noexcept;
    /// \brief	Gets a reference to the element at a specific index.
    [[nodiscard]] const node &operator[](size_t index) const noexcept;

    /// \brief	Returns a reference to the first element in the array.
    [[nodiscard]] node &front() noexcept;
    /// \brief	Returns a reference to the first element in the array.
    [[nodiscard]] const node &front() const noexcept;
    /// \brief	Returns a reference to the last element in the array.
    [[nodiscard]] node &back() noexcept;
    /// \brief	Returns a reference to the last element in the array.
    [[nodiscard]] const node &back() const noexcept;

    /// \brief	Returns an iterator to the first element.
    [[nodiscard]] iterator begin() noexcept;
    /// \brief	Returns an iterator to the first element.
    [[nodiscard]] const_iterator begin() const noexcept;
    /// \brief	Returns an iterator to the first element.
    [[nodiscard]] const_iterator cbegin() const noexcept;

    /// \brief	Returns an iterator to one-past-the-last element.
    [[nodiscard]] iterator end() noexcept;
    /// \brief	Returns an iterator to one-past-the-last element.
    [[nodiscard]] const_iterator end() const noexcept;
    /// \brief	Returns an iterator to one-past-the-last element.
    [[nodiscard]] const_iterator cend() const noexcept;

    /// \brief	Returns true if the array is empty.
    [[nodiscard]] bool empty() const noexcept;
    /// \brief	Returns the number of elements in the array.
    [[nodiscard]] size_t size() const noexcept;
    /// \brief	Reserves internal storage capacity up to a pre-determined number of
    /// elements.
    void reserve(size_t new_capacity);
    /// \brief	Removes all elements from the array.
    void clear() noexcept;

    /// \brief	Returns the maximum number of elements that can be stored in an array on
    /// the current platform.
    [[nodiscard]] size_t max_size() const noexcept;
    /// \brief	Returns the current max number of elements that may be held in the
    /// array's internal storage.
    [[nodiscard]] size_t capacity() const noexcept;
    /// \brief	Requests the removal of any unused internal storage capacity.
    void shrink_to_fit();

    /// \brief	Inserts a new element at a specific position in the array.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 1, 3 };
    ///	arr.insert(arr.cbegin() + 1, "two");
    ///	arr.insert(arr.cend(), toml::array{ 4, 5 });
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 'two', 3, [ 4, 5 ] ]
    /// \eout
    ///
    /// \tparam ElemType	toml::node, toml::node_view, toml::table, toml::array,
    /// or a native TOML value type 					(or a type
    /// promotable to one). \param 	pos The insertion position. \param 	val
    /// The node or value being inserted.
    ///
    /// \returns \conditional_return{Valid input}
    ///			 An iterator to the newly-inserted element.
    ///			 \conditional_return{Input is an empty toml::node_view}
    /// 		 end()
    ///
    /// \attention The return value will always be `end()` if the input value was an
    /// empty toml::node_view, 		   because no insertion can take place. This is
    /// the only circumstance in which this can occur.
    template <typename ElemType>
    iterator insert(const_iterator pos, ElemType &&val) noexcept {
      if constexpr (is_node_view<ElemType>) {
        if (!val)
          return end();
      }
      return {
          elements.emplace(pos.raw_, impl::make_node(static_cast<ElemType &&>(val)))};
    }

    /// \brief	Repeatedly inserts a new element starting at a specific position in the
    /// array.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{
    ///		"with an evil twinkle in its eye the goose said",
    ///		"and immediately we knew peace was never an option."
    ///	};
    ///	arr.insert(arr.cbegin() + 1, 3, "honk");
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [
    /// 	'with an evil twinkle in its eye the goose said',
    /// 	'honk',
    /// 	'honk',
    /// 	'honk',
    /// 	'and immediately we knew peace was never an option.'
    /// ]
    /// \eout
    ///
    /// \tparam ElemType	toml::node, toml::node_view, toml::table, toml::array,
    /// or a native TOML value type 					(or a type
    /// promotable to one). \param 	pos The insertion position. \param 	count
    /// The number of times the node or value should be inserted. \param 	val
    /// The node or value being inserted.
    ///
    /// \returns \conditional_return{Valid input}
    /// 		 An iterator to the newly-inserted element.
    /// 		 \conditional_return{count == 0}
    /// 		 A copy of pos
    /// 		 \conditional_return{Input is an empty toml::node_view}
    /// 		 end()
    ///
    /// \attention The return value will always be `end()` if the input value was an
    /// empty toml::node_view, 		   because no insertion can take place. This is
    /// the only circumstance in which this can occur.
    template <typename ElemType>
    iterator insert(const_iterator pos, size_t count, ElemType &&val) noexcept {
      if constexpr (is_node_view<ElemType>) {
        if (!val)
          return end();
      }
      switch (count) {
      case 0:
        return {elements.begin() + (pos.raw_ - elements.cbegin())};
      case 1:
        return insert(pos, static_cast<ElemType &&>(val));
      default: {
        const auto start_idx = static_cast<size_t>(pos.raw_ - elements.cbegin());
        preinsertion_resize(start_idx, count);
        size_t i = start_idx;
        for (size_t e = start_idx + count - 1_sz; i < e; i++)
          elements[i].reset(impl::make_node(val));

        //# potentially move the initial value into the last element
        elements[i].reset(impl::make_node(static_cast<ElemType &&>(val)));
        return {elements.begin() + static_cast<ptrdiff_t>(start_idx)};
      }
      }
    }

    /// \brief	Inserts a range of elements into the array at a specific position.
    ///
    /// \tparam	Iter	An iterator type. Must satisfy ForwardIterator.
    /// \param 	pos		The insertion position.
    /// \param 	first	Iterator to the first node or value being inserted.
    /// \param 	last	Iterator to the one-past-the-last node or value being inserted.
    ///
    /// \returns \conditional_return{Valid input}
    /// 		 An iterator to the first newly-inserted element.
    /// 		 \conditional_return{first >= last}
    /// 		 A copy of pos
    /// 		 \conditional_return{All objects in the range were empty
    /// toml::node_views} 		 A copy of pos
    template <typename Iter>
    iterator insert(const_iterator pos, Iter first, Iter last) noexcept {
      const auto distance = std::distance(first, last);
      if (distance <= 0)
        return {elements.begin() + (pos.raw_ - elements.cbegin())};
      else {
        auto count = distance;
        using deref_type = decltype(*first);
        if constexpr (is_node_view<deref_type>) {
          for (auto it = first; it != last; it++)
            if (!(*it))
              count--;
          if (!count)
            return {elements.begin() + (pos.raw_ - elements.cbegin())};
        }
        const auto start_idx = static_cast<size_t>(pos.raw_ - elements.cbegin());
        preinsertion_resize(start_idx, static_cast<size_t>(count));
        size_t i = start_idx;
        for (auto it = first; it != last; it++) {
          if constexpr (is_node_view<deref_type>) {
            if (!(*it))
              continue;
          }
          if constexpr (std::is_rvalue_reference_v<deref_type>)
            elements[i++].reset(impl::make_node(std::move(*it)));
          else
            elements[i++].reset(impl::make_node(*it));
        }
        return {elements.begin() + static_cast<ptrdiff_t>(start_idx)};
      }
    }

    /// \brief	Inserts a range of elements into the array at a specific position.
    ///
    /// \tparam ElemType	toml::node_view, toml::table, toml::array, or a native
    /// TOML value type 					(or a type promotable to
    /// one). \param 	pos			The insertion position. \param
    /// ilist		An initializer list containing the values to be inserted.
    ///
    /// \returns \conditional_return{Valid input}
    ///			 An iterator to the first newly-inserted element.
    /// 		 \conditional_return{Input list is empty}
    ///			 A copy of pos
    /// 		 \conditional_return{All objects in the list were empty
    /// toml::node_views}
    ///			 A copy of pos
    template <typename ElemType>
    iterator insert(const_iterator pos,
                    std::initializer_list<ElemType> ilist) noexcept {
      return insert(pos, ilist.begin(), ilist.end());
    }

    /// \brief	Emplaces a new element at a specific position in the array.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 1, 2 };
    ///
    ///	//add a string using std::string's substring constructor
    ///	arr.emplace<std::string>(arr.cbegin() + 1, "this is not a drill"sv, 14, 5);
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 'drill', 2 ]
    /// \eout
    ///
    /// \tparam ElemType	toml::table, toml::array, or any native TOML value type.
    /// \tparam	Args		Value constructor argument types.
    /// \param 	pos			The insertion position.
    /// \param 	args		Arguments to forward to the value's constructor.
    ///
    /// \returns	An iterator to the inserted element.
    ///
    /// \remarks There is no difference between insert() and emplace()
    /// 		 for trivial value types (floats, ints, bools).
    template <typename ElemType, typename... Args>
    iterator emplace(const_iterator pos, Args &&...args) noexcept {
      using type = impl::unwrap_node<ElemType>;
      static_assert((impl::is_native<type> ||
                     impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>,
                    "Emplacement type parameter must be one "
                    "of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);

      return {elements.emplace(
          pos.raw_, new impl::wrap_node<type>{static_cast<Args &&>(args)...})};
    }

    /// \brief	Removes the specified element from the array.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 1, 2, 3 };
    /// std::cout << arr << "\n";
    ///
    /// arr.erase(arr.cbegin() + 1);
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 2, 3 ]
    /// [ 1, 3 ]
    /// \eout
    ///
    /// \param 	pos		Iterator to the element being erased.
    ///
    /// \returns Iterator to the first element immediately following the removed
    /// element.
    iterator erase(const_iterator pos) noexcept;

    /// \brief	Removes the elements in the range [first, last) from the array.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 1, "bad", "karma" 2 };
    /// std::cout << arr << "\n";
    ///
    /// arr.erase(arr.cbegin() + 1, arr.cbegin() + 3);
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 'bad', 'karma', 3 ]
    /// [ 1, 3 ]
    /// \eout
    ///
    /// \param 	first	Iterator to the first element being erased.
    /// \param 	last	Iterator to the one-past-the-last element being erased.
    ///
    /// \returns Iterator to the first element immediately following the last removed
    /// element.
    iterator erase(const_iterator first, const_iterator last) noexcept;

    /// \brief	Resizes the array.
    ///
    /// \detail \godbolt{W5zqx3}
    ///
    /// \cpp
    /// auto arr = toml::array{ 1, 2, 3 };
    /// std::cout << arr << "\n";
    ///
    /// arr.resize(6, 42);
    /// std::cout << arr << "\n";
    ///
    /// arr.resize(2, 0);
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 2, 3 ]
    /// [ 1, 2, 3, 42, 42, 42 ]
    /// [ 1, 2 ]
    /// \eout
    ///
    /// \tparam ElemType	toml::node, toml::table, toml::array, or a native TOML
    /// value type 					(or a type promotable to one).
    ///
    /// \param 	new_size			The number of elements the array will
    /// have after resizing.
    /// \param 	default_init_val	The node or value used to initialize new
    /// elements if the array needs to grow.
    template <typename ElemType>
    void resize(size_t new_size, ElemType &&default_init_val) noexcept {
      static_assert(!is_node_view<ElemType>,
                    "The default element type argument to toml::array::resize may not "
                    "be toml::node_view.");

      if (!new_size)
        elements.clear();
      else if (new_size < elements.size())
        elements.resize(new_size);
      else if (new_size > elements.size())
        insert(cend(), new_size - elements.size(),
               static_cast<ElemType &&>(default_init_val));
    }

    /// \brief	Shrinks the array to the given size.
    ///
    /// \detail \godbolt{rxEzK5}
    ///
    /// \cpp
    /// auto arr = toml::array{ 1, 2, 3 };
    /// std::cout << arr << "\n";
    ///
    /// arr.truncate(5); // no-op
    /// std::cout << arr << "\n";
    ///
    /// arr.truncate(1);
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 2, 3 ]
    /// [ 1, 2, 3 ]
    /// [ 1]
    /// \eout
    ///
    /// \remarks	Does nothing if the requested size is larger than or equal to
    /// the current size.
    void truncate(size_t new_size);

    /// \brief	Appends a new element to the end of the array.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 1, 2 };
    ///	arr.push_back(3);
    ///	arr.push_back(4.0);
    ///	arr.push_back(toml::array{ 5, "six"sv });
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 2, 3, 4.0, [ 5, 'six' ] ]
    /// \eout
    ///
    /// \tparam ElemType	toml::node, toml::node_view, toml::table, toml::array,
    /// or a native TOML value type
    /// \param 	val			The node or value being added.
    ///
    /// \attention	No insertion takes place if the input value is an empty
    /// toml::node_view. 			This is the only circumstance in which
    /// this can occur.
    template <typename ElemType> void push_back(ElemType &&val) noexcept {
      emplace_back_if_not_empty_view(static_cast<ElemType &&>(val));
    }

    /// \brief	Emplaces a new element at the end of the array.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 1, 2 };
    ///	arr.emplace_back<toml::array>(3, "four"sv);
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 2, [ 3, 'four' ] ]
    /// \eout
    ///
    /// \tparam ElemType	toml::table, toml::array, or a native TOML value type
    /// \tparam	Args		Value constructor argument types.
    /// \param 	args		Arguments to forward to the value's constructor.
    ///
    /// \returns A reference to the newly-constructed element.
    ///
    /// \remarks There is no difference between push_back() and emplace_back()
    /// 		 For trivial value types (floats, ints, bools).
    template <typename ElemType, typename... Args>
    decltype(auto) emplace_back(Args &&...args) noexcept {
      using type = impl::unwrap_node<ElemType>;
      static_assert((impl::is_native<type> ||
                     impl::is_one_of<type, table, array>)&&!impl::is_cvref<type>,
                    "Emplacement type parameter must be one "
                    "of:" TOML_SA_UNWRAPPED_NODE_TYPE_LIST);

      auto nde = new impl::wrap_node<type>{static_cast<Args &&>(args)...};
      elements.emplace_back(nde);
      return *nde;
    }

    /// \brief	Removes the last element from the array.
    void pop_back() noexcept;

    /// \brief	Gets the element at a specific index.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 99, "bottles of beer on the wall" };
    ///	std::cout << "element [0] exists: "sv << !!arr.get(0) << "\n";
    ///	std::cout << "element [1] exists: "sv << !!arr.get(1) << "\n";
    ///	std::cout << "element [2] exists: "sv << !!arr.get(2) << "\n";
    /// if (toml::node* val = arr.get(0))
    ///		std::cout << "element [0] is an "sv << val->type() << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// element [0] exists: true
    /// element [1] exists: true
    /// element [2] exists: false
    /// element [0] is an integer
    /// \eout
    ///
    /// \param 	index	The element's index.
    ///
    /// \returns	A pointer to the element at the specified index if one existed,
    /// or nullptr.
    [[nodiscard]] node *get(size_t index) noexcept;

    /// \brief	Gets the element at a specific index (const overload).
    ///
    /// \param 	index	The element's index.
    ///
    /// \returns	A pointer to the element at the specified index if one existed,
    /// or nullptr.
    [[nodiscard]] const node *get(size_t index) const noexcept;

    /// \brief	Gets the element at a specific index if it is a particular type.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 42, "is the meaning of life, apparently."sv };
    /// if (toml::value<int64_t>* val = arr.get_as<int64_t>(0))
    ///		std::cout << "element [0] is an integer with value "sv << *val << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// element [0] is an integer with value 42
    /// \eout
    ///
    /// \tparam ElemType	toml::table, toml::array, or a native TOML value type
    /// \param 	index		The element's index.
    ///
    /// \returns	A pointer to the selected element if it existed and was of the
    /// specified type, or nullptr.
    template <typename ElemType>
    [[nodiscard]] impl::wrap_node<ElemType> *get_as(size_t index) noexcept {
      if (auto val = get(index))
        return val->as<ElemType>();
      return nullptr;
    }

    /// \brief	Gets the element at a specific index if it is a particular type (const
    /// overload).
    ///
    /// \tparam ElemType	toml::table, toml::array, or a native TOML value type
    /// \param 	index		The element's index.
    ///
    /// \returns	A pointer to the selected element if it existed and was of the
    /// specified type, or nullptr.
    template <typename ElemType>
    [[nodiscard]] const impl::wrap_node<ElemType> *get_as(size_t index) const noexcept {
      if (auto val = get(index))
        return val->as<ElemType>();
      return nullptr;
    }

    /// \brief	Flattens this array, recursively hoisting the contents of child arrays
    /// up into itself.
    ///
    /// \detail \cpp
    ///
    /// auto arr = toml::array{ 1, 2, toml::array{ 3, 4, toml::array{ 5 } }, 6,
    /// toml::array{} }; std::cout << arr << "\n";
    ///
    /// arr.flatten();
    /// std::cout << arr << "\n";
    ///
    /// \ecpp
    ///
    /// \out
    /// [ 1, 2, [ 3, 4, [ 5 ] ], 6, [] ]
    /// [ 1, 2, 3, 4, 5, 6 ]
    /// \eout
    ///
    /// \remarks	Arrays inside child tables are not flattened.
    ///
    /// \returns A reference to the array.
    array &flatten() &;

    /// \brief	 Flattens this array, recursively hoisting the contents of child arrays
    /// up into itself (rvalue overload). \returns An rvalue reference to the array.
    array &&flatten() && { return static_cast<toml::array &&>(this->flatten()); }

    /// @}

    /// \name Equality
    /// @{

    /// \brief	Equality operator.
    ///
    /// \param 	lhs	The LHS array.
    /// \param 	rhs	The RHS array.
    ///
    /// \returns	True if the arrays contained the same elements.
    friend bool operator==(const array &lhs, const array &rhs) noexcept;

    /// \brief	Inequality operator.
    ///
    /// \param 	lhs	The LHS array.
    /// \param 	rhs	The RHS array.
    ///
    /// \returns	True if the arrays did not contain the same elements.
    friend bool operator!=(const array &lhs, const array &rhs) noexcept;

  private:
    template <typename T>
    [[nodiscard]] static bool container_equality(const array &lhs,
                                                 const T &rhs) noexcept {
      using element_type = std::remove_const_t<typename T::value_type>;
      static_assert(impl::is_native<element_type> ||
                        impl::is_losslessly_convertible_to_native<element_type>,
                    "Container element type must be (or be promotable to) one of the "
                    "TOML value types");

      if (lhs.size() != rhs.size())
        return false;
      if (rhs.size() == 0_sz)
        return true;

      size_t i{};
      for (auto &list_elem : rhs) {
        const auto elem = lhs.get_as<impl::native_type_of<element_type>>(i++);
        if (!elem || *elem != list_elem)
          return false;
      }

      return true;
    }

  public:
    /// \brief	Initializer list equality operator.
    template <typename T>
    [[nodiscard]] friend bool operator==(const array &lhs,
                                         const std::initializer_list<T> &rhs) noexcept {
      return container_equality(lhs, rhs);
    }
    TOML_ASYMMETRICAL_EQUALITY_OPS(const array &, const std::initializer_list<T> &,
                                   template <typename T>
    );

    /// \brief	Vector equality operator.
    template <typename T>
    [[nodiscard]] friend bool operator==(const array &lhs,
                                         const std::vector<T> &rhs) noexcept {
      return container_equality(lhs, rhs);
    }
    TOML_ASYMMETRICAL_EQUALITY_OPS(const array &, const std::vector<T> &,
                                   template <typename T>
    );

    /// @}

    /// \brief	Prints the array out to a stream as formatted TOML.
    template <typename Char>
    friend std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &,
                                                const array &);
    // implemented in toml_default_formatter.h
  };
}
TOML_NAMESPACE_END;
