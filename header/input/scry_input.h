#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t scry_modifier_mask;

#define SCRY_MODIFIER_ANY UINT32_MAX

enum
{
	SCRY_MODIFIER_SHIFT = 1U << 0,
	SCRY_MODIFIER_CONTROL = 1U << 1,
	SCRY_MODIFIER_ALT = 1U << 2,
	SCRY_MODIFIER_SUPER = 1U << 3,
	SCRY_MODIFIER_CAPS_LOCK = 1U << 4,
	SCRY_MODIFIER_NUM_LOCK = 1U << 5,
};

enum
{
	SCRY_INPUT_TEXT_CAPACITY = 32
};

typedef struct scry_key_state
{
	bool down;
	bool pressed;
	bool released;
	bool repeated;
} scry_key_state;

typedef struct scry_button_state
{
	bool down;
	bool pressed;
	bool released;
} scry_button_state;

typedef struct scry_input
{
	scry_key_state	 keys[GLFW_KEY_LAST + 1];
	scry_button_state mouse_buttons[GLFW_MOUSE_BUTTON_LAST + 1];

	double mouse_x;
	double mouse_y;
	double mouse_dx;
	double mouse_dy;
	double scroll_x;
	double scroll_y;

	bool mouse_position_valid;

	bool cursor_inside;
	bool cursor_entered;
	bool cursor_left;

	bool focused;
	bool focus_gained;
	bool focus_lost;

	uint32_t text[SCRY_INPUT_TEXT_CAPACITY];
	size_t	 text_count;
} scry_input;

void scry_input_init(scry_input* input);
void scry_input_begin_frame(scry_input* input);

bool scry_input_key_down(const scry_input* input, int key);
bool scry_input_key_pressed(const scry_input* input, int key);
bool scry_input_key_released(const scry_input* input, int key);
bool scry_input_key_repeated(const scry_input* input, int key);

bool scry_input_mouse_down(const scry_input* input, int button);
bool scry_input_mouse_pressed(const scry_input* input, int button);
bool scry_input_mouse_released(const scry_input* input, int button);

void scry_input_on_key(scry_input* input, int key, int action);
void scry_input_on_mouse_button(scry_input* input, int button, int action);
void scry_input_on_cursor_position(scry_input* input, double x, double y);
void scry_input_on_scroll(scry_input* input, double xoffset, double yoffset);
void scry_input_on_text(scry_input* input, uint32_t codepoint);
void scry_input_on_cursor_enter(scry_input* input, bool entered);
void scry_input_on_focus(scry_input* input, bool focused);
scry_modifier_mask scry_input_modifiers(const scry_input* input);
