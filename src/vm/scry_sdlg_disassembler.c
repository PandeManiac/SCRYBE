#include "vm/scry_sdlg_disassembler.h"

#include "vm/scry_ir.h"
#include "vm/scry_sdlg.h"

#include <inttypes.h>
#include <limits.h>

typedef struct scry_sdlg_disassembly_choice_option
{
	uint32_t text_string_id;
	int32_t	 target_delta;
} scry_sdlg_disassembly_choice_option;

typedef struct scry_sdlg_disassembly_switch_case
{
	int32_t value;
	int32_t target_delta;
} scry_sdlg_disassembly_switch_case;

static const char* scry_sdlg_disassembly_opcode_name(uint8_t opcode);
static const char* scry_sdlg_disassembly_compare_name(uint64_t compare_op);
static const char* scry_sdlg_disassembly_table_name(uint16_t table_kind);
static bool		   scry_sdlg_disassembly_print_header(const scry_sdlg_view* view, FILE* output);
static bool		   scry_sdlg_disassembly_print_instructions(const scry_sdlg_view* view, FILE* output);
static bool		   scry_sdlg_disassembly_print_instruction(const scry_sdlg_view* view, FILE* output, size_t* cursor, uint32_t instruction_index);
static bool		   scry_sdlg_disassembly_print_tables(const scry_sdlg_view* view, FILE* output);
static bool		   scry_sdlg_disassembly_print_choice_table(const scry_sdlg_view*		 view,
															FILE*						 output,
															uint32_t					 table_index,
															const scry_sdlg_table_entry* table,
															size_t						 base_ip);
static bool		   scry_sdlg_disassembly_print_switch_table(const scry_sdlg_view*		 view,
															FILE*						 output,
															uint32_t					 table_index,
															const scry_sdlg_table_entry* table,
															size_t						 base_ip);
static bool		   scry_sdlg_disassembly_print_strings(const scry_sdlg_view* view, FILE* output);
static bool		   scry_sdlg_disassembly_print_jump_target(FILE* output, size_t base_ip, int64_t delta, size_t instruction_size);
static bool		   scry_sdlg_disassembly_print_string_literal(FILE* output, const char* data, size_t length);
static bool scry_sdlg_disassembly_read_choice_option(const scry_sdlg_span* payload, uint16_t option_index, scry_sdlg_disassembly_choice_option* out_option);
static bool scry_sdlg_disassembly_read_switch_case(const scry_sdlg_span* payload, uint16_t case_index, scry_sdlg_disassembly_switch_case* out_case);
static bool scry_sdlg_disassembly_read_u16_le(const uint8_t* data, size_t size, size_t offset, uint16_t* out_value);
static bool scry_sdlg_disassembly_read_u32_le(const uint8_t* data, size_t size, size_t offset, uint32_t* out_value);
static bool scry_sdlg_disassembly_read_s32_le(const uint8_t* data, size_t size, size_t offset, int32_t* out_value);
static bool scry_sdlg_disassembly_print_instruction_prefix(FILE* output, size_t offset, uint32_t instruction_index, const char* opcode_name);

bool scry_sdlg_disassemble(const scry_sdlg_view* view, FILE* output)
{
	ASSERT_FATAL(view);
	ASSERT_FATAL(output);

	return scry_sdlg_disassembly_print_header(view, output) && scry_sdlg_disassembly_print_instructions(view, output) &&
		   scry_sdlg_disassembly_print_tables(view, output) && scry_sdlg_disassembly_print_strings(view, output);
}

bool scry_sdlg_disassemble_file(const char* path, FILE* output)
{
	ASSERT_FATAL(path);
	ASSERT_FATAL(output);

	scry_sdlg_document document = { 0 };

	if (!scry_sdlg_document_load_file(&document, path))
	{
		return false;
	}

	const bool ok = scry_sdlg_disassemble(&document.view, output);
	scry_sdlg_document_destroy(&document);

	return ok;
}

static const char* scry_sdlg_disassembly_opcode_name(uint8_t opcode)
{
	switch ((scry_sdlg_opcode)opcode)
	{
		case SCRY_SDLG_OP_NOP:
			return "NOP";
		case SCRY_SDLG_OP_JUMP:
			return "JUMP";
		case SCRY_SDLG_OP_JUMP_IF_FALSE:
			return "JUMP_IF_FALSE";
		case SCRY_SDLG_OP_DIALOGUE:
			return "DIALOGUE";
		case SCRY_SDLG_OP_CHOICE:
			return "CHOICE";
		case SCRY_SDLG_OP_SWITCH:
			return "SWITCH";
		case SCRY_SDLG_OP_SET_VAR:
			return "SET_VAR";
		case SCRY_SDLG_OP_COMPARE:
			return "COMPARE";
		case SCRY_SDLG_OP_END:
			return "END";
		default:
			return "INVALID";
	}
}

