#include "vm/scry_sdlg_compiler.h"

#include "utils/io/scry_file.h"
#include "vm/scry_sdlg.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct scry_sdlg_compile_layout
{
	size_t*	  block_offsets;
	size_t*	  block_sizes;
	size_t*	  primary_jump_operand_sizes;
	size_t*	  secondary_jump_operand_sizes;
	uint32_t* node_table_indices;
	size_t	  instruction_size;
	size_t	  table_payload_size;
	size_t	  string_data_size;
	size_t	  entry_jump_operand_size;
	uint32_t  instruction_count;
	uint32_t  table_count;
} scry_sdlg_compile_layout;

static bool scry_sdlg_compile_layout_init(const scry_ir_program* program, scry_sdlg_compile_layout* layout);
static void scry_sdlg_compile_layout_destroy(scry_sdlg_compile_layout* layout);
static bool scry_sdlg_compile_compute_layout(const scry_ir_program* program, const scry_sdlg_string* strings, scry_sdlg_compile_layout* layout);

static size_t scry_sdlg_compile_block_size(const scry_ir_node* node,
										   uint32_t			   table_index,
										   size_t			   primary_jump_operand_size,
										   size_t			   secondary_jump_operand_size);

static bool scry_sdlg_compile_write_instructions(uint8_t*						 buffer,
												 size_t							 buffer_size,
												 size_t							 instruction_offset,
												 const scry_ir_program*			 program,
												 const scry_sdlg_compile_layout* layout);

static bool scry_sdlg_compile_write_tables(uint8_t*						   buffer,
										   size_t						   buffer_size,
										   size_t						   table_offset,
										   const scry_ir_program*		   program,
										   const scry_sdlg_compile_layout* layout);

static bool scry_sdlg_compile_write_strings(uint8_t*				buffer,
											size_t					buffer_size,
											size_t					string_index_offset,
											size_t					string_data_offset,
											const scry_ir_program*	program,
											const scry_sdlg_string* strings);

static bool	   scry_sdlg_compile_write_u16_le(uint8_t* buffer, size_t buffer_size, size_t* offset, uint16_t value);
static bool	   scry_sdlg_compile_write_u32_le(uint8_t* buffer, size_t buffer_size, size_t* offset, uint32_t value);
static bool	   scry_sdlg_compile_write_s32_le(uint8_t* buffer, size_t buffer_size, size_t* offset, int32_t value);
static int64_t scry_sdlg_compile_relative_delta(size_t target_offset, size_t base_offset);

void scry_sdlg_binary_destroy(scry_sdlg_binary* binary)
{
	ASSERT_FATAL(binary);

	free(binary->data);
	memset(binary, 0, sizeof(*binary));
}

