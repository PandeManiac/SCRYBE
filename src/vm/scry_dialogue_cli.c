#include "vm/scry_dialogue_cli.h"

#include "vm/scry_ir.h"
#include "vm/scry_sdlg.h"
#include "vm/scry_vm.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct scry_dialogue_choice_option
{
	uint32_t text_string_id;
	int32_t	 target_delta;
} scry_dialogue_choice_option;

typedef struct scry_dialogue_switch_case
{
	int32_t value;
	int32_t target_delta;
} scry_dialogue_switch_case;

static bool scry_dialogue_emit(void* user_data, uint32_t string_id, const char* text, size_t text_length);
static bool scry_dialogue_step_host_opcode(scry_vm* vm, FILE* input, FILE* output);
static bool scry_dialogue_step_compare(scry_vm* vm, FILE* input, FILE* output);
static bool scry_dialogue_step_choice(scry_vm* vm, FILE* input, FILE* output);
static bool scry_dialogue_step_switch(scry_vm* vm, FILE* input, FILE* output);
static bool scry_dialogue_prompt_continue(FILE* input, FILE* output);
static bool scry_dialogue_prompt_number(FILE* input, FILE* output, const char* prompt, int32_t* out_value);
static bool scry_dialogue_prompt_choice(FILE* input, FILE* output, uint16_t option_count, uint16_t* out_choice_index);
static bool scry_dialogue_read_table_payload(const scry_sdlg_view* view, uint32_t table_index, uint16_t expected_kind, scry_sdlg_span* out_payload);
static bool scry_dialogue_read_choice_option(const scry_sdlg_span* payload, uint16_t option_index, scry_dialogue_choice_option* out_option);
static bool scry_dialogue_read_switch_case(const scry_sdlg_span* payload, uint16_t case_index, scry_dialogue_switch_case* out_case);
static bool scry_dialogue_read_u16_le(const uint8_t* data, size_t size, size_t offset, uint16_t* out_value);
static bool scry_dialogue_read_u32_le(const uint8_t* data, size_t size, size_t offset, uint32_t* out_value);
static bool scry_dialogue_read_s32_le(const uint8_t* data, size_t size, size_t offset, int32_t* out_value);
static bool scry_dialogue_compare_value(int32_t left_value, scry_ir_compare_op compare_op, int32_t right_value, bool* out_result);
static bool scry_dialogue_set_ip(scry_vm* vm, size_t base_ip, int32_t delta);
static void scry_dialogue_fault(scry_vm* vm, scry_vm_fault fault);

bool scry_dialogue_run_file(const char* path)
{
	scry_sdlg_document document = { 0 };
	scry_vm			   vm		= { 0 };

	ASSERT_FATAL(path);

	if (!scry_sdlg_document_load_file(&document, path) || !scry_vm_init(&vm, &document.view))
	{
		scry_sdlg_document_destroy(&document);
		return false;
	}

	vm.callbacks.emit_dialogue = scry_dialogue_emit;
	vm.user_data				  = stdout;

	while (vm.status != SCRY_VM_STATUS_HALTED && vm.status != SCRY_VM_STATUS_FAULTED)
	{
		uint8_t opcode = 0U;

		if ((size_t)vm.ip >= vm.view->instructions.size)
		{
			(void)scry_vm_step(&vm);
			break;
		}

		opcode = vm.view->instructions.data[vm.ip];

		if (opcode == (uint8_t)SCRY_SDLG_OP_COMPARE || opcode == (uint8_t)SCRY_SDLG_OP_CHOICE || opcode == (uint8_t)SCRY_SDLG_OP_SWITCH)
		{
			if (!scry_dialogue_step_host_opcode(&vm, stdin, stdout))
			{
				break;
			}

			continue;
		}

		(void)scry_vm_step(&vm);

		if (vm.last_opcode == (uint8_t)SCRY_SDLG_OP_DIALOGUE && vm.status == SCRY_VM_STATUS_READY && !scry_dialogue_prompt_continue(stdin, stdout))
		{
			scry_dialogue_fault(&vm, SCRY_VM_FAULT_HOST_REJECTED);
			break;
		}
	}

	if (vm.status == SCRY_VM_STATUS_FAULTED)
	{
		(void)fprintf(stderr, "Dialogue VM fault: %d\n", (int)vm.fault);
	}

	scry_sdlg_document_destroy(&document);
	return vm.status == SCRY_VM_STATUS_HALTED;
}

static bool scry_dialogue_emit(void* user_data, uint32_t string_id, const char* text, size_t text_length)
{
	FILE* output = (FILE*)user_data;

	ASSERT_FATAL(output);
	ASSERT_FATAL(text || text_length == 0U);

	return fprintf(output, "\n[%" PRIu32 "] %.*s\n", string_id, (int)text_length, text) >= 0;
}

