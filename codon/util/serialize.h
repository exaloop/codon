// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <functional>
#include <string>
#include <tser/tser.hpp>
#include <unordered_map>

namespace codon {

template <class Archive, class Base> struct PolymorphicSerializer {
  struct Serializer {
    std::function<void(Base *, Archive &)> save;
    std::function<void(Base *&, Archive &)> load;
  };
  template <class Derived> static Serializer serializerFor() {
    return {[](Base *b, Archive &a) { a.save(*(static_cast<Derived *>(b))); },
            [](Base *&b, Archive &a) {
              b = new Derived();
              a.load(static_cast<Derived &>(*b));
            }};
  }

  static inline std::unordered_map<void *, std::string> _serializers;
  static inline std::unordered_map<std::string, Serializer> _factory;
  template <class... Derived> static void register_types() {
    (_serializers.emplace((void *)(Derived::nodeId()), Derived::_typeName), ...);
    (_factory.emplace(std::string(Derived::_typeName), serializerFor<Derived>()), ...);
  }
  static void save(const std::string &s, Base *b, Archive &a) {
    auto i = _factory.find(s);
    assert(i != _factory.end() && "bad op");
    i->second.save(b, a);
  }
  static void load(const std::string &s, Base *&b, Archive &a) {
    auto i = _factory.find(s);
    assert(i != _factory.end() && "bad op");
    i->second.load(b, a);
  }
};
} // namespace codon

#define SERIALIZE(Type, ...)                                                           \
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
      }()

#define BASE(T) tser::base<T>(this)
