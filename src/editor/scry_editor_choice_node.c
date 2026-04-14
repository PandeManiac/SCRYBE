#include "editor/scry_editor_choice_node.h"

#include "utils/core/scry_assert.h"
#include "utils/core/scry_storage.h"

#include <stdlib.h>
#include <string.h>

static char* scry_editor_choice_node_strdup(const char* text);

void scry_editor_choice_node_init(scry_editor_choice_node* choice_node)
{
	ASSERT_FATAL(choice_node);

	memset(choice_node, 0, sizeof(*choice_node));
}

void scry_editor_choice_node_destroy(scry_editor_choice_node* choice_node)
{
	ASSERT_FATAL(choice_node);

	for (size_t index = 0; index < choice_node->option_count; ++index)
	{
		free(choice_node->options[index].text);
	}

	free(choice_node->options);
	memset(choice_node, 0, sizeof(*choice_node));
}

scry_editor_choice_node* scry_editor_choice_node_create(void)
{
	scry_editor_choice_node* choice_node = malloc(sizeof(*choice_node));
	ASSERT_FATAL(choice_node);

	scry_editor_choice_node_init(choice_node);
	return choice_node;
}

void scry_editor_choice_node_destroy_owned(scry_editor_choice_node* choice_node)
{
	ASSERT_FATAL(choice_node);

	scry_editor_choice_node_destroy(choice_node);
	free(choice_node);
}

void scry_editor_choice_node_append_option(scry_editor_choice_node* choice_node, const char* text, uint32_t* out_option_index)
{
	ASSERT_FATAL(choice_node);
	ASSERT_FATAL(text);
	ASSERT_FATAL(out_option_index);
	ASSERT_FATAL(SCRY_STORAGE_RESERVE(choice_node->options, choice_node->option_capacity, choice_node->option_count + 1U, 4U));
	ASSERT_FATAL(choice_node->options);

	scry_editor_choice_option option = { 0 };
	option.text						 = scry_editor_choice_node_strdup(text);

	ASSERT_FATAL(option.text);

	choice_node->options[choice_node->option_count] = option;

	*out_option_index = (uint32_t)choice_node->option_count;
	choice_node->option_count++;
}

void scry_editor_choice_node_set_option_text(scry_editor_choice_node* choice_node, uint32_t option_index, const char* text)
{
	ASSERT_FATAL(choice_node);
	ASSERT_FATAL(text);
	ASSERT_FATAL(option_index < choice_node->option_count);

	char* replacement = scry_editor_choice_node_strdup(text);
	ASSERT_FATAL(replacement);

	free(choice_node->options[option_index].text);
	choice_node->options[option_index].text = replacement;
}

void scry_editor_choice_node_remove_option(scry_editor_choice_node* choice_node, uint32_t option_index)
{
	ASSERT_FATAL(choice_node);
	ASSERT_FATAL(option_index < choice_node->option_count);

	free(choice_node->options[option_index].text);

	if (option_index + 1U < choice_node->option_count)
	{
		memmove(&choice_node->options[option_index],
				&choice_node->options[option_index + 1U],
				(choice_node->option_count - option_index - 1U) * sizeof(choice_node->options[0]));
	}

	choice_node->option_count--;
}

void scry_editor_choice_node_reorder_option(scry_editor_choice_node* choice_node, uint32_t from_index, uint32_t to_index)
{
	ASSERT_FATAL(choice_node);
	ASSERT_FATAL(from_index < choice_node->option_count);
	ASSERT_FATAL(to_index < choice_node->option_count);

	if (from_index == to_index)
	{
		return;
	}

	const size_t element_size = sizeof(choice_node->options[0]);

	scry_editor_choice_option moved = choice_node->options[from_index];

	if (from_index < to_index)
	{
		memmove(&choice_node->options[from_index], &choice_node->options[from_index + 1U], (to_index - from_index) * element_size);
	}

	else
	{
		memmove(&choice_node->options[to_index + 1U], &choice_node->options[to_index], (from_index - to_index) * element_size);
	}

	choice_node->options[to_index] = moved;
}

static char* scry_editor_choice_node_strdup(const char* text)
{
	ASSERT_FATAL(text);

	size_t length = strlen(text) + 1U;
	char*  copy	  = malloc(length);

	ASSERT_FATAL(copy);

	memcpy(copy, text, length);
	return copy;
}