static bool scry_dialogue_step_host_opcode(scry_vm* vm, FILE* input, FILE* output)
{
	ASSERT_FATAL(vm);
	ASSERT_FATAL(vm->view);
	ASSERT_FATAL(input);
	ASSERT_FATAL(output);

	switch ((scry_sdlg_opcode)vm->view->instructions.data[vm->ip])
	{
		case SCRY_SDLG_OP_COMPARE:
			return scry_dialogue_step_compare(vm, input, output);
		case SCRY_SDLG_OP_CHOICE:
			return scry_dialogue_step_choice(vm, input, output);
		case SCRY_SDLG_OP_SWITCH:
			return scry_dialogue_step_switch(vm, input, output);
		case SCRY_SDLG_OP_NOP:
		case SCRY_SDLG_OP_JUMP:
		case SCRY_SDLG_OP_JUMP_IF_FALSE:
		case SCRY_SDLG_OP_DIALOGUE:
		case SCRY_SDLG_OP_SET_VAR:
		case SCRY_SDLG_OP_END:
		default:
			ASSERT_FATAL(false);
			return false;
	}
}

static bool scry_dialogue_step_compare(scry_vm* vm, FILE* input, FILE* output)
{
	size_t			 cursor = (size_t)vm->ip + 1U;
	uint64_t			 variable_id = 0U;
	uint64_t			 compare_op = 0U;
	int64_t			 compare_value = 0;
	int32_t			 variable_value = 0;
	bool				 result = false;
	char				 prompt[32] = { 0 };

	ASSERT_FATAL(vm);

	if (!scry_sdlg_read_uvarint(vm->view->instructions.data, vm->view->instructions.size, &cursor, &variable_id) ||
		!scry_sdlg_read_uvarint(vm->view->instructions.data, vm->view->instructions.size, &cursor, &compare_op) ||
		!scry_sdlg_read_svarint(vm->view->instructions.data, vm->view->instructions.size, &cursor, &compare_value) ||
		variable_id > UINT32_MAX || compare_op > UINT32_MAX || compare_value < INT32_MIN || compare_value > INT32_MAX)
	{
		scry_dialogue_fault(vm, SCRY_VM_FAULT_TRUNCATED_INSTRUCTION);
		return false;
	}

	if (snprintf(prompt, sizeof(prompt), "variable[%" PRIu64 "]> ", variable_id) < 0 ||
		!scry_dialogue_prompt_number(input, output, prompt, &variable_value) ||
		!scry_dialogue_compare_value(variable_value, (scry_ir_compare_op)compare_op, (int32_t)compare_value, &result))
	{
		scry_dialogue_fault(vm, SCRY_VM_FAULT_HOST_REJECTED);
		return false;
	}

	vm->condition = result;
	vm->ip		   = (uint32_t)cursor;
	vm->status	   = SCRY_VM_STATUS_READY;
	vm->fault	   = SCRY_VM_FAULT_NONE;
	return true;
}

static bool scry_dialogue_step_choice(scry_vm* vm, FILE* input, FILE* output)
{
	size_t		   cursor = (size_t)vm->ip + 1U;
	uint64_t		   table_index = 0U;
	scry_sdlg_span payload = { 0 };
	uint16_t		   option_count = 0U;
	uint16_t		   selected_option = 0U;

	ASSERT_FATAL(vm);

	if (!scry_sdlg_read_uvarint(vm->view->instructions.data, vm->view->instructions.size, &cursor, &table_index) ||
		table_index > UINT32_MAX ||
		!scry_dialogue_read_table_payload(vm->view, (uint32_t)table_index, (uint16_t)SCRY_SDLG_TABLE_KIND_CHOICE, &payload) ||
		!scry_dialogue_read_u16_le(payload.data, payload.size, 0U, &option_count))
	{
		scry_dialogue_fault(vm, SCRY_VM_FAULT_INVALID_OPERAND);
		return false;
	}

	for (uint16_t i = 0U; i < option_count; ++i)
	{
		scry_dialogue_choice_option option = { 0 };
		scry_sdlg_string			 string = { 0 };

		if (!scry_dialogue_read_choice_option(&payload, i, &option) || !scry_sdlg_string_at(vm->view, option.text_string_id, &string))
		{
			scry_dialogue_fault(vm, SCRY_VM_FAULT_INVALID_STRING_ID);
			return false;
		}

		if (fprintf(output, "%u. %.*s\n", (unsigned)(i + 1U), (int)string.length, string.data) < 0)
		{
			scry_dialogue_fault(vm, SCRY_VM_FAULT_HOST_REJECTED);
			return false;
		}
	}

	if (!scry_dialogue_prompt_choice(input, output, option_count, &selected_option))
	{
		scry_dialogue_fault(vm, SCRY_VM_FAULT_HOST_REJECTED);
		return false;
	}

	{
		scry_dialogue_choice_option option = { 0 };

		if (!scry_dialogue_read_choice_option(&payload, selected_option, &option) ||
			!scry_dialogue_set_ip(vm, cursor, option.target_delta))
		{
			scry_dialogue_fault(vm, SCRY_VM_FAULT_INVALID_JUMP_TARGET);
			return false;
		}
	}

	return true;
}

