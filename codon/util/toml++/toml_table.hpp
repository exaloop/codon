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

#include "toml_table.h"
#include "toml_node_view.h"

TOML_NAMESPACE_START
{
	#if TOML_LIFETIME_HOOKS

	TOML_EXTERNAL_LINKAGE
	void table::lh_ctor() noexcept
	{
		TOML_TABLE_CREATED;
	}

	TOML_EXTERNAL_LINKAGE
	void table::lh_dtor() noexcept
	{
		TOML_TABLE_DESTROYED;
	}

	#endif

	TOML_EXTERNAL_LINKAGE
	table::table() noexcept
	{
		#if TOML_LIFETIME_HOOKS
		lh_ctor();
		#endif
	}

	TOML_EXTERNAL_LINKAGE
	table::table(const table& other) noexcept
		: node( other ),
		inline_{ other.inline_ }
	{
		for (auto&& [k, v] : other)
			map.emplace_hint(map.end(), k, impl::make_node(v));

		#if TOML_LIFETIME_HOOKS
		lh_ctor();
		#endif
	}

	TOML_EXTERNAL_LINKAGE
	table::table(table&& other) noexcept
		: node( std::move(other) ),
		map{ std::move(other.map) },
		inline_{ other.inline_ }
	{
		#if TOML_LIFETIME_HOOKS
		lh_ctor();
		#endif
	}

	TOML_EXTERNAL_LINKAGE
	table& table::operator= (const table& rhs) noexcept
	{
		if (&rhs != this)
		{
			node::operator=(rhs);
			map.clear();
			for (auto&& [k, v] : rhs)
				map.emplace_hint(map.end(), k, impl::make_node(v));
			inline_ = rhs.inline_;
		}
		return *this;
	}

	TOML_EXTERNAL_LINKAGE
	table& table::operator= (table&& rhs) noexcept
	{
		if (&rhs != this)
		{
			node::operator=(std::move(rhs));
			map = std::move(rhs.map);
			inline_ = rhs.inline_;
		}
		return *this;
	}

	TOML_EXTERNAL_LINKAGE
	table::~table() noexcept
	{
		#if TOML_LIFETIME_HOOKS
		lh_dtor();
		#endif
	}

	TOML_EXTERNAL_LINKAGE
	table::table(impl::table_init_pair* pairs, size_t count) noexcept
	{
		for (size_t i = 0; i < count; i++)
		{
			if (!pairs[i].value) // empty node_views
				continue;
			map.insert_or_assign(
				std::move(pairs[i].key),
				std::move(pairs[i].value)
			);
		}
	}

	#define TOML_MEMBER_ATTR(attr) TOML_EXTERNAL_LINKAGE TOML_ATTR(attr)

	TOML_MEMBER_ATTR(const) node_type table::type()				const noexcept	{ return node_type::table; }
	TOML_MEMBER_ATTR(const) bool table::is_table()				const noexcept	{ return true; }
	TOML_MEMBER_ATTR(const) bool table::is_array()				const noexcept	{ return false; }
	TOML_MEMBER_ATTR(const) bool table::is_value()				const noexcept	{ return false; }
	TOML_MEMBER_ATTR(const) const table* table::as_table()		const noexcept	{ return this; }
	TOML_MEMBER_ATTR(const) table* table::as_table()			noexcept		{ return this; }

	TOML_MEMBER_ATTR(pure) bool table::is_inline()				const noexcept	{ return inline_; }
	TOML_EXTERNAL_LINKAGE void table::is_inline(bool val)		noexcept		{ inline_ = val; }

	TOML_EXTERNAL_LINKAGE table::const_iterator table::begin()	const noexcept	{ return { map.begin() }; }
	TOML_EXTERNAL_LINKAGE table::const_iterator table::end()	const noexcept	{ return { map.end() }; }
	TOML_EXTERNAL_LINKAGE table::const_iterator table::cbegin()	const noexcept	{ return { map.cbegin() }; }
	TOML_EXTERNAL_LINKAGE table::const_iterator table::cend()	const noexcept	{ return { map.cend() }; }
	TOML_EXTERNAL_LINKAGE table::iterator table::begin()		noexcept		{ return { map.begin() }; }
	TOML_EXTERNAL_LINKAGE table::iterator table::end()			noexcept		{ return { map.end() }; }

	TOML_MEMBER_ATTR(pure) bool table::empty()					const noexcept	{ return map.empty(); }
	TOML_MEMBER_ATTR(pure) size_t table::size()					const noexcept	{ return map.size(); }
	TOML_EXTERNAL_LINKAGE void table::clear()					noexcept		{ map.clear(); }

	#undef TOML_MEMBER_ATTR

	TOML_EXTERNAL_LINKAGE
	bool table::is_homogeneous(node_type ntype) const noexcept
	{
		if (map.empty())
			return false;
		
		if (ntype == node_type::none)
			ntype = map.cbegin()->second->type();

		for (const auto& [k, v] : map)
		{
			(void)k;
			if (v->type() != ntype)
				return false;
		}

		return true;
	}

	namespace impl
	{
		template <typename T, typename U>
		TOML_INTERNAL_LINKAGE
		bool table_is_homogeneous(T& map, node_type ntype, U& first_nonmatch) noexcept
		{
			if (map.empty())
			{
				first_nonmatch = {};
				return false;
			}
			if (ntype == node_type::none)
				ntype = map.cbegin()->second->type();
			for (const auto& [k, v] : map)
			{
				(void)k;
				if (v->type() != ntype)
				{
					first_nonmatch = v.get();
					return false;
				}
			}
			return true;
		}
	}

	TOML_EXTERNAL_LINKAGE
	bool table::is_homogeneous(node_type ntype, toml::node*& first_nonmatch) noexcept
	{
		return impl::table_is_homogeneous(map, ntype, first_nonmatch);
	}

	TOML_EXTERNAL_LINKAGE
	bool table::is_homogeneous(node_type ntype, const toml::node*& first_nonmatch) const noexcept
	{
		return impl::table_is_homogeneous(map, ntype, first_nonmatch);
	}

	TOML_EXTERNAL_LINKAGE
	node_view<node> table::operator[] (std::string_view key) noexcept
	{
		return node_view<node>{ this->get(key) };
	}
	TOML_EXTERNAL_LINKAGE
	node_view<const node> table::operator[] (std::string_view key) const noexcept
	{
		return node_view<const node>{ this->get(key) };
	}

	TOML_EXTERNAL_LINKAGE
	table::iterator table::erase(iterator pos) noexcept
	{
		return { map.erase(pos.raw_) };
	}

	TOML_EXTERNAL_LINKAGE
	table::iterator table::erase(const_iterator pos) noexcept
	{
		return { map.erase(pos.raw_) };
	}

	TOML_EXTERNAL_LINKAGE
	table::iterator table::erase(const_iterator first, const_iterator last) noexcept
	{
		return { map.erase(first.raw_, last.raw_) };
	}

	TOML_EXTERNAL_LINKAGE
	bool table::erase(std::string_view key) noexcept
	{
		if (auto it = map.find(key); it != map.end())
		{
			map.erase(it);
			return true;
		}
		return false;
	}

	TOML_EXTERNAL_LINKAGE
	node* table::get(std::string_view key) noexcept
	{
		return do_get(map, key);
	}

	TOML_EXTERNAL_LINKAGE
	const node* table::get(std::string_view key) const noexcept
	{
		return do_get(map, key);
	}

	TOML_EXTERNAL_LINKAGE
	table::iterator table::find(std::string_view key) noexcept
	{
		return { map.find(key) };
	}

	TOML_EXTERNAL_LINKAGE
	table::const_iterator table::find(std::string_view key) const noexcept
	{
		return { map.find(key) };
	}

	TOML_EXTERNAL_LINKAGE
	bool table::contains(std::string_view key) const noexcept
	{
		return do_contains(map, key);
	}

	#if TOML_WINDOWS_COMPAT

	TOML_EXTERNAL_LINKAGE
	node_view<node> table::operator[] (std::wstring_view key) noexcept
	{
		return node_view<node>{ this->get(key) };
	}
	TOML_EXTERNAL_LINKAGE
	node_view<const node> table::operator[] (std::wstring_view key) const noexcept
	{
		return node_view<const node>{ this->get(key) };
	}

	TOML_EXTERNAL_LINKAGE
	bool table::erase(std::wstring_view key) noexcept
	{
		return erase(impl::narrow(key));
	}

	TOML_EXTERNAL_LINKAGE
	node* table::get(std::wstring_view key) noexcept
	{
		return get(impl::narrow(key));
	}

	TOML_EXTERNAL_LINKAGE
	const node* table::get(std::wstring_view key) const noexcept
	{
		return get(impl::narrow(key));
	}

	TOML_EXTERNAL_LINKAGE
	table::iterator table::find(std::wstring_view key) noexcept
	{
		return find(impl::narrow(key));
	}

	TOML_EXTERNAL_LINKAGE
	table::const_iterator table::find(std::wstring_view key) const noexcept
	{
		return find(impl::narrow(key));
	}

	TOML_EXTERNAL_LINKAGE
	bool table::contains(std::wstring_view key) const noexcept
	{
		return contains(impl::narrow(key));
	}

	#endif // TOML_WINDOWS_COMPAT

	TOML_EXTERNAL_LINKAGE
	bool operator == (const table& lhs, const table& rhs) noexcept
	{
		if (&lhs == &rhs)
			return true;
		if (lhs.map.size() != rhs.map.size())
			return false;

		for (auto l = lhs.map.begin(), r = rhs.map.begin(), e = lhs.map.end(); l != e; l++, r++)
		{
			if (l->first != r->first)
				return false;

			const auto lhs_type = l->second->type();
			const node& rhs_ = *r->second;
			const auto rhs_type = rhs_.type();
			if (lhs_type != rhs_type)
				return false;

			const bool equal = l->second->visit([&](const auto& lhs_) noexcept
			{
				return lhs_ == *reinterpret_cast<std::remove_reference_t<decltype(lhs_)>*>(&rhs_);
			});
			if (!equal)
				return false;
		}
		return true;
	}

	TOML_EXTERNAL_LINKAGE
	bool operator != (const table& lhs, const table& rhs) noexcept
	{
		return !(lhs == rhs);
	}
}
TOML_NAMESPACE_END;
