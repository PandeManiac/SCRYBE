#include "render/scry_shader_stage.h"

#include "utils/core/scry_assert.h"
#include "utils/core/scry_hints.h"

#include <stdio.h>
#include <stdlib.h>

static NORETURN void scry_shader_stage_fail(const char* message);

static const char* const scry_shader_stage_names[SCRY_SHADER_STAGE_COUNT] = {
	[SCRY_SHADER_STAGE_VERTEX]			= "vertex",
	[SCRY_SHADER_STAGE_TESS_CONTROL]	= "tessellation control",
	[SCRY_SHADER_STAGE_TESS_EVALUATION] = "tessellation evaluation",
	[SCRY_SHADER_STAGE_GEOMETRY]		= "geometry",
	[SCRY_SHADER_STAGE_FRAGMENT]		= "fragment",
	[SCRY_SHADER_STAGE_COMPUTE]			= "compute",
};

uint32_t scry_shader_stage_bit(scry_shader_stage stage)
{
	scry_shader_stage_validate(stage);
	return 1U << (uint32_t)stage;
}

const char* scry_shader_stage_name(scry_shader_stage stage)
{
	scry_shader_stage_validate(stage);
	return scry_shader_stage_names[stage];
}

void scry_shader_stage_validate(scry_shader_stage stage)
{
	if ((uint32_t)stage >= SCRY_SHADER_STAGE_COUNT)
	{
		scry_shader_stage_fail("Invalid shader stage.");
	}
}

void scry_shader_stage_validate_mask(uint32_t stage_mask)
{
	const uint32_t known_stage_mask = ((uint32_t)1U << SCRY_SHADER_STAGE_COUNT) - 1U;

	if (stage_mask == 0U)
	{
		scry_shader_stage_fail("Shader stage mask must not be empty.");
	}

	if ((stage_mask & ~known_stage_mask) != 0U)
	{
		scry_shader_stage_fail("Shader stage mask contains unknown stage bits.");
	}

	if ((stage_mask & SCRY_SHADER_STAGE_MASK_COMPUTE) != 0U && stage_mask != SCRY_SHADER_STAGE_MASK_COMPUTE)
	{
		scry_shader_stage_fail("Compute shader stage cannot be combined with graphics shader stages.");
	}
}

void scry_shader_stage_validate_configs(const scry_shader_stage_config* configs, size_t config_count)
{
	uint32_t stage_mask = 0U;

	ASSERT_FATAL(configs);

	if (config_count == 0U)
	{
		scry_shader_stage_fail("Shader init requires at least one stage config.");
	}

	for (size_t i = 0U; i < config_count; ++i)
	{
		const scry_shader_stage_config* config	  = &configs[i];
		const uint32_t					stage_bit = scry_shader_stage_bit(config->stage);

		ASSERT_FATAL(config->src);

		if ((stage_mask & stage_bit) != 0U)
		{
			scry_shader_stage_fail("Shader init received duplicate shader stages.");
		}

		stage_mask |= stage_bit;
	}

	scry_shader_stage_validate_mask(stage_mask);
}

static NORETURN void scry_shader_stage_fail(const char* message)
{
	ASSERT_FATAL(message);

	flockfile(stderr);
	fputs("\n+-------------------------------------+\n", stderr);
	fputs("| SHADER STAGE CONFIGURATION FAILURE  |\n", stderr);
	fputs("+-------------------------------------+\n", stderr);
	fprintf(stderr, "| %s\n", message);
	fputs("+-------------------------------------+\n", stderr);
	funlockfile(stderr);

	abort();
	UNREACHABLE;
}
