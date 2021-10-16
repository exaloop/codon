//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT

#pragma once
#include "toml_preprocessor.h"

TOML_PUSH_WARNINGS;
TOML_DISABLE_SWITCH_WARNINGS;

/// \cond
TOML_IMPL_NAMESPACE_START {
  [[nodiscard]] TOML_ATTR(const) constexpr bool is_ascii_whitespace(
      char32_t codepoint) noexcept {
    return codepoint == U'\t' || codepoint == U' ';
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_non_ascii_whitespace(
      char32_t codepoint) noexcept {
    // see: https://en.wikipedia.org/wiki/Whitespace_character#Unicode
    // (characters that don't say "is a line-break")

    return codepoint == U'\u00A0'    // no-break space
           || codepoint == U'\u1680' // ogham space mark
           ||
           (codepoint >= U'\u2000' && codepoint <= U'\u200A') // em quad -> hair space
           || codepoint == U'\u202F'                          // narrow no-break space
           || codepoint == U'\u205F' // medium mathematical space
           || codepoint == U'\u3000' // ideographic space
        ;
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_whitespace(
      char32_t codepoint) noexcept {
    return is_ascii_whitespace(codepoint) || is_non_ascii_whitespace(codepoint);
  }

  template <bool IncludeCarriageReturn = true>
  [[nodiscard]] TOML_ATTR(const) constexpr bool is_ascii_line_break(
      char32_t codepoint) noexcept {
    constexpr auto low_range_end = IncludeCarriageReturn ? U'\r' : U'\f';
    return (codepoint >= U'\n' && codepoint <= low_range_end);
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_non_ascii_line_break(
      char32_t codepoint) noexcept {
    // see https://en.wikipedia.org/wiki/Whitespace_character#Unicode
    // (characters that say "is a line-break")

    return codepoint == U'\u0085'    // next line
           || codepoint == U'\u2028' // line separator
           || codepoint == U'\u2029' // paragraph separator
        ;
  }

  template <bool IncludeCarriageReturn = true>
  [[nodiscard]] TOML_ATTR(const) constexpr bool is_line_break(
      char32_t codepoint) noexcept {
    return is_ascii_line_break<IncludeCarriageReturn>(codepoint) ||
           is_non_ascii_line_break(codepoint);
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_string_delimiter(
      char32_t codepoint) noexcept {
    return codepoint == U'"' || codepoint == U'\'';
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_ascii_letter(
      char32_t codepoint) noexcept {
    return (codepoint >= U'a' && codepoint <= U'z') ||
           (codepoint >= U'A' && codepoint <= U'Z');
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_binary_digit(
      char32_t codepoint) noexcept {
    return codepoint == U'0' || codepoint == U'1';
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_octal_digit(
      char32_t codepoint) noexcept {
    return (codepoint >= U'0' && codepoint <= U'7');
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_decimal_digit(
      char32_t codepoint) noexcept {
    return (codepoint >= U'0' && codepoint <= U'9');
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_hexadecimal_digit(
      char32_t c) noexcept {
    return U'0' <= c && c <= U'f' &&
           (1ull << (static_cast<uint_least64_t>(c) - 0x30u)) & 0x7E0000007E03FFull;
  }

  template <typename T>
  [[nodiscard]] TOML_ATTR(const) constexpr std::uint_least32_t hex_to_dec(
      const T codepoint) noexcept {
    if constexpr (std::is_same_v<remove_cvref_t<T>, std::uint_least32_t>)
      return codepoint >= 0x41u                      // >= 'A'
                 ? 10u + (codepoint | 0x20u) - 0x61u // - 'a'
                 : codepoint - 0x30u                 // - '0'
          ;
    else
      return hex_to_dec(static_cast<std::uint_least32_t>(codepoint));
  }

#if TOML_LANG_UNRELEASED // toml/issues/687 (unicode bare keys)

  //# Returns true if a codepoint belongs to any of these categories:
  //# 	Ll, Lm, Lo, Lt, Lu
  [[nodiscard]] TOML_ATTR(const) constexpr bool is_non_ascii_letter(
      char32_t c) noexcept {
    if (U'\xAA' > c || c > U'\U0003134A')
      return false;

    const auto child_index_0 = (static_cast<uint_least64_t>(c) - 0xAAull) / 0xC4Bull;
    if ((1ull << child_index_0) & 0x26180C0000ull)
      return false;
    if ((1ull << child_index_0) & 0x8A7FFC004001CFA0ull)
      return true;
    switch (child_index_0) {
    case 0x00: // [0] 00AA - 0CF4
    {
      if (c > U'\u0CF2')
        return false;
      TOML_ASSUME(U'\xAA' <= c);

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFFDFFFFFC10801u, 0xFFFFFFFFFFFFDFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0x07C000FFF0FFFFFFu,
          0x0000000000000014u, 0x0000000000000000u, 0xFEFFFFF5D02F37C0u,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFEFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFF00FFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFC09FFFFFFFFFBFu, 0x000000007FFFFFFFu, 0xFFFFFFC000000000u,
          0xFFC00000000001E1u, 0x00000001FFFFFFFFu, 0xFFFFFFFFFFFFFFB0u,
          0x18000BFFFFFFFFFFu, 0xFFFFFF4000270030u, 0xFFFFFFF80000003Fu,
          0x0FFFFFFFFFFFFFFFu, 0xFFFFFFFF00000080u, 0x44010FFFFFC10C01u,
          0xFFC07FFFFFC00000u, 0xFFC0000000000001u, 0x000000003FFFF7FFu,
          0xFFFFFFFFFC000000u, 0x00FFC0400008FFFFu, 0x7FFFFE67F87FFF80u,
          0x00EC00100008F17Fu, 0x7FFFFE61F80400C0u, 0x001780000000DB7Fu,
          0x7FFFFEEFF8000700u, 0x00C000400008FB7Fu, 0x7FFFFE67F8008000u,
          0x00EC00000008FB7Fu, 0xC6358F71FA000080u, 0x000000400000FFF1u,
          0x7FFFFF77F8000000u, 0x00C1C0000008FFFFu, 0x7FFFFF77F8400000u,
          0x00D000000008FBFFu, 0x0000000000000180u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0xAAull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0xAAull) % 0x40ull));
      // 1922 code units from 124 ranges (spanning a search area of 3145)
    }
    case 0x01: // [1] 0CF5 - 193F
    {
      if (U'\u0D04' > c || c > U'\u191E')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x027FFFFFFFFFDDFFu, 0x0FC0000038070400u, 0xF2FFBFFFFFC7FFFEu,
          0xE000000000000007u, 0xF000DFFFFFFFFFFFu, 0x6000000000000007u,
          0xF200DFFAFFFFFF7Du, 0x100000000F000005u, 0xF000000000000000u,
          0x000001FFFFFFFFEFu, 0x00000000000001F0u, 0xF000000000000000u,
          0x0800007FFFFFFFFFu, 0x3FFE1C0623C3F000u, 0xFFFFFFFFF0000400u,
          0xFF7FFFFFFFFFF20Bu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFF3D7F3DFu, 0xD7F3DFFFFFFFF3DFu, 0xFFFFFFFFFFF7FFF3u,
          0xFFFFFFFFFFF3DFFFu, 0xF0000000007FFFFFu, 0xFFFFFFFFF0000FFFu,
          0xE3F3FFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xEFFFF9FFFFFFFFFFu, 0xFFFFFFFFF07FFFFFu,
          0xF01FE07FFFFFFFFFu, 0xF0003FFFF0003DFFu, 0xF0001DFFF0003FFFu,
          0x0000FFFFFFFFFFFFu, 0x0000000001080000u, 0xFFFFFFFFF0000000u,
          0xF01FFFFFFFFFFFFFu, 0xFFFFF05FFFFFFFF9u, 0xF003FFFFFFFFFFFFu,
          0x0000000007FFFFFFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0xD04ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0xD04ull) % 0x40ull));
      // 2239 code units from 83 ranges (spanning a search area of 3099)
    }
    case 0x02: // [2] 1940 - 258A
    {
      if (U'\u1950' > c || c > U'\u2184')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFF001F3FFFFFFFu, 0x03FFFFFF0FFFFFFFu, 0xFFFF000000000000u,
          0xFFFFFFFFFFFF007Fu, 0x000000000000001Fu, 0x0000000000800000u,
          0xFFE0000000000000u, 0x0FE0000FFFFFFFFFu, 0xFFF8000000000000u,
          0xFFFFFC00C001FFFFu, 0xFFFF0000003FFFFFu, 0xE0000000000FFFFFu,
          0x01FF3FFFFFFFFC00u, 0x0000E7FFFFFFFFFFu, 0xFFFF046FDE000000u,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0x0000FFFFFFFFFFFFu,
          0xFFFF000000000000u, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0x3F3FFFFFFFFF3F3Fu,
          0xFFFF3FFFFFFFAAFFu, 0x1FDC5FDFFFFFFFFFu, 0x00001FDC1FFF0FCFu,
          0x0000000000000000u, 0x0000800200000000u, 0x0000000000001FFFu,
          0xFC84000000000000u, 0x43E0F3FFBD503E2Fu, 0x0018000000000000u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x1950ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0x1950ull) % 0x40ull));
      // 1184 code units from 59 ranges (spanning a search area of 2101)
    }
    case 0x03: // [3] 258B - 31D5
    {
      if (U'\u2C00' > c || c > U'\u31BF')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFF7FFFFFFFFFFFu, 0xFFFFFFFF7FFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0x000C781FFFFFFFFFu, 0xFFFF20BFFFFFFFFFu, 0x000080FFFFFFFFFFu,
          0x7F7F7F7F007FFFFFu, 0x000000007F7F7F7Fu, 0x0000800000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x183E000000000060u, 0xFFFFFFFFFFFFFFFEu,
          0xFFFFFFFEE07FFFFFu, 0xF7FFFFFFFFFFFFFFu, 0xFFFEFFFFFFFFFFE0u,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFF00007FFFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x2C00ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 771 code units from 30 ranges (spanning a search area of 1472)
    }
    case 0x04:
      return (U'\u31F0' <= c && c <= U'\u31FF') || U'\u3400' <= c;
    case 0x06:
      return c <= U'\u4DBF' || U'\u4E00' <= c;
    case 0x0C:
      return c <= U'\u9FFC' || U'\uA000' <= c;
    case 0x0D: // [13] A079 - ACC3
    {
      TOML_ASSUME(U'\uA079' <= c && c <= U'\uACC3');

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x00000000000FFFFFu, 0xFFFFFFFFFF800000u,
          0xFFFFFFFFFFFFFF9Fu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x0006007FFF8FFFFFu, 0x003FFFFFFFFFFF80u,
          0xFFFFFF9FFFFFFFC0u, 0x00001FFFFFFFFFFFu, 0xFFFFFE7FC0000000u,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFCFFFFu, 0xF00000000003FE7Fu,
          0x000003FFFFFBDDFFu, 0x07FFFFFFFFFFFF80u, 0x07FFFFFFFFFFFE00u,
          0x7E00000000000000u, 0xFF801FFFFFFE0034u, 0xFFFFFF8000003FFFu,
          0x03FFFFFFFFFFF80Fu, 0x007FEF8000400000u, 0x0000FFFFFFFFFFBEu,
          0x3FFFFF800007FB80u, 0x317FFFFFFFFFFFE2u, 0x0E03FF9C0000029Fu,
          0xFFBFBF803F3F3F00u, 0xFF81FFFBFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0x000003FFFFFFFFFFu, 0xFFFFFFFFFFFFFF80u, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x00000000000007FFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0xA079ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0xA079ull) % 0x40ull));
      // 2554 code units from 52 ranges (spanning a search area of 3147)
    }
    case 0x11:
      return c <= U'\uD7A3' || (U'\uD7B0' <= c && c <= U'\uD7C6') ||
             (U'\uD7CB' <= c && c <= U'\uD7FB');
    case 0x14: // [20] F686 - 102D0
    {
      if (U'\uF900' > c)
        return false;
      TOML_ASSUME(c <= U'\U000102D0');

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFF3FFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x0000000003FFFFFFu, 0x5F7FFDFFA0F8007Fu,
          0xFFFFFFFFFFFFFFDBu, 0x0003FFFFFFFFFFFFu, 0xFFFFFFFFFFF80000u,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x3FFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFF0000u,
          0xFFFFFFFFFFFCFFFFu, 0x0FFF0000000000FFu, 0x0000000000000000u,
          0xFFDF000000000000u, 0xFFFFFFFFFFFFFFFFu, 0x1FFFFFFFFFFFFFFFu,
          0x07FFFFFE00000000u, 0xFFFFFFC007FFFFFEu, 0x7FFFFFFFFFFFFFFFu,
          0x000000001CFCFCFCu, 0xB7FFFF7FFFFFEFFFu, 0x000000003FFF3FFFu,
          0xFFFFFFFFFFFFFFFFu, 0x07FFFFFFFFFFFFFFu, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0xFFFFFFFF1FFFFFFFu,
          0x000000000001FFFFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0xF900ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 1710 code units from 34 ranges (spanning a search area of 2513)
    }
    case 0x15: // [21] 102D1 - 10F1B
    {
      if (U'\U00010300' > c)
        return false;
      TOML_ASSUME(c <= U'\U00010F1B');

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFFE000FFFFFFFFu, 0x003FFFFFFFFF03FDu, 0xFFFFFFFF3FFFFFFFu,
          0x000000000000FF0Fu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFF00003FFFFFFFu, 0x0FFFFFFFFF0FFFFFu, 0xFFFF00FFFFFFFFFFu,
          0x0000000FFFFFFFFFu, 0x0000000000000000u, 0x0000000000000000u,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x007FFFFFFFFFFFFFu, 0x000000FF003FFFFFu,
          0x0000000000000000u, 0x0000000000000000u, 0x91BFFFFFFFFFFD3Fu,
          0x007FFFFF003FFFFFu, 0x000000007FFFFFFFu, 0x0037FFFF00000000u,
          0x03FFFFFF003FFFFFu, 0x0000000000000000u, 0xC0FFFFFFFFFFFFFFu,
          0x0000000000000000u, 0x003FFFFFFEEF0001u, 0x1FFFFFFF00000000u,
          0x000000001FFFFFFFu, 0x0000001FFFFFFEFFu, 0x003FFFFFFFFFFFFFu,
          0x0007FFFF003FFFFFu, 0x000000000003FFFFu, 0x0000000000000000u,
          0xFFFFFFFFFFFFFFFFu, 0x00000000000001FFu, 0x0007FFFFFFFFFFFFu,
          0x0007FFFFFFFFFFFFu, 0x0000000FFFFFFFFFu, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x000303FFFFFFFFFFu, 0x0000000000000000u,
          0x000000000FFFFFFFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x10300ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 1620 code units from 48 ranges (spanning a search area of 3100)
    }
    case 0x16: // [22] 10F1C - 11B66
    {
      if (c > U'\U00011AF8')
        return false;
      TOML_ASSUME(U'\U00010F1C' <= c);

      constexpr uint_least64_t bitmask_table_1[] = {
          0x000003FFFFF00801u, 0x0000000000000000u, 0x000001FFFFF00000u,
          0xFFFFFF8007FFFFF0u, 0x000000000FFFFFFFu, 0xFFFFFF8000000000u,
          0xFFF00000000FFFFFu, 0xFFFFFF8000001FFFu, 0xFFF00900000007FFu,
          0xFFFFFF80047FFFFFu, 0x400001E0007FFFFFu, 0xFFBFFFF000000001u,
          0x000000000000FFFFu, 0xFFFBD7F000000000u, 0xFFFFFFFFFFF01FFBu,
          0xFF99FE0000000007u, 0x001000023EDFDFFFu, 0x000000000000003Eu,
          0x0000000000000000u, 0xFFFFFFF000000000u, 0x0000780001FFFFFFu,
          0xFFFFFFF000000038u, 0x00000B00000FFFFFu, 0x0000000000000000u,
          0x0000000000000000u, 0xFFFFFFF000000000u, 0xF00000000007FFFFu,
          0xFFFFFFF000000000u, 0x00000100000FFFFFu, 0xFFFFFFF000000000u,
          0x0000000010007FFFu, 0x7FFFFFF000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0xFFFFFFF000000000u,
          0x000000000000FFFFu, 0x0000000000000000u, 0xFFFFFFFFFFFFFFF0u,
          0xF6FF27F80000000Fu, 0x00000028000FFFFFu, 0x0000000000000000u,
          0x001FFFFFFFFFCFF0u, 0xFFFF8010000000A0u, 0x00100000407FFFFFu,
          0x00003FFFFFFFFFFFu, 0xFFFFFFF000000002u, 0x000000001FFFFFFFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x10F1Cull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0x10F1Cull) % 0x40ull));
      // 1130 code units from 67 ranges (spanning a search area of 3037)
    }
    case 0x17: // [23] 11B67 - 127B1
    {
      if (U'\U00011C00' > c || c > U'\U00012543')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x00007FFFFFFFFDFFu, 0xFFFC000000000001u, 0x000000000000FFFFu,
          0x0000000000000000u, 0x0001FFFFFFFFFB7Fu, 0xFFFFFDBF00000040u,
          0x00000000010003FFu, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0007FFFF00000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0001000000000000u,
          0x0000000000000000u, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0x0000000003FFFFFFu, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x000000000000000Fu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x11C00ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 1304 code units from 16 ranges (spanning a search area of 2372)
    }
    case 0x18:
      return U'\U00013000' <= c;
    case 0x19:
      return c <= U'\U0001342E';
    case 0x1A:
      return U'\U00014400' <= c && c <= U'\U00014646';
    case 0x1D: // [29] 16529 - 17173
    {
      if (U'\U00016800' > c)
        return false;
      TOML_ASSUME(c <= U'\U00017173');

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0x01FFFFFFFFFFFFFFu,
          0x000000007FFFFFFFu, 0x0000000000000000u, 0x00003FFFFFFF0000u,
          0x0000FFFFFFFFFFFFu, 0xE0FFFFF80000000Fu, 0x000000000000FFFFu,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0xFFFFFFFFFFFFFFFFu, 0x0000000000000000u,
          0x0000000000000000u, 0xFFFFFFFFFFFFFFFFu, 0x00000000000107FFu,
          0x00000000FFF80000u, 0x0000000B00000000u, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x000FFFFFFFFFFFFFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x16800ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 1250 code units from 14 ranges (spanning a search area of 2420)
    }
    case 0x1F:
      return c <= U'\U000187F7' || U'\U00018800' <= c;
    case 0x20:
      return c <= U'\U00018CD5' || (U'\U00018D00' <= c && c <= U'\U00018D08');
    case 0x23: // [35] 1AEEB - 1BB35
    {
      if (U'\U0001B000' > c || c > U'\U0001B2FB')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x000000007FFFFFFFu, 0xFFFF00F000070000u,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0x0FFFFFFFFFFFFFFFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x1B000ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 690 code units from 4 ranges (spanning a search area of 764)
    }
    case 0x24: // [36] 1BB36 - 1C780
    {
      if (U'\U0001BC00' > c || c > U'\U0001BC99')
        return false;

      switch ((static_cast<uint_least64_t>(c) - 0x1BC00ull) / 0x40ull) {
      case 0x01:
        return c <= U'\U0001BC7C' &&
               (1ull << (static_cast<uint_least64_t>(c) - 0x1BC40u)) &
                   0x1FFF07FFFFFFFFFFull;
      case 0x02:
        return (1u << (static_cast<uint_least32_t>(c) - 0x1BC80u)) & 0x3FF01FFu;
      default:
        return true;
      }
      // 139 code units from 4 ranges (spanning a search area of 154)
      TOML_UNREACHABLE;
    }
    case 0x26: // [38] 1D3CC - 1E016
    {
      if (U'\U0001D400' > c || c > U'\U0001D7CB')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFDFFFFFu, 0xEBFFDE64DFFFFFFFu,
          0xFFFFFFFFFFFFFFEFu, 0x7BFFFFFFDFDFE7BFu, 0xFFFFFFFFFFFDFC5Fu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0xFFFFFF3FFFFFFFFFu, 0xF7FFFFFFF7FFFFFDu,
          0xFFDFFFFFFFDFFFFFu, 0xFFFF7FFFFFFF7FFFu, 0xFFFFFDFFFFFFFDFFu,
          0x0000000000000FF7u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x1D400ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 936 code units from 30 ranges (spanning a search area of 972)
    }
    case 0x27: // [39] 1E017 - 1EC61
    {
      if (U'\U0001E100' > c || c > U'\U0001E94B')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x3F801FFFFFFFFFFFu, 0x0000000000004000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x00000FFFFFFFFFFFu, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu,
          0xFFFFFFFFFFFFFFFFu, 0x000000000000001Fu, 0xFFFFFFFFFFFFFFFFu,
          0x000000000000080Fu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x1E100ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 363 code units from 7 ranges (spanning a search area of 2124)
    }
    case 0x28: // [40] 1EC62 - 1F8AC
    {
      if (U'\U0001EE00' > c || c > U'\U0001EEBB')
        return false;

      switch ((static_cast<uint_least64_t>(c) - 0x1EE00ull) / 0x40ull) {
      case 0x00:
        return c <= U'\U0001EE3B' &&
               (1ull << (static_cast<uint_least64_t>(c) - 0x1EE00u)) &
                   0xAF7FE96FFFFFFEFull;
      case 0x01:
        return U'\U0001EE42' <= c && c <= U'\U0001EE7E' &&
               (1ull << (static_cast<uint_least64_t>(c) - 0x1EE42u)) &
                   0x17BDFDE5AAA5BAA1ull;
      case 0x02:
        return (1ull << (static_cast<uint_least64_t>(c) - 0x1EE80u)) &
               0xFFFFBEE0FFFFBFFull;
        TOML_NO_DEFAULT_CASE;
      }
      // 141 code units from 33 ranges (spanning a search area of 188)
      TOML_UNREACHABLE;
    }
    case 0x29:
      return U'\U00020000' <= c;
    case 0x37:
      return c <= U'\U0002A6DD' || U'\U0002A700' <= c;
    case 0x38:
      return c <= U'\U0002B734' || (U'\U0002B740' <= c && c <= U'\U0002B81D') ||
             U'\U0002B820' <= c;
    case 0x3A:
      return c <= U'\U0002CEA1' || U'\U0002CEB0' <= c;
    case 0x3C:
      return c <= U'\U0002EBE0';
    case 0x3D:
      return U'\U0002F800' <= c && c <= U'\U0002FA1D';
    case 0x3E:
      return U'\U00030000' <= c;
      TOML_NO_DEFAULT_CASE;
    }
    // 131189 code units from 620 ranges (spanning a search area of 201377)
    TOML_UNREACHABLE;
  }

  //# Returns true if a codepoint belongs to any of these categories:
  //# 	Nd, Nl
  [[nodiscard]] TOML_ATTR(const) constexpr bool is_non_ascii_number(
      char32_t c) noexcept {
    if (U'\u0660' > c || c > U'\U0001FBF9')
      return false;

    const auto child_index_0 = (static_cast<uint_least64_t>(c) - 0x660ull) / 0x7D7ull;
    if ((1ull << child_index_0) & 0x47FFDFE07FCFFFD0ull)
      return false;
    switch (child_index_0) {
    case 0x00: // [0] 0660 - 0E36
    {
      if (c > U'\u0DEF')
        return false;
      TOML_ASSUME(U'\u0660' <= c);

      constexpr uint_least64_t bitmask_table_1[] = {
          0x00000000000003FFu, 0x0000000000000000u, 0x0000000003FF0000u,
          0x0000000000000000u, 0x0000000000000000u, 0x000003FF00000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x000000000000FFC0u, 0x0000000000000000u, 0x000000000000FFC0u,
          0x0000000000000000u, 0x000000000000FFC0u, 0x0000000000000000u,
          0x000000000000FFC0u, 0x0000000000000000u, 0x000000000000FFC0u,
          0x0000000000000000u, 0x000000000000FFC0u, 0x0000000000000000u,
          0x000000000000FFC0u, 0x0000000000000000u, 0x000000000000FFC0u,
          0x0000000000000000u, 0x000000000000FFC0u, 0x0000000000000000u,
          0x000000000000FFC0u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x660ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0x660ull) % 0x40ull));
      // 130 code units from 13 ranges (spanning a search area of 1936)
    }
    case 0x01: // [1] 0E37 - 160D
    {
      if (U'\u0E50' > c || c > U'\u1099')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x00000000000003FFu, 0x0000000000000000u, 0x00000000000003FFu,
          0x0000000003FF0000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x03FF000000000000u, 0x0000000000000000u,
          0x00000000000003FFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0xE50ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0xE50ull) % 0x40ull));
      // 50 code units from 5 ranges (spanning a search area of 586)
    }
    case 0x02: // [2] 160E - 1DE4
    {
      if (U'\u16EE' > c || c > U'\u1C59')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x0000000000000007u, 0x0000000000000000u, 0x0000000000000000u,
          0x0FFC000000000000u, 0x00000FFC00000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x00000003FF000000u, 0x0000000000000000u, 0x00000FFC00000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x00000FFC0FFC0000u,
          0x0000000000000000u, 0x0000000000000000u, 0x00000FFC00000000u,
          0x0000000000000000u, 0x0000000000000FFCu, 0x0000000000000000u,
          0x00000FFC0FFC0000u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x16EEull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0x16EEull) % 0x40ull));
      // 103 code units from 11 ranges (spanning a search area of 1388)
    }
    case 0x03:
      return U'\u2160' <= c && c <= U'\u2188' &&
             (1ull << (static_cast<uint_least64_t>(c) - 0x2160u)) & 0x1E7FFFFFFFFull;
    case 0x05:
      return U'\u3007' <= c && c <= U'\u303A' &&
             (1ull << (static_cast<uint_least64_t>(c) - 0x3007u)) & 0xE0007FC000001ull;
    case 0x14: // [20] A32C - AB02
    {
      if (U'\uA620' > c || c > U'\uAA59')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x00000000000003FFu, 0x0000000000000000u, 0x0000000000000000u,
          0x000000000000FFC0u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x03FF000000000000u, 0x000003FF00000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x03FF000000000000u,
          0x0000000003FF0000u, 0x03FF000000000000u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0xA620ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0xA620ull) % 0x40ull));
      // 70 code units from 7 ranges (spanning a search area of 1082)
    }
    case 0x15:
      return U'\uABF0' <= c && c <= U'\uABF9';
    case 0x1F:
      return U'\uFF10' <= c && c <= U'\uFF19';
    case 0x20: // [32] 10140 - 10916
    {
      if (c > U'\U000104A9')
        return false;
      TOML_ASSUME(U'\U00010140' <= c);

      constexpr uint_least64_t bitmask_table_1[] = {
          0x001FFFFFFFFFFFFFu, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000402u,
          0x0000000000000000u, 0x00000000003E0000u, 0x0000000000000000u,
          0x0000000000000000u, 0x000003FF00000000u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x10140ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 70 code units from 5 ranges (spanning a search area of 874)
    }
    case 0x21:
      return (U'\U00010D30' <= c && c <= U'\U00010D39') ||
             (U'\U00011066' <= c && c <= U'\U0001106F');
    case 0x22: // [34] 110EE - 118C4
    {
      if (U'\U000110F0' > c || c > U'\U00011739')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x00000000000003FFu, 0x000000000000FFC0u, 0x0000000000000000u,
          0x000003FF00000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x00000000000003FFu,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x000003FF00000000u, 0x0000000000000000u,
          0x000003FF00000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x000003FF00000000u, 0x0000000000000000u, 0x0000000003FF0000u,
          0x0000000000000000u, 0x00000000000003FFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x110F0ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0x110F0ull) % 0x40ull));
      // 90 code units from 9 ranges (spanning a search area of 1610)
    }
    case 0x23: // [35] 118C5 - 1209B
    {
      if (U'\U000118E0' > c || c > U'\U00011DA9')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x00000000000003FFu, 0x03FF000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x03FF000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x03FF000000000000u,
          0x0000000000000000u, 0x00000000000003FFu,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x118E0ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0x118E0ull) % 0x40ull));
      // 50 code units from 5 ranges (spanning a search area of 1226)
    }
    case 0x24:
      return U'\U00012400' <= c && c <= U'\U0001246E';
    case 0x2D:
      return (U'\U00016A60' <= c && c <= U'\U00016A69') ||
             (U'\U00016B50' <= c && c <= U'\U00016B59');
    case 0x3B:
      return U'\U0001D7CE' <= c && c <= U'\U0001D7FF';
    case 0x3C:
      return (U'\U0001E140' <= c && c <= U'\U0001E149') ||
             (U'\U0001E2F0' <= c && c <= U'\U0001E2F9');
    case 0x3D:
      return U'\U0001E950' <= c && c <= U'\U0001E959';
    case 0x3F:
      return U'\U0001FBF0' <= c;
      TOML_NO_DEFAULT_CASE;
    }
    // 876 code units from 72 ranges (spanning a search area of 128410)
    TOML_UNREACHABLE;
  }

  //# Returns true if a codepoint belongs to any of these categories:
  //# 	Mn, Mc
  [[nodiscard]] TOML_ATTR(const) constexpr bool is_combining_mark(char32_t c) noexcept {
    if (U'\u0300' > c || c > U'\U000E01EF')
      return false;

    const auto child_index_0 = (static_cast<uint_least64_t>(c) - 0x300ull) / 0x37FCull;
    if ((1ull << child_index_0) & 0x7FFFFFFFFFFFFE02ull)
      return false;
    switch (child_index_0) {
    case 0x00: // [0] 0300 - 3AFB
    {
      if (c > U'\u309A')
        return false;
      TOML_ASSUME(U'\u0300' <= c);

      constexpr uint_least64_t bitmask_table_1[] = {
          0xFFFFFFFFFFFFFFFFu, 0x0000FFFFFFFFFFFFu, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x00000000000000F8u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0xBFFFFFFFFFFE0000u, 0x00000000000000B6u,
          0x0000000007FF0000u, 0x00010000FFFFF800u, 0x0000000000000000u,
          0x00003D9F9FC00000u, 0xFFFF000000020000u, 0x00000000000007FFu,
          0x0001FFC000000000u, 0x200FF80000000000u, 0x00003EEFFBC00000u,
          0x000000000E000000u, 0x0000000000000000u, 0xFFFFFFFBFFF80000u,
          0xDC0000000000000Fu, 0x0000000C00FEFFFFu, 0xD00000000000000Eu,
          0x4000000C0080399Fu, 0xD00000000000000Eu, 0x0023000000023987u,
          0xD00000000000000Eu, 0xFC00000C00003BBFu, 0xD00000000000000Eu,
          0x0000000C00E0399Fu, 0xC000000000000004u, 0x0000000000803DC7u,
          0xC00000000000001Fu, 0x0000000C00603DDFu, 0xD00000000000000Eu,
          0x0000000C00603DDFu, 0xD80000000000000Fu, 0x0000000C00803DDFu,
          0x000000000000000Eu, 0x000C0000FF5F8400u, 0x07F2000000000000u,
          0x0000000000007F80u, 0x1FF2000000000000u, 0x0000000000003F00u,
          0xC2A0000003000000u, 0xFFFE000000000000u, 0x1FFFFFFFFEFFE0DFu,
          0x0000000000000040u, 0x7FFFF80000000000u, 0x001E3F9DC3C00000u,
          0x000000003C00BFFCu, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x00000000E0000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x001C0000001C0000u,
          0x000C0000000C0000u, 0xFFF0000000000000u, 0x00000000200FFFFFu,
          0x0000000000003800u, 0x0000000000000000u, 0x0000020000000060u,
          0x0000000000000000u, 0x0FFF0FFF00000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x000000000F800000u,
          0x9FFFFFFF7FE00000u, 0xBFFF000000000000u, 0x0000000000000001u,
          0xFFF000000000001Fu, 0x000FF8000000001Fu, 0x00003FFE00000007u,
          0x000FFFC000000000u, 0x00FFFFF000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x039021FFFFF70000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0xFBFFFFFFFFFFFFFFu,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0001FFE21FFF0000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0003800000000000u,
          0x0000000000000000u, 0x8000000000000000u, 0x0000000000000000u,
          0xFFFFFFFF00000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000FC0000000000u, 0x0000000000000000u, 0x0000000006000000u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x300ull) / 0x40ull] &
             (0x1ull << (static_cast<uint_least64_t>(c) % 0x40ull));
      // 1106 code units from 156 ranges (spanning a search area of 11675)
    }
    case 0x02: // [2] 72F8 - AAF3
    {
      if (U'\uA66F' > c || c > U'\uAAEF')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x0001800000007FE1u, 0x0000000000000000u, 0x0000000000000006u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x21F0000010880000u, 0x0000000000000000u, 0x0000000000060000u,
          0xFFFE0000007FFFE0u, 0x7F80000000010007u, 0x0000001FFF000000u,
          0x00000000001E0000u, 0x004000000003FFF0u, 0xFC00000000000000u,
          0x00000000601000FFu, 0x0000000000007000u, 0xF00000000005833Au,
          0x0000000000000001u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0xA66Full) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0xA66Full) % 0x40ull));
      // 137 code units from 28 ranges (spanning a search area of 1153)
    }
    case 0x03:
      return (U'\uAAF5' <= c && c <= U'\uAAF6') || (U'\uABE3' <= c && c <= U'\uABEA') ||
             (U'\uABEC' <= c && c <= U'\uABED');
    case 0x04: // [4] E2F0 - 11AEB
    {
      if (U'\uFB1E' > c || c > U'\U00011A99')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x0000000000000001u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0003FFFC00000000u,
          0x000000000003FFFCu, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000080000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000004u, 0x0000000000000000u,
          0x000000001F000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0003C1B800000000u,
          0x000000021C000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000180u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x00000000000003C0u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000006000u, 0x0000000000000000u, 0x0007FF0000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000001C00000000u,
          0x000001FFFC000000u, 0x0000001E00000000u, 0x000000001FFC0000u,
          0x0000001C00000000u, 0x00000180007FFE00u, 0x0000001C00200000u,
          0x00037807FFE00000u, 0x0000000000000000u, 0x0000000103FFC000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000003C00001FFEu,
          0x0200E67F60000000u, 0x00000000007C7F30u, 0x0000000000000000u,
          0x0000000000000000u, 0x000001FFFF800000u, 0x0000000000000001u,
          0x0000003FFFFC0000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0xC0000007FCFE0000u, 0x0000000000000000u,
          0x00000007FFFC0000u, 0x0000000000000000u, 0x0000000003FFE000u,
          0x8000000000000000u, 0x0000000000003FFFu, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x000000001FFFC000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x00000035E6FC0000u, 0x0000000000000000u, 0xF3F8000000000000u,
          0x00001FF800000047u, 0x3FF80201EFE00000u, 0x0FFFF00000000000u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0xFB1Eull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0xFB1Eull) % 0x40ull));
      // 402 code units from 63 ranges (spanning a search area of 8060)
    }
    case 0x05: // [5] 11AEC - 152E7
    {
      if (U'\U00011C2F' > c || c > U'\U00011EF6')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x000000000001FEFFu, 0xFDFFFFF800000000u, 0x00000000000000FFu,
          0x0000000000000000u, 0x00000000017F68FCu, 0x000001F6F8000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x00000000000000F0u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x11C2Full) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0x11C2Full) % 0x40ull));
      // 85 code units from 13 ranges (spanning a search area of 712)
    }
    case 0x06: // [6] 152E8 - 18AE3
    {
      if (U'\U00016AF0' > c || c > U'\U00016FF1')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x000000000000001Fu, 0x000000000000007Fu, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0xFFFFFFFE80000000u,
          0x0000000780FFFFFFu, 0x0010000000000000u, 0x0000000000000003u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x16AF0ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0x16AF0ull) % 0x40ull));
      // 75 code units from 7 ranges (spanning a search area of 1282)
    }
    case 0x07:
      return U'\U0001BC9D' <= c && c <= U'\U0001BC9E';
    case 0x08: // [8] 1C2E0 - 1FADB
    {
      if (U'\U0001D165' > c || c > U'\U0001E94A')
        return false;

      constexpr uint_least64_t bitmask_table_1[] = {
          0x0000007F3FC03F1Fu, 0x00000000000001E0u, 0x0000000000000000u,
          0x00000000E0000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0xFFFFFFFFF8000000u, 0xFFFFFFFFFFC3FFFFu,
          0xF7C00000800100FFu, 0x00000000000007FFu, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0xDFCFFFFBF8000000u, 0x000000000000003Eu,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x000000000003F800u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000780u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u,
          0x0003F80000000000u, 0x0000000000000000u, 0x0000003F80000000u,
      };
      return bitmask_table_1[(static_cast<uint_least64_t>(c) - 0x1D165ull) / 0x40ull] &
             (0x1ull << ((static_cast<uint_least64_t>(c) - 0x1D165ull) % 0x40ull));
      // 223 code units from 21 ranges (spanning a search area of 6118)
    }
    case 0x3F:
      return U'\U000E0100' <= c;
      TOML_NO_DEFAULT_CASE;
    }
    // 2282 code units from 293 ranges (spanning a search area of 917232)
    TOML_UNREACHABLE;
  }

