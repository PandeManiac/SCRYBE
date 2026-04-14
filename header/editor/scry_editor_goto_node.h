#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct scry_editor_goto_node
{
	char*	 title;
	bool	 has_target;
	uint32_t target_node_id;
} scry_editor_goto_node;

void scry_editor_goto_node_init(scry_editor_goto_node* goto_node);
void scry_editor_goto_node_destroy(scry_editor_goto_node* goto_node);
void scry_editor_goto_node_set_title(scry_editor_goto_node* goto_node, const char* title);

scry_editor_goto_node* scry_editor_goto_node_create(void);
void				   scry_editor_goto_node_destroy_owned(scry_editor_goto_node* goto_node);
