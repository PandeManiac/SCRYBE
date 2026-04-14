#pragma once

#include <stdint.h>

/*
 * MERGE is a flow-domain node. It is not valid for value-domain operations.
 * The authored payload is intentionally minimal until the port/flow layer lands.
 */
typedef struct scry_editor_merge_node
{
	uint32_t reserved;
} scry_editor_merge_node;

void scry_editor_merge_node_init(scry_editor_merge_node* merge_node);
void scry_editor_merge_node_destroy(scry_editor_merge_node* merge_node);

scry_editor_merge_node* scry_editor_merge_node_create(void);
void					scry_editor_merge_node_destroy_owned(scry_editor_merge_node* merge_node);
