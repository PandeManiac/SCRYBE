#include "editor/scry_editor_goto_node.h"

#include "utils/core/scry_assert.h"

#include <stdlib.h>
#include <string.h>

static char* scry_editor_goto_node_strdup(const char* text);

void scry_editor_goto_node_init(scry_editor_goto_node* goto_node)
{
	ASSERT_FATAL(goto_node);

	memset(goto_node, 0, sizeof(*goto_node));

	goto_node->title = scry_editor_goto_node_strdup("");
	ASSERT_FATAL(goto_node->title);
	goto_node->target_node_id = UINT32_MAX;
}

void scry_editor_goto_node_destroy(scry_editor_goto_node* goto_node)
{
	ASSERT_FATAL(goto_node);

	free(goto_node->title);
	memset(goto_node, 0, sizeof(*goto_node));
}

void scry_editor_goto_node_set_title(scry_editor_goto_node* goto_node, const char* title)
{
	ASSERT_FATAL(goto_node);
	ASSERT_FATAL(title);

	char* replacement = scry_editor_goto_node_strdup(title);

	ASSERT_FATAL(replacement);

	free(goto_node->title);
	goto_node->title = replacement;
}

scry_editor_goto_node* scry_editor_goto_node_create(void)
{
	scry_editor_goto_node* goto_node = malloc(sizeof(*goto_node));
	ASSERT_FATAL(goto_node);

	scry_editor_goto_node_init(goto_node);
	return goto_node;
}

void scry_editor_goto_node_destroy_owned(scry_editor_goto_node* goto_node)
{
	ASSERT_FATAL(goto_node);

	scry_editor_goto_node_destroy(goto_node);
	free(goto_node);
}

static char* scry_editor_goto_node_strdup(const char* text)
{
	ASSERT_FATAL(text);

	const size_t length = strlen(text);
	char*		 copy	= malloc(length + 1U);

	ASSERT_FATAL(copy);

	memcpy(copy, text, length + 1U);
	return copy;
}
