#pragma once

#include <cglm/cglm.h>
#include <cglm/struct.h>

typedef struct scry_viewport
{
	int width;
	int height;
} scry_viewport;

typedef struct scry_camera
{
	vec2s center;
	float zoom;
} scry_camera;

void  scry_camera_init(scry_camera* camera);
void  scry_camera_build_matrix(const scry_camera* camera, const scry_viewport* viewport, mat4 out_matrix);
vec2s scry_camera_screen_to_world(const scry_camera* camera, vec2s screen_position, const scry_viewport* viewport);
void  scry_camera_zoom_at_screen(scry_camera* camera, float zoom_factor, vec2s screen_position, const scry_viewport* viewport);
void  scry_camera_pan_pixels(scry_camera* camera, vec2s delta_pixels);
