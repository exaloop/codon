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

#include "toml_array.h"

TOML_NAMESPACE_START
{
	#if TOML_LIFETIME_HOOKS

	TOML_EXTERNAL_LINKAGE
	void array::lh_ctor() noexcept
	{
		TOML_ARRAY_CREATED;
	}

	TOML_EXTERNAL_LINKAGE
	void array::lh_dtor() noexcept
	{
		TOML_ARRAY_DESTROYED;
	}

	#endif

	TOML_EXTERNAL_LINKAGE
	array::array() noexcept
	{
		#if TOML_LIFETIME_HOOKS
		lh_ctor();
		#endif
	}

	TOML_EXTERNAL_LINKAGE
	array::array(const array& other) noexcept
		: node( other )
	{
		elements.reserve(other.elements.size());
		for (const auto& elem : other)
			elements.emplace_back(impl::make_node(elem));

		#if TOML_LIFETIME_HOOKS
		lh_ctor();
		#endif
	}

	TOML_EXTERNAL_LINKAGE
	array::array(array&& other) noexcept
		: node( std::move(other) ),
		elements{ std::move(other.elements) }
	{
		#if TOML_LIFETIME_HOOKS
		lh_ctor();
		#endif
	}

	TOML_EXTERNAL_LINKAGE
	array& array::operator= (const array& rhs) noexcept
	{
		if (&rhs != this)
		{
			node::operator=(rhs);
			elements.clear();
			elements.reserve(rhs.elements.size());
			for (const auto& elem : rhs)
				elements.emplace_back(impl::make_node(elem));
		}
		return *this;
	}

	TOML_EXTERNAL_LINKAGE
	array& array::operator= (array&& rhs) noexcept
	{
		if (&rhs != this)
		{
			node::operator=(std::move(rhs));
			elements = std::move(rhs.elements);
		}
		return *this;
	}

	TOML_EXTERNAL_LINKAGE
	array::~array() noexcept
	{
		#if TOML_LIFETIME_HOOKS
		lh_dtor();
		#endif
	}

	TOML_EXTERNAL_LINKAGE
	void array::preinsertion_resize(size_t idx, size_t count) noexcept
	{
		TOML_ASSERT(idx <= elements.size());
		TOML_ASSERT(count >= 1_sz);
		const auto old_size = elements.size();
		const auto new_size = old_size + count;
		const auto inserting_at_end = idx == old_size;
		elements.resize(new_size);
		if (!inserting_at_end)
		{
			for(size_t left = old_size, right = new_size - 1_sz; left --> idx; right--)
				elements[right] = std::move(elements[left]);
		}
	}

	#define TOML_MEMBER_ATTR(attr) TOML_EXTERNAL_LINKAGE TOML_ATTR(attr)

	TOML_MEMBER_ATTR(const) node_type array::type()			const noexcept	{ return node_type::array; }
	TOML_MEMBER_ATTR(const) bool array::is_table()			const noexcept	{ return false; }
	TOML_MEMBER_ATTR(const) bool array::is_array()			const noexcept	{ return true; }
	TOML_MEMBER_ATTR(const) bool array::is_value()			const noexcept	{ return false; }
	TOML_MEMBER_ATTR(const) const array* array::as_array()	const noexcept	{ return this; }
	TOML_MEMBER_ATTR(const) array* array::as_array()		noexcept		{ return this; }

	TOML_MEMBER_ATTR(pure) const node& array::operator[] (size_t index)	const noexcept	{ return *elements[index]; }
	TOML_MEMBER_ATTR(pure) node& array::operator[] (size_t index)		noexcept		{ return *elements[index]; }

	TOML_MEMBER_ATTR(pure) const node& array::front()				const noexcept	{ return *elements.front(); }
	TOML_MEMBER_ATTR(pure) const node& array::back()				const noexcept	{ return *elements.back(); }
	TOML_MEMBER_ATTR(pure) node& array::front()						noexcept		{ return *elements.front(); }
	TOML_MEMBER_ATTR(pure) node& array::back()						noexcept		{ return *elements.back(); }

	TOML_MEMBER_ATTR(pure) array::const_iterator array::begin()		const noexcept	{ return { elements.begin() }; }
	TOML_MEMBER_ATTR(pure) array::const_iterator array::end()		const noexcept	{ return { elements.end() }; }
	TOML_MEMBER_ATTR(pure) array::const_iterator array::cbegin()	const noexcept	{ return { elements.cbegin() }; }
	TOML_MEMBER_ATTR(pure) array::const_iterator array::cend()		const noexcept	{ return { elements.cend() }; }
	TOML_MEMBER_ATTR(pure) array::iterator array::begin()			noexcept		{ return { elements.begin() }; }
	TOML_MEMBER_ATTR(pure) array::iterator array::end()				noexcept		{ return { elements.end() }; }

	TOML_MEMBER_ATTR(pure) size_t array::size()						const noexcept	{ return elements.size(); }
	TOML_MEMBER_ATTR(pure) size_t array::capacity()					const noexcept	{ return elements.capacity(); }
	TOML_MEMBER_ATTR(pure) bool array::empty()						const noexcept	{ return elements.empty(); }
	TOML_MEMBER_ATTR(const) size_t array::max_size()				const noexcept	{ return elements.max_size(); }

	TOML_EXTERNAL_LINKAGE void array::reserve(size_t new_capacity)	{ elements.reserve(new_capacity); }
	TOML_EXTERNAL_LINKAGE void array::clear() noexcept				{ elements.clear(); }
	TOML_EXTERNAL_LINKAGE void array::shrink_to_fit()				{ elements.shrink_to_fit(); }

	#undef TOML_MEMBER_ATTR

	TOML_EXTERNAL_LINKAGE
	bool array::is_homogeneous(node_type ntype) const noexcept
	{
		if (elements.empty())
			return false;
		
		if (ntype == node_type::none)
			ntype = elements[0]->type();

		for (const auto& val : elements)
			if (val->type() != ntype)
				return false;

		return true;
	}

	namespace impl
	{
		template <typename T, typename U>
		TOML_INTERNAL_LINKAGE
		bool array_is_homogeneous(T& elements, node_type ntype, U& first_nonmatch) noexcept
		{
			if (elements.empty())
			{
				first_nonmatch = {};
				return false;
			}
			if (ntype == node_type::none)
				ntype = elements[0]->type();
			for (const auto& val : elements)
			{
				if (val->type() != ntype)
				{
					first_nonmatch = val.get();
					return false;
				}
			}
			return true;
		}
	}

	TOML_EXTERNAL_LINKAGE
	bool array::is_homogeneous(node_type ntype, toml::node*& first_nonmatch) noexcept
	{
		return impl::array_is_homogeneous(elements, ntype, first_nonmatch);
	}

	TOML_EXTERNAL_LINKAGE
	bool array::is_homogeneous(node_type ntype, const toml::node*& first_nonmatch) const noexcept
	{
		return impl::array_is_homogeneous(elements, ntype, first_nonmatch);
	}

	TOML_EXTERNAL_LINKAGE
	void array::truncate(size_t new_size)
	{
		if (new_size < elements.size())
			elements.resize(new_size);
	}

	TOML_EXTERNAL_LINKAGE
	array::iterator array::erase(const_iterator pos) noexcept
	{
		return { elements.erase(pos.raw_) };
	}

	TOML_EXTERNAL_LINKAGE
	array::iterator array::erase(const_iterator first, const_iterator last) noexcept
	{
		return { elements.erase(first.raw_, last.raw_) };
	}

	TOML_EXTERNAL_LINKAGE
	void array::pop_back() noexcept
	{
		elements.pop_back();
	}

	TOML_EXTERNAL_LINKAGE
	TOML_ATTR(pure)
	node* array::get(size_t index) noexcept
	{
		return index < elements.size() ? elements[index].get() : nullptr;
	}

	TOML_EXTERNAL_LINKAGE
	TOML_ATTR(pure)
	const node* array::get(size_t index) const noexcept
	{
		return index < elements.size() ? elements[index].get() : nullptr;
	}

	TOML_EXTERNAL_LINKAGE
	bool operator == (const array& lhs, const array& rhs) noexcept
	{
		if (&lhs == &rhs)
			return true;
		if (lhs.elements.size() != rhs.elements.size())
			return false;
		for (size_t i = 0, e = lhs.elements.size(); i < e; i++)
		{
			const auto lhs_type = lhs.elements[i]->type();
			const node& rhs_ = *rhs.elements[i];
			const auto rhs_type = rhs_.type();
			if (lhs_type != rhs_type)
				return false;

			const bool equal = lhs.elements[i]->visit([&](const auto& lhs_) noexcept
			{
				return lhs_ == *reinterpret_cast<std::remove_reference_t<decltype(lhs_)>*>(&rhs_);
			});
			if (!equal)
				return false;
		}
		return true;
	}

	TOML_EXTERNAL_LINKAGE
	bool operator != (const array& lhs, const array& rhs) noexcept
	{
		return !(lhs == rhs);
	}

	TOML_EXTERNAL_LINKAGE
	size_t array::total_leaf_count() const noexcept
	{
		size_t leaves{};
		for (size_t i = 0, e = elements.size(); i < e; i++)
		{
			auto arr = elements[i]->as_array();
			leaves += arr ? arr->total_leaf_count() : 1_sz;
		}
		return leaves;
	}

	TOML_EXTERNAL_LINKAGE
	void array::flatten_child(array&& child, size_t& dest_index) noexcept
	{
		for (size_t i = 0, e = child.size(); i < e; i++)
		{
			auto type = child.elements[i]->type();
			if (type == node_type::array)
			{
				array& arr = *reinterpret_cast<array*>(child.elements[i].get());
				if (!arr.empty())
					flatten_child(std::move(arr), dest_index);
			}
			else
				elements[dest_index++] = std::move(child.elements[i]);
		}
	}

	TOML_EXTERNAL_LINKAGE
	array& array::flatten() &
	{
		if (elements.empty())
			return *this;

		bool requires_flattening = false;
		size_t size_after_flattening = elements.size();
		for (size_t i = elements.size(); i --> 0_sz;)
		{
			auto arr = elements[i]->as_array();
			if (!arr)
				continue;
			size_after_flattening--; //discount the array itself
			const auto leaf_count = arr->total_leaf_count();
			if (leaf_count > 0_sz)
			{
				requires_flattening = true;
				size_after_flattening += leaf_count;
			}
			else
				elements.erase(elements.cbegin() + static_cast<ptrdiff_t>(i));
		}

		if (!requires_flattening)
			return *this;

		elements.reserve(size_after_flattening);

		size_t i = 0;
		while (i < elements.size())
		{
			auto arr = elements[i]->as_array();
			if (!arr)
			{
				i++;
				continue;
			}

			std::unique_ptr<node> arr_storage = std::move(elements[i]);
			const auto leaf_count = arr->total_leaf_count();
			if (leaf_count > 1_sz)
				preinsertion_resize(i + 1_sz, leaf_count - 1_sz);
			flatten_child(std::move(*arr), i); //increments i
		}

		return *this;
	}

	TOML_EXTERNAL_LINKAGE
	bool array::is_array_of_tables() const noexcept
	{
		return is_homogeneous(node_type::table);
	}
}
TOML_NAMESPACE_END;