static bool scry_dialogue_step_switch(scry_vm* vm, FILE* input, FILE* output)
{
	size_t		   cursor = (size_t)vm->ip + 1U;
	uint64_t		   table_index = 0U;
	scry_sdlg_span payload = { 0 };
	uint16_t		   case_count = 0U;
	uint16_t		   variable_id = 0U;
	int32_t		   default_delta = 0;
	int32_t		   variable_value = 0;
	int32_t		   selected_delta = 0;
	bool			   matched = false;

	ASSERT_FATAL(vm);

	if (!scry_sdlg_read_uvarint(vm->view->instructions.data, vm->view->instructions.size, &cursor, &table_index) ||
		table_index > UINT32_MAX ||
		!scry_dialogue_read_table_payload(vm->view, (uint32_t)table_index, (uint16_t)SCRY_SDLG_TABLE_KIND_SWITCH, &payload) ||
		!scry_dialogue_read_u16_le(payload.data, payload.size, 0U, &case_count) ||
		!scry_dialogue_read_u16_le(payload.data, payload.size, 2U, &variable_id) ||
		!scry_dialogue_read_s32_le(payload.data, payload.size, 4U, &default_delta))
	{
		scry_dialogue_fault(vm, SCRY_VM_FAULT_INVALID_OPERAND);
		return false;
	}

	if (fprintf(output, "switch variable[%" PRIu16 "]\n", variable_id) < 0 ||
		!scry_dialogue_prompt_number(input, output, "variable> ", &variable_value))
	{
		scry_dialogue_fault(vm, SCRY_VM_FAULT_HOST_REJECTED);
		return false;
	}

	selected_delta = default_delta;

	for (uint16_t i = 0U; i < case_count; ++i)
	{
		scry_dialogue_switch_case switch_case = { 0 };

		if (!scry_dialogue_read_switch_case(&payload, i, &switch_case))
		{
			scry_dialogue_fault(vm, SCRY_VM_FAULT_INVALID_OPERAND);
			return false;
		}

		if (switch_case.value == variable_value)
		{
			selected_delta = switch_case.target_delta;
			matched		   = true;
			break;
		}
	}

	if (!matched && fprintf(output, "default\n") < 0)
	{
		scry_dialogue_fault(vm, SCRY_VM_FAULT_HOST_REJECTED);
		return false;
	}

	if (!scry_dialogue_set_ip(vm, cursor, selected_delta))
	{
		scry_dialogue_fault(vm, SCRY_VM_FAULT_INVALID_JUMP_TARGET);
		return false;
	}

	return true;
}

static bool scry_dialogue_prompt_continue(FILE* input, FILE* output)
{
	char buffer[8] = { 0 };

	ASSERT_FATAL(input);
	ASSERT_FATAL(output);

	if (fprintf(output, "enter> ") < 0 || fflush(output) != 0)
	{
		return false;
	}

	return fgets(buffer, sizeof(buffer), input) != NULL;
}

static bool scry_dialogue_prompt_number(FILE* input, FILE* output, const char* prompt, int32_t* out_value)
{
	char* end = NULL;
	char  buffer[64] = { 0 };
	long  parsed = 0L;

	ASSERT_FATAL(input);
	ASSERT_FATAL(output);
	ASSERT_FATAL(prompt);
	ASSERT_FATAL(out_value);

	for (;;)
	{
		if (fputs(prompt, output) == EOF || fflush(output) != 0)
		{
			return false;
		}

		if (fgets(buffer, sizeof(buffer), input) == NULL)
		{
			return false;
		}

		parsed = strtol(buffer, &end, 10);

		if (end != buffer && (*end == '\n' || *end == '\0') && parsed >= (long)INT32_MIN && parsed <= (long)INT32_MAX)
		{
			*out_value = (int32_t)parsed;
			return true;
		}
	}
}

static bool scry_dialogue_prompt_choice(FILE* input, FILE* output, uint16_t option_count, uint16_t* out_choice_index)
{
	int32_t selected = 0;

	ASSERT_FATAL(out_choice_index);

	for (;;)
	{
		if (!scry_dialogue_prompt_number(input, output, "choice> ", &selected))
		{
			return false;
		}

		if (selected >= 1 && selected <= option_count)
		{
			*out_choice_index = (uint16_t)(selected - 1);
			return true;
		}
	}
}

