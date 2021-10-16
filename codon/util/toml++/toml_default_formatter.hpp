//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license text.
// SPDX-License-Identifier: MIT

#pragma once
//# {{
#include "toml_preprocessor.h"
#if !TOML_IMPLEMENTATION
	#error This is an implementation-only header.
#endif
//# }}

#include "toml_default_formatter.h"
TOML_DISABLE_WARNINGS;
#include <cmath>
TOML_ENABLE_WARNINGS;

TOML_PUSH_WARNINGS;
TOML_DISABLE_SWITCH_WARNINGS;
TOML_DISABLE_ARITHMETIC_WARNINGS;

TOML_IMPL_NAMESPACE_START
{
	inline constexpr size_t default_formatter_line_wrap = 120_sz;

	TOML_EXTERNAL_LINKAGE
	std::string default_formatter_make_key_segment(const std::string& str) noexcept
	{
		if (str.empty())
			return "''"s;
		else
		{
			bool requires_quotes = false;
			{
				utf8_decoder decoder;
				for (size_t i = 0; i < str.length() && !requires_quotes; i++)
				{
					decoder(static_cast<uint8_t>(str[i]));
					if (decoder.error())
						requires_quotes = true;
					else if (decoder.has_code_point())
						requires_quotes = !is_bare_key_character(decoder.codepoint);
				}
			}

			if (requires_quotes)
			{
				std::string s;
				s.reserve(str.length() + 2_sz);
				s += '"';
				for (auto c : str)
				{
					if TOML_UNLIKELY(c >= '\x00' && c <= '\x1F')
						s.append(low_character_escape_table[c]);
					else if TOML_UNLIKELY(c == '\x7F')
						s.append("\\u007F"sv);
					else if TOML_UNLIKELY(c == '"')
						s.append("\\\""sv);
					else
						s += c;
				}
				s += '"';
				return s;
			}
			else
				return str;
		}
	}

	TOML_EXTERNAL_LINKAGE
	size_t default_formatter_inline_columns(const node& node) noexcept
	{
		switch (node.type())
		{
			case node_type::table:
			{
				auto& n = *reinterpret_cast<const table*>(&node);
				if (n.empty())
					return 2_sz; // "{}"
				size_t weight = 3_sz; // "{ }"
				for (auto&& [k, v] : n)
				{
					weight += k.length() + default_formatter_inline_columns(v) + 2_sz; // +  ", "
					if (weight >= default_formatter_line_wrap)
						break;
				}
				return weight;
			}

			case node_type::array:
			{
				auto& n = *reinterpret_cast<const array*>(&node);
				if (n.empty())
					return 2_sz; // "[]"
				size_t weight = 3_sz; // "[ ]"
				for (auto& elem : n)
				{
					weight += default_formatter_inline_columns(elem) + 2_sz; // +  ", "
					if (weight >= default_formatter_line_wrap)
						break;
				}
				return weight;
			}

			case node_type::string:
			{
				auto& n = *reinterpret_cast<const value<std::string>*>(&node);
				return n.get().length() + 2_sz; // + ""
			}

			case node_type::integer:
			{
				auto& n = *reinterpret_cast<const value<int64_t>*>(&node);
				auto v = n.get();
				if (!v)
					return 1_sz;
				size_t weight = {};
				if (v < 0)
				{
					weight += 1;
					v *= -1;
				}
				return weight + static_cast<size_t>(log10(static_cast<double>(v))) + 1_sz;
			}

			case node_type::floating_point:
			{
				auto& n = *reinterpret_cast<const value<double>*>(&node);
				auto v = n.get();
				if (v == 0.0)
					return 3_sz;  // "0.0"
				size_t weight = 2_sz; // ".0"
				if (v < 0.0)
				{
					weight += 1;
					v *= -1.0;
				}
				return weight + static_cast<size_t>(log10(v)) + 1_sz;
				break;
			}

			case node_type::boolean: return 5_sz;
			case node_type::date: [[fallthrough]];
			case node_type::time: return 10_sz;
			case node_type::date_time: return 30_sz;
			case node_type::none: TOML_UNREACHABLE;
			TOML_NO_DEFAULT_CASE;
		}

		TOML_UNREACHABLE;
	}

	TOML_EXTERNAL_LINKAGE
	bool default_formatter_forces_multiline(const node& node, size_t starting_column_bias) noexcept
	{
		return (default_formatter_inline_columns(node) + starting_column_bias) >= default_formatter_line_wrap;
	}
}
TOML_IMPL_NAMESPACE_END;

