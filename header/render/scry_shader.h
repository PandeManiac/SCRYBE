#pragma once

#include "render/scry_shader_stage.h"

#include <stdint.h>

typedef struct scry_shader
{
	unsigned int handle;
	uint32_t	 stage_mask;
} scry_shader;

void scry_shader_init(scry_shader* shader, const scry_shader_stage_config* stage_configs, size_t stage_count);
void scry_shader_destroy(scry_shader* shader);
void scry_shader_bind(const scry_shader* shader);
void scry_shader_unbind(void);
