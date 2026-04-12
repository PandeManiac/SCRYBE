#include "editor/scry_camera.h"

#include "utils/scry_assert.h"

#include <cglm/cglm.h>

static float scry_camera_safe_zoom(float zoom);
static void scry_camera_validate_viewport(int viewport_width, int viewport_height);

void scry_camera2d_init(scry_camera2d* camera)
{
	ASSERT_FATAL(camera);

	camera->center = (vec2s){ { 0.0f, 0.0f } };
	camera->zoom   = 1.0f;
}

void scry_camera2d_build_matrix(const scry_camera2d* camera, int viewport_width, int viewport_height, mat4 out_matrix)
{
	const float zoom		  = scry_camera_safe_zoom(camera->zoom);
	const float half_width  = (float)viewport_width / (2.0f * zoom);
	const float half_height = (float)viewport_height / (2.0f * zoom);
	const float left		  = camera->center.x - half_width;
	const float right		  = camera->center.x + half_width;
	const float top		  = camera->center.y - half_height;
	const float bottom	  = camera->center.y + half_height;

	ASSERT_FATAL(camera);
	ASSERT_FATAL(out_matrix);
	scry_camera_validate_viewport(viewport_width, viewport_height);

	glm_ortho(left, right, bottom, top, -1.0f, 1.0f, out_matrix);
}

vec2s scry_camera2d_screen_to_world(const scry_camera2d* camera, float screen_x, float screen_y, int viewport_width, int viewport_height)
{
	const float zoom = scry_camera_safe_zoom(camera->zoom);
	vec2s		world = { { 0.0f, 0.0f } };

	ASSERT_FATAL(camera);
	scry_camera_validate_viewport(viewport_width, viewport_height);

	world.x = camera->center.x + ((screen_x - ((float)viewport_width * 0.5f)) / zoom);
	world.y = camera->center.y + ((screen_y - ((float)viewport_height * 0.5f)) / zoom);
	return world;
}

void scry_camera2d_zoom_at_screen(scry_camera2d* camera, float zoom_factor, float screen_x, float screen_y, int viewport_width, int viewport_height)
{
	const vec2s world_before = scry_camera2d_screen_to_world(camera, screen_x, screen_y, viewport_width, viewport_height);
	vec2s	   world_after  = { { 0.0f, 0.0f } };

	ASSERT_FATAL(camera);
	ASSERT_FATAL(zoom_factor > 0.0f);
	scry_camera_validate_viewport(viewport_width, viewport_height);

	camera->zoom = scry_camera_safe_zoom(camera->zoom * zoom_factor);
	world_after  = scry_camera2d_screen_to_world(camera, screen_x, screen_y, viewport_width, viewport_height);

	camera->center.x += world_before.x - world_after.x;
	camera->center.y += world_before.y - world_after.y;
}

void scry_camera2d_pan_pixels(scry_camera2d* camera, float delta_screen_x, float delta_screen_y)
{
	const float zoom = scry_camera_safe_zoom(camera->zoom);

	ASSERT_FATAL(camera);

	camera->center.x -= delta_screen_x / zoom;
	camera->center.y -= delta_screen_y / zoom;
}

static float scry_camera_safe_zoom(float zoom)
{
	return zoom > 0.0f ? zoom : 1.0f;
}

static void scry_camera_validate_viewport(int viewport_width, int viewport_height)
{
	ASSERT_FATAL(viewport_width > 0);
	ASSERT_FATAL(viewport_height > 0);
}
