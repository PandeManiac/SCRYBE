#include "vm/scry_ir.h"

#include "utils/core/scry_storage.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define SCRY_IR_INVALID_INDEX UINT32_MAX

static bool scry_ir_node_index_is_valid(const scry_ir_program* program, uint32_t node_index);
static bool scry_ir_string_id_is_valid(const scry_ir_program* program, uint32_t string_id);
static bool scry_ir_variable_id_is_valid(const scry_ir_program* program, uint32_t variable_id);
static bool scry_ir_choice_range_is_valid(const scry_ir_program* program, uint32_t start, uint32_t count);
static bool scry_ir_switch_range_is_valid(const scry_ir_program* program, uint32_t start, uint32_t count);
static bool scry_ir_condition_is_valid(const scry_ir_program* program, const scry_ir_condition* condition);
static bool scry_ir_validate_node(const scry_ir_program* program, const scry_ir_node* node);
static bool scry_ir_builder_node_index_is_valid(const scry_ir_program_builder* builder, uint32_t node_index);
static bool scry_ir_builder_option_index_is_valid(const scry_ir_program_builder* builder, uint32_t option_index);
static bool scry_ir_builder_case_index_is_valid(const scry_ir_program_builder* builder, uint32_t case_index);
static bool scry_ir_builder_string_id_is_valid(const scry_ir_program_builder* builder, uint32_t string_id);
static bool scry_ir_builder_variable_id_is_valid(const scry_ir_program_builder* builder, uint32_t variable_id);
static bool scry_ir_builder_condition_is_valid(const scry_ir_program_builder* builder, const scry_ir_condition* condition);
static bool scry_ir_builder_append_node(scry_ir_program_builder* builder, scry_ir_node node, uint32_t* out_node_index);
static bool scry_ir_builder_choice_node_can_append(const scry_ir_program_builder* builder, uint32_t node_index);
static bool scry_ir_builder_switch_node_can_append(const scry_ir_program_builder* builder, uint32_t node_index);

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

void scry_ir_program_destroy(scry_ir_program* program)
{
	ASSERT_FATAL(program);

	union
	{
		const scry_ir_node* view;
		scry_ir_node*		owned;
	} nodes = { 0 };

	union
	{
		const scry_ir_choice_option* view;
		scry_ir_choice_option*		 owned;
	} choice_options = { 0 };

	union
	{
		const scry_ir_switch_case* view;
		scry_ir_switch_case*	   owned;
	} switch_cases = { 0 };

	nodes.view			= program->nodes;
	choice_options.view = program->choice_options;
	switch_cases.view	= program->switch_cases;

	free(nodes.owned);
	free(choice_options.owned);
	free(switch_cases.owned);

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
	node.kind		  = SCRY_IR_NODE_KIND_END;

	return node;
}

void scry_ir_program_builder_init(scry_ir_program_builder* builder, uint32_t string_count, uint32_t variable_count)
{
	ASSERT_FATAL(builder);

	memset(builder, 0, sizeof(*builder));

	builder->string_count	= string_count;
	builder->variable_count = variable_count;
}

void scry_ir_program_builder_destroy(scry_ir_program_builder* builder)
{
	ASSERT_FATAL(builder);

	free(builder->nodes);
	free(builder->choice_options);
	free(builder->switch_cases);

	memset(builder, 0, sizeof(*builder));
}

bool scry_ir_program_builder_set_entry_node(scry_ir_program_builder* builder, uint32_t entry_node_index)
{
	ASSERT_FATAL(builder);

	builder->entry_node_index = entry_node_index;
	builder->has_entry_node	  = true;

	return true;
}

bool scry_ir_program_builder_add_dialogue_node(scry_ir_program_builder* builder, uint32_t string_id, uint32_t* out_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_node_index);
	ASSERT_FATAL(scry_ir_builder_string_id_is_valid(builder, string_id));

	return scry_ir_builder_append_node(builder, scry_ir_make_dialogue_node(string_id, SCRY_IR_INVALID_INDEX), out_node_index);
}

bool scry_ir_program_builder_add_jump_node(scry_ir_program_builder* builder, uint32_t* out_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_node_index);

	return scry_ir_builder_append_node(builder, scry_ir_make_jump_node(SCRY_IR_INVALID_INDEX), out_node_index);
}

bool scry_ir_program_builder_add_branch_node(scry_ir_program_builder* builder, scry_ir_condition condition, uint32_t* out_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_node_index);
	ASSERT_FATAL(scry_ir_builder_condition_is_valid(builder, &condition));

	return scry_ir_builder_append_node(builder, scry_ir_make_branch_node(condition, SCRY_IR_INVALID_INDEX, SCRY_IR_INVALID_INDEX), out_node_index);
}