static const char* scry_sdlg_disassembly_compare_name(uint64_t compare_op)
{
	switch ((scry_ir_compare_op)compare_op)
	{
		case SCRY_IR_COMPARE_OP_EQUAL:
			return "EQ";
		case SCRY_IR_COMPARE_OP_NOT_EQUAL:
			return "NE";
		case SCRY_IR_COMPARE_OP_LESS:
			return "LT";
		case SCRY_IR_COMPARE_OP_LESS_EQUAL:
			return "LE";
		case SCRY_IR_COMPARE_OP_GREATER:
			return "GT";
		case SCRY_IR_COMPARE_OP_GREATER_EQUAL:
			return "GE";
		case SCRY_IR_COMPARE_OP_NONE:
		default:
			return NULL;
	}
}

static const char* scry_sdlg_disassembly_table_name(uint16_t table_kind)
{
	switch ((scry_sdlg_table_kind)table_kind)
	{
		case SCRY_SDLG_TABLE_KIND_CHOICE:
			return "CHOICE";
		case SCRY_SDLG_TABLE_KIND_SWITCH:
			return "SWITCH";
		case SCRY_SDLG_TABLE_KIND_NONE:
		default:
			return "INVALID";
	}
}

static bool scry_sdlg_disassembly_print_header(const scry_sdlg_view* view, FILE* output)
{
	ASSERT_FATAL(view);
	ASSERT_FATAL(output);

	return fprintf(output,
				   "SDLG Disassembly\n"
				   "Header:\n"
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
				   view->header.string_data_size) >= 0;
}

static bool scry_sdlg_disassembly_print_instructions(const scry_sdlg_view* view, FILE* output)
{
	size_t	 cursor			   = 0U;
	uint32_t instruction_index = 0U;

	ASSERT_FATAL(view);
	ASSERT_FATAL(output);

	if (fputs("Instructions:\n", output) == EOF)
	{
		return false;
	}

	while (cursor < view->instructions.size)
	{
		if (!scry_sdlg_disassembly_print_instruction(view, output, &cursor, instruction_index))
		{
			return false;
		}

		instruction_index += 1U;
	}

	if (instruction_index != view->header.instruction_count &&
		fprintf(output, "  note: decoded=%" PRIu32 " header=%" PRIu32 "\n", instruction_index, view->header.instruction_count) < 0)
	{
		return false;
	}

	return true;
}

