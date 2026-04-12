#include "input/scry_action.h"

#include "utils/scry_assert.h"

#include <stddef.h>

static bool scry_action_stack_dispatch_action(scry_action_stack* stack, const scry_action* action);
static bool scry_action_binding_matches(const scry_action_binding* binding, const scry_action* action);

void scry_action_stack_init(scry_action_stack* stack)
{
	ASSERT_FATAL(stack);
	stack->count = 0U;
}

void scry_action_stack_clear(scry_action_stack* stack)
{
	ASSERT_FATAL(stack);
	stack->count = 0U;
}

bool scry_action_stack_push(scry_action_stack* stack, scry_action_map* map)
{
	ASSERT_FATAL(stack);
	ASSERT_FATAL(map);

	if (stack->count >= (uint32_t)(sizeof(stack->maps) / sizeof(stack->maps[0])))
	{
		return false;
	}

	stack->maps[stack->count++] = map;
	return true;
}

scry_action_map* scry_action_stack_pop(scry_action_stack* stack)
{
	ASSERT_FATAL(stack);

	if (stack->count == 0U)
	{
		return NULL;
	}

	stack->count -= 1U;
	return stack->maps[stack->count];
}

bool scry_action_stack_dispatch(scry_action_stack* stack, const scry_input* input)
{
	ASSERT_FATAL(stack);
	ASSERT_FATAL(input);

	bool					 consumed  = false;
	const scry_modifier_mask modifiers = scry_input_modifiers(input);

	for (int key = 0; key <= GLFW_KEY_LAST; ++key)
	{
		const scry_key_state* const state = &input->keys[key];

		if (state->pressed)
		{
			const scry_action action = {
				.id		   = 0U,
				.source	   = SCRY_ACTION_SOURCE_KEY,
				.trigger   = SCRY_ACTION_TRIGGER_PRESSED,
				.code	   = key,
				.modifiers = modifiers,
				.x		   = 0.0,
				.y		   = 0.0,
				.text	   = 0U,
			};

			consumed = scry_action_stack_dispatch_action(stack, &action) || consumed;
		}

		if (state->released)
		{
			const scry_action action = {
				.id		   = 0U,
				.source	   = SCRY_ACTION_SOURCE_KEY,
				.trigger   = SCRY_ACTION_TRIGGER_RELEASED,
				.code	   = key,
				.modifiers = modifiers,
				.x		   = 0.0,
				.y		   = 0.0,
				.text	   = 0U,
			};

			consumed = scry_action_stack_dispatch_action(stack, &action) || consumed;
		}

		if (state->repeated)
		{
			const scry_action action = {
				.id		   = 0U,
				.source	   = SCRY_ACTION_SOURCE_KEY,
				.trigger   = SCRY_ACTION_TRIGGER_REPEATED,
				.code	   = key,
				.modifiers = modifiers,
				.x		   = 0.0,
				.y		   = 0.0,
				.text	   = 0U,
			};

			consumed = scry_action_stack_dispatch_action(stack, &action) || consumed;
		}

		if (state->down)
		{
			const scry_action action = {
				.id		   = 0U,
				.source	   = SCRY_ACTION_SOURCE_KEY,
				.trigger   = SCRY_ACTION_TRIGGER_DOWN,
				.code	   = key,
				.modifiers = modifiers,
				.x		   = 0.0,
				.y		   = 0.0,
				.text	   = 0U,
			};

			consumed = scry_action_stack_dispatch_action(stack, &action) || consumed;
		}
	}

	for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
	{
		const scry_button_state* const state = &input->mouse_buttons[button];

		if (state->pressed)
		{
			const scry_action action = {
				.id		   = 0U,
				.source	   = SCRY_ACTION_SOURCE_MOUSE_BUTTON,
				.trigger   = SCRY_ACTION_TRIGGER_PRESSED,
				.code	   = button,
				.modifiers = modifiers,
				.x		   = input->mouse_x,
				.y		   = input->mouse_y,
				.text	   = 0U,
			};

			consumed = scry_action_stack_dispatch_action(stack, &action) || consumed;
		}

		if (state->released)
		{
			const scry_action action = {
				.id		   = 0U,
				.source	   = SCRY_ACTION_SOURCE_MOUSE_BUTTON,
				.trigger   = SCRY_ACTION_TRIGGER_RELEASED,
				.code	   = button,
				.modifiers = modifiers,
				.x		   = input->mouse_x,
				.y		   = input->mouse_y,
				.text	   = 0U,
			};

			consumed = scry_action_stack_dispatch_action(stack, &action) || consumed;
		}

		if (state->down)
		{
			const scry_action action = {
				.id		   = 0U,
				.source	   = SCRY_ACTION_SOURCE_MOUSE_BUTTON,
				.trigger   = SCRY_ACTION_TRIGGER_DOWN,
				.code	   = button,
				.modifiers = modifiers,
				.x		   = input->mouse_x,
				.y		   = input->mouse_y,
				.text	   = 0U,
			};

			consumed = scry_action_stack_dispatch_action(stack, &action) || consumed;
		}
	}

	if (input->scroll_x != 0.0 || input->scroll_y != 0.0)
	{
		const scry_action action = {
			.id		   = 0U,
			.source	   = SCRY_ACTION_SOURCE_SCROLL,
			.trigger   = SCRY_ACTION_TRIGGER_CHANGED,
			.code	   = 0,
			.modifiers = modifiers,
			.x		   = input->scroll_x,
			.y		   = input->scroll_y,
			.text	   = 0U,
		};

		consumed = scry_action_stack_dispatch_action(stack, &action) || consumed;
	}

	for (size_t i = 0; i < input->text_count; ++i)
	{
		const scry_action action = {
			.id		   = 0U,
			.source	   = SCRY_ACTION_SOURCE_TEXT,
			.trigger   = SCRY_ACTION_TRIGGER_CHANGED,
			.code	   = 0,
			.modifiers = modifiers,
			.x		   = 0.0,
			.y		   = 0.0,
			.text	   = input->text[i],
		};

		consumed = scry_action_stack_dispatch_action(stack, &action) || consumed;
	}

	return consumed;
}

