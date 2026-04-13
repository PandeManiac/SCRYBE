#include "window/scry_window.h"

#include "utils/core/scry_assert.h"

#include <GLFW/glfw3.h>

static scry_window* scry_window_from_handle(GLFWwindow* handle);
static void			key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void			mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void			cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void			scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void			char_callback(GLFWwindow* window, unsigned int codepoint);
static void			cursor_enter_callback(GLFWwindow* window, int entered);
static void			focus_callback(GLFWwindow* window, int focused);

void scry_window_init(scry_window* window, const scry_window_config* config)
{
	ASSERT_FATAL(window);
	ASSERT_FATAL(config);

	const scry_window_config cfg = *config;

	const char*	 title = cfg.title;
	const ivec2s size  = cfg.size;
	const ivec2s pos   = cfg.pos;

	const int major	  = cfg.major;
	const int minor	  = cfg.minor;
	const int profile = cfg.profile;

	const int samples = cfg.samples;

	const bool fullscreen = cfg.fullscreen;
	const bool vsync	  = cfg.vsync;

	ASSERT_FATAL(title);
	ASSERT_FATAL(size.x > 0);
	ASSERT_FATAL(size.y > 0);
	ASSERT_FATAL(samples >= 0);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, profile);
	glfwWindowHint(GLFW_SAMPLES, samples);

	GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : NULL;
	GLFWwindow*	 share	 = NULL;
	GLFWwindow*	 handle	 = glfwCreateWindow(size.x, size.y, title, monitor, share);

	ASSERT_FATAL(handle);

	scry_input_init(&window->input);
	window->cfg	   = cfg;
	window->handle = handle;

	glfwSetWindowUserPointer(handle, window);
	glfwSetKeyCallback(handle, key_callback);
	glfwSetMouseButtonCallback(handle, mouse_button_callback);
	glfwSetCursorPosCallback(handle, cursor_position_callback);
	glfwSetScrollCallback(handle, scroll_callback);
	glfwSetCharCallback(handle, char_callback);
	glfwSetCursorEnterCallback(handle, cursor_enter_callback);
	glfwSetWindowFocusCallback(handle, focus_callback);
	glfwMakeContextCurrent(handle);

	glfwSwapInterval(vsync ? 1 : 0);

	if (!fullscreen)
	{
		glfwSetWindowPos(handle, pos.x, pos.y);
	}
}

void scry_window_destroy(scry_window* const window)
{
	ASSERT_FATAL(window);

	if (window->handle)
	{
		glfwDestroyWindow(window->handle);
		window->handle = NULL;
	}
}

bool scry_window_should_close(const scry_window* const window)
{
	ASSERT_FATAL(window);
	return glfwWindowShouldClose(window->handle);
}

void scry_window_update_input(scry_window* window, double timeout_seconds)
{
	ASSERT_FATAL(window);
	ASSERT_FATAL(window->handle);

	scry_input_begin_frame(&window->input);

	if (timeout_seconds > 0.0)
	{
		glfwWaitEventsTimeout(timeout_seconds);
		return;
	}

	glfwPollEvents();
}

const scry_input* scry_window_get_input(const scry_window* window)
{
	ASSERT_FATAL(window);
	return &window->input;
}

void scry_window_swap_buffers(const scry_window* window)
{
	ASSERT_FATAL(window);
	ASSERT_FATAL(window->handle);

	glfwSwapBuffers(window->handle);
}

void scry_window_get_framebuffer_size(const scry_window* window, int* out_width, int* out_height)
{
	ASSERT_FATAL(window);
	ASSERT_FATAL(window->handle);
	ASSERT_FATAL(out_width);
	ASSERT_FATAL(out_height);

	glfwGetFramebufferSize(window->handle, out_width, out_height);
}

static scry_window* scry_window_from_handle(GLFWwindow* handle)
{
	ASSERT_FATAL(handle);

	scry_window* const window = glfwGetWindowUserPointer(handle);
	ASSERT_FATAL(window);
	return window;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode;
	(void)mods;

	scry_window* const scry_window = scry_window_from_handle(window);
	scry_input_on_key(&scry_window->input, key, action);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	(void)mods;

	scry_window* const scry_window = scry_window_from_handle(window);
	scry_input_on_mouse_button(&scry_window->input, button, action);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	scry_window* const scry_window = scry_window_from_handle(window);
	scry_input_on_cursor_position(&scry_window->input, xpos, ypos);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	scry_window* const scry_window = scry_window_from_handle(window);
	scry_input_on_scroll(&scry_window->input, xoffset, yoffset);
}

static void char_callback(GLFWwindow* window, unsigned int codepoint)
{
	scry_window* const scry_window = scry_window_from_handle(window);
	scry_input_on_text(&scry_window->input, codepoint);
}

static void cursor_enter_callback(GLFWwindow* window, int entered)
{
	scry_window* const scry_window = scry_window_from_handle(window);
	scry_input_on_cursor_enter(&scry_window->input, entered == GLFW_TRUE);
}

static void focus_callback(GLFWwindow* window, int focused)
{
	scry_window* const scry_window = scry_window_from_handle(window);
	scry_input_on_focus(&scry_window->input, focused == GLFW_TRUE);
}
