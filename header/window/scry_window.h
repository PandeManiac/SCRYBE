#pragma once

#include "input/scry_input.h"

#include <cglm/struct.h>
#include <stdbool.h>

typedef struct GLFWwindow GLFWwindow;

typedef struct scry_window_config
{
	const char* title;
	ivec2s		pos;
	ivec2s		size;

	int major;
	int minor;
	int profile;

	int samples;

	bool fullscreen;
	bool vsync;
} scry_window_config;

typedef struct scry_window
{
	scry_window_config cfg;
	scry_input		   input;
	GLFWwindow*		   handle;
} scry_window;

void scry_window_init(scry_window* const window, const scry_window_config* config);
void scry_window_destroy(scry_window* const window);
bool scry_window_should_close(const scry_window* const window);
void scry_window_update_input(scry_window* window, double timeout_seconds);
const scry_input* scry_window_get_input(const scry_window* window);
void scry_window_swap_buffers(const scry_window* window);
void scry_window_get_framebuffer_size(const scry_window* window, int* out_width, int* out_height);
