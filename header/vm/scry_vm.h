#pragma once

#include "vm/scry_sdlg_loader.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum scry_vm_status
{
	SCRY_VM_STATUS_READY = 0,
	SCRY_VM_STATUS_RUNNING,
	SCRY_VM_STATUS_HALTED,
	SCRY_VM_STATUS_FAULTED,
} scry_vm_status;

typedef enum scry_vm_fault
{
	SCRY_VM_FAULT_NONE = 0,
	SCRY_VM_FAULT_INVALID_ARGUMENT,
	SCRY_VM_FAULT_INVALID_VIEW,
	SCRY_VM_FAULT_IP_OUT_OF_RANGE,
	SCRY_VM_FAULT_TRUNCATED_INSTRUCTION,
	SCRY_VM_FAULT_INVALID_OPCODE,
	SCRY_VM_FAULT_INVALID_OPERAND,
	SCRY_VM_FAULT_INVALID_JUMP_TARGET,
	SCRY_VM_FAULT_INVALID_STRING_ID,
	SCRY_VM_FAULT_UNSUPPORTED_OPCODE,
	SCRY_VM_FAULT_HOST_REJECTED,
} scry_vm_fault;

typedef bool (*scry_vm_dialogue_callback)(void* user_data, uint32_t string_id, const char* text, size_t text_length);

typedef struct scry_vm_callbacks
{
	scry_vm_dialogue_callback emit_dialogue;
} scry_vm_callbacks;

typedef struct scry_vm
{
	const scry_sdlg_view* view;
	uint32_t			  ip;
	bool				  condition;
	scry_vm_status		  status;
	scry_vm_fault		  fault;
	uint8_t				  last_opcode;
	uint32_t			  last_dialogue_string_id;
	scry_sdlg_string	  last_dialogue;
	scry_vm_callbacks	  callbacks;
	void*				  user_data;
} scry_vm;

bool scry_vm_init(scry_vm* vm, const scry_sdlg_view* view);
void scry_vm_reset(scry_vm* vm);
bool scry_vm_step(scry_vm* vm);
