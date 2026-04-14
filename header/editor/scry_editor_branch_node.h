#pragma once

#include "vm/scry_ir.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct scry_editor_branch_node
{
	bool			   has_variable;
	uint32_t		   variable_id;
	scry_ir_compare_op compare_op;
	int32_t			   compare_value;
	bool			   has_true_target;
	uint32_t		   true_target_node_id;
	bool			   has_false_target;
	uint32_t		   false_target_node_id;
} scry_editor_branch_node;

void scry_editor_branch_node_init(scry_editor_branch_node* branch_node);
void scry_editor_branch_node_destroy(scry_editor_branch_node* branch_node);

scry_editor_branch_node* scry_editor_branch_node_create(void);
void					 scry_editor_branch_node_destroy_owned(scry_editor_branch_node* branch_node);
