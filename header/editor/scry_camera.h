#pragma once

#include <cglm/cglm.h>
#include <cglm/struct.h>

typedef struct scry_camera2d
{
	vec2s center;
	float zoom;
} scry_camera2d;

void  scry_camera2d_init(scry_camera2d* camera);
void  scry_camera2d_build_matrix(const scry_camera2d* camera, int viewport_width, int viewport_height, mat4 out_matrix);
vec2s scry_camera2d_screen_to_world(const scry_camera2d* camera, float screen_x, float screen_y, int viewport_width, int viewport_height);
void  scry_camera2d_zoom_at_screen(scry_camera2d* camera, float zoom_factor, float screen_x, float screen_y, int viewport_width, int viewport_height);
void  scry_camera2d_pan_pixels(scry_camera2d* camera, float delta_screen_x, float delta_screen_y);