bool scry_sdlg_compile_ir(const scry_ir_program* program, const scry_sdlg_string* strings, scry_sdlg_binary* out_binary)
{
	ASSERT_FATAL(program);
	ASSERT_FATAL(strings || program->string_count == 0U);
	ASSERT_FATAL(out_binary);
	ASSERT_FATAL(program->nodes);

	scry_sdlg_compile_layout layout				 = { 0 };
	scry_sdlg_header		 header				 = { 0 };
	uint8_t*				 buffer				 = NULL;
	size_t					 buffer_size		 = 0U;
	size_t					 instruction_offset	 = 0U;
	size_t					 table_offset		 = 0U;
	size_t					 string_index_offset = 0U;
	size_t					 string_data_offset	 = 0U;

	memset(out_binary, 0, sizeof(*out_binary));

	for (uint32_t i = 0U; i < program->string_count; ++i)
	{
		ASSERT_FATAL(strings[i].data || strings[i].length == 0U);
	}

	if (!scry_sdlg_compile_layout_init(program, &layout))
	{
		return false;
	}

	if (!scry_sdlg_compile_compute_layout(program, strings, &layout))
	{
		scry_sdlg_compile_layout_destroy(&layout);
		return false;
	}

	instruction_offset	= (size_t)SCRY_SDLG_HEADER_SIZE;
	table_offset		= instruction_offset + layout.instruction_size;
	string_index_offset = table_offset + ((size_t)layout.table_count * sizeof(scry_sdlg_table_entry)) + layout.table_payload_size;
	string_data_offset	= string_index_offset + ((size_t)program->string_count * sizeof(scry_sdlg_string_index_entry));
	buffer_size			= string_data_offset + layout.string_data_size;

	if (buffer_size > (size_t)UINT32_MAX)
	{
		scry_sdlg_compile_layout_destroy(&layout);
		return false;
	}

	buffer = (uint8_t*)calloc(buffer_size, sizeof(*buffer));

	if (buffer == NULL)
	{
		scry_sdlg_compile_layout_destroy(&layout);
		return false;
	}

	ASSERT_FATAL(scry_sdlg_header_init(&header));

	header.instruction_count   = layout.instruction_count;
	header.instruction_offset  = (uint32_t)instruction_offset;
	header.instruction_size	   = (uint32_t)layout.instruction_size;
	header.table_count		   = layout.table_count;
	header.table_offset		   = (uint32_t)table_offset;
	header.table_size		   = (uint32_t)(((size_t)layout.table_count * sizeof(scry_sdlg_table_entry)) + layout.table_payload_size);
	header.string_count		   = program->string_count;
	header.string_index_offset = (uint32_t)string_index_offset;
	header.string_data_offset  = (uint32_t)string_data_offset;
	header.string_data_size	   = (uint32_t)layout.string_data_size;

	ASSERT_FATAL(scry_sdlg_header_validate(&header, buffer_size));

	if (!scry_sdlg_write_header(buffer, buffer_size, &header) ||
		!scry_sdlg_compile_write_instructions(buffer, buffer_size, instruction_offset, program, &layout) ||
		!scry_sdlg_compile_write_tables(buffer, buffer_size, table_offset, program, &layout) ||
		!scry_sdlg_compile_write_strings(buffer, buffer_size, string_index_offset, string_data_offset, program, strings))
	{
		free(buffer);
		scry_sdlg_compile_layout_destroy(&layout);
		return false;
	}

	out_binary->data = buffer;
	out_binary->size = buffer_size;

	scry_sdlg_compile_layout_destroy(&layout);
	return true;
}

bool scry_sdlg_write_ir_file(const char* path, const scry_ir_program* program, const scry_sdlg_string* strings)
{
	ASSERT_FATAL(path);

	scry_sdlg_binary binary = { 0 };

	if (!scry_sdlg_compile_ir(program, strings, &binary))
	{
		return false;
	}

	const bool ok = scry_file_write_bytes(path, binary.data, binary.size);
	scry_sdlg_binary_destroy(&binary);

	return ok;
}

static bool scry_sdlg_compile_layout_init(const scry_ir_program* program, scry_sdlg_compile_layout* layout)
{
	ASSERT_FATAL(program);
	ASSERT_FATAL(layout);

	memset(layout, 0, sizeof(*layout));

	layout->block_offsets = (size_t*)calloc(program->node_count, sizeof(*layout->block_offsets));

	if (layout->block_offsets == NULL)
	{
		return false;
	}

	layout->block_sizes = (size_t*)calloc(program->node_count, sizeof(*layout->block_sizes));

	if (layout->block_sizes == NULL)
	{
		scry_sdlg_compile_layout_destroy(layout);
		return false;
	}

	layout->primary_jump_operand_sizes = (size_t*)calloc(program->node_count, sizeof(*layout->primary_jump_operand_sizes));

	if (layout->primary_jump_operand_sizes == NULL)
	{
		scry_sdlg_compile_layout_destroy(layout);
		return false;
	}

	layout->secondary_jump_operand_sizes = (size_t*)calloc(program->node_count, sizeof(*layout->secondary_jump_operand_sizes));

	if (layout->secondary_jump_operand_sizes == NULL)
	{
		scry_sdlg_compile_layout_destroy(layout);
		return false;
	}

	layout->node_table_indices = (uint32_t*)malloc((size_t)program->node_count * sizeof(*layout->node_table_indices));

	if (layout->node_table_indices == NULL)
	{
		scry_sdlg_compile_layout_destroy(layout);
		return false;
	}

	for (uint32_t i = 0U; i < program->node_count; ++i)
	{
		layout->primary_jump_operand_sizes[i]	= 1U;
		layout->secondary_jump_operand_sizes[i] = 1U;
		layout->node_table_indices[i]			= UINT32_MAX;
	}

	layout->entry_jump_operand_size = 1U;
	return true;
}