TOML_NAMESPACE_START
{
	template <typename Char>
	inline void default_formatter<Char>::print_inline(const toml::table& tbl)
	{
		if (tbl.empty())
			impl::print_to_stream("{}"sv, base::stream());
		else
		{
			impl::print_to_stream("{ "sv, base::stream());

			bool first = false;
			for (auto&& [k, v] : tbl)
			{
				if (first)
					impl::print_to_stream(", "sv, base::stream());
				first = true;

				print_key_segment(k);
				impl::print_to_stream(" = "sv, base::stream());

				const auto type = v.type();
				TOML_ASSUME(type != node_type::none);
				switch (type)
				{
					case node_type::table: print_inline(*reinterpret_cast<const table*>(&v)); break;
					case node_type::array: print(*reinterpret_cast<const array*>(&v)); break;
					default:
						base::print_value(v, type);
				}
			}

			impl::print_to_stream(" }"sv, base::stream());
		}
		base::clear_naked_newline();
	}
}
TOML_NAMESPACE_END;

// implementations of windows wide string nonsense
#if TOML_WINDOWS_COMPAT

#ifndef _WINDOWS_
	#if TOML_INCLUDE_WINDOWS_H
		#include <Windows.h>
	#else
		extern "C"
		{
			__declspec(dllimport)
			int __stdcall WideCharToMultiByte(
				unsigned int CodePage,
				unsigned long dwFlags,
				const wchar_t* lpWideCharStr,
				int cchWideChar,
				char* lpMultiByteStr,
				int cbMultiByte,
				const char* lpDefaultChar,
				int* lpUsedDefaultChar
			);
 
			__declspec(dllimport)
			int __stdcall MultiByteToWideChar(
				unsigned int CodePage,
				unsigned long dwFlags,
				const char* lpMultiByteStr,
				int cbMultiByte,
				wchar_t* lpWideCharStr,
				int cchWideChar
			);
		}
	#endif
#endif // _WINDOWS_

TOML_IMPL_NAMESPACE_START
{
	TOML_EXTERNAL_LINKAGE
	std::string narrow(std::wstring_view str) noexcept
	{
		if (str.empty())
			return {};

		std::string s;
		const auto len = ::WideCharToMultiByte(
			65001, 0, str.data(), static_cast<int>(str.length()), nullptr, 0, nullptr, nullptr
		);
		if (len)
		{
			s.resize(static_cast<size_t>(len));
			::WideCharToMultiByte(65001, 0, str.data(), static_cast<int>(str.length()), s.data(), len, nullptr, nullptr);
		}
		return s;
	}

	TOML_EXTERNAL_LINKAGE
	std::wstring widen(std::string_view str) noexcept
	{
		if (str.empty())
			return {};

		std::wstring s;
		const auto len = ::MultiByteToWideChar(65001, 0, str.data(), static_cast<int>(str.length()), nullptr, 0);
		if (len)
		{
			s.resize(static_cast<size_t>(len));
			::MultiByteToWideChar(65001, 0, str.data(), static_cast<int>(str.length()), s.data(), len);
		}
		return s;
	}

	#if TOML_HAS_CHAR8

	TOML_EXTERNAL_LINKAGE
	std::wstring widen(std::u8string_view str) noexcept
	{
		if (str.empty())
			return {};

		return widen(std::string_view{ reinterpret_cast<const char*>(str.data()), str.length() });
	}

	#endif // TOML_HAS_CHAR8
}
TOML_IMPL_NAMESPACE_END;

#endif // TOML_WINDOWS_COMPAT

TOML_POP_WARNINGS; // TOML_DISABLE_SWITCH_WARNINGS, TOML_DISABLE_ARITHMETIC_WARNINGS
