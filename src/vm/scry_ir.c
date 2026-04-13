#include "vm/scry_ir.h"

#include <string.h>

static bool scry_ir_node_index_is_valid(const scry_ir_program* program, uint32_t node_index);
static bool scry_ir_string_id_is_valid(const scry_ir_program* program, uint32_t string_id);
static bool scry_ir_variable_id_is_valid(const scry_ir_program* program, uint32_t variable_id);
static bool scry_ir_choice_range_is_valid(const scry_ir_program* program, uint32_t start, uint32_t count);
static bool scry_ir_switch_range_is_valid(const scry_ir_program* program, uint32_t start, uint32_t count);
static bool scry_ir_condition_is_valid(const scry_ir_program* program, const scry_ir_condition* condition);
static bool scry_ir_validate_node(const scry_ir_program* program, const scry_ir_node* node);

bool scry_ir_node_kind_is_valid(scry_ir_node_kind kind)
{
	switch (kind)
	{
		case SCRY_IR_NODE_KIND_DIALOGUE:
		case SCRY_IR_NODE_KIND_JUMP:
		case SCRY_IR_NODE_KIND_BRANCH:
		case SCRY_IR_NODE_KIND_CHOICE:
		case SCRY_IR_NODE_KIND_SWITCH:
		case SCRY_IR_NODE_KIND_END:
			return true;
		case SCRY_IR_NODE_KIND_NONE:
		default:
			return false;
	}
}

bool scry_ir_compare_op_is_valid(scry_ir_compare_op compare_op)
{
	switch (compare_op)
	{
		case SCRY_IR_COMPARE_OP_EQUAL:
		case SCRY_IR_COMPARE_OP_NOT_EQUAL:
		case SCRY_IR_COMPARE_OP_LESS:
		case SCRY_IR_COMPARE_OP_LESS_EQUAL:
		case SCRY_IR_COMPARE_OP_GREATER:
		case SCRY_IR_COMPARE_OP_GREATER_EQUAL:
			return true;
		case SCRY_IR_COMPARE_OP_NONE:
		default:
			return false;
	}
}

void scry_ir_program_init(scry_ir_program* program)
{
	ASSERT_FATAL(program);

	memset(program, 0, sizeof(*program));
}

void scry_ir_node_init(scry_ir_node* node)
{
	ASSERT_FATAL(node);

	memset(node, 0, sizeof(*node));
}

scry_ir_node scry_ir_make_dialogue_node(uint32_t string_id, uint32_t next_node_index)
{
	scry_ir_node node = { 0 };

	node.kind						   = SCRY_IR_NODE_KIND_DIALOGUE;
	node.data.dialogue.string_id	   = string_id;
	node.data.dialogue.next_node_index = next_node_index;
	return node;
}

scry_ir_node scry_ir_make_jump_node(uint32_t target_node_index)
{
	scry_ir_node node = { 0 };

	node.kind						 = SCRY_IR_NODE_KIND_JUMP;
	node.data.jump.target_node_index = target_node_index;
	return node;
}

scry_ir_node scry_ir_make_branch_node(scry_ir_condition condition, uint32_t true_node_index, uint32_t false_node_index)
{
	scry_ir_node node = { 0 };

	node.kind						  = SCRY_IR_NODE_KIND_BRANCH;
	node.data.branch.condition		  = condition;
	node.data.branch.true_node_index  = true_node_index;
	node.data.branch.false_node_index = false_node_index;
	return node;
}

scry_ir_node scry_ir_make_choice_node(uint32_t option_start, uint32_t option_count)
{
	scry_ir_node node = { 0 };

	node.kind					  = SCRY_IR_NODE_KIND_CHOICE;
	node.data.choice.option_start = option_start;
	node.data.choice.option_count = option_count;
	return node;
}

scry_ir_node scry_ir_make_switch_node(uint32_t variable_id, uint32_t case_start, uint32_t case_count, uint32_t default_node_index)
{
	scry_ir_node node = { 0 };

	node.kind								   = SCRY_IR_NODE_KIND_SWITCH;
	node.data.switch_branch.variable_id		   = variable_id;
	node.data.switch_branch.case_start		   = case_start;
	node.data.switch_branch.case_count		   = case_count;
	node.data.switch_branch.default_node_index = default_node_index;
	return node;
}

scry_ir_node scry_ir_make_end_node(void)
{
	scry_ir_node node = { 0 };

	node.kind = SCRY_IR_NODE_KIND_END;
	return node;
}

bool scry_ir_program_validate(const scry_ir_program* program)
{
	ASSERT_FATAL(program);

	if (program->nodes == NULL || program->node_count == 0U)
	{
		return false;
	}

	if (program->entry_node_index >= program->node_count)
	{
		return false;
	}

	if ((program->choice_option_count > 0U) && program->choice_options == NULL)
	{
		return false;
	}

	if ((program->switch_case_count > 0U) && program->switch_cases == NULL)
	{
		return false;
	}

	for (uint32_t i = 0U; i < program->node_count; ++i)
	{
		if (!scry_ir_validate_node(program, &program->nodes[i]))
		{
			return false;
		}
	}

	return true;
}

