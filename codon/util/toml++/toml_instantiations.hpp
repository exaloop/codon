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
#if TOML_HEADER_ONLY && !TOML_INTELLISENSE
	#error This header cannot not be included when TOML_HEADER_ONLY is enabled.
#endif
//# }}

TOML_DISABLE_WARNINGS;
#include <ostream>
#include <istream>
TOML_ENABLE_WARNINGS;

#include "toml_node_view.h"
#include "toml_default_formatter.h"
#include "toml_json_formatter.h"
#if TOML_PARSER
	#include "toml_parser.h"
#endif

// internal implementation namespace
TOML_IMPL_NAMESPACE_START
{
	// formatters
	template class formatter<char>;

	// print to stream machinery
	template void print_floating_point_to_stream(double, std::ostream&, bool);
}
TOML_IMPL_NAMESPACE_END;

// public namespace
TOML_NAMESPACE_START
{
	// value<>
	template class value<std::string>;
	template class value<int64_t>;
	template class value<double>;
	template class value<bool>;
	template class value<date>;
	template class value<time>;
	template class value<date_time>;

	// node_view
	template class node_view<node>;
	template class node_view<const node>;

	// formatters
	template class default_formatter<char>;
	template class json_formatter<char>;

	// various ostream operators
	template std::ostream& operator << (std::ostream&, const source_position&);
	template std::ostream& operator << (std::ostream&, const source_region&);
	template std::ostream& operator << (std::ostream&, const date&);
	template std::ostream& operator << (std::ostream&, const time&);
	template std::ostream& operator << (std::ostream&, const time_offset&);
	template std::ostream& operator << (std::ostream&, const date_time&);
	template std::ostream& operator << (std::ostream&, const value<std::string>&);
	template std::ostream& operator << (std::ostream&, const value<int64_t>&);
	template std::ostream& operator << (std::ostream&, const value<double>&);
	template std::ostream& operator << (std::ostream&, const value<bool>&);
	template std::ostream& operator << (std::ostream&, const value<toml::date>&);
	template std::ostream& operator << (std::ostream&, const value<toml::time>&);
	template std::ostream& operator << (std::ostream&, const value<toml::date_time>&);
	template std::ostream& operator << (std::ostream&, default_formatter<char>&);
	template std::ostream& operator << (std::ostream&, default_formatter<char>&&);
	template std::ostream& operator << (std::ostream&, json_formatter<char>&);
	template std::ostream& operator << (std::ostream&, json_formatter<char>&&);
	template std::ostream& operator << (std::ostream&, const table&);
	template std::ostream& operator << (std::ostream&, const array&);
	template std::ostream& operator << (std::ostream&, const node_view<node>&);
	template std::ostream& operator << (std::ostream&, const node_view<const node>&);
	template std::ostream& operator << (std::ostream&, node_type);

	// node::value, node_view:::value etc
	#define TOML_INSTANTIATE(name, T)														\
		template optional<T>		node::name<T>() const noexcept;					\
		template optional<T>		node_view<node>::name<T>() const noexcept;		\
		template optional<T>		node_view<const node>::name<T>() const noexcept
	TOML_INSTANTIATE(value_exact, std::string_view);
	TOML_INSTANTIATE(value_exact, std::string);
	TOML_INSTANTIATE(value_exact, const char*);
	TOML_INSTANTIATE(value_exact, int64_t);
	TOML_INSTANTIATE(value_exact, double);
	TOML_INSTANTIATE(value_exact, date);
	TOML_INSTANTIATE(value_exact, time);
	TOML_INSTANTIATE(value_exact, date_time);
	TOML_INSTANTIATE(value_exact, bool);
	TOML_INSTANTIATE(value, std::string_view);
	TOML_INSTANTIATE(value, std::string);
	TOML_INSTANTIATE(value, const char*);
	TOML_INSTANTIATE(value, signed char);
	TOML_INSTANTIATE(value, signed short);
	TOML_INSTANTIATE(value, signed int);
	TOML_INSTANTIATE(value, signed long);
	TOML_INSTANTIATE(value, signed long long);
	TOML_INSTANTIATE(value, unsigned char);
	TOML_INSTANTIATE(value, unsigned short);
	TOML_INSTANTIATE(value, unsigned int);
	TOML_INSTANTIATE(value, unsigned long);
	TOML_INSTANTIATE(value, unsigned long long);
	TOML_INSTANTIATE(value, double);
	TOML_INSTANTIATE(value, float);
	TOML_INSTANTIATE(value, date);
	TOML_INSTANTIATE(value, time);
	TOML_INSTANTIATE(value, date_time);
	TOML_INSTANTIATE(value, bool);
	#if TOML_HAS_CHAR8
	TOML_INSTANTIATE(value_exact, std::u8string_view);
	TOML_INSTANTIATE(value_exact, std::u8string);
	TOML_INSTANTIATE(value_exact, const char8_t*);
	TOML_INSTANTIATE(value, std::u8string_view);
	TOML_INSTANTIATE(value, std::u8string);
	TOML_INSTANTIATE(value, const char8_t*);
	#endif
	#if TOML_WINDOWS_COMPAT
	TOML_INSTANTIATE(value_exact, std::wstring);
	TOML_INSTANTIATE(value, std::wstring);
	#endif
	#undef TOML_INSTANTIATE

	// parser instantiations
	#if TOML_PARSER

		// parse error ostream
		template std::ostream& operator << (std::ostream&, const parse_error&);

		// parse() and parse_file()
		TOML_ABI_NAMESPACE_BOOL(TOML_EXCEPTIONS, ex, noex);

		template parse_result parse(std::istream&, std::string_view) TOML_MAY_THROW;
		template parse_result parse(std::istream&, std::string&&) TOML_MAY_THROW;

		TOML_ABI_NAMESPACE_END; // TOML_EXCEPTIONS

	#endif // TOML_PARSER
}
TOML_NAMESPACE_END;