static void scry_sdlg_compile_layout_destroy(scry_sdlg_compile_layout* layout)
{
	ASSERT_FATAL(layout);

	free(layout->block_offsets);
	free(layout->block_sizes);
	free(layout->primary_jump_operand_sizes);
	free(layout->secondary_jump_operand_sizes);
	free(layout->node_table_indices);

	memset(layout, 0, sizeof(*layout));
}

static bool scry_sdlg_compile_compute_layout(const scry_ir_program* program, const scry_sdlg_string* strings, scry_sdlg_compile_layout* layout)
{
	ASSERT_FATAL(program);
	ASSERT_FATAL(strings || program->string_count == 0U);
	ASSERT_FATAL(layout);

	bool changed = false;

	for (uint32_t i = 0U; i < program->node_count; ++i)
	{
		const scry_ir_node* node = &program->nodes[i];

		if (node->kind == SCRY_IR_NODE_KIND_CHOICE || node->kind == SCRY_IR_NODE_KIND_SWITCH)
		{
			layout->node_table_indices[i] = layout->table_count;
			layout->table_count += 1U;
		}

		if (node->kind == SCRY_IR_NODE_KIND_CHOICE)
		{
			layout->table_payload_size +=
				sizeof(scry_sdlg_choice_table_header) + ((size_t)node->data.choice.option_count * (sizeof(uint32_t) + sizeof(int32_t)));
		}

		else if (node->kind == SCRY_IR_NODE_KIND_SWITCH)
		{
			layout->table_payload_size +=
				sizeof(scry_sdlg_switch_table_header) + ((size_t)node->data.switch_branch.case_count * (sizeof(int32_t) + sizeof(int32_t)));
		}

		layout->block_sizes[i] =
			scry_sdlg_compile_block_size(node, layout->node_table_indices[i], layout->primary_jump_operand_sizes[i], layout->secondary_jump_operand_sizes[i]);
	}

	for (uint32_t i = 0U; i < program->string_count; ++i)
	{
		layout->string_data_size += scry_sdlg_uvarint_size(strings[i].length) + strings[i].length;
	}

	for (uint32_t iteration = 0U; iteration < 16U; ++iteration)
	{
		size_t offset = 1U + layout->entry_jump_operand_size;

		changed = false;

		for (uint32_t i = 0U; i < program->node_count; ++i)
		{
			layout->block_offsets[i] = offset;

			layout->block_sizes[i] = scry_sdlg_compile_block_size(&program->nodes[i],
																  layout->node_table_indices[i],
																  layout->primary_jump_operand_sizes[i],
																  layout->secondary_jump_operand_sizes[i]);

			offset += layout->block_sizes[i];
		}

		{
			const size_t  entry_base = 1U + layout->entry_jump_operand_size;
			const int64_t delta		 = scry_sdlg_compile_relative_delta(layout->block_offsets[program->entry_node_index], entry_base);
			const size_t  new_size	 = scry_sdlg_svarint_size(delta);

			if (new_size != layout->entry_jump_operand_size)
			{
				layout->entry_jump_operand_size = new_size;
				changed							= true;
			}
		}

		for (uint32_t i = 0U; i < program->node_count; ++i)
		{
			const scry_ir_node* node		 = &program->nodes[i];
			const size_t		block_offset = layout->block_offsets[i];

			switch (node->kind)
			{
				case SCRY_IR_NODE_KIND_DIALOGUE:
				{
					const size_t  dialogue_size = 1U + scry_sdlg_uvarint_size(node->data.dialogue.string_id);
					const size_t  jump_base		= block_offset + dialogue_size + 1U + layout->primary_jump_operand_sizes[i];
					const int64_t delta			= scry_sdlg_compile_relative_delta(layout->block_offsets[node->data.dialogue.next_node_index], jump_base);
					const size_t  new_size		= scry_sdlg_svarint_size(delta);

					if (new_size != layout->primary_jump_operand_sizes[i])
					{
						layout->primary_jump_operand_sizes[i] = new_size;
						changed								  = true;
					}
				}
				break;

				case SCRY_IR_NODE_KIND_JUMP:
				{
					const size_t  jump_base = block_offset + 1U + layout->primary_jump_operand_sizes[i];
					const int64_t delta		= scry_sdlg_compile_relative_delta(layout->block_offsets[node->data.jump.target_node_index], jump_base);
					const size_t  new_size	= scry_sdlg_svarint_size(delta);

					if (new_size != layout->primary_jump_operand_sizes[i])
					{
						layout->primary_jump_operand_sizes[i] = new_size;
						changed								  = true;
					}
				}
				break;

				case SCRY_IR_NODE_KIND_BRANCH:
				{
					const size_t compare_size = 1U + scry_sdlg_uvarint_size(node->data.branch.condition.variable_id) +
												scry_sdlg_uvarint_size((uint64_t)node->data.branch.condition.compare_op) +
												scry_sdlg_svarint_size(node->data.branch.condition.value);

					const size_t  false_base	 = block_offset + compare_size + 1U + layout->primary_jump_operand_sizes[i];
					const size_t  true_base		 = false_base + 1U + layout->secondary_jump_operand_sizes[i];
					const int64_t false_delta	 = scry_sdlg_compile_relative_delta(layout->block_offsets[node->data.branch.false_node_index], false_base);
					const int64_t true_delta	 = scry_sdlg_compile_relative_delta(layout->block_offsets[node->data.branch.true_node_index], true_base);
					const size_t  new_false_size = scry_sdlg_svarint_size(false_delta);
					const size_t  new_true_size	 = scry_sdlg_svarint_size(true_delta);

					if (new_false_size != layout->primary_jump_operand_sizes[i])
					{
						layout->primary_jump_operand_sizes[i] = new_false_size;
						changed								  = true;
					}

					if (new_true_size != layout->secondary_jump_operand_sizes[i])
					{
						layout->secondary_jump_operand_sizes[i] = new_true_size;
						changed									= true;
					}
				}
				break;

				case SCRY_IR_NODE_KIND_CHOICE:
				case SCRY_IR_NODE_KIND_SWITCH:
				case SCRY_IR_NODE_KIND_END:
					break;

				case SCRY_IR_NODE_KIND_NONE:
				default:
					ASSERT_FATAL(false);
			}
		}

		if (!changed)
		{
			break;
		}
	}

	if (changed)
	{
		return false;
	}

	layout->instruction_size  = 1U + layout->entry_jump_operand_size;
	layout->instruction_count = 1U;

	for (uint32_t i = 0U; i < program->node_count; ++i)
	{
		const scry_ir_node* node = &program->nodes[i];

		layout->block_offsets[i] = layout->instruction_size;

		layout->block_sizes[i] =
			scry_sdlg_compile_block_size(node, layout->node_table_indices[i], layout->primary_jump_operand_sizes[i], layout->secondary_jump_operand_sizes[i]);

		layout->instruction_size += layout->block_sizes[i];

		switch (node->kind)
		{
			case SCRY_IR_NODE_KIND_DIALOGUE:
				layout->instruction_count += 2U;
				break;

			case SCRY_IR_NODE_KIND_BRANCH:
				layout->instruction_count += 3U;
				break;

			case SCRY_IR_NODE_KIND_JUMP:
			case SCRY_IR_NODE_KIND_CHOICE:
			case SCRY_IR_NODE_KIND_SWITCH:
			case SCRY_IR_NODE_KIND_END:
				layout->instruction_count += 1U;
				break;

			case SCRY_IR_NODE_KIND_NONE:
			default:
				ASSERT_FATAL(false);
		}
	}

	return true;
}

