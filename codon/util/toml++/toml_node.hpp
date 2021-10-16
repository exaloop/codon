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

#include "toml_node.h"

TOML_NAMESPACE_START
{
	TOML_EXTERNAL_LINKAGE
	node::node(const node& /*other*/) noexcept
	{
		// does not copy source information - this is not an error
		// 
		// see https://github.com/marzer/tomlplusplus/issues/49#issuecomment-665089577
	}

	TOML_EXTERNAL_LINKAGE
	node::node(node && other) noexcept
		: source_{ std::move(other.source_) }
	{
		other.source_.begin = {};
		other.source_.end = {};
	}

	TOML_EXTERNAL_LINKAGE
	node& node::operator= (const node& /*rhs*/) noexcept
	{
		// does not copy source information - this is not an error
		// 
		// see https://github.com/marzer/tomlplusplus/issues/49#issuecomment-665089577

		source_ = {};
		return *this;
	}

	TOML_EXTERNAL_LINKAGE
	node& node::operator= (node && rhs) noexcept
	{
		source_ = std::move(rhs.source_);
		rhs.source_.begin = {};
		rhs.source_.end = {};
		return *this;
	}

	#define TOML_MEMBER_ATTR(attr) TOML_EXTERNAL_LINKAGE TOML_ATTR(attr)

	TOML_MEMBER_ATTR(const) bool node::is_string()			const noexcept { return false; }
	TOML_MEMBER_ATTR(const) bool node::is_integer()			const noexcept { return false; }
	TOML_MEMBER_ATTR(const) bool node::is_floating_point()	const noexcept { return false; }
	TOML_MEMBER_ATTR(const) bool node::is_number()			const noexcept { return false; }
	TOML_MEMBER_ATTR(const) bool node::is_boolean()			const noexcept { return false; }
	TOML_MEMBER_ATTR(const) bool node::is_date()			const noexcept { return false; }
	TOML_MEMBER_ATTR(const) bool node::is_time()			const noexcept { return false; }
	TOML_MEMBER_ATTR(const) bool node::is_date_time()		const noexcept { return false; }
	TOML_MEMBER_ATTR(const) bool node::is_array_of_tables()	const noexcept { return false; }
	
	TOML_MEMBER_ATTR(const) table* node::as_table()						noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) array* node::as_array()						noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) value<std::string>* node::as_string()		noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) value<int64_t>* node::as_integer()			noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) value<double>* node::as_floating_point()	noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) value<bool>* node::as_boolean()				noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) value<date>* node::as_date()				noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) value<time>* node::as_time()				noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) value<date_time>* node::as_date_time()		noexcept { return nullptr; }
	
	TOML_MEMBER_ATTR(const) const table* node::as_table()					const noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) const array* node::as_array()					const noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) const value<std::string>* node::as_string()		const noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) const value<int64_t>* node::as_integer()		const noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) const value<double>* node::as_floating_point()	const noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) const value<bool>* node::as_boolean()			const noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) const value<date>* node::as_date()				const noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) const value<time>* node::as_time()				const noexcept { return nullptr; }
	TOML_MEMBER_ATTR(const) const value<date_time>* node::as_date_time()	const noexcept { return nullptr; }

	TOML_MEMBER_ATTR(const) const source_region& node::source()				const noexcept { return source_; }

	#undef TOML_MEMBER_ATTR

	TOML_EXTERNAL_LINKAGE
	node::operator node_view<node>() noexcept
	{
		return node_view<node>(this);
	}

	TOML_EXTERNAL_LINKAGE
	node::operator node_view<const node>() const noexcept
	{
		return node_view<const node>(this);
	}
}
TOML_NAMESPACE_END;
