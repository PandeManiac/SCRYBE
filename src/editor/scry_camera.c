#include "editor/scry_camera.h"
#include "utils/scry_assert.h"

#include <cglm/cglm.h>

static float scry_camera_safe_zoom(float zoom);
static void	 scry_camera_validate_viewport(const scry_viewport* viewport);

void scry_camera_init(scry_camera* camera)
{
	ASSERT_FATAL(camera);

	camera->center = (vec2s) { { 0.0f, 0.0f } };
	camera->zoom   = 1.0f;
}

void scry_camera_build_matrix(const scry_camera* camera, const scry_viewport* viewport, mat4 out_matrix)
{
	ASSERT_FATAL(camera);
	ASSERT_FATAL(viewport);
	ASSERT_FATAL(out_matrix);

	scry_camera_validate_viewport(viewport);

	const float zoom		= scry_camera_safe_zoom(camera->zoom);
	const float half_width	= (float)viewport->width / (2.0f * zoom);
	const float half_height = (float)viewport->height / (2.0f * zoom);
	const float left		= camera->center.x - half_width;
	const float right		= camera->center.x + half_width;
	const float top			= camera->center.y - half_height;
	const float bottom		= camera->center.y + half_height;

	glm_ortho(left, right, bottom, top, -1.0f, 1.0f, out_matrix);
}

vec2s scry_camera_screen_to_world(const scry_camera* camera, vec2s screen_position, const scry_viewport* viewport)
{
	ASSERT_FATAL(camera);
	ASSERT_FATAL(viewport);

	scry_camera_validate_viewport(viewport);

	const float zoom  = scry_camera_safe_zoom(camera->zoom);
	vec2s		world = { { 0.0f, 0.0f } };

	world.x = camera->center.x + ((screen_position.x - ((float)viewport->width * 0.5f)) / zoom);
	world.y = camera->center.y + ((screen_position.y - ((float)viewport->height * 0.5f)) / zoom);

	return world;
}

void scry_camera_zoom_at_screen(scry_camera* camera, float zoom_factor, vec2s screen_position, const scry_viewport* viewport)
{
	ASSERT_FATAL(camera);
	ASSERT_FATAL(viewport);
	ASSERT_FATAL(zoom_factor > 0.0f);

	scry_camera_validate_viewport(viewport);

	const vec2s world_before = scry_camera_screen_to_world(camera, screen_position, viewport);
	vec2s		world_after	 = { { 0.0f, 0.0f } };

	camera->zoom = scry_camera_safe_zoom(camera->zoom * zoom_factor);
	world_after	 = scry_camera_screen_to_world(camera, screen_position, viewport);

	camera->center.x += world_before.x - world_after.x;
	camera->center.y += world_before.y - world_after.y;
}

void scry_camera_pan_pixels(scry_camera* camera, vec2s delta_pixels)
{
	ASSERT_FATAL(camera);

	const float zoom = scry_camera_safe_zoom(camera->zoom);

	camera->center.x -= delta_pixels.x / zoom;
	camera->center.y -= delta_pixels.y / zoom;
}

static float scry_camera_safe_zoom(float zoom)
{
	return zoom > 0.0f ? zoom : 1.0f;
}

static void scry_camera_validate_viewport(const scry_viewport* viewport)
{
	ASSERT_FATAL(viewport);
	ASSERT_FATAL(viewport->width > 0);
	ASSERT_FATAL(viewport->height > 0);
}