#endif // TOML_LANG_UNRELEASED

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_bare_key_character(
      char32_t codepoint) noexcept {
    return is_ascii_letter(codepoint) || is_decimal_digit(codepoint) ||
           codepoint == U'-' || codepoint == U'_'
#if TOML_LANG_UNRELEASED // toml/issues/644 ('+' in bare keys) & toml/issues/687
                         // (unicode bare keys)
           || codepoint == U'+' || is_non_ascii_letter(codepoint) ||
           is_non_ascii_number(codepoint) || is_combining_mark(codepoint)
#endif
        ;
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_value_terminator(
      char32_t codepoint) noexcept {
    return is_ascii_line_break(codepoint) || is_ascii_whitespace(codepoint) ||
           codepoint == U']' || codepoint == U'}' || codepoint == U',' ||
           codepoint == U'#' || is_non_ascii_line_break(codepoint) ||
           is_non_ascii_whitespace(codepoint);
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_control_character(
      char32_t codepoint) noexcept {
    return codepoint <= U'\u001F' || codepoint == U'\u007F';
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_nontab_control_character(
      char32_t codepoint) noexcept {
    return codepoint <= U'\u0008' ||
           (codepoint >= U'\u000A' && codepoint <= U'\u001F') || codepoint == U'\u007F';
  }

  [[nodiscard]] TOML_ATTR(const) constexpr bool is_unicode_surrogate(
      char32_t codepoint) noexcept {
    return codepoint >= 0xD800u && codepoint <= 0xDFFF;
  }

  struct utf8_decoder final {
    // utf8_decoder based on this: https://bjoern.hoehrmann.de/utf-8/decoder/dfa/
    // Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

    uint_least32_t state{};
    char32_t codepoint{};

    static constexpr uint8_t state_table[]{
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
        7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
        7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  2,  2,  2,  2,  2,  2,
        2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
        2,  2,  2,  2,  10, 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  3,  3,
        11, 6,  6,  6,  5,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,

        0,  12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 0,  12, 12, 12, 12, 12, 0,  12, 0,  12, 12, 12, 24, 12, 12,
        12, 12, 12, 24, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12,
        12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 36,
        12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12};

    [[nodiscard]] constexpr bool error() const noexcept {
      return state == uint_least32_t{12u};
    }

    [[nodiscard]] constexpr bool has_code_point() const noexcept {
      return state == uint_least32_t{};
    }

    [[nodiscard]] constexpr bool needs_more_input() const noexcept {
      return state > uint_least32_t{} && state != uint_least32_t{12u};
    }

    constexpr void operator()(uint8_t byte) noexcept {
      TOML_ASSERT(!error());

      const auto type = state_table[byte];

      codepoint = static_cast<char32_t>(
          has_code_point() ? (uint_least32_t{255u} >> type) & byte
                           : (byte & uint_least32_t{63u}) |
                                 (static_cast<uint_least32_t>(codepoint) << 6));

      state = state_table[state + uint_least32_t{256u} + type];
    }
  };
}
TOML_IMPL_NAMESPACE_END;
/// \endcond

TOML_POP_WARNINGS; // TOML_DISABLE_SWITCH_WARNINGS
