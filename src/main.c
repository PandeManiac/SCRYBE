#include "utils/core/scry_assert.h"
#include "vm/scry_dialogue_cli.h"
#include "window/scry_window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <stdio.h>
#include <string.h>

static const scry_window_config window_config = {
	.title = "SCRYBE WINDOW",
	.pos   = { { 0, 0 } },
	.size  = { { 1920, 1080 } },

	.major	 = 4,
	.minor	 = 6,
	.profile = GLFW_OPENGL_CORE_PROFILE,

	.samples = 4,

	.fullscreen = true,
	.vsync		= true,
};

static scry_window window = { 0 };

int main(int argc, char** argv)
{
	if (argc == 3 && strcmp(argv[1], "dialogue-run") == 0)
	{
		return scry_dialogue_run_file(argv[2]) ? 0 : 1;
	}

	if (argc != 1)
	{
		(void)fprintf(stderr, "usage: %s [dialogue-run <path>]\n", argv[0]);
		return 1;
	}

	ASSERT_FATAL(glfwInit());

	scry_window_init(&window, &window_config);
	ASSERT_FATAL(gladLoaderLoadGL());

	while (!scry_window_should_close(&window))
	{
		scry_window_update_input(&window, 1.0 / 60.0);
		scry_window_swap_buffers(&window);
	}

	scry_window_destroy(&window);
	glfwTerminate();

	return 0;
}
