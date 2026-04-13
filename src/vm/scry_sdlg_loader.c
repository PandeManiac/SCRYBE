#include "vm/scry_sdlg_loader.h"

#include "utils/io/scry_file.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool scry_sdlg_validate_table_entries(const scry_sdlg_view* view);
static bool scry_sdlg_validate_string_index(const scry_sdlg_view* view);
static bool scry_sdlg_read_string_record(const scry_sdlg_view* view, uint32_t offset, scry_sdlg_string* out_string, uint32_t* out_record_size);
static bool scry_sdlg_read_table_entry(const scry_sdlg_view* view, uint32_t table_index, scry_sdlg_table_entry* out_table);
static bool scry_sdlg_read_string_index_entry(const scry_sdlg_view* view, uint32_t string_id, uint32_t* out_offset);

bool scry_sdlg_view_init(scry_sdlg_view* view, const uint8_t* data, size_t size)
{
	ASSERT_FATAL(view);

	memset(view, 0, sizeof(*view));

	if (data == NULL || size < (size_t)SCRY_SDLG_HEADER_SIZE)
	{
		return false;
	}

	if (!scry_sdlg_read_header(data, size, &view->header))
	{
		return false;
	}

	if (!scry_sdlg_header_validate(&view->header, size))
	{
		return false;
	}

	if ((view->header.flags & (uint16_t)SCRY_SDLG_FLAG_STRINGS_COMPRESSED) != 0U)
	{
		return false;
	}

	view->file_data = data;
	view->file_size = size;

	view->instructions.data = &data[view->header.instruction_offset];
	view->instructions.size = (size_t)view->header.instruction_size;

	view->table_directory.data = &data[view->header.table_offset];
	view->table_directory.size = (size_t)view->header.table_size;

	view->string_index_data.data = &data[view->header.string_index_offset];
	view->string_index_data.size = (size_t)view->header.string_count * sizeof(scry_sdlg_string_index_entry);

	view->string_data.data = &data[view->header.string_data_offset];
	view->string_data.size = (size_t)view->header.string_data_size;

	if (!scry_sdlg_validate_table_entries(view))
	{
		return false;
	}

	if (!scry_sdlg_validate_string_index(view))
	{
		return false;
	}

	return true;
}

void scry_sdlg_document_destroy(scry_sdlg_document* document)
{
	ASSERT_FATAL(document);

	free(document->buffer);
	memset(document, 0, sizeof(*document));
}

bool scry_sdlg_document_load_file(scry_sdlg_document* document, const char* path)
{
	uint8_t* buffer = NULL;
	size_t	 size	= 0U;

	ASSERT_FATAL(document);
	ASSERT_FATAL(path);

	scry_sdlg_document_destroy(document);

	if (!scry_file_read_bytes(path, &buffer, &size))
	{
		return false;
	}

	if (!scry_sdlg_view_init(&document->view, buffer, size))
	{
		free(buffer);
		return false;
	}

	document->buffer = buffer;
	document->size	 = size;
	return true;
}

bool scry_sdlg_table_at(const scry_sdlg_view* view, uint32_t table_index, scry_sdlg_table_entry* out_table)
{
	ASSERT_FATAL(view);
	ASSERT_FATAL(out_table);

	return scry_sdlg_read_table_entry(view, table_index, out_table);
}

bool scry_sdlg_table_payload(const scry_sdlg_view* view, const scry_sdlg_table_entry* table, scry_sdlg_span* out_payload)
{
	ASSERT_FATAL(view);
	ASSERT_FATAL(table);
	ASSERT_FATAL(out_payload);
	ASSERT_FATAL((size_t)table->offset + (size_t)table->size <= view->file_size);

	if (table->size == 0U)
	{
		return false;
	}

	out_payload->data = &view->file_data[table->offset];
	out_payload->size = (size_t)table->size;
	return true;
}

bool scry_sdlg_string_at(const scry_sdlg_view* view, uint32_t string_id, scry_sdlg_string* out_string)
{
	uint32_t offset = 0U;

	ASSERT_FATAL(view);
	ASSERT_FATAL(out_string);

	if (string_id >= view->header.string_count)
	{
		return false;
	}

	if (!scry_sdlg_read_string_index_entry(view, string_id, &offset))
	{
		return false;
	}

	return scry_sdlg_read_string_record(view, offset, out_string, NULL);
}

bool scry_sdlg_debug_dump(const scry_sdlg_view* view)
{
	const uint32_t preview_count = view->header.string_count < 3U ? view->header.string_count : 3U;

	ASSERT_FATAL(view);

	if (printf("SDLG header:\n"
			   "  version=%" PRIu16 "\n"
			   "  flags=0x%04" PRIx16 "\n"
			   "  instruction_count=%" PRIu32 "\n"
			   "  instruction_offset=%" PRIu32 "\n"
			   "  instruction_size=%" PRIu32 "\n"
			   "  table_count=%" PRIu32 "\n"
			   "  table_offset=%" PRIu32 "\n"
			   "  table_size=%" PRIu32 "\n"
			   "  string_count=%" PRIu32 "\n"
			   "  string_index_offset=%" PRIu32 "\n"
			   "  string_data_offset=%" PRIu32 "\n"
			   "  string_data_size=%" PRIu32 "\n",
			   view->header.version,
			   view->header.flags,
			   view->header.instruction_count,
			   view->header.instruction_offset,
			   view->header.instruction_size,
			   view->header.table_count,
			   view->header.table_offset,
			   view->header.table_size,
			   view->header.string_count,
			   view->header.string_index_offset,
			   view->header.string_data_offset,
			   view->header.string_data_size) < 0)
	{
		return false;
	}

	for (uint32_t i = 0U; i < preview_count; ++i)
	{
		scry_sdlg_string string = { 0 };

		if (!scry_sdlg_string_at(view, i, &string))
		{
			return false;
		}

		if (printf("  string[%" PRIu32 "]: %.*s\n", i, (int)string.length, string.data) < 0)
		{
			return false;
		}
	}

	return true;
}