static size_t scry_sdlg_compile_block_size(const scry_ir_node* node, uint32_t table_index, size_t primary_jump_operand_size, size_t secondary_jump_operand_size)
{
	ASSERT_FATAL(node);

	switch (node->kind)
	{
		case SCRY_IR_NODE_KIND_DIALOGUE:
			return 1U + scry_sdlg_uvarint_size(node->data.dialogue.string_id) + 1U + primary_jump_operand_size;

		case SCRY_IR_NODE_KIND_JUMP:
			return 1U + primary_jump_operand_size;

		case SCRY_IR_NODE_KIND_BRANCH:
			return 1U + scry_sdlg_uvarint_size(node->data.branch.condition.variable_id) +
				   scry_sdlg_uvarint_size((uint64_t)node->data.branch.condition.compare_op) + scry_sdlg_svarint_size(node->data.branch.condition.value) + 1U +
				   primary_jump_operand_size + 1U + secondary_jump_operand_size;

		case SCRY_IR_NODE_KIND_CHOICE:
			ASSERT_FATAL(table_index != UINT32_MAX);
			return 1U + scry_sdlg_uvarint_size(table_index);

		case SCRY_IR_NODE_KIND_SWITCH:
			ASSERT_FATAL(table_index != UINT32_MAX);
			return 1U + scry_sdlg_uvarint_size(table_index);

		case SCRY_IR_NODE_KIND_END:
			return 1U;

		case SCRY_IR_NODE_KIND_NONE:
		default:
			ASSERT_FATAL(false);
			return 0U;
	}
}

