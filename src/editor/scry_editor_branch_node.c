#include "editor/scry_editor_branch_node.h"

#include "utils/core/scry_assert.h"

#include <stdlib.h>
#include <string.h>

void scry_editor_branch_node_init(scry_editor_branch_node* branch_node)
{
	ASSERT_FATAL(branch_node);

	memset(branch_node, 0, sizeof(*branch_node));

	branch_node->compare_op			  = SCRY_IR_COMPARE_OP_EQUAL;
	branch_node->variable_id		  = UINT32_MAX;
	branch_node->true_target_node_id  = UINT32_MAX;
	branch_node->false_target_node_id = UINT32_MAX;
}

void scry_editor_branch_node_destroy(scry_editor_branch_node* branch_node)
{
	ASSERT_FATAL(branch_node);

	memset(branch_node, 0, sizeof(*branch_node));
}

scry_editor_branch_node* scry_editor_branch_node_create(void)
{
	scry_editor_branch_node* branch_node = malloc(sizeof(*branch_node));
	ASSERT_FATAL(branch_node);

	scry_editor_branch_node_init(branch_node);
	return branch_node;
}

void scry_editor_branch_node_destroy_owned(scry_editor_branch_node* branch_node)
{
	ASSERT_FATAL(branch_node);

	scry_editor_branch_node_destroy(branch_node);
	free(branch_node);
}
