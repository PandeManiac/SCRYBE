#include "editor/scry_editor_node.h"

#include "utils/core/scry_assert.h"
#include "utils/core/scry_gen_utils.h"

#include <string.h>

typedef struct
{
	void* (*create)(void);
	void (*destroy)(void*);
} scry_editor_node_ops;

static void* scry_editor_node_allocate_payload(scry_editor_node_kind kind);
static void	 scry_editor_node_free_payload(scry_editor_node_kind kind, void* payload);

static void* scry_editor_node_create_dialogue_payload(void);
static void* scry_editor_node_create_branch_payload(void);
static void* scry_editor_node_create_choice_payload(void);
static void* scry_editor_node_create_switch_payload(void);
static void* scry_editor_node_create_goto_payload(void);
static void* scry_editor_node_create_split_payload(void);
static void* scry_editor_node_create_merge_payload(void);
static void	 scry_editor_node_destroy_dialogue_payload(void* payload);
static void	 scry_editor_node_destroy_branch_payload(void* payload);
static void	 scry_editor_node_destroy_choice_payload(void* payload);
static void	 scry_editor_node_destroy_switch_payload(void* payload);
static void	 scry_editor_node_destroy_goto_payload(void* payload);
static void	 scry_editor_node_destroy_split_payload(void* payload);
static void	 scry_editor_node_destroy_merge_payload(void* payload);

// clang-format off
static const char* const scry_editor_node_kind_names[] = {
	[SCRY_EDITOR_NODE_KIND_NONE]     = "none",
	[SCRY_EDITOR_NODE_KIND_DIALOGUE] = "dialogue",
	[SCRY_EDITOR_NODE_KIND_BRANCH]   = "branch",
	[SCRY_EDITOR_NODE_KIND_CHOICE]   = "choice",
	[SCRY_EDITOR_NODE_KIND_SWITCH]   = "switch",
	[SCRY_EDITOR_NODE_KIND_END]      = "end",
	[SCRY_EDITOR_NODE_KIND_GOTO]     = "goto",
	[SCRY_EDITOR_NODE_KIND_SPLIT]    = "split",
	[SCRY_EDITOR_NODE_KIND_MERGE]    = "merge",
};

static const scry_editor_node_ops scry_editor_node_kind_ops[] = {
	[SCRY_EDITOR_NODE_KIND_DIALOGUE] = {
		.create	 = scry_editor_node_create_dialogue_payload,
		.destroy = scry_editor_node_destroy_dialogue_payload,
	},
	[SCRY_EDITOR_NODE_KIND_BRANCH] = {
		.create	 = scry_editor_node_create_branch_payload,
		.destroy = scry_editor_node_destroy_branch_payload,
	},
	[SCRY_EDITOR_NODE_KIND_CHOICE] = {
		.create	 = scry_editor_node_create_choice_payload,
		.destroy = scry_editor_node_destroy_choice_payload,
	},
	[SCRY_EDITOR_NODE_KIND_SWITCH] = {
		.create	 = scry_editor_node_create_switch_payload,
		.destroy = scry_editor_node_destroy_switch_payload,
	},
	[SCRY_EDITOR_NODE_KIND_END] = {
		.create	 = NULL,
		.destroy = NULL,
	},
	[SCRY_EDITOR_NODE_KIND_GOTO] = {
		.create	 = scry_editor_node_create_goto_payload,
		.destroy = scry_editor_node_destroy_goto_payload,
	},
	[SCRY_EDITOR_NODE_KIND_SPLIT] = {
		.create	 = scry_editor_node_create_split_payload,
		.destroy = scry_editor_node_destroy_split_payload,
	},
	[SCRY_EDITOR_NODE_KIND_MERGE] = {
		.create	 = scry_editor_node_create_merge_payload,
		.destroy = scry_editor_node_destroy_merge_payload,
	},
};
// clang-format on

static const size_t scry_editor_node_kind_name_count = COUNT_OF(scry_editor_node_kind_names);
static const size_t scry_editor_node_kind_op_count	 = COUNT_OF(scry_editor_node_kind_ops);

STATIC_ASSERT(scry_editor_node_kind_name_count == scry_editor_node_kind_op_count, "node kind metadata tables must stay in sync");

static bool scry_editor_node_kind_index_is_valid(size_t kind_index)
{
	return kind_index < scry_editor_node_kind_name_count && kind_index < scry_editor_node_kind_op_count && scry_editor_node_kind_names[kind_index] != NULL;
}

bool scry_editor_node_kind_is_valid(scry_editor_node_kind kind)
{
	const size_t kind_index = (size_t)kind;
	return kind != SCRY_EDITOR_NODE_KIND_NONE && scry_editor_node_kind_index_is_valid(kind_index);
}

const char* scry_editor_node_kind_name(scry_editor_node_kind kind)
{
	const size_t kind_index = (size_t)kind;

	ASSERT_FATAL(kind_index < scry_editor_node_kind_name_count);
	ASSERT_FATAL(scry_editor_node_kind_names[kind_index] != NULL);

	return scry_editor_node_kind_names[kind_index];
}