static bool scry_sdlg_validate_table_entries(const scry_sdlg_view* view)
{
	const size_t directory_end	   = (size_t)view->header.table_offset + ((size_t)view->header.table_count * sizeof(scry_sdlg_table_entry));
	const size_t table_section_end = (size_t)view->header.table_offset + (size_t)view->header.table_size;
	size_t		 previous_end	   = directory_end;

	ASSERT_FATAL(view);

	for (uint32_t i = 0U; i < view->header.table_count; ++i)
	{
		scry_sdlg_table_entry table		= { 0 };
		size_t				  table_end = 0U;

		if (!scry_sdlg_read_table_entry(view, i, &table))
		{
			return false;
		}

		if (!scry_sdlg_table_kind_is_valid(table.kind))
		{
			return false;
		}

		if (table.reserved != 0U)
		{
			return false;
		}

		if (table.size == 0U)
		{
			return false;
		}

		if ((size_t)table.offset > SIZE_MAX - (size_t)table.size)
		{
			return false;
		}

		table_end = (size_t)table.offset + (size_t)table.size;

		if ((size_t)table.offset < directory_end)
		{
			return false;
		}

		if ((size_t)table.offset < previous_end)
		{
			return false;
		}

		if (table_end > table_section_end)
		{
			return false;
		}

		previous_end = table_end;
	}

	return true;
}

static bool scry_sdlg_validate_string_index(const scry_sdlg_view* view)
{
	uint32_t previous_offset = 0U;

	ASSERT_FATAL(view);

	for (uint32_t i = 0U; i < view->header.string_count; ++i)
	{
		scry_sdlg_string string		 = { 0 };
		uint32_t		 record_size = 0U;
		uint32_t		 offset		 = 0U;

		if (!scry_sdlg_read_string_index_entry(view, i, &offset))
		{
			return false;
		}

		if (offset < previous_offset)
		{
			return false;
		}

		if (!scry_sdlg_read_string_record(view, offset, &string, &record_size))
		{
			return false;
		}

		if (record_size == 0U)
		{
			return false;
		}

		if ((uint64_t)offset + (uint64_t)record_size > (uint64_t)UINT32_MAX)
		{
			return false;
		}

		previous_offset = offset + record_size;

		if ((size_t)previous_offset > view->string_data.size)
		{
			return false;
		}
	}

	return true;
}

static bool scry_sdlg_read_table_entry(const scry_sdlg_view* view, uint32_t table_index, scry_sdlg_table_entry* out_table)
{
	const size_t offset = (size_t)table_index * sizeof(*out_table);

	ASSERT_FATAL(view);
	ASSERT_FATAL(out_table);

	if (table_index >= view->header.table_count || offset > view->table_directory.size || (view->table_directory.size - offset) < sizeof(*out_table))
	{
		return false;
	}

	memcpy(out_table, &view->table_directory.data[offset], sizeof(*out_table));
	return true;
}

static bool scry_sdlg_read_string_index_entry(const scry_sdlg_view* view, uint32_t string_id, uint32_t* out_offset)
{
	const size_t offset = (size_t)string_id * sizeof(*out_offset);

	ASSERT_FATAL(view);
	ASSERT_FATAL(out_offset);

	if (string_id >= view->header.string_count || offset > view->string_index_data.size || (view->string_index_data.size - offset) < sizeof(*out_offset))
	{
		return false;
	}

	memcpy(out_offset, &view->string_index_data.data[offset], sizeof(*out_offset));
	return true;
}

static bool scry_sdlg_read_string_record(const scry_sdlg_view* view, uint32_t offset, scry_sdlg_string* out_string, uint32_t* out_record_size)
{
	size_t	 cursor			= (size_t)offset;
	uint64_t length			= 0U;
	size_t	 payload_offset = 0U;
	size_t	 record_size	= 0U;

	ASSERT_FATAL(view);
	ASSERT_FATAL(out_string);

	if ((size_t)offset >= view->string_data.size)
	{
		return false;
	}

	if (!scry_sdlg_read_uvarint(view->string_data.data, view->string_data.size, &cursor, &length))
	{
		return false;
	}

	if (length > SIZE_MAX || cursor > view->string_data.size || length > (uint64_t)(view->string_data.size - cursor))
	{
		return false;
	}

	payload_offset	   = cursor;
	out_string->data   = (const char*)&view->string_data.data[payload_offset];
	out_string->length = (size_t)length;

	if (memchr(out_string->data, '\0', out_string->length) != NULL)
	{
		return false;
	}

	record_size = payload_offset + out_string->length - (size_t)offset;

	if (record_size > (size_t)UINT32_MAX)
	{
		return false;
	}

	if (out_record_size)
	{
		*out_record_size = (uint32_t)record_size;
	}

	return true;
}
