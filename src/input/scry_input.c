#include "input/scry_input.h"

#include "utils/scry_assert.h"

#include <string.h>

static bool scry_input_valid_key(int key);
static bool scry_input_valid_mouse_button(int button);
static void scry_input_release_all_controls(scry_input* input);
static bool scry_input_modifier_key_down(const scry_input* input, int key);

void scry_input_init(scry_input* input)
{
	ASSERT_FATAL(input);
	memset(input, 0, sizeof(*input));
}

void scry_input_begin_frame(scry_input* input)
{
	ASSERT_FATAL(input);

	for (size_t i = 0; i < (size_t)(GLFW_KEY_LAST + 1); ++i)
	{
		input->keys[i].pressed	= false;
		input->keys[i].released = false;
		input->keys[i].repeated = false;
	}

	for (size_t i = 0; i < (size_t)(GLFW_MOUSE_BUTTON_LAST + 1); ++i)
	{
		input->mouse_buttons[i].pressed	 = false;
		input->mouse_buttons[i].released = false;
	}

	input->mouse_dx		  = 0.0;
	input->mouse_dy		  = 0.0;
	input->scroll_x		  = 0.0;
	input->scroll_y		  = 0.0;
	input->cursor_entered = false;
	input->cursor_left	  = false;
	input->focus_gained	  = false;
	input->focus_lost	  = false;
	input->text_count	  = 0U;
}

bool scry_input_key_down(const scry_input* input, int key)
{
	ASSERT_FATAL(input);
	return scry_input_valid_key(key) ? input->keys[key].down : false;
}

bool scry_input_key_pressed(const scry_input* input, int key)
{
	ASSERT_FATAL(input);
	return scry_input_valid_key(key) ? input->keys[key].pressed : false;
}

bool scry_input_key_released(const scry_input* input, int key)
{
	ASSERT_FATAL(input);
	return scry_input_valid_key(key) ? input->keys[key].released : false;
}

bool scry_input_key_repeated(const scry_input* input, int key)
{
	ASSERT_FATAL(input);
	return scry_input_valid_key(key) ? input->keys[key].repeated : false;
}

bool scry_input_mouse_down(const scry_input* input, int button)
{
	ASSERT_FATAL(input);
	return scry_input_valid_mouse_button(button) ? input->mouse_buttons[button].down : false;
}

bool scry_input_mouse_pressed(const scry_input* input, int button)
{
	ASSERT_FATAL(input);
	return scry_input_valid_mouse_button(button) ? input->mouse_buttons[button].pressed : false;
}

bool scry_input_mouse_released(const scry_input* input, int button)
{
	ASSERT_FATAL(input);
	return scry_input_valid_mouse_button(button) ? input->mouse_buttons[button].released : false;
}

void scry_input_on_key(scry_input* input, int key, int action)
{
	ASSERT_FATAL(input);

	if (!scry_input_valid_key(key))
	{
		return;
	}

	scry_key_state* const state = &input->keys[key];

	switch (action)
	{
		case GLFW_PRESS:
			state->pressed = !state->down;
			state->down	   = true;
			break;
		case GLFW_RELEASE:
			state->released = state->down;
			state->down		= false;
			break;
		case GLFW_REPEAT:
			state->repeated = true;
			state->down		= true;
			break;
		default:
			break;
	}
}

void scry_input_on_mouse_button(scry_input* input, int button, int action)
{
	ASSERT_FATAL(input);

	if (!scry_input_valid_mouse_button(button))
	{
		return;
	}

	scry_button_state* const state = &input->mouse_buttons[button];

	switch (action)
	{
		case GLFW_PRESS:
			state->pressed = !state->down;
			state->down	   = true;
			break;
		case GLFW_RELEASE:
			state->released = state->down;
			state->down		= false;
			break;
		default:
			break;
	}
}

void scry_input_on_cursor_position(scry_input* input, double x, double y)
{
	ASSERT_FATAL(input);

	if (input->mouse_position_valid)
	{
		input->mouse_dx += x - input->mouse_x;
		input->mouse_dy += y - input->mouse_y;
	}
	else
	{
		input->mouse_position_valid = true;
	}

	input->mouse_x = x;
	input->mouse_y = y;
}

void scry_input_on_scroll(scry_input* input, double xoffset, double yoffset)
{
	ASSERT_FATAL(input);

	input->scroll_x += xoffset;
	input->scroll_y += yoffset;
}

void scry_input_on_text(scry_input* input, uint32_t codepoint)
{
	ASSERT_FATAL(input);

	if (input->text_count >= SCRY_INPUT_TEXT_CAPACITY)
	{
		return;
	}

	input->text[input->text_count++] = codepoint;
}

void scry_input_on_cursor_enter(scry_input* input, bool entered)
{
	ASSERT_FATAL(input);

	input->cursor_inside  = entered;
	input->cursor_entered = entered;
	input->cursor_left	  = !entered;
}

void scry_input_on_focus(scry_input* input, bool focused)
{
	ASSERT_FATAL(input);

	if (input->focused == focused)
	{
		return;
	}

	input->focused		= focused;
	input->focus_gained = focused;
	input->focus_lost	= !focused;

	if (!focused)
	{
		scry_input_release_all_controls(input);
	}
}

scry_modifier_mask scry_input_modifiers(const scry_input* input)
{
	scry_modifier_mask modifiers = 0U;

	ASSERT_FATAL(input);

	if (scry_input_modifier_key_down(input, GLFW_KEY_LEFT_SHIFT) || scry_input_modifier_key_down(input, GLFW_KEY_RIGHT_SHIFT))
	{
		modifiers |= SCRY_MODIFIER_SHIFT;
	}

	if (scry_input_modifier_key_down(input, GLFW_KEY_LEFT_CONTROL) || scry_input_modifier_key_down(input, GLFW_KEY_RIGHT_CONTROL))
	{
		modifiers |= SCRY_MODIFIER_CONTROL;
	}

	if (scry_input_modifier_key_down(input, GLFW_KEY_LEFT_ALT) || scry_input_modifier_key_down(input, GLFW_KEY_RIGHT_ALT))
	{
		modifiers |= SCRY_MODIFIER_ALT;
	}

	if (scry_input_modifier_key_down(input, GLFW_KEY_LEFT_SUPER) || scry_input_modifier_key_down(input, GLFW_KEY_RIGHT_SUPER))
	{
		modifiers |= SCRY_MODIFIER_SUPER;
	}

	if (input->keys[GLFW_KEY_CAPS_LOCK].down)
	{
		modifiers |= SCRY_MODIFIER_CAPS_LOCK;
	}

	if (input->keys[GLFW_KEY_NUM_LOCK].down)
	{
		modifiers |= SCRY_MODIFIER_NUM_LOCK;
	}

	return modifiers;
}

static bool scry_input_valid_key(int key)
{
	return key >= 0 && key <= GLFW_KEY_LAST;
}

static bool scry_input_valid_mouse_button(int button)
{
	return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST;
}

static void scry_input_release_all_controls(scry_input* input)
{
	ASSERT_FATAL(input);

	for (size_t i = 0; i < (size_t)(GLFW_KEY_LAST + 1); ++i)
	{
		input->keys[i].down = false;
	}

	for (size_t i = 0; i < (size_t)(GLFW_MOUSE_BUTTON_LAST + 1); ++i)
	{
		input->mouse_buttons[i].down = false;
	}
}

static bool scry_input_modifier_key_down(const scry_input* input, int key)
{
	ASSERT_FATAL(input);
	return scry_input_valid_key(key) ? input->keys[key].down : false;
}