static bool scry_ir_node_index_is_valid(const scry_ir_program* program, uint32_t node_index)
{
	ASSERT_FATAL(program);

	return node_index < program->node_count;
}

static bool scry_ir_string_id_is_valid(const scry_ir_program* program, uint32_t string_id)
{
	ASSERT_FATAL(program);

	return string_id < program->string_count;
}

static bool scry_ir_variable_id_is_valid(const scry_ir_program* program, uint32_t variable_id)
{
	ASSERT_FATAL(program);

	return variable_id < program->variable_count;
}

static bool scry_ir_choice_range_is_valid(const scry_ir_program* program, uint32_t start, uint32_t count)
{
	ASSERT_FATAL(program);

	if (count == 0U)
	{
		return false;
	}

	if (start >= program->choice_option_count)
	{
		return false;
	}

	return (uint64_t)start + (uint64_t)count <= (uint64_t)program->choice_option_count;
}

static bool scry_ir_switch_range_is_valid(const scry_ir_program* program, uint32_t start, uint32_t count)
{
	ASSERT_FATAL(program);

	if (count == 0U)
	{
		return false;
	}

	if (start >= program->switch_case_count)
	{
		return false;
	}

	return (uint64_t)start + (uint64_t)count <= (uint64_t)program->switch_case_count;
}

static bool scry_ir_condition_is_valid(const scry_ir_program* program, const scry_ir_condition* condition)
{
	ASSERT_FATAL(program);
	ASSERT_FATAL(condition);

	return scry_ir_compare_op_is_valid(condition->compare_op) && scry_ir_variable_id_is_valid(program, condition->variable_id);
}

static bool scry_ir_validate_node(const scry_ir_program* program, const scry_ir_node* node)
{
	ASSERT_FATAL(program);
	ASSERT_FATAL(node);

	if (!scry_ir_node_kind_is_valid(node->kind))
	{
		return false;
	}

	switch (node->kind)
	{
		case SCRY_IR_NODE_KIND_DIALOGUE:
			return scry_ir_string_id_is_valid(program, node->data.dialogue.string_id) &&
				   scry_ir_node_index_is_valid(program, node->data.dialogue.next_node_index);
		case SCRY_IR_NODE_KIND_JUMP:
			return scry_ir_node_index_is_valid(program, node->data.jump.target_node_index);
		case SCRY_IR_NODE_KIND_BRANCH:
			return scry_ir_condition_is_valid(program, &node->data.branch.condition) &&
				   scry_ir_node_index_is_valid(program, node->data.branch.true_node_index) &&
				   scry_ir_node_index_is_valid(program, node->data.branch.false_node_index);
		case SCRY_IR_NODE_KIND_CHOICE:
			if (!scry_ir_choice_range_is_valid(program, node->data.choice.option_start, node->data.choice.option_count))
			{
				return false;
			}

			for (uint32_t i = 0U; i < node->data.choice.option_count; ++i)
			{
				const uint32_t				 option_index = node->data.choice.option_start + i;
				const scry_ir_choice_option* option		  = &program->choice_options[option_index];

				if (!scry_ir_string_id_is_valid(program, option->text_string_id) || !scry_ir_node_index_is_valid(program, option->target_node_index))
				{
					return false;
				}
			}

			return true;
		case SCRY_IR_NODE_KIND_SWITCH:
			if (!scry_ir_switch_range_is_valid(program, node->data.switch_branch.case_start, node->data.switch_branch.case_count) ||
				!scry_ir_variable_id_is_valid(program, node->data.switch_branch.variable_id) ||
				!scry_ir_node_index_is_valid(program, node->data.switch_branch.default_node_index))
			{
				return false;
			}

			for (uint32_t i = 0U; i < node->data.switch_branch.case_count; ++i)
			{
				const uint32_t			   case_index  = node->data.switch_branch.case_start + i;
				const scry_ir_switch_case* switch_case = &program->switch_cases[case_index];

				if (!scry_ir_node_index_is_valid(program, switch_case->target_node_index))
				{
					return false;
				}

				if (i > 0U)
				{
					const uint32_t			   previous_case_index = case_index - 1U;
					const scry_ir_switch_case* previous_switch_case = &program->switch_cases[previous_case_index];

					if (previous_switch_case->value >= switch_case->value)
					{
						return false;
					}
				}
			}

			return true;
		case SCRY_IR_NODE_KIND_END:
			return true;
		case SCRY_IR_NODE_KIND_NONE:
		default:
			return false;
	}
}