static bool scry_sdlg_compile_write_instructions(uint8_t*						 buffer,
												 size_t							 buffer_size,
												 size_t							 instruction_offset,
												 const scry_ir_program*			 program,
												 const scry_sdlg_compile_layout* layout)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(program);
	ASSERT_FATAL(layout);

	size_t absolute_offset = instruction_offset;
	size_t cursor		   = 0U;

	buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_JUMP;
	cursor += 1U;

	if (!scry_sdlg_write_svarint(buffer,
								 buffer_size,
								 &absolute_offset,
								 scry_sdlg_compile_relative_delta(layout->block_offsets[program->entry_node_index], cursor + layout->entry_jump_operand_size)))
	{
		return false;
	}

	cursor = absolute_offset - instruction_offset;
	ASSERT_FATAL(cursor == 1U + layout->entry_jump_operand_size);

	for (uint32_t i = 0U; i < program->node_count; ++i)
	{
		const scry_ir_node* node = &program->nodes[i];

		ASSERT_FATAL(cursor == layout->block_offsets[i]);

		switch (node->kind)
		{
			case SCRY_IR_NODE_KIND_DIALOGUE:
			{
				buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_DIALOGUE;
				cursor += 1U;

				if (!scry_sdlg_write_uvarint(buffer, buffer_size, &absolute_offset, node->data.dialogue.string_id))
				{
					return false;
				}

				cursor					  = absolute_offset - instruction_offset;
				buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_JUMP;
				cursor += 1U;

				if (!scry_sdlg_write_svarint(buffer,
											 buffer_size,
											 &absolute_offset,
											 scry_sdlg_compile_relative_delta(layout->block_offsets[node->data.dialogue.next_node_index],
																			  cursor + layout->primary_jump_operand_sizes[i])))
				{
					return false;
				}

				cursor = absolute_offset - instruction_offset;
			}
			break;

			case SCRY_IR_NODE_KIND_JUMP:
				buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_JUMP;
				cursor += 1U;

				if (!scry_sdlg_write_svarint(buffer,
											 buffer_size,
											 &absolute_offset,
											 scry_sdlg_compile_relative_delta(layout->block_offsets[node->data.jump.target_node_index],
																			  cursor + layout->primary_jump_operand_sizes[i])))
				{
					return false;
				}
				cursor = absolute_offset - instruction_offset;
				break;

			case SCRY_IR_NODE_KIND_BRANCH:
				buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_COMPARE;
				cursor += 1U;

				if (!scry_sdlg_write_uvarint(buffer, buffer_size, &absolute_offset, node->data.branch.condition.variable_id) ||
					!scry_sdlg_write_uvarint(buffer, buffer_size, &absolute_offset, (uint64_t)node->data.branch.condition.compare_op) ||
					!scry_sdlg_write_svarint(buffer, buffer_size, &absolute_offset, node->data.branch.condition.value))
				{
					return false;
				}

				cursor					  = absolute_offset - instruction_offset;
				buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_JUMP_IF_FALSE;
				cursor += 1U;

				if (!scry_sdlg_write_svarint(buffer,
											 buffer_size,
											 &absolute_offset,
											 scry_sdlg_compile_relative_delta(layout->block_offsets[node->data.branch.false_node_index],
																			  cursor + layout->primary_jump_operand_sizes[i])))
				{
					return false;
				}

				cursor					  = absolute_offset - instruction_offset;
				buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_JUMP;
				cursor += 1U;

				if (!scry_sdlg_write_svarint(buffer,
											 buffer_size,
											 &absolute_offset,
											 scry_sdlg_compile_relative_delta(layout->block_offsets[node->data.branch.true_node_index],
																			  cursor + layout->secondary_jump_operand_sizes[i])))
				{
					return false;
				}
				cursor = absolute_offset - instruction_offset;
				break;

			case SCRY_IR_NODE_KIND_CHOICE:
				ASSERT_FATAL(layout->node_table_indices[i] != UINT32_MAX);
				buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_CHOICE;
				cursor += 1U;

				if (!scry_sdlg_write_uvarint(buffer, buffer_size, &absolute_offset, layout->node_table_indices[i]))
				{
					return false;
				}

				cursor = absolute_offset - instruction_offset;
				break;

			case SCRY_IR_NODE_KIND_SWITCH:
				ASSERT_FATAL(layout->node_table_indices[i] != UINT32_MAX);
				buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_SWITCH;
				cursor += 1U;

				if (!scry_sdlg_write_uvarint(buffer, buffer_size, &absolute_offset, layout->node_table_indices[i]))
				{
					return false;
				}

				cursor = absolute_offset - instruction_offset;
				break;

			case SCRY_IR_NODE_KIND_END:
				buffer[absolute_offset++] = (uint8_t)SCRY_SDLG_OP_END;
				cursor += 1U;
				break;

			case SCRY_IR_NODE_KIND_NONE:
			default:
				ASSERT_FATAL(false);
		}
	}

	return true;
}