static bool scry_sdlg_disassembly_print_instruction(const scry_sdlg_view* view, FILE* output, size_t* cursor, uint32_t instruction_index)
{
	size_t	start  = 0U;
	uint8_t opcode = 0U;

	ASSERT_FATAL(view);
	ASSERT_FATAL(output);
	ASSERT_FATAL(cursor);

	start = *cursor;

	if (start >= view->instructions.size)
	{
		return false;
	}

	opcode = view->instructions.data[*cursor];
	*cursor += 1U;

	if (!scry_sdlg_disassembly_print_instruction_prefix(output, start, instruction_index, scry_sdlg_disassembly_opcode_name(opcode)))
	{
		return false;
	}

	switch ((scry_sdlg_opcode)opcode)
	{
		case SCRY_SDLG_OP_NOP:
			return fputc('\n', output) != EOF;

		case SCRY_SDLG_OP_JUMP:
		case SCRY_SDLG_OP_JUMP_IF_FALSE:
		{
			int64_t delta = 0;

			if (!scry_sdlg_read_svarint(view->instructions.data, view->instructions.size, cursor, &delta))
			{
				return false;
			}

			if (fprintf(output, " delta=%" PRId64, delta) < 0)
			{
				return false;
			}

			return scry_sdlg_disassembly_print_jump_target(output, *cursor, delta, view->instructions.size) && fputc('\n', output) != EOF;
		}

		case SCRY_SDLG_OP_DIALOGUE:
		{
			uint64_t		 string_id = 0U;
			scry_sdlg_string string	   = { 0 };

			if (!scry_sdlg_read_uvarint(view->instructions.data, view->instructions.size, cursor, &string_id) || string_id > UINT32_MAX ||
				!scry_sdlg_string_at(view, (uint32_t)string_id, &string))
			{
				return false;
			}

			if (fprintf(output, " string[%" PRIu64 "]=", string_id) < 0 || !scry_sdlg_disassembly_print_string_literal(output, string.data, string.length))
			{
				return false;
			}

			return fputc('\n', output) != EOF;
		}

		case SCRY_SDLG_OP_CHOICE:
		{
			uint64_t			  table_index = 0U;
			scry_sdlg_table_entry table		  = { 0 };

			if (!scry_sdlg_read_uvarint(view->instructions.data, view->instructions.size, cursor, &table_index) || table_index > UINT32_MAX ||
				!scry_sdlg_table_at(view, (uint32_t)table_index, &table))
			{
				return false;
			}

			if (fprintf(output, " table[%" PRIu64 "]\n", table_index) < 0)
			{
				return false;
			}

			return scry_sdlg_disassembly_print_choice_table(view, output, (uint32_t)table_index, &table, *cursor);
		}

		case SCRY_SDLG_OP_SWITCH:
		{
			uint64_t			  table_index = 0U;
			scry_sdlg_table_entry table		  = { 0 };

			if (!scry_sdlg_read_uvarint(view->instructions.data, view->instructions.size, cursor, &table_index) || table_index > UINT32_MAX ||
				!scry_sdlg_table_at(view, (uint32_t)table_index, &table))
			{
				return false;
			}

			if (fprintf(output, " table[%" PRIu64 "]\n", table_index) < 0)
			{
				return false;
			}

			return scry_sdlg_disassembly_print_switch_table(view, output, (uint32_t)table_index, &table, *cursor);
		}

		case SCRY_SDLG_OP_COMPARE:
		{
			uint64_t	variable_id	  = 0U;
			uint64_t	compare_op	  = 0U;
			int64_t		compare_value = 0;
			const char* compare_name  = NULL;

			if (!scry_sdlg_read_uvarint(view->instructions.data, view->instructions.size, cursor, &variable_id) ||
				!scry_sdlg_read_uvarint(view->instructions.data, view->instructions.size, cursor, &compare_op) ||
				!scry_sdlg_read_svarint(view->instructions.data, view->instructions.size, cursor, &compare_value))
			{
				return false;
			}

			compare_name = scry_sdlg_disassembly_compare_name(compare_op);

			if (compare_name == NULL)
			{
				return false;
			}

			return fprintf(output, " variable[%" PRIu64 "] %s %" PRId64 "\n", variable_id, compare_name, compare_value) >= 0;
		}

		case SCRY_SDLG_OP_END:
			return fputc('\n', output) != EOF;

		case SCRY_SDLG_OP_SET_VAR:
			return fprintf(output, " <unsupported operand encoding>\n") >= 0 && false;

		default:
			return false;
	}
}

static bool scry_sdlg_disassembly_print_tables(const scry_sdlg_view* view, FILE* output)
{
	ASSERT_FATAL(view);
	ASSERT_FATAL(output);

	if (fputs("Tables:\n", output) == EOF)
	{
		return false;
	}

	for (uint32_t table_index = 0U; table_index < view->header.table_count; ++table_index)
	{
		scry_sdlg_table_entry table = { 0 };

		if (!scry_sdlg_table_at(view, table_index, &table))
		{
			return false;
		}

		if (fprintf(output,
					"  table[%" PRIu32 "]: %s offset=%" PRIu32 " size=%" PRIu32 "\n",
					table_index,
					scry_sdlg_disassembly_table_name(table.kind),
					table.offset,
					table.size) < 0)
		{
			return false;
		}
	}

	return true;
}

static bool scry_sdlg_disassembly_print_choice_table(const scry_sdlg_view*		  view,
													 FILE*						  output,
													 uint32_t					  table_index,
													 const scry_sdlg_table_entry* table,
													 size_t						  base_ip)
{
	scry_sdlg_span payload		 = { 0 };
	uint16_t	   option_count	 = 0U;
	size_t		   expected_size = 0U;

	ASSERT_FATAL(view);
	ASSERT_FATAL(output);
	ASSERT_FATAL(table);

	if (table->kind != (uint16_t)SCRY_SDLG_TABLE_KIND_CHOICE || !scry_sdlg_table_payload(view, table, &payload) ||
		!scry_sdlg_disassembly_read_u16_le(payload.data, payload.size, 0U, &option_count))
	{
		return false;
	}

	expected_size = sizeof(scry_sdlg_choice_table_header) + ((size_t)option_count * (sizeof(uint32_t) + sizeof(int32_t)));

	if (payload.size != expected_size)
	{
		return false;
	}

	for (uint16_t option_index = 0U; option_index < option_count; ++option_index)
	{
		scry_sdlg_disassembly_choice_option option = { 0 };
		scry_sdlg_string					string = { 0 };

		if (!scry_sdlg_disassembly_read_choice_option(&payload, option_index, &option) || !scry_sdlg_string_at(view, option.text_string_id, &string))
		{
			return false;
		}

		if (fprintf(output, "    option[%" PRIu16 "] table[%" PRIu32 "] string[%" PRIu32 "]=", option_index, table_index, option.text_string_id) < 0 ||
			!scry_sdlg_disassembly_print_string_literal(output, string.data, string.length) || fprintf(output, " delta=%" PRId32, option.target_delta) < 0 ||
			!scry_sdlg_disassembly_print_jump_target(output, base_ip, option.target_delta, view->instructions.size) || fputc('\n', output) == EOF)
		{
			return false;
		}
	}

	return true;
}

