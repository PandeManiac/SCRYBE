#include "editor/scry_editor_switch_node.h"

#include "utils/core/scry_assert.h"
#include "utils/core/scry_storage.h"

#include <stdlib.h>
#include <string.h>

void scry_editor_switch_node_init(scry_editor_switch_node* switch_node)
{
	ASSERT_FATAL(switch_node);

	memset(switch_node, 0, sizeof(*switch_node));
}

void scry_editor_switch_node_destroy(scry_editor_switch_node* switch_node)
{
	ASSERT_FATAL(switch_node);

	free(switch_node->cases);
	memset(switch_node, 0, sizeof(*switch_node));
}

scry_editor_switch_node* scry_editor_switch_node_create(void)
{
	scry_editor_switch_node* switch_node = malloc(sizeof(*switch_node));

	ASSERT_FATAL(switch_node);

	scry_editor_switch_node_init(switch_node);

	return switch_node;
}

void scry_editor_switch_node_destroy_owned(scry_editor_switch_node* switch_node)
{
	ASSERT_FATAL(switch_node);

	scry_editor_switch_node_destroy(switch_node);
	free(switch_node);
}

void scry_editor_switch_node_append_case(scry_editor_switch_node* switch_node, int32_t value, uint32_t* out_case_index)
{
	ASSERT_FATAL(switch_node);
	ASSERT_FATAL(out_case_index);

	scry_editor_switch_case switch_case = { 0 };

	ASSERT_FATAL(SCRY_STORAGE_RESERVE(switch_node->cases, switch_node->case_capacity, switch_node->case_count + 1U, 4U));

	switch_case.value							= value;
	switch_node->cases[switch_node->case_count] = switch_case;
	*out_case_index								= (uint32_t)switch_node->case_count;
	switch_node->case_count++;
}

void scry_editor_switch_node_remove_case(scry_editor_switch_node* switch_node, uint32_t case_index)
{
	ASSERT_FATAL(switch_node);
	ASSERT_FATAL(case_index < switch_node->case_count);

	memmove(&switch_node->cases[case_index], &switch_node->cases[case_index + 1U], (switch_node->case_count - case_index - 1U) * sizeof(switch_node->cases[0]));
	switch_node->case_count--;
}

void scry_editor_switch_node_reorder_case(scry_editor_switch_node* switch_node, uint32_t from_index, uint32_t to_index)
{
	ASSERT_FATAL(switch_node);

	scry_editor_switch_case moved = { 0 };

	ASSERT_FATAL(from_index < switch_node->case_count);
	ASSERT_FATAL(to_index < switch_node->case_count);

	if (from_index == to_index)
	{
		return;
	}

	moved = switch_node->cases[from_index];

	if (from_index < to_index)
	{
		memmove(&switch_node->cases[from_index], &switch_node->cases[from_index + 1U], (to_index - from_index) * sizeof(switch_node->cases[0]));
	}

	else
	{
		memmove(&switch_node->cases[to_index + 1U], &switch_node->cases[to_index], (from_index - to_index) * sizeof(switch_node->cases[0]));
	}

	switch_node->cases[to_index] = moved;
}
