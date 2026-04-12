#include "vm/scry_vm.h"

#include <limits.h>
#include <string.h>

static bool scry_vm_step_jump(scry_vm* vm, size_t cursor);
static bool scry_vm_step_jump_if_false(scry_vm* vm, size_t cursor);
static bool scry_vm_step_dialogue(scry_vm* vm, size_t cursor);
static void scry_vm_fault_now(scry_vm* vm, scry_vm_fault fault);
static bool scry_vm_read_opcode(scry_vm* vm, size_t* cursor, uint8_t* out_opcode);
static bool scry_vm_read_uvarint_operand(scry_vm* vm, size_t* cursor, uint64_t* out_value);
static bool scry_vm_read_svarint_operand(scry_vm* vm, size_t* cursor, int64_t* out_value);
static bool scry_vm_commit_ip(scry_vm* vm, size_t next_ip);
static bool scry_vm_apply_relative_jump(scry_vm* vm, size_t base_ip, int64_t delta);
static bool scry_vm_validate_cursor(scry_vm* vm, size_t cursor);

bool scry_vm_init(scry_vm* vm, const scry_sdlg_view* view)
{
	ASSERT_FATAL(vm);

	memset(vm, 0, sizeof(*vm));

	if (view == NULL)
	{
		vm->status = SCRY_VM_STATUS_FAULTED;
		vm->fault  = SCRY_VM_FAULT_INVALID_VIEW;
		return false;
	}

	vm->view   = view;
	vm->status = SCRY_VM_STATUS_READY;
	return true;
}

void scry_vm_reset(scry_vm* vm)
{
	const scry_sdlg_view* view		= NULL;
	scry_vm_callbacks	  callbacks = { 0 };
	void*				  user_data = NULL;

	ASSERT_FATAL(vm);

	view	  = vm->view;
	callbacks = vm->callbacks;
	user_data = vm->user_data;

	memset(vm, 0, sizeof(*vm));

	vm->view	  = view;
	vm->callbacks = callbacks;
	vm->user_data = user_data;
	vm->status	  = (view != NULL) ? SCRY_VM_STATUS_READY : SCRY_VM_STATUS_FAULTED;
	vm->fault	  = (view != NULL) ? SCRY_VM_FAULT_NONE : SCRY_VM_FAULT_INVALID_VIEW;
}

bool scry_vm_step(scry_vm* vm)
{
	size_t	cursor = 0U;
	uint8_t opcode = 0U;

	ASSERT_FATAL(vm);

	if (vm->view == NULL)
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_INVALID_VIEW);
		return false;
	}

	if (vm->status == SCRY_VM_STATUS_HALTED || vm->status == SCRY_VM_STATUS_FAULTED)
	{
		return false;
	}

	if ((size_t)vm->ip >= vm->view->instructions.size)
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_IP_OUT_OF_RANGE);
		return false;
	}

	cursor			  = (size_t)vm->ip;
	vm->status		  = SCRY_VM_STATUS_RUNNING;
	vm->fault		  = SCRY_VM_FAULT_NONE;
	vm->last_opcode	  = UINT8_C(0);
	vm->last_dialogue = (scry_sdlg_string) { 0 };

	if (!scry_vm_read_opcode(vm, &cursor, &opcode))
	{
		return false;
	}

	vm->last_opcode = opcode;

	switch ((scry_sdlg_opcode)opcode)
	{
		case SCRY_SDLG_OP_NOP:
			return scry_vm_commit_ip(vm, cursor);
		case SCRY_SDLG_OP_JUMP:
			return scry_vm_step_jump(vm, cursor);
		case SCRY_SDLG_OP_JUMP_IF_FALSE:
			return scry_vm_step_jump_if_false(vm, cursor);
		case SCRY_SDLG_OP_DIALOGUE:
			return scry_vm_step_dialogue(vm, cursor);
		case SCRY_SDLG_OP_END:
			vm->ip	   = (uint32_t)cursor;
			vm->status = SCRY_VM_STATUS_HALTED;
			vm->fault  = SCRY_VM_FAULT_NONE;
			return false;
		case SCRY_SDLG_OP_CHOICE:
		case SCRY_SDLG_OP_SWITCH:
		case SCRY_SDLG_OP_SET_VAR:
		case SCRY_SDLG_OP_COMPARE:
			scry_vm_fault_now(vm, SCRY_VM_FAULT_UNSUPPORTED_OPCODE);
			return false;
		default:
			scry_vm_fault_now(vm, SCRY_VM_FAULT_INVALID_OPCODE);
			return false;
	}
}

