#include "editor/scry_editor_split_node.h"

#include "utils/core/scry_assert.h"

#include <stdlib.h>
#include <string.h>

void scry_editor_split_node_init(scry_editor_split_node* split_node)
{
	ASSERT_FATAL(split_node);

	memset(split_node, 0, sizeof(*split_node));
}

void scry_editor_split_node_destroy(scry_editor_split_node* split_node)
{
	ASSERT_FATAL(split_node);

	memset(split_node, 0, sizeof(*split_node));
}

scry_editor_split_node* scry_editor_split_node_create(void)
{
	scry_editor_split_node* split_node = malloc(sizeof(*split_node));
	ASSERT_FATAL(split_node);

	scry_editor_split_node_init(split_node);
	return split_node;
}

void scry_editor_split_node_destroy_owned(scry_editor_split_node* split_node)
{
	ASSERT_FATAL(split_node);

	scry_editor_split_node_destroy(split_node);
	free(split_node);
}
