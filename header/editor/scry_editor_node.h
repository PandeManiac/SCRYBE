#pragma once

#include "editor/scry_editor_branch_node.h"
#include "editor/scry_editor_choice_node.h"
#include "editor/scry_editor_dialogue_node.h"
#include "editor/scry_editor_goto_node.h"
#include "editor/scry_editor_merge_node.h"
#include "editor/scry_editor_split_node.h"
#include "editor/scry_editor_switch_node.h"

#include <cglm/struct.h>

#include <stdint.h>

#define SCRY_EDITOR_INVALID_ID UINT32_MAX

typedef enum scry_editor_node_kind
{
	SCRY_EDITOR_NODE_KIND_NONE = 0,
	SCRY_EDITOR_NODE_KIND_DIALOGUE,
	SCRY_EDITOR_NODE_KIND_BRANCH,
	SCRY_EDITOR_NODE_KIND_CHOICE,
	SCRY_EDITOR_NODE_KIND_SWITCH,
	SCRY_EDITOR_NODE_KIND_END,
	SCRY_EDITOR_NODE_KIND_GOTO,
	SCRY_EDITOR_NODE_KIND_SPLIT,
	SCRY_EDITOR_NODE_KIND_MERGE,
} scry_editor_node_kind;

typedef struct scry_editor_node
{
	uint32_t			  id;
	scry_editor_node_kind kind;
	ivec2s				  position;
	bool				  collapsed;
	bool				  has_input_connection;
	uint32_t			  input_source_node_id;
	uint32_t			  input_source_subitem_index;
	void*				  payload;
} scry_editor_node;

bool		scry_editor_node_kind_is_valid(scry_editor_node_kind kind);
const char* scry_editor_node_kind_name(scry_editor_node_kind kind);

void scry_editor_node_init(scry_editor_node* node);
void scry_editor_node_create(scry_editor_node* node, scry_editor_node_kind kind, uint32_t id);
void scry_editor_node_destroy(scry_editor_node* node);

scry_editor_dialogue_node* scry_editor_node_dialogue(scry_editor_node* node);
scry_editor_branch_node*   scry_editor_node_branch(scry_editor_node* node);
scry_editor_choice_node*   scry_editor_node_choice(scry_editor_node* node);
scry_editor_switch_node*   scry_editor_node_switch(scry_editor_node* node);
scry_editor_goto_node*	   scry_editor_node_goto(scry_editor_node* node);
scry_editor_split_node*	   scry_editor_node_split(scry_editor_node* node);
scry_editor_merge_node*	   scry_editor_node_merge(scry_editor_node* node);

const scry_editor_dialogue_node* scry_editor_node_dialogue_view(const scry_editor_node* node);
const scry_editor_branch_node*	 scry_editor_node_branch_view(const scry_editor_node* node);
const scry_editor_choice_node*	 scry_editor_node_choice_view(const scry_editor_node* node);
const scry_editor_switch_node*	 scry_editor_node_switch_view(const scry_editor_node* node);
const scry_editor_goto_node*	 scry_editor_node_goto_view(const scry_editor_node* node);
const scry_editor_split_node*	 scry_editor_node_split_view(const scry_editor_node* node);
const scry_editor_merge_node*	 scry_editor_node_merge_view(const scry_editor_node* node);
