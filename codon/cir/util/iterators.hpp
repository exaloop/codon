// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <iterator>
#include <memory>
#include <type_traits>

namespace codon {
namespace ir {
namespace util {

/// Iterator wrapper that applies a function to the iterator.
template <typename It, typename DereferenceFunc, typename MemberFunc>
struct function_iterator_adaptor {
  It internal;
  DereferenceFunc d;
  MemberFunc m;

  using iterator_category = std::input_iterator_tag;
  using value_type = typename std::remove_reference<decltype(d(*internal))>::type;
  using reference = void;
  using pointer = void;
  using difference_type = typename std::iterator_traits<It>::difference_type;

  /// Constructs an adaptor.
  /// @param internal the internal iterator
  /// @param d the dereference function
  /// @param m the member access function
  function_iterator_adaptor(It internal, DereferenceFunc &&d, MemberFunc &&m)
      : internal(std::move(internal)), d(std::move(d)), m(std::move(m)) {}

  decltype(auto) operator*() { return d(*internal); }
  decltype(auto) operator->() { return m(*internal); }

  function_iterator_adaptor &operator++() {
    internal++;
    return *this;
  }
  function_iterator_adaptor operator++(int) {
    function_iterator_adaptor<It, DereferenceFunc, MemberFunc> copy(*this);
    internal++;
    return copy;
  }

  template <typename OtherIt, typename OtherDereferenceFunc, typename OtherMemberFunc>
  bool operator==(const function_iterator_adaptor<OtherIt, OtherDereferenceFunc,
                                                  OtherMemberFunc> &other) const {
    return other.internal == internal;
  }

  template <typename OtherIt, typename OtherDereferenceFunc, typename OtherMemberFunc>
  bool operator!=(const function_iterator_adaptor<OtherIt, OtherDereferenceFunc,
                                                  OtherMemberFunc> &other) const {
    return other.internal != internal;
  }
};

/// Creates an adaptor that dereferences values.
/// @param it the internal iterator
/// @return the adaptor
template <typename It> auto dereference_adaptor(It it) {
  auto f = [](const auto &v) -> auto & { return *v; };
  auto m = [](const auto &v) -> auto { return v.get(); };
  return function_iterator_adaptor<It, decltype(f), decltype(m)>(it, std::move(f),
                                                                 std::move(m));
}

/// Creates an adaptor that gets the address of its values.
/// @param it the internal iterator
/// @return the adaptor
template <typename It> auto raw_ptr_adaptor(It it) {
  auto f = [](auto &v) -> auto * { return v.get(); };
  auto m = [](auto &v) -> auto * { return v.get(); };
  return function_iterator_adaptor<It, decltype(f), decltype(m)>(it, std::move(f),
                                                                 std::move(m));
}

/// Creates an adaptor that gets the const address of its values.
/// @param it the internal iterator
/// @return the adaptor
template <typename It> auto const_raw_ptr_adaptor(It it) {
  auto f = [](auto &v) -> const auto * { return v.get(); };
  auto m = [](auto &v) -> const auto * { return v.get(); };
  return function_iterator_adaptor<It, decltype(f), decltype(m)>(it, std::move(f),
                                                                 std::move(m));
}

/// Creates an adaptor that gets the keys of its values.
/// @param it the internal iterator
/// @return the adaptor
template <typename It> auto map_key_adaptor(It it) {
  auto f = [](auto &v) -> auto & { return v.first; };
  auto m = [](auto &v) -> auto & { return v.first; };
  return function_iterator_adaptor<It, decltype(f), decltype(m)>(it, std::move(f),
                                                                 std::move(m));
}

/// Creates an adaptor that gets the const keys of its values.
/// @param it the internal iterator
/// @return the adaptor
template <typename It> auto const_map_key_adaptor(It it) {
  auto f = [](auto &v) -> const auto & { return v.first; };
  auto m = [](auto &v) -> const auto & { return v.first; };
  return function_iterator_adaptor<It, decltype(f), decltype(m)>(it, std::move(f),
                                                                 std::move(m));
}

} // namespace util
} // namespace ir
} // namespace codon
