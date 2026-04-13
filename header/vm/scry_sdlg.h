#pragma once

#include "utils/core/scry_assert.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
	SCRY_SDLG_MAGIC_SIZE	  = 4,
	SCRY_SDLG_VERSION_CURRENT = 1,
	SCRY_SDLG_HEADER_SIZE	  = 48,
};

#define SCRY_SDLG_MAGIC "SDLG"

typedef enum scry_sdlg_flag
{
	SCRY_SDLG_FLAG_NONE				  = 0U,
	SCRY_SDLG_FLAG_STRINGS_COMPRESSED = 1U << 0,
} scry_sdlg_flag;

typedef enum scry_sdlg_opcode
{
	SCRY_SDLG_OP_NOP = 0,
	SCRY_SDLG_OP_JUMP,
	SCRY_SDLG_OP_JUMP_IF_FALSE,
	SCRY_SDLG_OP_DIALOGUE,
	SCRY_SDLG_OP_CHOICE,
	SCRY_SDLG_OP_SWITCH,
	SCRY_SDLG_OP_SET_VAR,
	SCRY_SDLG_OP_COMPARE,
	SCRY_SDLG_OP_END,
} scry_sdlg_opcode;

typedef enum scry_sdlg_table_kind
{
	SCRY_SDLG_TABLE_KIND_NONE = 0,
	SCRY_SDLG_TABLE_KIND_CHOICE,
	SCRY_SDLG_TABLE_KIND_SWITCH,
} scry_sdlg_table_kind;

typedef struct scry_sdlg_header
{
	char	 magic[SCRY_SDLG_MAGIC_SIZE];
	uint16_t version;
	uint16_t flags;

	// The instruction stream is variable-width; this is metadata, not a sizing field.
	uint32_t instruction_count;
	uint32_t instruction_offset;
	uint32_t instruction_size;

	uint32_t table_count;
	uint32_t table_offset;
	uint32_t table_size;

	// String ids index into the string index table, which points into string data.
	uint32_t string_count;
	uint32_t string_index_offset;
	uint32_t string_data_offset;
	uint32_t string_data_size;
} scry_sdlg_header;

typedef struct scry_sdlg_table_entry
{
	uint16_t kind;
	uint16_t reserved;
	uint32_t offset;
	uint32_t size;
} scry_sdlg_table_entry;

typedef struct scry_sdlg_choice_table_header
{
	uint16_t option_count;
} scry_sdlg_choice_table_header;

typedef struct scry_sdlg_switch_table_header
{
	uint16_t case_count;
	uint16_t value_expr;
	int32_t	 default_delta;
} scry_sdlg_switch_table_header;

typedef uint32_t scry_sdlg_string_index_entry;

STATIC_ASSERT(sizeof(scry_sdlg_header) == SCRY_SDLG_HEADER_SIZE, "SDLG header size must stay stable");
STATIC_ASSERT(sizeof(scry_sdlg_table_entry) == 12, "SDLG table entry size must stay stable");
STATIC_ASSERT(sizeof(scry_sdlg_choice_table_header) == 2, "Choice table header size must stay stable");
STATIC_ASSERT(sizeof(scry_sdlg_switch_table_header) == 8, "Switch table header size must stay stable");
STATIC_ASSERT(sizeof(scry_sdlg_string_index_entry) == 4, "String index entry size must stay stable");

bool   scry_sdlg_header_init(scry_sdlg_header* header);
bool   scry_sdlg_header_validate(const scry_sdlg_header* header, size_t file_size);
bool   scry_sdlg_write_header(uint8_t* buffer, size_t buffer_size, const scry_sdlg_header* header);
bool   scry_sdlg_read_header(const uint8_t* buffer, size_t buffer_size, scry_sdlg_header* out_header);
bool   scry_sdlg_table_kind_is_valid(uint16_t kind);
size_t scry_sdlg_uvarint_size(uint64_t value);
size_t scry_sdlg_svarint_size(int64_t value);
bool   scry_sdlg_write_uvarint(uint8_t* buffer, size_t buffer_size, size_t* offset, uint64_t value);
bool   scry_sdlg_read_uvarint(const uint8_t* buffer, size_t buffer_size, size_t* offset, uint64_t* out_value);
bool   scry_sdlg_write_svarint(uint8_t* buffer, size_t buffer_size, size_t* offset, int64_t value);
bool   scry_sdlg_read_svarint(const uint8_t* buffer, size_t buffer_size, size_t* offset, int64_t* out_value);
