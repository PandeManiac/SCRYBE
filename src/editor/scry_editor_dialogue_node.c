#include "editor/scry_editor_dialogue_node.h"

#include "utils/core/scry_assert.h"

#include <stdlib.h>
#include <string.h>

static char* scry_editor_dialogue_node_strdup(const char* text);

void scry_editor_dialogue_node_init(scry_editor_dialogue_node* dialogue_node)
{
	ASSERT_FATAL(dialogue_node);

	memset(dialogue_node, 0, sizeof(*dialogue_node));

	dialogue_node->text = scry_editor_dialogue_node_strdup("");
	ASSERT_FATAL(dialogue_node->text);
	dialogue_node->next_target_node_id = UINT32_MAX;
}

void scry_editor_dialogue_node_destroy(scry_editor_dialogue_node* dialogue_node)
{
	ASSERT_FATAL(dialogue_node);

	free(dialogue_node->text);
	memset(dialogue_node, 0, sizeof(*dialogue_node));
}

void scry_editor_dialogue_node_set_text(scry_editor_dialogue_node* dialogue_node, const char* text)
{
	ASSERT_FATAL(dialogue_node);
	ASSERT_FATAL(text);

	char* replacement = scry_editor_dialogue_node_strdup(text);

	ASSERT_FATAL(replacement);

	free(dialogue_node->text);
	dialogue_node->text = replacement;
}

scry_editor_dialogue_node* scry_editor_dialogue_node_create(void)
{
	scry_editor_dialogue_node* dialogue_node = malloc(sizeof(*dialogue_node));
	ASSERT_FATAL(dialogue_node);

	scry_editor_dialogue_node_init(dialogue_node);
	return dialogue_node;
}

void scry_editor_dialogue_node_destroy_owned(scry_editor_dialogue_node* dialogue_node)
{
	ASSERT_FATAL(dialogue_node);

	scry_editor_dialogue_node_destroy(dialogue_node);
	free(dialogue_node);
}

static char* scry_editor_dialogue_node_strdup(const char* text)
{
	ASSERT_FATAL(text);

	const size_t length = strlen(text);
	char*		 copy	= malloc(length + 1U);

	ASSERT_FATAL(copy);

	memcpy(copy, text, length + 1U);
	return copy;
}
