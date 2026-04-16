#include "render/scry_shader.h"

#include "utils/core/scry_assert.h"
#include "utils/core/scry_hints.h"

#include <glad/gl.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCRY_SHADER_LOG_BUFFER_SIZE 16384

typedef struct scry_gl_shader_stage
{
	unsigned int	  handle;
	scry_shader_stage stage;
} scry_gl_shader_stage;

static GLenum		 scry_shader_stage_gl_enum(scry_shader_stage stage);
static void			 scry_shader_compile_stage(const scry_shader_stage_config* stage_config, scry_gl_shader_stage* compiled_stage);
static void			 scry_shader_link_program(scry_shader* shader, scry_gl_shader_stage* compiled_stages, size_t stage_count);
static void			 scry_shader_append_line(char* buffer, size_t buffer_size, size_t* offset, const char* fmt, ...) __attribute__((format(printf, 4, 5)));
static NORETURN void scry_shader_fail_message(const char* title, const char* message);
static NORETURN void scry_shader_fail_stage_log(const char* title, const scry_shader_stage_config* stage_config, const char* detail, char* info_log);
static NORETURN void scry_shader_fail_program_log(const char* title, uint32_t stage_mask, char* info_log);

static const GLenum scry_shader_stage_gl_enums[SCRY_SHADER_STAGE_COUNT] = {
	[SCRY_SHADER_STAGE_VERTEX]			= GL_VERTEX_SHADER,
	[SCRY_SHADER_STAGE_TESS_CONTROL]	= GL_TESS_CONTROL_SHADER,
	[SCRY_SHADER_STAGE_TESS_EVALUATION] = GL_TESS_EVALUATION_SHADER,
	[SCRY_SHADER_STAGE_GEOMETRY]		= GL_GEOMETRY_SHADER,
	[SCRY_SHADER_STAGE_FRAGMENT]		= GL_FRAGMENT_SHADER,
	[SCRY_SHADER_STAGE_COMPUTE]			= GL_COMPUTE_SHADER,
};

void scry_shader_init(scry_shader* shader, const scry_shader_stage_config* stage_configs, size_t stage_count)
{
	ASSERT_FATAL(shader);
	ASSERT_FATAL(shader->handle == 0U);

	scry_gl_shader_stage* compiled_stages = NULL;
	uint32_t			  stage_mask	  = 0U;

	scry_shader_stage_validate_configs(stage_configs, stage_count);
	ASSERT_FATAL(shader->stage_mask == 0U);

	compiled_stages = calloc(stage_count, sizeof(*compiled_stages));
	ASSERT_FATAL(compiled_stages);

	for (size_t i = 0U; i < stage_count; ++i)
	{
		scry_shader_compile_stage(&stage_configs[i], &compiled_stages[i]);
		stage_mask |= scry_shader_stage_bit(stage_configs[i].stage);
	}

	shader->stage_mask = stage_mask;
	scry_shader_link_program(shader, compiled_stages, stage_count);

	free(compiled_stages);
}

void scry_shader_destroy(scry_shader* shader)
{
	ASSERT_FATAL(shader);
	ASSERT_FATAL(shader->handle != 0U);
	ASSERT_FATAL(shader->stage_mask != 0U);

	glDeleteProgram(shader->handle);
	shader->handle	   = 0U;
	shader->stage_mask = 0U;
}

void scry_shader_bind(const scry_shader* shader)
{
	ASSERT_FATAL(shader);
	ASSERT_FATAL(shader->handle != 0U);
	ASSERT_FATAL(shader->stage_mask != 0U);

	glUseProgram(shader->handle);
}

void scry_shader_unbind(void)
{
	glUseProgram(0U);
}

static GLenum scry_shader_stage_gl_enum(scry_shader_stage stage)
{
	scry_shader_stage_validate(stage);
	return scry_shader_stage_gl_enums[stage];
}

static void scry_shader_compile_stage(const scry_shader_stage_config* stage_config, scry_gl_shader_stage* compiled_stage)
{
	ASSERT_FATAL(stage_config);
	ASSERT_FATAL(compiled_stage);
	ASSERT_FATAL(stage_config->src);

	GLint compile_status  = GL_FALSE;
	GLint info_log_length = 0;
	char* info_log		  = NULL;

	compiled_stage->stage  = stage_config->stage;
	compiled_stage->handle = glCreateShader(scry_shader_stage_gl_enum(stage_config->stage));

	if (compiled_stage->handle == 0U)
	{
		scry_shader_fail_stage_log("SHADER STAGE CREATION FAILURE", stage_config, "glCreateShader returned 0.", NULL);
	}

	const GLchar* gl_src = stage_config->src;

	glShaderSource(compiled_stage->handle, 1, &gl_src, NULL);
	glCompileShader(compiled_stage->handle);
	glGetShaderiv(compiled_stage->handle, GL_COMPILE_STATUS, &compile_status);

	if (compile_status == GL_TRUE)
	{
		return;
	}

	glGetShaderiv(compiled_stage->handle, GL_INFO_LOG_LENGTH, &info_log_length);

	if (info_log_length > 0)
	{
		info_log = malloc((size_t)info_log_length + 1U);
		ASSERT_FATAL(info_log);

		glGetShaderInfoLog(compiled_stage->handle, info_log_length, NULL, info_log);
		info_log[info_log_length] = '\0';
	}

	glDeleteShader(compiled_stage->handle);

	scry_shader_fail_stage_log("SHADER COMPILATION FAILURE", stage_config, "OpenGL rejected the shader stage source.", info_log);
}

