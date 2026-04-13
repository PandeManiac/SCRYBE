#include "vm/scry_sdlg.h"

#include <string.h>

static const uint16_t scry_sdlg_known_flags = (uint16_t)SCRY_SDLG_FLAG_STRINGS_COMPRESSED;

static bool		scry_sdlg_range_is_valid(uint32_t offset, uint32_t size, size_t file_size);
static bool		scry_sdlg_range_is_empty(uint32_t offset, uint32_t size);
static bool		scry_sdlg_ranges_overlap(uint32_t left_offset, uint32_t left_size, uint32_t right_offset, uint32_t right_size);
static bool		scry_sdlg_write_u16_le(uint8_t* buffer, size_t buffer_size, size_t* offset, uint16_t value);
static bool		scry_sdlg_write_u32_le(uint8_t* buffer, size_t buffer_size, size_t* offset, uint32_t value);
static bool		scry_sdlg_read_u16_le(const uint8_t* buffer, size_t buffer_size, size_t* offset, uint16_t* out_value);
static bool		scry_sdlg_read_u32_le(const uint8_t* buffer, size_t buffer_size, size_t* offset, uint32_t* out_value);
static bool		scry_sdlg_write_u8(uint8_t* buffer, size_t buffer_size, size_t* offset, uint8_t value);
static bool		scry_sdlg_read_u8(const uint8_t* buffer, size_t buffer_size, size_t* offset, uint8_t* out_value);
static uint64_t scry_sdlg_zigzag_encode(int64_t value);
static int64_t	scry_sdlg_zigzag_decode(uint64_t value);

bool scry_sdlg_header_init(scry_sdlg_header* header)
{
	ASSERT_FATAL(header);

	memset(header, 0, sizeof(*header));
	memcpy(header->magic, SCRY_SDLG_MAGIC, SCRY_SDLG_MAGIC_SIZE);
	header->version = (uint16_t)SCRY_SDLG_VERSION_CURRENT;

	return true;
}

bool scry_sdlg_header_validate(const scry_sdlg_header* header, size_t file_size)
{
	const uint32_t header_end		 = (uint32_t)SCRY_SDLG_HEADER_SIZE;
	const uint32_t string_index_size = (uint32_t)((size_t)header->string_count * sizeof(scry_sdlg_string_index_entry));

	ASSERT_FATAL(header);

	if (memcmp(header->magic, SCRY_SDLG_MAGIC, SCRY_SDLG_MAGIC_SIZE) != 0)
	{
		return false;
	}

	if (header->version != (uint16_t)SCRY_SDLG_VERSION_CURRENT)
	{
		return false;
	}

	if ((header->flags & (uint16_t)(~scry_sdlg_known_flags)) != 0U)
	{
		return false;
	}

	if (header->instruction_count > 0U)
	{
		if (!scry_sdlg_range_is_valid(header->instruction_offset, header->instruction_size, file_size))
		{
			return false;
		}

		if (header->instruction_offset < header_end)
		{
			return false;
		}

		if (header->instruction_size == 0U)
		{
			return false;
		}
	}
	else if (!scry_sdlg_range_is_empty(header->instruction_offset, header->instruction_size))
	{
		return false;
	}

	if (header->table_count > 0U)
	{
		const size_t minimum_table_bytes = (size_t)header->table_count * sizeof(scry_sdlg_table_entry);

		if (!scry_sdlg_range_is_valid(header->table_offset, header->table_size, file_size))
		{
			return false;
		}

		if (header->table_offset < header_end || (size_t)header->table_size < minimum_table_bytes)
		{
			return false;
		}
	}
	else if (!scry_sdlg_range_is_empty(header->table_offset, header->table_size))
	{
		return false;
	}

	if (header->string_count > 0U)
	{
		if (!scry_sdlg_range_is_valid(header->string_index_offset, string_index_size, file_size))
		{
			return false;
		}

		if (!scry_sdlg_range_is_valid(header->string_data_offset, header->string_data_size, file_size))
		{
			return false;
		}

		if (header->string_index_offset < header_end || header->string_data_offset < header_end)
		{
			return false;
		}

		if ((size_t)header->string_index_offset + (size_t)string_index_size > (size_t)header->string_data_offset)
		{
			return false;
		}
	}
	else if (!scry_sdlg_range_is_empty(header->string_index_offset, 0U) || !scry_sdlg_range_is_empty(header->string_data_offset, header->string_data_size))
	{
		return false;
	}

	if (scry_sdlg_ranges_overlap(header->instruction_offset, header->instruction_size, header->table_offset, header->table_size) ||
		scry_sdlg_ranges_overlap(header->instruction_offset, header->instruction_size, header->string_index_offset, string_index_size) ||
		scry_sdlg_ranges_overlap(header->instruction_offset, header->instruction_size, header->string_data_offset, header->string_data_size) ||
		scry_sdlg_ranges_overlap(header->table_offset, header->table_size, header->string_index_offset, string_index_size) ||
		scry_sdlg_ranges_overlap(header->table_offset, header->table_size, header->string_data_offset, header->string_data_size) ||
		scry_sdlg_ranges_overlap(header->string_index_offset, string_index_size, header->string_data_offset, header->string_data_size))
	{
		return false;
	}

	return true;
}

