#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct scry_editor_dialogue_node
{
	char*	 text;
	bool	 has_next_target;
	uint32_t next_target_node_id;
} scry_editor_dialogue_node;

void scry_editor_dialogue_node_init(scry_editor_dialogue_node* dialogue_node);
void scry_editor_dialogue_node_destroy(scry_editor_dialogue_node* dialogue_node);
void scry_editor_dialogue_node_set_text(scry_editor_dialogue_node* dialogue_node, const char* text);

scry_editor_dialogue_node* scry_editor_dialogue_node_create(void);
void					   scry_editor_dialogue_node_destroy_owned(scry_editor_dialogue_node* dialogue_node);