bool scry_ir_program_builder_add_choice_node(scry_ir_program_builder* builder, uint32_t* out_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_node_index);
	ASSERT_FATAL(builder->choice_option_count < UINT32_MAX);

	return scry_ir_builder_append_node(builder, scry_ir_make_choice_node(builder->choice_option_count, 0U), out_node_index);
}

bool scry_ir_program_builder_add_switch_node(scry_ir_program_builder* builder, uint32_t variable_id, uint32_t* out_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_node_index);
	ASSERT_FATAL(scry_ir_builder_variable_id_is_valid(builder, variable_id));
	ASSERT_FATAL(builder->switch_case_count < UINT32_MAX);

	return scry_ir_builder_append_node(builder, scry_ir_make_switch_node(variable_id, builder->switch_case_count, 0U, SCRY_IR_INVALID_INDEX), out_node_index);
}

bool scry_ir_program_builder_add_end_node(scry_ir_program_builder* builder, uint32_t* out_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_node_index);

	return scry_ir_builder_append_node(builder, scry_ir_make_end_node(), out_node_index);
}

bool scry_ir_program_builder_link(scry_ir_program_builder* builder, uint32_t node_index, uint32_t target_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, node_index));
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, target_node_index));

	scry_ir_node* node = &builder->nodes[node_index];

	ASSERT_FATAL((node->kind == SCRY_IR_NODE_KIND_DIALOGUE) || (node->kind == SCRY_IR_NODE_KIND_JUMP));

	if (node->kind == SCRY_IR_NODE_KIND_DIALOGUE)
	{
		node->data.dialogue.next_node_index = target_node_index;
	}

	else
	{
		node->data.jump.target_node_index = target_node_index;
	}

	return true;
}

bool scry_ir_program_builder_link_branch_true(scry_ir_program_builder* builder, uint32_t node_index, uint32_t target_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, node_index));
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, target_node_index));
	ASSERT_FATAL(builder->nodes[node_index].kind == SCRY_IR_NODE_KIND_BRANCH);

	builder->nodes[node_index].data.branch.true_node_index = target_node_index;
	return true;
}

bool scry_ir_program_builder_link_branch_false(scry_ir_program_builder* builder, uint32_t node_index, uint32_t target_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, node_index));
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, target_node_index));
	ASSERT_FATAL(builder->nodes[node_index].kind == SCRY_IR_NODE_KIND_BRANCH);

	builder->nodes[node_index].data.branch.false_node_index = target_node_index;
	return true;
}

bool scry_ir_program_builder_set_switch_default(scry_ir_program_builder* builder, uint32_t node_index, uint32_t target_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, node_index));
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, target_node_index));
	ASSERT_FATAL(builder->nodes[node_index].kind == SCRY_IR_NODE_KIND_SWITCH);

	builder->nodes[node_index].data.switch_branch.default_node_index = target_node_index;
	return true;
}

bool scry_ir_program_builder_append_choice_option(scry_ir_program_builder* builder, uint32_t node_index, uint32_t text_string_id, uint32_t* out_option_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_option_index);
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, node_index));
	ASSERT_FATAL(builder->nodes[node_index].kind == SCRY_IR_NODE_KIND_CHOICE);
	ASSERT_FATAL(scry_ir_builder_choice_node_can_append(builder, node_index));
	ASSERT_FATAL(scry_ir_builder_string_id_is_valid(builder, text_string_id));
	ASSERT_FATAL(builder->choice_option_count < UINT32_MAX);

	if (!SCRY_STORAGE_RESERVE(builder->choice_options, builder->choice_option_capacity, builder->choice_option_count + 1U, 8U))
	{
		return false;
	}

	*out_option_index			  = builder->choice_option_count;
	scry_ir_choice_option* option = &builder->choice_options[*out_option_index];

	option->text_string_id	  = text_string_id;
	option->target_node_index = SCRY_IR_INVALID_INDEX;

	builder->choice_option_count += 1U;
	builder->nodes[node_index].data.choice.option_count += 1U;

	return true;
}

bool scry_ir_program_builder_link_choice_option(scry_ir_program_builder* builder, uint32_t option_index, uint32_t target_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(scry_ir_builder_option_index_is_valid(builder, option_index));
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, target_node_index));

	builder->choice_options[option_index].target_node_index = target_node_index;
	return true;
}

