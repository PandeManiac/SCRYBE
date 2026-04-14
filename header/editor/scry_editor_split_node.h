#pragma once

#include <stdint.h>

/*
 * SPLIT is a value-domain node. It is not valid for flow-domain operations.
 * The authored payload is intentionally minimal until the port/value layer lands.
 */
typedef struct scry_editor_split_node
{
	uint32_t reserved;
} scry_editor_split_node;

void scry_editor_split_node_init(scry_editor_split_node* split_node);
void scry_editor_split_node_destroy(scry_editor_split_node* split_node);

scry_editor_split_node* scry_editor_split_node_create(void);
void					scry_editor_split_node_destroy_owned(scry_editor_split_node* split_node);
