#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct scry_editor_choice_option
{
	char*	 text;
	bool	 has_target;
	uint32_t target_node_id;
} scry_editor_choice_option;

typedef struct scry_editor_choice_node
{
	scry_editor_choice_option* options;
	size_t					   option_count;
	size_t					   option_capacity;
} scry_editor_choice_node;

void scry_editor_choice_node_init(scry_editor_choice_node* choice_node);
void scry_editor_choice_node_destroy(scry_editor_choice_node* choice_node);

scry_editor_choice_node* scry_editor_choice_node_create(void);
void					 scry_editor_choice_node_destroy_owned(scry_editor_choice_node* choice_node);

void scry_editor_choice_node_append_option(scry_editor_choice_node* choice_node, const char* text, uint32_t* out_option_index);
void scry_editor_choice_node_set_option_text(scry_editor_choice_node* choice_node, uint32_t option_index, const char* text);
void scry_editor_choice_node_remove_option(scry_editor_choice_node* choice_node, uint32_t option_index);
void scry_editor_choice_node_reorder_option(scry_editor_choice_node* choice_node, uint32_t from_index, uint32_t to_index);
