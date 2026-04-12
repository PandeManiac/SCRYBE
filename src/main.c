#include "utils/scry_assert.h"
#include "window/scry_window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

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

int main(void)
{
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
