// Licensed under the Boost License <https://opensource.org/licenses/BSL-1.0>.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <array>
#include <cstring>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
// #include "tser/varint_encoding.hpp"// Licensed under the Boost License
// <https://opensource.org/licenses/BSL-1.0>. SPDX-License-Identifier: BSL-1.0

#include <type_traits>
namespace tser {
template <typename T> size_t encode_varint(T value, char *output) {
  size_t i = 0;
  if constexpr (std::is_signed_v<T>)
    value = static_cast<T>(value << 1 ^ (value >> (sizeof(T) * 8 - 1)));
  for (; value > 127; ++i, value >>= 7)
    output[i] = static_cast<char>(static_cast<uint8_t>(value & 127) | 128);
  output[i++] = static_cast<uint8_t>(value) & 127;
  return i;
}
template <typename T> size_t decode_varint(T &value, const char *const input) {
  size_t i = 0;
  for (value = 0; i == 0 || (input[i - 1] & 128); i++)
    value |= static_cast<T>(input[i] & 127) << (7 * i);
  if constexpr (std::is_signed_v<T>)
    value = (value & 1) ? -static_cast<T>((value + 1) >> 1) : (value + 1) >> 1;
  return i;
}
} // namespace tser

// #include "tser/base64_encoding.hpp"// Licensed under the Boost License
// <https://opensource.org/licenses/BSL-1.0>. SPDX-License-Identifier: BSL-1.0

