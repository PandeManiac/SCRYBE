#pragma once

#include "input/scry_input.h"

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t scry_action_id;

typedef enum scry_action_source
{
	SCRY_ACTION_SOURCE_KEY = 0,
	SCRY_ACTION_SOURCE_MOUSE_BUTTON,
	SCRY_ACTION_SOURCE_SCROLL,
	SCRY_ACTION_SOURCE_TEXT,
} scry_action_source;

typedef enum scry_action_trigger
{
	SCRY_ACTION_TRIGGER_PRESSED = 0,
	SCRY_ACTION_TRIGGER_RELEASED,
	SCRY_ACTION_TRIGGER_REPEATED,
	SCRY_ACTION_TRIGGER_DOWN,
	SCRY_ACTION_TRIGGER_CHANGED,
} scry_action_trigger;

typedef struct scry_action
{
	scry_action_id		id;
	scry_action_source	source;
	scry_action_trigger trigger;
	int					code;
	scry_modifier_mask	modifiers;
	double				x;
	double				y;
	uint32_t			text;
} scry_action;

typedef struct scry_action_binding
{
	scry_action_id		id;
	scry_action_source	source;
	scry_action_trigger trigger;
	int					code;
	scry_modifier_mask	modifiers;
} scry_action_binding;

struct scry_action_map;

typedef bool (*scry_action_map_handle_fn)(struct scry_action_map* map, const scry_action* action);

typedef struct scry_action_map
{
	const scry_action_binding* bindings;
	uint32_t				   binding_count;
	void*					   user_data;
	scry_action_map_handle_fn  handle;
} scry_action_map;

typedef struct scry_action_stack
{
	scry_action_map* maps[8];
	uint32_t		 count;
} scry_action_stack;

void			 scry_action_stack_init(scry_action_stack* stack);
void			 scry_action_stack_clear(scry_action_stack* stack);
bool			 scry_action_stack_push(scry_action_stack* stack, scry_action_map* map);
scry_action_map* scry_action_stack_pop(scry_action_stack* stack);
bool			 scry_action_stack_dispatch(scry_action_stack* stack, const scry_input* input);
