#pragma once

#include "utils/core/scry_assert.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum scry_ir_node_kind
{
	SCRY_IR_NODE_KIND_NONE = 0,
	SCRY_IR_NODE_KIND_DIALOGUE,
	SCRY_IR_NODE_KIND_JUMP,
	SCRY_IR_NODE_KIND_BRANCH,
	SCRY_IR_NODE_KIND_CHOICE,
	SCRY_IR_NODE_KIND_SWITCH,
	SCRY_IR_NODE_KIND_END,
} scry_ir_node_kind;

typedef enum scry_ir_compare_op
{
	SCRY_IR_COMPARE_OP_NONE = 0,
	SCRY_IR_COMPARE_OP_EQUAL,
	SCRY_IR_COMPARE_OP_NOT_EQUAL,
	SCRY_IR_COMPARE_OP_LESS,
	SCRY_IR_COMPARE_OP_LESS_EQUAL,
	SCRY_IR_COMPARE_OP_GREATER,
	SCRY_IR_COMPARE_OP_GREATER_EQUAL,
} scry_ir_compare_op;

typedef struct scry_ir_condition
{
	uint32_t		   variable_id;
	scry_ir_compare_op compare_op;
	int32_t			   value;
} scry_ir_condition;

typedef struct scry_ir_choice_option
{
	uint32_t text_string_id;
	uint32_t target_node_index;
} scry_ir_choice_option;

typedef struct scry_ir_switch_case
{
	int32_t	 value;
	uint32_t target_node_index;
} scry_ir_switch_case;

typedef struct scry_ir_node
{
	scry_ir_node_kind kind;

	union
	{
		struct
		{
			uint32_t string_id;
			uint32_t next_node_index;
		} dialogue;

		struct
		{
			uint32_t target_node_index;
		} jump;

		struct
		{
			scry_ir_condition condition;
			uint32_t		  true_node_index;
			uint32_t		  false_node_index;
		} branch;

		struct
		{
			uint32_t option_start;
			uint32_t option_count;
		} choice;

		struct
		{
			// Cases are validated as a strictly increasing sequence by value.
			uint32_t variable_id;
			uint32_t case_start;
			uint32_t case_count;
			uint32_t default_node_index;
		} switch_branch;
	} data;
} scry_ir_node;

typedef struct scry_ir_program
{
	const scry_ir_node*			 nodes;
	const scry_ir_choice_option* choice_options;
	const scry_ir_switch_case*	 switch_cases;
	uint32_t					 node_count;
	uint32_t					 choice_option_count;
	uint32_t					 switch_case_count;
	uint32_t					 string_count;
	uint32_t					 variable_count;
	uint32_t					 entry_node_index;
} scry_ir_program;

bool		 scry_ir_node_kind_is_valid(scry_ir_node_kind kind);
bool		 scry_ir_compare_op_is_valid(scry_ir_compare_op compare_op);
void		 scry_ir_program_init(scry_ir_program* program);
void		 scry_ir_node_init(scry_ir_node* node);
scry_ir_node scry_ir_make_dialogue_node(uint32_t string_id, uint32_t next_node_index);
scry_ir_node scry_ir_make_jump_node(uint32_t target_node_index);
scry_ir_node scry_ir_make_branch_node(scry_ir_condition condition, uint32_t true_node_index, uint32_t false_node_index);
scry_ir_node scry_ir_make_choice_node(uint32_t option_start, uint32_t option_count);
scry_ir_node scry_ir_make_switch_node(uint32_t variable_id, uint32_t case_start, uint32_t case_count, uint32_t default_node_index);
scry_ir_node scry_ir_make_end_node(void);
bool		 scry_ir_program_validate(const scry_ir_program* program);
