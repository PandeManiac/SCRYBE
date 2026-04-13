#include "vm/scry_ir.h"
#include "vm/scry_sdlg_compiler.h"
#include "vm/scry_sdlg_disassembler.h"

#include <stdbool.h>
#include <stdio.h>

static bool scry_build_disassembler_sample(scry_ir_program* out_program);
static bool scry_run_disassembler_sample(void);

int main(void)
{
	return scry_run_disassembler_sample() ? 0 : 1;
}

static bool scry_build_disassembler_sample(scry_ir_program* out_program)
{
	scry_ir_program_builder builder = { 0 };
	uint32_t				intro_node = 0U;
	uint32_t				branch_node = 0U;
	uint32_t				choice_node = 0U;
	uint32_t				switch_node = 0U;
	uint32_t				accepted_node = 0U;
	uint32_t				declined_node = 0U;
	uint32_t				end_node = 0U;
	uint32_t				option_index = 0U;
	uint32_t				case_index = 0U;
	const scry_ir_condition condition = {
		.variable_id = 0U,
		.compare_op	 = SCRY_IR_COMPARE_OP_GREATER_EQUAL,
		.value		 = 10,
	};
	bool ok = false;

	scry_ir_program_builder_init(&builder, 5U, 2U);

	ok = scry_ir_program_builder_add_dialogue_node(&builder, 0U, &intro_node) &&
		 scry_ir_program_builder_add_branch_node(&builder, condition, &branch_node) &&
		 scry_ir_program_builder_add_choice_node(&builder, &choice_node) &&
		 scry_ir_program_builder_add_switch_node(&builder, 1U, &switch_node) &&
		 scry_ir_program_builder_add_dialogue_node(&builder, 3U, &accepted_node) &&
		 scry_ir_program_builder_add_dialogue_node(&builder, 4U, &declined_node) &&
		 scry_ir_program_builder_add_end_node(&builder, &end_node) && scry_ir_program_builder_set_entry_node(&builder, intro_node) &&
		 scry_ir_program_builder_link(&builder, intro_node, branch_node) &&
		 scry_ir_program_builder_link_branch_true(&builder, branch_node, choice_node) &&
		 scry_ir_program_builder_link_branch_false(&builder, branch_node, switch_node) &&
		 scry_ir_program_builder_append_choice_option(&builder, choice_node, 1U, &option_index) &&
		 scry_ir_program_builder_link_choice_option(&builder, option_index, accepted_node) &&
		 scry_ir_program_builder_append_choice_option(&builder, choice_node, 2U, &option_index) &&
		 scry_ir_program_builder_link_choice_option(&builder, option_index, declined_node) &&
		 scry_ir_program_builder_append_switch_case(&builder, switch_node, 1, &case_index) &&
		 scry_ir_program_builder_link_switch_case(&builder, case_index, accepted_node) &&
		 scry_ir_program_builder_append_switch_case(&builder, switch_node, 2, &case_index) &&
		 scry_ir_program_builder_link_switch_case(&builder, case_index, declined_node) &&
		 scry_ir_program_builder_set_switch_default(&builder, switch_node, end_node) &&
		 scry_ir_program_builder_link(&builder, accepted_node, end_node) &&
		 scry_ir_program_builder_link(&builder, declined_node, end_node) && scry_ir_program_builder_build(&builder, out_program);

	scry_ir_program_builder_destroy(&builder);
	return ok;
}

static bool scry_run_disassembler_sample(void)
{
	static const scry_sdlg_string strings[] = {
		{ "Intro", sizeof("Intro") - 1U },
		{ "Accept", sizeof("Accept") - 1U },
		{ "Decline", sizeof("Decline") - 1U },
		{ "Accepted.", sizeof("Accepted.") - 1U },
		{ "Declined.", sizeof("Declined.") - 1U },
	};

	scry_ir_program	 program = { 0 };
	scry_sdlg_binary binary	 = { 0 };
	scry_sdlg_view	 view	 = { 0 };
	bool			 ok		 = false;

	ok = scry_build_disassembler_sample(&program) && scry_sdlg_compile_ir(&program, strings, &binary) &&
		 scry_sdlg_view_init(&view, binary.data, binary.size) && scry_sdlg_disassemble(&view, stdout);

	scry_sdlg_binary_destroy(&binary);
	scry_ir_program_destroy(&program);

	return ok;
}
