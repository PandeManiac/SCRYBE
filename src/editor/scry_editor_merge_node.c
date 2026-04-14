#include "editor/scry_editor_merge_node.h"

#include "utils/core/scry_assert.h"

#include <stdlib.h>
#include <string.h>

void scry_editor_merge_node_init(scry_editor_merge_node* merge_node)
{
	ASSERT_FATAL(merge_node);

	memset(merge_node, 0, sizeof(*merge_node));
}

void scry_editor_merge_node_destroy(scry_editor_merge_node* merge_node)
{
	ASSERT_FATAL(merge_node);

	memset(merge_node, 0, sizeof(*merge_node));
}

scry_editor_merge_node* scry_editor_merge_node_create(void)
{
	scry_editor_merge_node* merge_node = malloc(sizeof(*merge_node));
	ASSERT_FATAL(merge_node);

	scry_editor_merge_node_init(merge_node);
	return merge_node;
}

void scry_editor_merge_node_destroy_owned(scry_editor_merge_node* merge_node)
{
	ASSERT_FATAL(merge_node);

	scry_editor_merge_node_destroy(merge_node);
	free(merge_node);
}