#include <array>
#include <string>
#include <string_view>
namespace tser {
// tables for the base64 conversions
static constexpr auto g_encodingTable =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static constexpr auto g_decodingTable = []() {
  std::array<unsigned char, 256> decTable{};
  for (unsigned char i = 0; i < 64u; ++i)
    decTable[static_cast<unsigned char>(g_encodingTable[i])] = i;
  return decTable;
}();
static std::string encode_base64(std::string_view in) {
  std::string out;
  unsigned val = 0;
  int valb = -6;
  for (char c : in) {
    val = (val << 8) + static_cast<unsigned char>(c);
    valb += 8;
    while (valb >= 0) {
      out.push_back(g_encodingTable[(val >> valb) & 63u]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(g_encodingTable[((val << 8) >> (valb + 8)) & 0x3F]);
  return out;
}
static std::string decode_base64(std::string_view in) {
  std::string out;
  unsigned val = 0;
  int valb = -8;
  for (char c : in) {
    val = (val << 6) + g_decodingTable[static_cast<unsigned char>(c)];
    valb += 6;
    if (valb >= 0) {
      out.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}
} // namespace tser

namespace tser {
// implementation details for C++20 is_detected
namespace detail {
struct ns {
  ~ns() = delete;
  ns(ns const &) = delete;
};
template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
struct detector {
  using value_t = std::false_type;
  using type = Default;
};
template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
  using value_t = std::true_type;
  using type = Op<Args...>;
};
template <class T> struct is_array : std::is_array<T> {};
template <template <typename, size_t> class TArray, typename T, size_t N>
struct is_array<TArray<T, N>> : std::true_type {};
constexpr size_t n_args(char const *c, size_t nargs = 1) {
  for (; *c; ++c)
    if (*c == ',')
      ++nargs;
  return nargs;
}
constexpr size_t str_size(char const *c, size_t strSize = 1) {
  for (; *c; ++c)
    ++strSize;
  return strSize;
}
} // namespace detail
// we need a bunch of template metaprogramming for being able to differentiate between
// different types
template <template <class...> class Op, class... Args>
constexpr bool is_detected_v =
    detail::detector<detail::ns, void, Op, Args...>::value_t::value;

class BinaryArchive;
template <class T> using has_begin_t = decltype(*std::begin(std::declval<T>()));
template <class T> using has_members_t = decltype(std::declval<T>().members());
template <class T>
using has_smaller_t = decltype(std::declval<T>() < std::declval<T>());
template <class T> using has_equal_t = decltype(std::declval<T>() == std::declval<T>());
template <class T>
using has_nequal_t = decltype(std::declval<T>() != std::declval<T>());
template <class T>
using has_outstream_op_t = decltype(std::declval<std::ostream>() << std::declval<T>());
template <class T> using has_tuple_t = std::tuple_element_t<0, T>;
template <class T> using has_optional_t = decltype(std::declval<T>().has_value());
template <class T> using has_element_t = typename T::element_type;
template <class T> using has_mapped_t = typename T::mapped_type;
template <class T>
using has_custom_save_t =
    decltype(std::declval<T>().save(std::declval<BinaryArchive &>()));
template <class T>
using has_free_save_t =
    decltype(std::declval<const T &>() << std::declval<BinaryArchive &>());
template <class T> constexpr bool is_container_v = is_detected_v<has_begin_t, T>;
template <class T> constexpr bool is_tuple_v = is_detected_v<has_tuple_t, T>;
template <class T> constexpr bool is_tser_t_v = is_detected_v<has_members_t, T>;
template <class T>
constexpr bool is_pointer_like_v =
    std::is_pointer_v<T> || is_detected_v<has_element_t, T> ||
    is_detected_v<has_optional_t, T>;
// implementation of the recursive json printing
template <typename T> constexpr inline decltype(auto) print(std::ostream &os, T &&val) {
  using V = std::decay_t<T>;
  if constexpr (std::is_constructible_v<std::string, T> || std::is_same_v<V, char>)
    os << "\"" << val << "\"";
  else if constexpr (is_container_v<V>) {
    size_t i = 0;
    os << "\n[";
    for (auto &elem : val)
      os << (i++ == 0 ? "" : ",") << tser::print(os, elem);
    os << "]\n";
  } else if constexpr (is_tser_t_v<V> && !is_detected_v<has_outstream_op_t, V>) {
    auto pMem = [&](auto &...memberVal) {
      size_t i = 0;
      (((os << (i != 0 ? ", " : "") << '\"'),
        os << V::_memberNames[i++] << "\" : " << tser::print(os, memberVal)),
       ...);
    };
    os << "{ \"" << V::_typeName << "\": {";
    std::apply(pMem, val.members());
    os << "}}\n";
  } else if constexpr (std::is_enum_v<V> && !is_detected_v<has_outstream_op_t, V>) {
    os << tser::print(os, static_cast<std::underlying_type_t<V>>(val));
  } else if constexpr (is_tuple_v<V> && !is_detected_v<has_outstream_op_t, V>) {
    std::apply(
        [&](auto &...t) {
          int i = 0;
          os << "{";
          (((i++ != 0 ? os << ", " : os), tser::print(os, t)), ...);
          os << "}";
        },
        val);
  } else if constexpr (is_pointer_like_v<V>) {
    os << (val ? (os << (tser::print(os, *val)), "") : "null");
  } else
    os << val;
  return "";
}
// we have to implement the tuple < operator ourselves for recursive introspection
template <typename T> constexpr inline bool less(const T &lhs, const T &rhs);
template <class T, std::size_t... I>
constexpr inline bool less(const T &lhs, const T &rhs, std::index_sequence<I...>) {
  bool isSmaller = false;
  (void)((less(std::get<I>(lhs), std::get<I>(rhs))
              ? (static_cast<void>(isSmaller = true), false)
              : (less(std::get<I>(rhs), std::get<I>(lhs)) ? false : true)) &&
         ...);
  return isSmaller;
}
template <typename T> constexpr inline bool less(const T &lhs, const T &rhs) {
  if constexpr (is_tser_t_v<T>)
    return less(lhs.members(), rhs.members());
  else if constexpr (is_tuple_v<T>)
    return less(lhs, rhs, std::make_index_sequence<std::tuple_size_v<T>>());
  else if constexpr (is_container_v<T> &&
                     !tser::is_detected_v<tser::has_smaller_t, T>) {
    if (lhs.size() != rhs.size())
      return lhs.size() < rhs.size();
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
  } else if constexpr (std::is_enum_v<T>)
    return static_cast<std::underlying_type_t<T>>(lhs) <
           static_cast<std::underlying_type_t<T>>(rhs);
  else
    return lhs < rhs;
}

class BinaryArchive {
  std::string m_bytes = std::string(1024, '\0');
  size_t m_bufferSize = 0, m_readOffset = 0;

public:
  explicit BinaryArchive(const size_t initialSize = 1024)
      : m_bytes(initialSize, '\0') {}
  explicit BinaryArchive(std::string encodedStr)
      : m_bytes(decode_base64(encodedStr)), m_bufferSize(m_bytes.size()) {}
  template <typename T, std::enable_if_t<!std::is_integral_v<T>, int> = 0>
  explicit BinaryArchive(const T &t) {
    save(t);
  }
  template <typename T> void save(const T &t) {
    if constexpr (is_detected_v<has_free_save_t, T>)
      operator<<(t, *this);
    else if constexpr (is_detected_v<has_custom_save_t, T>)
      t.save(*this);
    else if constexpr (is_tser_t_v<T>)
      std::apply([&](auto &...mVal) { (save(mVal), ...); }, t.members());
    else if constexpr (is_tuple_v<T>)
      std::apply([&](auto &...tVal) { (save(tVal), ...); }, t);
    else if constexpr (is_pointer_like_v<T>) {
      save(static_cast<bool>(t));
      if (t)
        save(*t);
    } else if constexpr (is_container_v<T>) {
      if constexpr (!detail::is_array<T>::value)
        save(t.size());
      for (auto &val : t)
        save(val);
    } else {
      if (m_bufferSize + sizeof(T) + sizeof(T) / 4 > m_bytes.size())
        m_bytes.resize((m_bufferSize + sizeof(T)) * 2);
      if constexpr (std::is_integral_v<T> && sizeof(T) > 2)
        m_bufferSize += encode_varint(t, m_bytes.data() + m_bufferSize);
      else {
        std::memcpy(m_bytes.data() + m_bufferSize, std::addressof(t), sizeof(T));
        m_bufferSize += sizeof(T);
      }
    }
  }
  template <typename T> void load(T &t) {
    using V = std::decay_t<T>;
    if constexpr (is_detected_v<has_free_save_t, V>)
      operator>>(t, *this);
    else if constexpr (is_detected_v<has_custom_save_t, T>)
      t.load(*this);
    else if constexpr (is_tser_t_v<T>)
      std::apply([&](auto &...mVal) { (load(mVal), ...); }, t.members());
    else if constexpr (is_tuple_v<V>)
      std::apply([&](auto &...tVal) { (load(tVal), ...); }, t);
    else if constexpr (is_pointer_like_v<T>) {
      if constexpr (std::is_pointer_v<T>) {
        t = load<bool>() ? (t = new std::remove_pointer_t<T>(), load(*t), t) : nullptr;
      } else if constexpr (is_detected_v<has_optional_t, T>)
        t = load<bool>() ? T(load<typename V::value_type>()) : T();
      else // smart pointer
        t = T(load<has_element_t<V> *>());
    } else if constexpr (is_container_v<T>) {
      if constexpr (!detail::is_array<T>::value) {
        const auto size = load<decltype(t.size())>();
        using VT = typename V::value_type;
        for (size_t i = 0; i < size; ++i)
          if constexpr (!is_detected_v<has_mapped_t, V>)
            t.insert(t.end(), load<VT>());
          else // we have to special case map, because of the const key
            t.emplace(
                VT{load<typename V::key_type>(), load<typename V::mapped_type>()});
      } else {
        for (auto &val : t)
          load(val);
      }
    } else {
      if constexpr (std::is_integral_v<T> && sizeof(T) > 2)
        m_readOffset += decode_varint(t, m_bytes.data() + m_readOffset);
      else {
        std::memcpy(&t, m_bytes.data() + m_readOffset, sizeof(T));
        m_readOffset += sizeof(T);
      }
    }
  }
  template <typename T> T load() {
    std::remove_const_t<T> t{};
    load(t);
    return t;
  }
  template <typename T>
  friend BinaryArchive &operator<<(BinaryArchive &ba, const T &t) {
    ba.save(t);
    return ba;
  }
  template <typename T> friend BinaryArchive &operator>>(BinaryArchive &ba, T &t) {
    ba.load(t);
    return ba;
  }
  void reset() {
    m_bufferSize = 0;
    m_readOffset = 0;
  }
  void initialize(std::string_view str) {
    m_bytes = str;
    m_bufferSize = str.size();
    m_readOffset = 0;
  }
  std::string_view get_buffer() const {
    return std::string_view(m_bytes.data(), m_bufferSize);
  }
  friend std::ostream &operator<<(std::ostream &os, const BinaryArchive &ba) {
    return os << encode_base64(ba.get_buffer()) << '\n';
  }
};
template <class Base, typename Derived>
std::conditional_t<std::is_const_v<Derived>, const Base, Base> &base(Derived *thisPtr) {
  return *thisPtr;
}
template <typename T> auto load(std::string_view encoded) {
  BinaryArchive ba(encoded);
  return ba.load<T>();
}
} // namespace tser
// this macro defines printing, serialisation and comparision operators (==,!=,<) for
// custom types
#define DEFINE_SERIALIZABLE(Type, ...)                                                 \
  inline decltype(auto) members() const { return std::tie(__VA_ARGS__); }              \
  inline decltype(auto) members() { return std::tie(__VA_ARGS__); }                    \
  static constexpr std::array<char, tser::detail::str_size(#__VA_ARGS__)>              \
      _memberNameData = []() {                                                         \
        std::array<char, tser::detail::str_size(#__VA_ARGS__)> chars{'\0'};            \
        size_t _idx = 0;                                                               \
        constexpr auto *ini(#__VA_ARGS__);                                             \
        for (char const *_c = ini; *_c; ++_c, ++_idx)                                  \
          if (*_c != ',' && *_c != ' ')                                                \
            chars[_idx] = *_c;                                                         \
        return chars;                                                                  \
      }();                                                                             \
  static constexpr const char *_typeName = #Type;                                      \
  static constexpr std::array<const char *, tser::detail::n_args(#__VA_ARGS__)>        \
      _memberNames = []() {                                                            \
        std::array<const char *, tser::detail::n_args(#__VA_ARGS__)> out{};            \
        for (size_t _i = 0, nArgs = 0; nArgs < tser::detail::n_args(#__VA_ARGS__);     \
             ++_i) {                                                                   \
          while (Type::_memberNameData[_i] == '\0')                                    \
            _i++;                                                                      \
          out[nArgs++] = &Type::_memberNameData[_i];                                   \
          while (Type::_memberNameData[++_i] != '\0')                                  \
            ;                                                                          \
        }                                                                              \
        return out;                                                                    \
      }();                                                                             \
  template <typename OT,                                                               \
            std::enable_if_t<std::is_same_v<OT, Type> &&                               \
                                 !tser::is_detected_v<tser::has_equal_t, OT>,          \
                             int> = 0>                                                 \
  friend bool operator==(const Type &lhs, const OT &rhs) {                             \
    return lhs.members() == rhs.members();                                             \
  }                                                                                    \
  template <typename OT,                                                               \
            std::enable_if_t<std::is_same_v<OT, Type> &&                               \
                                 !tser::is_detected_v<tser::has_nequal_t, OT>,         \
                             int> = 0>                                                 \
  friend bool operator!=(const Type &lhs, const OT &rhs) {                             \
    return !(lhs == rhs);                                                              \
  }                                                                                    \
  template <typename OT,                                                               \
            std::enable_if_t<std::is_same_v<OT, Type> &&                               \
                                 !tser::is_detected_v<tser::has_smaller_t, OT>,        \
                             int> = 0>                                                 \
  friend bool operator<(const OT &lhs, const OT &rhs) {                                \
    return tser::less(lhs, rhs);                                                       \
  }                                                                                    \
  template <typename OT,                                                               \
            std::enable_if_t<std::is_same_v<OT, Type> &&                               \
                                 !tser::is_detected_v<tser::has_outstream_op_t, OT>,   \
                             int> = 0>                                                 \
  friend std::ostream &operator<<(std::ostream &os, const OT &t) {                     \
    tser::print(os, t);                                                                \
    return os;                                                                         \
  }