static bool scry_sdlg_disassembly_print_switch_table(const scry_sdlg_view*		  view,
													 FILE*						  output,
													 uint32_t					  table_index,
													 const scry_sdlg_table_entry* table,
													 size_t						  base_ip)
{
	scry_sdlg_span payload		 = { 0 };
	uint16_t	   case_count	 = 0U;
	uint16_t	   variable_id	 = 0U;
	int32_t		   default_delta = 0;
	size_t		   expected_size = 0U;

	ASSERT_FATAL(view);
	ASSERT_FATAL(output);
	ASSERT_FATAL(table);

	if (table->kind != (uint16_t)SCRY_SDLG_TABLE_KIND_SWITCH || !scry_sdlg_table_payload(view, table, &payload) ||
		!scry_sdlg_disassembly_read_u16_le(payload.data, payload.size, 0U, &case_count) ||
		!scry_sdlg_disassembly_read_u16_le(payload.data, payload.size, 2U, &variable_id) ||
		!scry_sdlg_disassembly_read_s32_le(payload.data, payload.size, 4U, &default_delta))
	{
		return false;
	}

	expected_size = sizeof(scry_sdlg_switch_table_header) + ((size_t)case_count * (sizeof(int32_t) + sizeof(int32_t)));

	if (payload.size != expected_size)
	{
		return false;
	}

	if (fprintf(output, "    switch table[%" PRIu32 "] variable[%" PRIu16 "] default_delta=%" PRId32, table_index, variable_id, default_delta) < 0 ||
		!scry_sdlg_disassembly_print_jump_target(output, base_ip, default_delta, view->instructions.size) || fputc('\n', output) == EOF)
	{
		return false;
	}

	for (uint16_t case_index = 0U; case_index < case_count; ++case_index)
	{
		scry_sdlg_disassembly_switch_case switch_case = { 0 };

		if (!scry_sdlg_disassembly_read_switch_case(&payload, case_index, &switch_case))
		{
			return false;
		}

		if (fprintf(output, "    case[%" PRIu16 "] value=%" PRId32 " delta=%" PRId32, case_index, switch_case.value, switch_case.target_delta) < 0 ||
			!scry_sdlg_disassembly_print_jump_target(output, base_ip, switch_case.target_delta, view->instructions.size) || fputc('\n', output) == EOF)
		{
			return false;
		}
	}

	return true;
}

static bool scry_sdlg_disassembly_print_strings(const scry_sdlg_view* view, FILE* output)
{
	ASSERT_FATAL(view);
	ASSERT_FATAL(output);

	if (fputs("Strings:\n", output) == EOF)
	{
		return false;
	}

	for (uint32_t string_id = 0U; string_id < view->header.string_count; ++string_id)
	{
		scry_sdlg_string string = { 0 };

		if (!scry_sdlg_string_at(view, string_id, &string))
		{
			return false;
		}

		if (fprintf(output, "  string[%" PRIu32 "]=", string_id) < 0 || !scry_sdlg_disassembly_print_string_literal(output, string.data, string.length) ||
			fputc('\n', output) == EOF)
		{
			return false;
		}
	}

	return true;
}

static bool scry_sdlg_disassembly_print_jump_target(FILE* output, size_t base_ip, int64_t delta, size_t instruction_size)
{
	int64_t target = 0;

	ASSERT_FATAL(output);

	if (base_ip > (size_t)INT64_MAX)
	{
		return false;
	}

	target = (int64_t)base_ip + delta;

	if (target < 0)
	{
		return false;
	}

	if ((uint64_t)target > (uint64_t)instruction_size)
	{
		return false;
	}

	return fprintf(output, " target=%" PRId64, target) >= 0;
}