static bool scry_sdlg_compile_write_tables(uint8_t*						   buffer,
										   size_t						   buffer_size,
										   size_t						   table_offset,
										   const scry_ir_program*		   program,
										   const scry_sdlg_compile_layout* layout)
{
	size_t	 directory_offset = table_offset;
	size_t	 payload_offset	  = table_offset + ((size_t)layout->table_count * sizeof(scry_sdlg_table_entry));
	uint32_t table_index	  = 0U;

	ASSERT_FATAL(buffer);
	ASSERT_FATAL(program);
	ASSERT_FATAL(layout);

	for (uint32_t i = 0U; i < program->node_count; ++i)
	{
		const scry_ir_node* node		  = &program->nodes[i];
		size_t				payload_start = payload_offset;

		if (node->kind != SCRY_IR_NODE_KIND_CHOICE && node->kind != SCRY_IR_NODE_KIND_SWITCH)
		{
			continue;
		}

		ASSERT_FATAL(table_index == layout->node_table_indices[i]);

		if (node->kind == SCRY_IR_NODE_KIND_CHOICE)
		{
			if (!scry_sdlg_compile_write_u16_le(buffer, buffer_size, &payload_offset, (uint16_t)node->data.choice.option_count))
			{
				return false;
			}

			for (uint32_t option_offset = 0U; option_offset < node->data.choice.option_count; ++option_offset)
			{
				const scry_ir_choice_option* option = &program->choice_options[node->data.choice.option_start + option_offset];

				if (!scry_sdlg_compile_write_u32_le(buffer, buffer_size, &payload_offset, option->text_string_id) ||
					!scry_sdlg_compile_write_s32_le(buffer,
													buffer_size,
													&payload_offset,
													(int32_t)scry_sdlg_compile_relative_delta(layout->block_offsets[option->target_node_index],
																							  layout->block_offsets[i] + layout->block_sizes[i])))
				{
					return false;
				}
			}

			if (!scry_sdlg_compile_write_u16_le(buffer, buffer_size, &directory_offset, (uint16_t)SCRY_SDLG_TABLE_KIND_CHOICE) ||
				!scry_sdlg_compile_write_u16_le(buffer, buffer_size, &directory_offset, 0U) ||
				!scry_sdlg_compile_write_u32_le(buffer, buffer_size, &directory_offset, (uint32_t)payload_start) ||
				!scry_sdlg_compile_write_u32_le(buffer, buffer_size, &directory_offset, (uint32_t)(payload_offset - payload_start)))
			{
				return false;
			}
		}

		else
		{
			if (!scry_sdlg_compile_write_u16_le(buffer, buffer_size, &payload_offset, (uint16_t)node->data.switch_branch.case_count) ||
				!scry_sdlg_compile_write_u16_le(buffer, buffer_size, &payload_offset, (uint16_t)node->data.switch_branch.variable_id) ||
				!scry_sdlg_compile_write_s32_le(buffer,
												buffer_size,
												&payload_offset,
												(int32_t)scry_sdlg_compile_relative_delta(layout->block_offsets[node->data.switch_branch.default_node_index],
																						  layout->block_offsets[i] + layout->block_sizes[i])))
			{
				return false;
			}

			for (uint32_t case_offset = 0U; case_offset < node->data.switch_branch.case_count; ++case_offset)
			{
				const scry_ir_switch_case* switch_case = &program->switch_cases[node->data.switch_branch.case_start + case_offset];

				if (!scry_sdlg_compile_write_s32_le(buffer, buffer_size, &payload_offset, switch_case->value) ||
					!scry_sdlg_compile_write_s32_le(buffer,
													buffer_size,
													&payload_offset,
													(int32_t)scry_sdlg_compile_relative_delta(layout->block_offsets[switch_case->target_node_index],
																							  layout->block_offsets[i] + layout->block_sizes[i])))
				{
					return false;
				}
			}

			if (!scry_sdlg_compile_write_u16_le(buffer, buffer_size, &directory_offset, (uint16_t)SCRY_SDLG_TABLE_KIND_SWITCH) ||
				!scry_sdlg_compile_write_u16_le(buffer, buffer_size, &directory_offset, 0U) ||
				!scry_sdlg_compile_write_u32_le(buffer, buffer_size, &directory_offset, (uint32_t)payload_start) ||
				!scry_sdlg_compile_write_u32_le(buffer, buffer_size, &directory_offset, (uint32_t)(payload_offset - payload_start)))
			{
				return false;
			}
		}

		table_index += 1U;
	}

	return true;
}