bool scry_sdlg_write_header(uint8_t* buffer, size_t buffer_size, const scry_sdlg_header* header)
{
	size_t offset = 0U;

	ASSERT_FATAL(buffer);
	ASSERT_FATAL(header);

	if (buffer_size < (size_t)SCRY_SDLG_HEADER_SIZE)
	{
		return false;
	}

	memcpy(buffer, header->magic, SCRY_SDLG_MAGIC_SIZE);
	offset = (size_t)SCRY_SDLG_MAGIC_SIZE;

	return scry_sdlg_write_u16_le(buffer, buffer_size, &offset, header->version) && scry_sdlg_write_u16_le(buffer, buffer_size, &offset, header->flags) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->instruction_count) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->instruction_offset) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->instruction_size) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->table_count) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->table_offset) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->table_size) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->string_count) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->string_index_offset) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->string_data_offset) &&
		   scry_sdlg_write_u32_le(buffer, buffer_size, &offset, header->string_data_size) && offset == (size_t)SCRY_SDLG_HEADER_SIZE;
}

bool scry_sdlg_read_header(const uint8_t* buffer, size_t buffer_size, scry_sdlg_header* out_header)
{
	size_t offset = 0U;

	ASSERT_FATAL(buffer);
	ASSERT_FATAL(out_header);

	if (buffer_size < (size_t)SCRY_SDLG_HEADER_SIZE)
	{
		return false;
	}

	memset(out_header, 0, sizeof(*out_header));
	memcpy(out_header->magic, buffer, SCRY_SDLG_MAGIC_SIZE);
	offset = (size_t)SCRY_SDLG_MAGIC_SIZE;

	return scry_sdlg_read_u16_le(buffer, buffer_size, &offset, &out_header->version) &&
		   scry_sdlg_read_u16_le(buffer, buffer_size, &offset, &out_header->flags) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->instruction_count) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->instruction_offset) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->instruction_size) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->table_count) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->table_offset) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->table_size) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->string_count) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->string_index_offset) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->string_data_offset) &&
		   scry_sdlg_read_u32_le(buffer, buffer_size, &offset, &out_header->string_data_size) && offset == (size_t)SCRY_SDLG_HEADER_SIZE;
}

bool scry_sdlg_table_kind_is_valid(uint16_t kind)
{
	switch ((scry_sdlg_table_kind)kind)
	{
		case SCRY_SDLG_TABLE_KIND_CHOICE:
		case SCRY_SDLG_TABLE_KIND_SWITCH:
			return true;
		case SCRY_SDLG_TABLE_KIND_NONE:
		default:
			return false;
	}
}

size_t scry_sdlg_uvarint_size(uint64_t value)
{
	size_t size = 1U;

	while (value >= UINT64_C(0x80))
	{
		value >>= 7U;
		size += 1U;
	}

	return size;
}

size_t scry_sdlg_svarint_size(int64_t value)
{
	return scry_sdlg_uvarint_size(scry_sdlg_zigzag_encode(value));
}

bool scry_sdlg_write_uvarint(uint8_t* buffer, size_t buffer_size, size_t* offset, uint64_t value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);

	do
	{
		uint8_t byte = (uint8_t)(value & UINT64_C(0x7f));

		value >>= 7U;

		if (value != 0U)
		{
			byte |= UINT8_C(0x80);
		}

		if (!scry_sdlg_write_u8(buffer, buffer_size, offset, byte))
		{
			return false;
		}
	} while (value != 0U);

	return true;
}

bool scry_sdlg_read_uvarint(const uint8_t* buffer, size_t buffer_size, size_t* offset, uint64_t* out_value)
{
	uint64_t value = 0U;
	uint32_t shift = 0U;

	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);
	ASSERT_FATAL(out_value);

	for (;;)
	{
		uint8_t byte = 0U;

		if (!scry_sdlg_read_u8(buffer, buffer_size, offset, &byte))
		{
			return false;
		}

		if (shift == 63U && (byte & UINT8_C(0x7e)) != 0U)
		{
			return false;
		}

		value |= ((uint64_t)(byte & UINT8_C(0x7f))) << shift;

		if ((byte & UINT8_C(0x80)) == 0U)
		{
			*out_value = value;
			return true;
		}

		if (shift >= 63U)
		{
			return false;
		}

		shift += 7U;
	}
}

