#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum scry_shader_stage
{
	SCRY_SHADER_STAGE_VERTEX = 0,
	SCRY_SHADER_STAGE_TESS_CONTROL,
	SCRY_SHADER_STAGE_TESS_EVALUATION,
	SCRY_SHADER_STAGE_GEOMETRY,
	SCRY_SHADER_STAGE_FRAGMENT,
	SCRY_SHADER_STAGE_COMPUTE,
	SCRY_SHADER_STAGE_COUNT
} scry_shader_stage;

typedef enum scry_shader_stage_mask
{
	SCRY_SHADER_STAGE_MASK_NONE			   = 0,
	SCRY_SHADER_STAGE_MASK_VERTEX		   = 1 << SCRY_SHADER_STAGE_VERTEX,
	SCRY_SHADER_STAGE_MASK_TESS_CONTROL	   = 1 << SCRY_SHADER_STAGE_TESS_CONTROL,
	SCRY_SHADER_STAGE_MASK_TESS_EVALUATION = 1 << SCRY_SHADER_STAGE_TESS_EVALUATION,
	SCRY_SHADER_STAGE_MASK_GEOMETRY		   = 1 << SCRY_SHADER_STAGE_GEOMETRY,
	SCRY_SHADER_STAGE_MASK_FRAGMENT		   = 1 << SCRY_SHADER_STAGE_FRAGMENT,
	SCRY_SHADER_STAGE_MASK_COMPUTE		   = 1 << SCRY_SHADER_STAGE_COMPUTE
} scry_shader_stage_mask;

typedef struct scry_shader_stage_config
{
	scry_shader_stage stage;
	const char*		  name;
	const char*		  src;
} scry_shader_stage_config;

uint32_t	scry_shader_stage_bit(scry_shader_stage stage);
const char* scry_shader_stage_name(scry_shader_stage stage);
void		scry_shader_stage_validate(scry_shader_stage stage);
void		scry_shader_stage_validate_mask(uint32_t stage_mask);
void		scry_shader_stage_validate_configs(const scry_shader_stage_config* configs, size_t config_count);