static bool scry_action_stack_dispatch_action(scry_action_stack* stack, const scry_action* action)
{
	ASSERT_FATAL(stack);
	ASSERT_FATAL(action);

	for (int32_t i = (int32_t)stack->count - 1; i >= 0; --i)
	{
		scry_action_map* const map = stack->maps[i];

		ASSERT_FATAL(map);
		ASSERT_FATAL(map->bindings);
		ASSERT_FATAL(map->handle);

		for (uint32_t binding_index = 0U; binding_index < map->binding_count; ++binding_index)
		{
			const scry_action_binding* const binding = &map->bindings[binding_index];

			if (!scry_action_binding_matches(binding, action))
			{
				continue;
			}

			scry_action resolved_action = *action;
			resolved_action.id			= binding->id;

			if (map->handle(map, &resolved_action))
			{
				return true;
			}
		}
	}

	return false;
}

static bool scry_action_binding_matches(const scry_action_binding* binding, const scry_action* action)
{
	ASSERT_FATAL(binding);
	ASSERT_FATAL(action);

	if (binding->source != action->source)
	{
		return false;
	}

	if (binding->trigger != action->trigger)
	{
		return false;
	}

	if (binding->source == SCRY_ACTION_SOURCE_KEY || binding->source == SCRY_ACTION_SOURCE_MOUSE_BUTTON)
	{
		if (binding->code != action->code)
		{
			return false;
		}
	}

	if (binding->modifiers == SCRY_MODIFIER_ANY)
	{
		return true;
	}

	return binding->modifiers == action->modifiers;
}
