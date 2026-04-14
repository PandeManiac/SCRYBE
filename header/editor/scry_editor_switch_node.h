#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct scry_editor_switch_case
{
	int32_t	 value;
	bool	 has_target;
	uint32_t target_node_id;
} scry_editor_switch_case;

typedef struct scry_editor_switch_node
{
	bool					 has_variable;
	uint32_t				 variable_id;
	scry_editor_switch_case* cases;
	size_t					 case_count;
	size_t					 case_capacity;
	bool					 has_default_target;
	uint32_t				 default_target_node_id;
} scry_editor_switch_node;

void scry_editor_switch_node_init(scry_editor_switch_node* switch_node);
void scry_editor_switch_node_destroy(scry_editor_switch_node* switch_node);

scry_editor_switch_node* scry_editor_switch_node_create(void);
void					 scry_editor_switch_node_destroy_owned(scry_editor_switch_node* switch_node);

void scry_editor_switch_node_append_case(scry_editor_switch_node* switch_node, int32_t value, uint32_t* out_case_index);
void scry_editor_switch_node_remove_case(scry_editor_switch_node* switch_node, uint32_t case_index);
void scry_editor_switch_node_reorder_case(scry_editor_switch_node* switch_node, uint32_t from_index, uint32_t to_index);
