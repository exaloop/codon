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
#if !TOML_PARSER
	#error This header cannot not be included when TOML_PARSER is disabled.
#endif
//# }}

#include "toml_utf8_streams.h"

#if !TOML_EXCEPTIONS
	#undef TOML_ERROR_CHECK
	#define TOML_ERROR_CHECK	if (reader.error()) return nullptr
#endif

TOML_IMPL_NAMESPACE_START
{
	TOML_ABI_NAMESPACE_BOOL(TOML_EXCEPTIONS, ex, noex);

	TOML_EXTERNAL_LINKAGE
	utf8_buffered_reader::utf8_buffered_reader(utf8_reader_interface& reader_) noexcept
		: reader{ reader_ }
	{}

	TOML_EXTERNAL_LINKAGE
	const source_path_ptr& utf8_buffered_reader::source_path() const noexcept
	{
		return reader.source_path();
	}

	TOML_EXTERNAL_LINKAGE
	const utf8_codepoint* utf8_buffered_reader::read_next()
	{
		TOML_ERROR_CHECK;

		if (negative_offset)
		{
			negative_offset--;

			// an entry negative offset of 1 just means "replay the current head"
			if (!negative_offset)
				return head;

			// otherwise step back into the history buffer
			else
				return history.buffer + ((history.first + history.count - negative_offset) % history_buffer_size);
		}
		else
		{
			// first character read from stream
			if TOML_UNLIKELY(!history.count && !head)
				head = reader.read_next();

			// subsequent characters and not eof
			else if (head)
			{
				if TOML_UNLIKELY(history.count < history_buffer_size)
					history.buffer[history.count++] = *head;
				else
					history.buffer[(history.first++ + history_buffer_size) % history_buffer_size] = *head;

				head = reader.read_next();
			}

			return head;
		}
	}

	TOML_EXTERNAL_LINKAGE
	const utf8_codepoint* utf8_buffered_reader::step_back(size_t count) noexcept
	{
		TOML_ERROR_CHECK;
		TOML_ASSERT(history.count);
		TOML_ASSERT(negative_offset + count <= history.count);

		negative_offset += count;

		return negative_offset
			? history.buffer + ((history.first + history.count - negative_offset) % history_buffer_size)
			: head;
	}

	TOML_EXTERNAL_LINKAGE
	bool utf8_buffered_reader::peek_eof() const
	{
		return reader.peek_eof();
	}

	#if !TOML_EXCEPTIONS
	TOML_EXTERNAL_LINKAGE
	optional<parse_error>&& utf8_buffered_reader::error() noexcept
	{
		return reader.error();
	}
	#endif

	TOML_ABI_NAMESPACE_END; // TOML_EXCEPTIONS
}
TOML_IMPL_NAMESPACE_END;

#undef TOML_ERROR_CHECK
#undef TOML_ERROR