bool scry_ir_program_builder_append_switch_case(scry_ir_program_builder* builder, uint32_t node_index, int32_t value, uint32_t* out_case_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_case_index);
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, node_index));
	ASSERT_FATAL(builder->nodes[node_index].kind == SCRY_IR_NODE_KIND_SWITCH);
	ASSERT_FATAL(scry_ir_builder_switch_node_can_append(builder, node_index));
	ASSERT_FATAL(builder->switch_case_count < UINT32_MAX);

	const scry_ir_node* node = &builder->nodes[node_index];

	if (node->data.switch_branch.case_count > 0U)
	{
		const uint32_t last_case_index = node->data.switch_branch.case_start + node->data.switch_branch.case_count - 1U;

		ASSERT_FATAL(builder->switch_cases[last_case_index].value < value);
	}

	if (!SCRY_STORAGE_RESERVE(builder->switch_cases, builder->switch_case_capacity, builder->switch_case_count + 1U, 8U))
	{
		return false;
	}

	*out_case_index					 = builder->switch_case_count;
	scry_ir_switch_case* switch_case = &builder->switch_cases[*out_case_index];

	switch_case->value			   = value;
	switch_case->target_node_index = SCRY_IR_INVALID_INDEX;

	builder->switch_case_count += 1U;
	builder->nodes[node_index].data.switch_branch.case_count += 1U;

	return true;
}

bool scry_ir_program_builder_link_switch_case(scry_ir_program_builder* builder, uint32_t case_index, uint32_t target_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(scry_ir_builder_case_index_is_valid(builder, case_index));
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, target_node_index));

	builder->switch_cases[case_index].target_node_index = target_node_index;
	return true;
}

bool scry_ir_program_builder_build(scry_ir_program_builder* builder, scry_ir_program* out_program)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_program);

	if (!builder->has_entry_node)
	{
		return false;
	}

	scry_ir_program program = { 0 };

	program.nodes				= builder->nodes;
	program.choice_options		= builder->choice_options;
	program.switch_cases		= builder->switch_cases;
	program.node_count			= builder->node_count;
	program.choice_option_count = builder->choice_option_count;
	program.switch_case_count	= builder->switch_case_count;
	program.string_count		= builder->string_count;
	program.variable_count		= builder->variable_count;
	program.entry_node_index	= builder->entry_node_index;

	if (!scry_ir_program_validate(&program))
	{
		return false;
	}

	*out_program = program;
	memset(builder, 0, sizeof(*builder));
	return true;
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
					const uint32_t			   previous_case_index	= case_index - 1U;
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

static bool scry_ir_builder_node_index_is_valid(const scry_ir_program_builder* builder, uint32_t node_index)
{
	ASSERT_FATAL(builder);

	return node_index < builder->node_count;
}

static bool scry_ir_builder_option_index_is_valid(const scry_ir_program_builder* builder, uint32_t option_index)
{
	ASSERT_FATAL(builder);

	return option_index < builder->choice_option_count;
}

static bool scry_ir_builder_case_index_is_valid(const scry_ir_program_builder* builder, uint32_t case_index)
{
	ASSERT_FATAL(builder);

	return case_index < builder->switch_case_count;
}

static bool scry_ir_builder_string_id_is_valid(const scry_ir_program_builder* builder, uint32_t string_id)
{
	ASSERT_FATAL(builder);

	return string_id < builder->string_count;
}

static bool scry_ir_builder_variable_id_is_valid(const scry_ir_program_builder* builder, uint32_t variable_id)
{
	ASSERT_FATAL(builder);

	return variable_id < builder->variable_count;
}

static bool scry_ir_builder_condition_is_valid(const scry_ir_program_builder* builder, const scry_ir_condition* condition)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(condition);

	return scry_ir_compare_op_is_valid(condition->compare_op) && scry_ir_builder_variable_id_is_valid(builder, condition->variable_id);
}

static bool scry_ir_builder_append_node(scry_ir_program_builder* builder, scry_ir_node node, uint32_t* out_node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(out_node_index);
	ASSERT_FATAL(builder->node_count < UINT32_MAX);

	if (!SCRY_STORAGE_RESERVE(builder->nodes, builder->node_capacity, builder->node_count + 1U, 8U))
	{
		return false;
	}

	*out_node_index					= builder->node_count;
	builder->nodes[*out_node_index] = node;
	builder->node_count += 1U;
	return true;
}

static bool scry_ir_builder_choice_node_can_append(const scry_ir_program_builder* builder, uint32_t node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, node_index));

	const scry_ir_node* node = &builder->nodes[node_index];
	ASSERT_FATAL(node->kind == SCRY_IR_NODE_KIND_CHOICE);

	return node->data.choice.option_start + node->data.choice.option_count == builder->choice_option_count;
}

static bool scry_ir_builder_switch_node_can_append(const scry_ir_program_builder* builder, uint32_t node_index)
{
	ASSERT_FATAL(builder);
	ASSERT_FATAL(scry_ir_builder_node_index_is_valid(builder, node_index));

	const scry_ir_node* node = &builder->nodes[node_index];
	ASSERT_FATAL(node->kind == SCRY_IR_NODE_KIND_SWITCH);

	return node->data.switch_branch.case_start + node->data.switch_branch.case_count == builder->switch_case_count;
}