static bool scry_vm_step_jump(scry_vm* vm, size_t cursor)
{
	int64_t delta = 0;

	if (!scry_vm_read_svarint_operand(vm, &cursor, &delta))
	{
		return false;
	}

	if (!scry_vm_validate_cursor(vm, cursor))
	{
		return false;
	}

	return scry_vm_apply_relative_jump(vm, cursor, delta);
}

static bool scry_vm_step_jump_if_false(scry_vm* vm, size_t cursor)
{
	int64_t delta = 0;

	if (!scry_vm_read_svarint_operand(vm, &cursor, &delta))
	{
		return false;
	}

	if (!scry_vm_validate_cursor(vm, cursor))
	{
		return false;
	}

	if (!vm->condition)
	{
		return scry_vm_apply_relative_jump(vm, cursor, delta);
	}

	return scry_vm_commit_ip(vm, cursor);
}

static bool scry_vm_step_dialogue(scry_vm* vm, size_t cursor)
{
	uint64_t string_id = 0U;

	if (!scry_vm_read_uvarint_operand(vm, &cursor, &string_id))
	{
		return false;
	}

	if (!scry_vm_validate_cursor(vm, cursor))
	{
		return false;
	}

	if (string_id > (uint64_t)UINT32_MAX || !scry_sdlg_string_at(vm->view, (uint32_t)string_id, &vm->last_dialogue))
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_INVALID_STRING_ID);
		return false;
	}

	vm->last_dialogue_string_id = (uint32_t)string_id;

	if (vm->callbacks.emit_dialogue != NULL &&
		!vm->callbacks.emit_dialogue(vm->user_data, vm->last_dialogue_string_id, vm->last_dialogue.data, vm->last_dialogue.length))
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_HOST_REJECTED);
		return false;
	}

	return scry_vm_commit_ip(vm, cursor);
}

static void scry_vm_fault_now(scry_vm* vm, scry_vm_fault fault)
{
	ASSERT_FATAL(vm);

	vm->status = SCRY_VM_STATUS_FAULTED;
	vm->fault  = fault;
}

static bool scry_vm_read_opcode(scry_vm* vm, size_t* cursor, uint8_t* out_opcode)
{
	ASSERT_FATAL(vm);
	ASSERT_FATAL(cursor);
	ASSERT_FATAL(out_opcode);

	if (*cursor >= vm->view->instructions.size)
	{
		return false;
	}

	*out_opcode = vm->view->instructions.data[*cursor];
	*cursor += 1U;
	return true;
}

static bool scry_vm_read_uvarint_operand(scry_vm* vm, size_t* cursor, uint64_t* out_value)
{
	ASSERT_FATAL(vm);
	ASSERT_FATAL(cursor);
	ASSERT_FATAL(out_value);

	if (!scry_sdlg_read_uvarint(vm->view->instructions.data, vm->view->instructions.size, cursor, out_value))
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_TRUNCATED_INSTRUCTION);
		return false;
	}

	return true;
}

static bool scry_vm_read_svarint_operand(scry_vm* vm, size_t* cursor, int64_t* out_value)
{
	ASSERT_FATAL(vm);
	ASSERT_FATAL(cursor);
	ASSERT_FATAL(out_value);

	if (!scry_sdlg_read_svarint(vm->view->instructions.data, vm->view->instructions.size, cursor, out_value))
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_TRUNCATED_INSTRUCTION);
		return false;
	}

	return true;
}

static bool scry_vm_commit_ip(scry_vm* vm, size_t next_ip)
{
	ASSERT_FATAL(vm);

	if (next_ip > (size_t)UINT32_MAX || next_ip > vm->view->instructions.size)
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_IP_OUT_OF_RANGE);
		return false;
	}

	vm->ip	   = (uint32_t)next_ip;
	vm->status = SCRY_VM_STATUS_READY;
	vm->fault  = SCRY_VM_FAULT_NONE;
	return true;
}

static bool scry_vm_apply_relative_jump(scry_vm* vm, size_t base_ip, int64_t delta)
{
	int64_t target = 0;

	ASSERT_FATAL(vm);

	if (base_ip > (size_t)INT64_MAX)
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_INVALID_JUMP_TARGET);
		return false;
	}

	target = (int64_t)base_ip + delta;

	if (target < 0 || (uint64_t)target > (uint64_t)vm->view->instructions.size)
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_INVALID_JUMP_TARGET);
		return false;
	}

	return scry_vm_commit_ip(vm, (size_t)target);
}

static bool scry_vm_validate_cursor(scry_vm* vm, size_t cursor)
{
	ASSERT_FATAL(vm);

	if (cursor > vm->view->instructions.size)
	{
		scry_vm_fault_now(vm, SCRY_VM_FAULT_TRUNCATED_INSTRUCTION);
		return false;
	}

	return true;
}