void scry_editor_node_init(scry_editor_node* node)
{
	ASSERT_FATAL(node);

	memset(node, 0, sizeof(*node));

	node->input_source_node_id		 = SCRY_EDITOR_INVALID_ID;
	node->input_source_subitem_index = SCRY_EDITOR_INVALID_ID;
}

void scry_editor_node_create(scry_editor_node* node, scry_editor_node_kind kind, uint32_t id)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(scry_editor_node_kind_is_valid(kind));

	void* payload = NULL;

	if (node->kind != SCRY_EDITOR_NODE_KIND_NONE)
	{
		scry_editor_node_destroy(node);
	}

	scry_editor_node_init(node);

	payload = scry_editor_node_allocate_payload(kind);
	ASSERT_FATAL(payload != NULL || kind == SCRY_EDITOR_NODE_KIND_END);

	node->id	  = id;
	node->kind	  = kind;
	node->payload = payload;
}

void scry_editor_node_destroy(scry_editor_node* node)
{
	ASSERT_FATAL(node);

	if (node->kind != SCRY_EDITOR_NODE_KIND_NONE && node->kind != SCRY_EDITOR_NODE_KIND_END)
	{
		scry_editor_node_free_payload(node->kind, node->payload);
	}

	scry_editor_node_init(node);
}

scry_editor_dialogue_node* scry_editor_node_dialogue(scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_DIALOGUE);

	return node->payload;
}

scry_editor_branch_node* scry_editor_node_branch(scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_BRANCH);

	return node->payload;
}

scry_editor_choice_node* scry_editor_node_choice(scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_CHOICE);

	return node->payload;
}

scry_editor_switch_node* scry_editor_node_switch(scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_SWITCH);

	return node->payload;
}

scry_editor_goto_node* scry_editor_node_goto(scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_GOTO);

	return node->payload;
}

scry_editor_split_node* scry_editor_node_split(scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_SPLIT);

	return node->payload;
}

scry_editor_merge_node* scry_editor_node_merge(scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_MERGE);

	return node->payload;
}

const scry_editor_dialogue_node* scry_editor_node_dialogue_view(const scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_DIALOGUE);

	return node->payload;
}

const scry_editor_branch_node* scry_editor_node_branch_view(const scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_BRANCH);

	return node->payload;
}

const scry_editor_choice_node* scry_editor_node_choice_view(const scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_CHOICE);

	return node->payload;
}

const scry_editor_switch_node* scry_editor_node_switch_view(const scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_SWITCH);

	return node->payload;
}

const scry_editor_goto_node* scry_editor_node_goto_view(const scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_GOTO);

	return node->payload;
}

const scry_editor_split_node* scry_editor_node_split_view(const scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_SPLIT);

	return node->payload;
}

const scry_editor_merge_node* scry_editor_node_merge_view(const scry_editor_node* node)
{
	ASSERT_FATAL(node);
	ASSERT_FATAL(node->kind == SCRY_EDITOR_NODE_KIND_MERGE);

	return node->payload;
}

static void* scry_editor_node_allocate_payload(scry_editor_node_kind kind)
{
	ASSERT_FATAL(scry_editor_node_kind_is_valid(kind));

	return scry_editor_node_kind_ops[kind].create ? scry_editor_node_kind_ops[kind].create() : NULL;
}

static void scry_editor_node_free_payload(scry_editor_node_kind kind, void* payload)
{
	ASSERT_FATAL(scry_editor_node_kind_is_valid(kind));
	ASSERT_FATAL(payload);
	ASSERT_FATAL(scry_editor_node_kind_ops[kind].destroy);

	scry_editor_node_kind_ops[kind].destroy(payload);
}

static void* scry_editor_node_create_dialogue_payload(void)
{
	return scry_editor_dialogue_node_create();
}

static void* scry_editor_node_create_branch_payload(void)
{
	return scry_editor_branch_node_create();
}

static void* scry_editor_node_create_choice_payload(void)
{
	return scry_editor_choice_node_create();
}

static void* scry_editor_node_create_switch_payload(void)
{
	return scry_editor_switch_node_create();
}

static void* scry_editor_node_create_goto_payload(void)
{
	return scry_editor_goto_node_create();
}

static void* scry_editor_node_create_split_payload(void)
{
	return scry_editor_split_node_create();
}

static void* scry_editor_node_create_merge_payload(void)
{
	return scry_editor_merge_node_create();
}

static void scry_editor_node_destroy_dialogue_payload(void* payload)
{
	scry_editor_dialogue_node_destroy_owned(payload);
}

static void scry_editor_node_destroy_branch_payload(void* payload)
{
	scry_editor_branch_node_destroy_owned(payload);
}

static void scry_editor_node_destroy_choice_payload(void* payload)
{
	scry_editor_choice_node_destroy_owned(payload);
}

static void scry_editor_node_destroy_switch_payload(void* payload)
{
	scry_editor_switch_node_destroy_owned(payload);
}

static void scry_editor_node_destroy_goto_payload(void* payload)
{
	scry_editor_goto_node_destroy_owned(payload);
}

static void scry_editor_node_destroy_split_payload(void* payload)
{
	scry_editor_split_node_destroy_owned(payload);
}

static void scry_editor_node_destroy_merge_payload(void* payload)
{
	scry_editor_merge_node_destroy_owned(payload);
}