static bool scry_dialogue_read_table_payload(const scry_sdlg_view* view, uint32_t table_index, uint16_t expected_kind, scry_sdlg_span* out_payload)
{
	scry_sdlg_table_entry table = { 0 };

	ASSERT_FATAL(view);
	ASSERT_FATAL(out_payload);

	return scry_sdlg_table_at(view, table_index, &table) && table.kind == expected_kind && scry_sdlg_table_payload(view, &table, out_payload);
}

static bool scry_dialogue_read_choice_option(const scry_sdlg_span* payload, uint16_t option_index, scry_dialogue_choice_option* out_option)
{
	const size_t offset = sizeof(scry_sdlg_choice_table_header) + ((size_t)option_index * (sizeof(uint32_t) + sizeof(int32_t)));

	ASSERT_FATAL(payload);
	ASSERT_FATAL(out_option);

	return scry_dialogue_read_u32_le(payload->data, payload->size, offset, &out_option->text_string_id) &&
		   scry_dialogue_read_s32_le(payload->data, payload->size, offset + sizeof(uint32_t), &out_option->target_delta);
}

static bool scry_dialogue_read_switch_case(const scry_sdlg_span* payload, uint16_t case_index, scry_dialogue_switch_case* out_case)
{
	const size_t offset = sizeof(scry_sdlg_switch_table_header) + ((size_t)case_index * (sizeof(int32_t) + sizeof(int32_t)));

	ASSERT_FATAL(payload);
	ASSERT_FATAL(out_case);

	return scry_dialogue_read_s32_le(payload->data, payload->size, offset, &out_case->value) &&
		   scry_dialogue_read_s32_le(payload->data, payload->size, offset + sizeof(int32_t), &out_case->target_delta);
}

static bool scry_dialogue_read_u16_le(const uint8_t* data, size_t size, size_t offset, uint16_t* out_value)
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

static bool scry_dialogue_read_u32_le(const uint8_t* data, size_t size, size_t offset, uint32_t* out_value)
{
	ASSERT_FATAL(data);
	ASSERT_FATAL(out_value);

	if (offset > size || (size - offset) < 4U)
	{
		return false;
	}

	*out_value = (uint32_t)data[offset + 0U] | ((uint32_t)data[offset + 1U] << 8U) |
				 ((uint32_t)data[offset + 2U] << 16U) | ((uint32_t)data[offset + 3U] << 24U);
	return true;
}

static bool scry_dialogue_read_s32_le(const uint8_t* data, size_t size, size_t offset, int32_t* out_value)
{
	union
	{
		uint32_t unsigned_value;
		int32_t	 signed_value;
	} bits = { 0 };

	ASSERT_FATAL(out_value);

	if (!scry_dialogue_read_u32_le(data, size, offset, &bits.unsigned_value))
	{
		return false;
	}

	*out_value = bits.signed_value;
	return true;
}

static bool scry_dialogue_compare_value(int32_t left_value, scry_ir_compare_op compare_op, int32_t right_value, bool* out_result)
{
	ASSERT_FATAL(out_result);

	switch (compare_op)
	{
		case SCRY_IR_COMPARE_OP_EQUAL:
			*out_result = left_value == right_value;
			return true;
		case SCRY_IR_COMPARE_OP_NOT_EQUAL:
			*out_result = left_value != right_value;
			return true;
		case SCRY_IR_COMPARE_OP_LESS:
			*out_result = left_value < right_value;
			return true;
		case SCRY_IR_COMPARE_OP_LESS_EQUAL:
			*out_result = left_value <= right_value;
			return true;
		case SCRY_IR_COMPARE_OP_GREATER:
			*out_result = left_value > right_value;
			return true;
		case SCRY_IR_COMPARE_OP_GREATER_EQUAL:
			*out_result = left_value >= right_value;
			return true;
		case SCRY_IR_COMPARE_OP_NONE:
		default:
			return false;
	}
}

static bool scry_dialogue_set_ip(scry_vm* vm, size_t base_ip, int32_t delta)
{
	const int64_t target = (int64_t)base_ip + (int64_t)delta;

	ASSERT_FATAL(vm);

	if (target < 0 || (uint64_t)target > (uint64_t)vm->view->instructions.size)
	{
		return false;
	}

	vm->ip	 = (uint32_t)target;
	vm->status = SCRY_VM_STATUS_READY;
	vm->fault  = SCRY_VM_FAULT_NONE;
	return true;
}

static void scry_dialogue_fault(scry_vm* vm, scry_vm_fault fault)
{
	ASSERT_FATAL(vm);

	vm->status = SCRY_VM_STATUS_FAULTED;
	vm->fault  = fault;
}