bool scry_sdlg_write_svarint(uint8_t* buffer, size_t buffer_size, size_t* offset, int64_t value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);

	return scry_sdlg_write_uvarint(buffer, buffer_size, offset, scry_sdlg_zigzag_encode(value));
}

bool scry_sdlg_read_svarint(const uint8_t* buffer, size_t buffer_size, size_t* offset, int64_t* out_value)
{
	uint64_t encoded = 0U;

	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);
	ASSERT_FATAL(out_value);

	if (!scry_sdlg_read_uvarint(buffer, buffer_size, offset, &encoded))
	{
		return false;
	}

	*out_value = scry_sdlg_zigzag_decode(encoded);
	return true;
}

static bool scry_sdlg_range_is_valid(uint32_t offset, uint32_t size, size_t file_size)
{
	return (size_t)offset <= file_size && (size_t)size <= file_size && ((size_t)offset + (size_t)size) <= file_size;
}

static bool scry_sdlg_range_is_empty(uint32_t offset, uint32_t size)
{
	return offset == 0U && size == 0U;
}

static bool scry_sdlg_ranges_overlap(uint32_t left_offset, uint32_t left_size, uint32_t right_offset, uint32_t right_size)
{
	const size_t left_end  = (size_t)left_offset + (size_t)left_size;
	const size_t right_end = (size_t)right_offset + (size_t)right_size;

	if (left_size == 0U || right_size == 0U)
	{
		return false;
	}

	return !((left_end <= (size_t)right_offset) || (right_end <= (size_t)left_offset));
}

static bool scry_sdlg_write_u16_le(uint8_t* buffer, size_t buffer_size, size_t* offset, uint16_t value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);

	if (*offset > buffer_size || (buffer_size - *offset) < 2U)
	{
		return false;
	}

	buffer[*offset + 0U] = (uint8_t)(value & UINT16_C(0x00ff));
	buffer[*offset + 1U] = (uint8_t)((value >> 8U) & UINT16_C(0x00ff));
	*offset += 2U;
	return true;
}

static bool scry_sdlg_write_u32_le(uint8_t* buffer, size_t buffer_size, size_t* offset, uint32_t value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);

	if (*offset > buffer_size || (buffer_size - *offset) < 4U)
	{
		return false;
	}

	buffer[*offset + 0U] = (uint8_t)(value & UINT32_C(0x000000ff));
	buffer[*offset + 1U] = (uint8_t)((value >> 8U) & UINT32_C(0x000000ff));
	buffer[*offset + 2U] = (uint8_t)((value >> 16U) & UINT32_C(0x000000ff));
	buffer[*offset + 3U] = (uint8_t)((value >> 24U) & UINT32_C(0x000000ff));
	*offset += 4U;
	return true;
}

static bool scry_sdlg_read_u16_le(const uint8_t* buffer, size_t buffer_size, size_t* offset, uint16_t* out_value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);
	ASSERT_FATAL(out_value);

	if (*offset > buffer_size || (buffer_size - *offset) < 2U)
	{
		return false;
	}

	*out_value = (uint16_t)buffer[*offset + 0U] | (uint16_t)((uint16_t)buffer[*offset + 1U] << 8U);
	*offset += 2U;
	return true;
}

static bool scry_sdlg_read_u32_le(const uint8_t* buffer, size_t buffer_size, size_t* offset, uint32_t* out_value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);
	ASSERT_FATAL(out_value);

	if (*offset > buffer_size || (buffer_size - *offset) < 4U)
	{
		return false;
	}

	*out_value = (uint32_t)buffer[*offset + 0U] | ((uint32_t)buffer[*offset + 1U] << 8U) | ((uint32_t)buffer[*offset + 2U] << 16U) |
				 ((uint32_t)buffer[*offset + 3U] << 24U);
	*offset += 4U;
	return true;
}

static bool scry_sdlg_write_u8(uint8_t* buffer, size_t buffer_size, size_t* offset, uint8_t value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);

	if (*offset >= buffer_size)
	{
		return false;
	}

	buffer[*offset] = value;
	*offset += 1U;
	return true;
}

static bool scry_sdlg_read_u8(const uint8_t* buffer, size_t buffer_size, size_t* offset, uint8_t* out_value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);
	ASSERT_FATAL(out_value);

	if (*offset >= buffer_size)
	{
		return false;
	}

	*out_value = buffer[*offset];
	*offset += 1U;
	return true;
}

static uint64_t scry_sdlg_zigzag_encode(int64_t value)
{
	return ((uint64_t)value << 1U) ^ (uint64_t)(value >> 63U);
}

static int64_t scry_sdlg_zigzag_decode(uint64_t value)
{
	const uint64_t sign = 0U - (value & UINT64_C(1));
	return (int64_t)((value >> 1U) ^ sign);
}