static void scry_shader_link_program(scry_shader* shader, scry_gl_shader_stage* compiled_stages, size_t stage_count)
{
	ASSERT_FATAL(shader);
	ASSERT_FATAL(compiled_stages);
	ASSERT_FATAL(stage_count > 0U);

	GLint link_status	  = GL_FALSE;
	GLint info_log_length = 0;
	char* info_log		  = NULL;

	shader->handle = glCreateProgram();

	for (size_t i = 0; i < stage_count; ++i)
	{
		glAttachShader(shader->handle, compiled_stages[i].handle);
	}

	glLinkProgram(shader->handle);
	glGetProgramiv(shader->handle, GL_LINK_STATUS, &link_status);

	if (link_status == GL_TRUE)
	{
		for (size_t i = 0; i < stage_count; ++i)
		{
			glDetachShader(shader->handle, compiled_stages[i].handle);
			glDeleteShader(compiled_stages[i].handle);
		}

		return;
	}

	glGetProgramiv(shader->handle, GL_INFO_LOG_LENGTH, &info_log_length);

	if (info_log_length > 0)
	{
		info_log = malloc((size_t)info_log_length + 1U);
		ASSERT_FATAL(info_log);

		glGetProgramInfoLog(shader->handle, info_log_length, NULL, info_log);
		info_log[info_log_length] = '\0';
	}

	for (size_t i = 0; i < stage_count; ++i)
	{
		glDetachShader(shader->handle, compiled_stages[i].handle);
		glDeleteShader(compiled_stages[i].handle);
	}

	glDeleteProgram(shader->handle);
	shader->handle = 0U;

	scry_shader_fail_program_log("SHADER PROGRAM LINK FAILURE", shader->stage_mask, info_log);
}

static void scry_shader_append_line(char* buffer, size_t buffer_size, size_t* offset, const char* fmt, ...)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(offset);
	ASSERT_FATAL(fmt);
	ASSERT_FATAL(buffer_size > 0U);
	ASSERT_FATAL(*offset < buffer_size);

	va_list args;
	va_start(args, fmt);
	const int written = vsnprintf(buffer + *offset, buffer_size - *offset, fmt, args);
	va_end(args);

	ASSERT_FATAL(written >= 0);

	const size_t remaining = buffer_size - *offset;
	const size_t advance   = (size_t)written < remaining ? (size_t)written : remaining - 1U;

	*offset += advance;
	ASSERT_FATAL(*offset < buffer_size);
}

static NORETURN void scry_shader_fail_message(const char* title, const char* message)
{
	ASSERT_FATAL(title);
	ASSERT_FATAL(message);

	flockfile(stderr);

	fputs("\n+====================================================================+\n", stderr);
	fprintf(stderr, "| %-66s |\n", title);
	fputs("+====================================================================+\n", stderr);
	fputs(message, stderr);
	fputs("+====================================================================+\n", stderr);

	funlockfile(stderr);

	abort();
	UNREACHABLE;
}

static NORETURN void scry_shader_fail_stage_log(const char* title, const scry_shader_stage_config* stage_config, const char* detail, char* info_log)
{
	ASSERT_FATAL(title);
	ASSERT_FATAL(stage_config);
	ASSERT_FATAL(detail);

	char   message[SCRY_SHADER_LOG_BUFFER_SIZE];
	size_t offset = 0U;

	scry_shader_append_line(message, sizeof(message), &offset, "| Stage:      %s\n", scry_shader_stage_name(stage_config->stage));
	scry_shader_append_line(message, sizeof(message), &offset, "| Debug Name: %s\n", stage_config->name ? stage_config->name : "<none>");
	scry_shader_append_line(message, sizeof(message), &offset, "| Detail:     %s\n", detail);
	scry_shader_append_line(message, sizeof(message), &offset, "|\n");
	scry_shader_append_line(message, sizeof(message), &offset, "| OpenGL Info Log:\n");

	scry_shader_append_line(message,
							sizeof(message),
							&offset,
							"%s%s\n",
							info_log ? info_log : "<no info log provided>",
							(info_log && info_log[0] != '\0' && info_log[strlen(info_log) - 1] == '\n') ? "" : "\n");

	free(info_log);

	scry_shader_fail_message(title, message);
}

static NORETURN void scry_shader_fail_program_log(const char* title, uint32_t stage_mask, char* info_log)
{
	ASSERT_FATAL(title);

	char   message[SCRY_SHADER_LOG_BUFFER_SIZE];
	size_t offset = 0U;

	scry_shader_append_line(message, sizeof(message), &offset, "| Stage Mask: 0x%X\n", stage_mask);
	scry_shader_append_line(message, sizeof(message), &offset, "| Detail:     OpenGL rejected the linked shader program.\n");
	scry_shader_append_line(message, sizeof(message), &offset, "|\n");
	scry_shader_append_line(message, sizeof(message), &offset, "| OpenGL Info Log:\n");

	scry_shader_append_line(message,
							sizeof(message),
							&offset,
							"%s%s\n",
							info_log ? info_log : "<no info log provided>",
							(info_log && info_log[0] != '\0' && info_log[strlen(info_log) - 1] == '\n') ? "" : "\n");

	free(info_log);

	scry_shader_fail_message(title, message);
}