static bool scry_sdlg_compile_write_strings(uint8_t*				buffer,
											size_t					buffer_size,
											size_t					string_index_offset,
											size_t					string_data_offset,
											const scry_ir_program*	program,
											const scry_sdlg_string* strings)
{
	size_t index_offset = string_index_offset;
	size_t data_offset	= string_data_offset;

	ASSERT_FATAL(buffer);
	ASSERT_FATAL(program);
	ASSERT_FATAL(strings || program->string_count == 0U);

	for (uint32_t i = 0U; i < program->string_count; ++i)
	{
		const size_t relative_offset = data_offset - string_data_offset;

		if (!scry_sdlg_compile_write_u32_le(buffer, buffer_size, &index_offset, (uint32_t)relative_offset) ||
			!scry_sdlg_write_uvarint(buffer, buffer_size, &data_offset, strings[i].length))
		{
			return false;
		}

		if (strings[i].length > 0U)
		{
			if (memchr(strings[i].data, '\0', strings[i].length) != NULL || data_offset > buffer_size || strings[i].length > (buffer_size - data_offset))
			{
				return false;
			}

			memcpy(&buffer[data_offset], strings[i].data, strings[i].length);
			data_offset += strings[i].length;
		}
	}

	return true;
}

static bool scry_sdlg_compile_write_u16_le(uint8_t* buffer, size_t buffer_size, size_t* offset, uint16_t value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);

	if (*offset > buffer_size || (buffer_size - *offset) < 2U)
	{
		return false;
	}

	buffer[*offset + 0U] = (uint8_t)(value & UINT16_C(0xff));
	buffer[*offset + 1U] = (uint8_t)((value >> 8U) & UINT16_C(0xff));
	*offset += 2U;
	return true;
}

static bool scry_sdlg_compile_write_u32_le(uint8_t* buffer, size_t buffer_size, size_t* offset, uint32_t value)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);

	if (*offset > buffer_size || (buffer_size - *offset) < 4U)
	{
		return false;
	}

	buffer[*offset + 0U] = (uint8_t)(value & UINT32_C(0xff));
	buffer[*offset + 1U] = (uint8_t)((value >> 8U) & UINT32_C(0xff));
	buffer[*offset + 2U] = (uint8_t)((value >> 16U) & UINT32_C(0xff));
	buffer[*offset + 3U] = (uint8_t)((value >> 24U) & UINT32_C(0xff));
	*offset += 4U;

	return true;
}

static bool scry_sdlg_compile_write_s32_le(uint8_t* buffer, size_t buffer_size, size_t* offset, int32_t value)
{
	union
	{
		int32_t	 signed_value;
		uint32_t unsigned_value;
	} bits = { 0 };

	bits.signed_value = value;
	return scry_sdlg_compile_write_u32_le(buffer, buffer_size, offset, bits.unsigned_value);
}

static int64_t scry_sdlg_compile_relative_delta(size_t target_offset, size_t base_offset)
{
	ASSERT_FATAL(target_offset <= (size_t)INT64_MAX);
	ASSERT_FATAL(base_offset <= (size_t)INT64_MAX);

	return (int64_t)target_offset - (int64_t)base_offset;
}