static bool scry_sdlg_disassembly_print_string_literal(FILE* output, const char* data, size_t length)
{
	ASSERT_FATAL(output);
	ASSERT_FATAL(data || length == 0U);

	if (fputc('"', output) == EOF)
	{
		return false;
	}

	for (size_t i = 0U; i < length; ++i)
	{
		const unsigned char ch = (unsigned char)data[i];

		switch (ch)
		{
			case '\n':
				if (fputs("\\n", output) == EOF)
				{
					return false;
				}
				break;

			case '\r':
				if (fputs("\\r", output) == EOF)
				{
					return false;
				}
				break;

			case '\t':
				if (fputs("\\t", output) == EOF)
				{
					return false;
				}
				break;

			case '"':
				if (fputs("\\\"", output) == EOF)
				{
					return false;
				}
				break;

			case '\\':
				if (fputs("\\\\", output) == EOF)
				{
					return false;
				}
				break;

			default:
				if (ch < 0x20U || ch >= 0x7fU)
				{
					if (fprintf(output, "\\x%02X", (unsigned)ch) < 0)
					{
						return false;
					}
				}
				else if (fputc((int)ch, output) == EOF)
				{
					return false;
				}
				break;
		}
	}

	return fputc('"', output) != EOF;
}

static bool scry_sdlg_disassembly_read_choice_option(const scry_sdlg_span* payload, uint16_t option_index, scry_sdlg_disassembly_choice_option* out_option)
{
	const size_t offset = sizeof(scry_sdlg_choice_table_header) + ((size_t)option_index * (sizeof(uint32_t) + sizeof(int32_t)));

	ASSERT_FATAL(payload);
	ASSERT_FATAL(out_option);

	return scry_sdlg_disassembly_read_u32_le(payload->data, payload->size, offset, &out_option->text_string_id) &&
		   scry_sdlg_disassembly_read_s32_le(payload->data, payload->size, offset + sizeof(uint32_t), &out_option->target_delta);
}

static bool scry_sdlg_disassembly_read_switch_case(const scry_sdlg_span* payload, uint16_t case_index, scry_sdlg_disassembly_switch_case* out_case)
{
	const size_t offset = sizeof(scry_sdlg_switch_table_header) + ((size_t)case_index * (sizeof(int32_t) + sizeof(int32_t)));

	ASSERT_FATAL(payload);
	ASSERT_FATAL(out_case);

	return scry_sdlg_disassembly_read_s32_le(payload->data, payload->size, offset, &out_case->value) &&
		   scry_sdlg_disassembly_read_s32_le(payload->data, payload->size, offset + sizeof(int32_t), &out_case->target_delta);
}

static bool scry_sdlg_disassembly_read_u16_le(const uint8_t* data, size_t size, size_t offset, uint16_t* out_value)
{
	ASSERT_FATAL(data);
	ASSERT_FATAL(out_value);

	if (offset > size || (size - offset) < 2U)
	{
		return false;
	}

	*out_value = (uint16_t)((uint16_t)data[offset + 0U] | ((uint16_t)data[offset + 1U] << 8U));
	return true;
}

static bool scry_sdlg_disassembly_read_u32_le(const uint8_t* data, size_t size, size_t offset, uint32_t* out_value)
{
	ASSERT_FATAL(data);
	ASSERT_FATAL(out_value);

	if (offset > size || (size - offset) < 4U)
	{
		return false;
	}

	*out_value =
		(uint32_t)data[offset + 0U] | ((uint32_t)data[offset + 1U] << 8U) | ((uint32_t)data[offset + 2U] << 16U) | ((uint32_t)data[offset + 3U] << 24U);

	return true;
}

static bool scry_sdlg_disassembly_read_s32_le(const uint8_t* data, size_t size, size_t offset, int32_t* out_value)
{
	union
	{
		uint32_t unsigned_value;
		int32_t	 signed_value;
	} bits = { 0 };

	ASSERT_FATAL(out_value);

	if (!scry_sdlg_disassembly_read_u32_le(data, size, offset, &bits.unsigned_value))
	{
		return false;
	}

	*out_value = bits.signed_value;
	return true;
}

static bool scry_sdlg_disassembly_print_instruction_prefix(FILE* output, size_t offset, uint32_t instruction_index, const char* opcode_name)
{
	ASSERT_FATAL(output);
	ASSERT_FATAL(opcode_name);

	return fprintf(output, "  [%04" PRIu32 "] %04zu: %s", instruction_index, offset, opcode_name) >= 0;
}
