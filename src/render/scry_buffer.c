#include "render/scry_buffer.h"

#include "utils/core/scry_assert.h"
#include "utils/core/scry_hints.h"

#include <glad/gl.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef union scry_buffer_clear_value
{
	uint32_t u32[4];
	float	 f32[4];
} scry_buffer_clear_value;

static GLenum		 scry_buffer_target_gl_enum(scry_buffer_target target);
static GLenum		 scry_buffer_usage_gl_enum(scry_buffer_usage usage);
static GLbitfield	 scry_buffer_storage_gl_flags(uint32_t storage_flags);
static GLbitfield	 scry_buffer_map_gl_flags(uint32_t map_flags);
static void			 scry_buffer_validate(const scry_buffer* buffer);
static void			 scry_buffer_validate_range(const scry_buffer* buffer, size_t offset, size_t size);
static void			 scry_buffer_validate_indexed_target(scry_buffer_target target);
static void			 scry_buffer_validate_map_request(const scry_buffer* buffer, uint32_t map_flags);
static NORETURN void scry_buffer_fail(const char* title, const char* detail);

static const char* const scry_buffer_target_names[SCRY_BUFFER_TARGET_COUNT] = {
	[SCRY_BUFFER_TARGET_ARRAY]				= "array",
	[SCRY_BUFFER_TARGET_ATOMIC_COUNTER]		= "atomic counter",
	[SCRY_BUFFER_TARGET_COPY_READ]			= "copy read",
	[SCRY_BUFFER_TARGET_COPY_WRITE]			= "copy write",
	[SCRY_BUFFER_TARGET_DISPATCH_INDIRECT]	= "dispatch indirect",
	[SCRY_BUFFER_TARGET_DRAW_INDIRECT]		= "draw indirect",
	[SCRY_BUFFER_TARGET_ELEMENT_ARRAY]		= "element array",
	[SCRY_BUFFER_TARGET_PIXEL_PACK]			= "pixel pack",
	[SCRY_BUFFER_TARGET_PIXEL_UNPACK]		= "pixel unpack",
	[SCRY_BUFFER_TARGET_QUERY]				= "query",
	[SCRY_BUFFER_TARGET_SHADER_STORAGE]		= "shader storage",
	[SCRY_BUFFER_TARGET_TEXTURE]			= "texture",
	[SCRY_BUFFER_TARGET_TRANSFORM_FEEDBACK] = "transform feedback",
	[SCRY_BUFFER_TARGET_UNIFORM]			= "uniform",
};

static const GLenum scry_buffer_target_gl_enums[SCRY_BUFFER_TARGET_COUNT] = {
	[SCRY_BUFFER_TARGET_ARRAY]				= GL_ARRAY_BUFFER,
	[SCRY_BUFFER_TARGET_ATOMIC_COUNTER]		= GL_ATOMIC_COUNTER_BUFFER,
	[SCRY_BUFFER_TARGET_COPY_READ]			= GL_COPY_READ_BUFFER,
	[SCRY_BUFFER_TARGET_COPY_WRITE]			= GL_COPY_WRITE_BUFFER,
	[SCRY_BUFFER_TARGET_DISPATCH_INDIRECT]	= GL_DISPATCH_INDIRECT_BUFFER,
	[SCRY_BUFFER_TARGET_DRAW_INDIRECT]		= GL_DRAW_INDIRECT_BUFFER,
	[SCRY_BUFFER_TARGET_ELEMENT_ARRAY]		= GL_ELEMENT_ARRAY_BUFFER,
	[SCRY_BUFFER_TARGET_PIXEL_PACK]			= GL_PIXEL_PACK_BUFFER,
	[SCRY_BUFFER_TARGET_PIXEL_UNPACK]		= GL_PIXEL_UNPACK_BUFFER,
	[SCRY_BUFFER_TARGET_QUERY]				= GL_QUERY_BUFFER,
	[SCRY_BUFFER_TARGET_SHADER_STORAGE]		= GL_SHADER_STORAGE_BUFFER,
	[SCRY_BUFFER_TARGET_TEXTURE]			= GL_TEXTURE_BUFFER,
	[SCRY_BUFFER_TARGET_TRANSFORM_FEEDBACK] = GL_TRANSFORM_FEEDBACK_BUFFER,
	[SCRY_BUFFER_TARGET_UNIFORM]			= GL_UNIFORM_BUFFER,
};

static const char* const scry_buffer_usage_names[SCRY_BUFFER_USAGE_COUNT] = {
	[SCRY_BUFFER_USAGE_STREAM_DRAW] = "stream draw",   [SCRY_BUFFER_USAGE_STREAM_READ] = "stream read",	  [SCRY_BUFFER_USAGE_STREAM_COPY] = "stream copy",
	[SCRY_BUFFER_USAGE_STATIC_DRAW] = "static draw",   [SCRY_BUFFER_USAGE_STATIC_READ] = "static read",	  [SCRY_BUFFER_USAGE_STATIC_COPY] = "static copy",
	[SCRY_BUFFER_USAGE_DYNAMIC_DRAW] = "dynamic draw", [SCRY_BUFFER_USAGE_DYNAMIC_READ] = "dynamic read", [SCRY_BUFFER_USAGE_DYNAMIC_COPY] = "dynamic copy",
};

static const GLenum scry_buffer_usage_gl_enums[SCRY_BUFFER_USAGE_COUNT] = {
	[SCRY_BUFFER_USAGE_STREAM_DRAW] = GL_STREAM_DRAW,	[SCRY_BUFFER_USAGE_STREAM_READ] = GL_STREAM_READ,	[SCRY_BUFFER_USAGE_STREAM_COPY] = GL_STREAM_COPY,
	[SCRY_BUFFER_USAGE_STATIC_DRAW] = GL_STATIC_DRAW,	[SCRY_BUFFER_USAGE_STATIC_READ] = GL_STATIC_READ,	[SCRY_BUFFER_USAGE_STATIC_COPY] = GL_STATIC_COPY,
	[SCRY_BUFFER_USAGE_DYNAMIC_DRAW] = GL_DYNAMIC_DRAW, [SCRY_BUFFER_USAGE_DYNAMIC_READ] = GL_DYNAMIC_READ, [SCRY_BUFFER_USAGE_DYNAMIC_COPY] = GL_DYNAMIC_COPY,
};

const char* scry_buffer_target_name(scry_buffer_target target)
{
	scry_buffer_target_validate(target);
	return scry_buffer_target_names[target];
}

const char* scry_buffer_usage_name(scry_buffer_usage usage)
{
	scry_buffer_usage_validate(usage);
	return scry_buffer_usage_names[usage];
}

void scry_buffer_target_validate(scry_buffer_target target)
{
	if ((uint32_t)target >= SCRY_BUFFER_TARGET_COUNT)
	{
		scry_buffer_fail("BUFFER TARGET FAILURE", "Invalid buffer target.");
	}
}

void scry_buffer_usage_validate(scry_buffer_usage usage)
{
	if ((uint32_t)usage >= SCRY_BUFFER_USAGE_COUNT)
	{
		scry_buffer_fail("BUFFER USAGE FAILURE", "Invalid buffer usage.");
	}
}

void scry_buffer_validate_storage_flags(uint32_t storage_flags)
{
	const uint32_t known_flags = SCRY_BUFFER_STORAGE_FLAG_DYNAMIC | SCRY_BUFFER_STORAGE_FLAG_MAP_READ | SCRY_BUFFER_STORAGE_FLAG_MAP_WRITE |
								 SCRY_BUFFER_STORAGE_FLAG_MAP_PERSISTENT | SCRY_BUFFER_STORAGE_FLAG_MAP_COHERENT | SCRY_BUFFER_STORAGE_FLAG_CLIENT;

	if ((storage_flags & ~known_flags) != 0U)
	{
		scry_buffer_fail("BUFFER STORAGE FAILURE", "Storage flags contain unknown bits.");
	}

	if ((storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_COHERENT) != 0U && (storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_PERSISTENT) == 0U)
	{
		scry_buffer_fail("BUFFER STORAGE FAILURE", "Coherent mapping requires persistent mapping.");
	}

	if ((storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_PERSISTENT) != 0U &&
		(storage_flags & (SCRY_BUFFER_STORAGE_FLAG_MAP_READ | SCRY_BUFFER_STORAGE_FLAG_MAP_WRITE)) == 0U)
	{
		scry_buffer_fail("BUFFER STORAGE FAILURE", "Persistent mapping requires read or write mapping.");
	}
}

void scry_buffer_validate_map_flags(uint32_t map_flags)
{
	const uint32_t known_flags = SCRY_BUFFER_MAP_FLAG_READ | SCRY_BUFFER_MAP_FLAG_WRITE | SCRY_BUFFER_MAP_FLAG_PERSISTENT | SCRY_BUFFER_MAP_FLAG_COHERENT |
								 SCRY_BUFFER_MAP_FLAG_INVALIDATE_RANGE | SCRY_BUFFER_MAP_FLAG_INVALIDATE_BUFFER | SCRY_BUFFER_MAP_FLAG_FLUSH_EXPLICIT |
								 SCRY_BUFFER_MAP_FLAG_UNSYNCHRONIZED;

	if ((map_flags & ~known_flags) != 0U)
	{
		scry_buffer_fail("BUFFER MAP FAILURE", "Map flags contain unknown bits.");
	}

	if ((map_flags & (SCRY_BUFFER_MAP_FLAG_READ | SCRY_BUFFER_MAP_FLAG_WRITE)) == 0U)
	{
		scry_buffer_fail("BUFFER MAP FAILURE", "Map flags must include read or write access.");
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_COHERENT) != 0U && (map_flags & SCRY_BUFFER_MAP_FLAG_PERSISTENT) == 0U)
	{
		scry_buffer_fail("BUFFER MAP FAILURE", "Coherent mapping requires persistent mapping.");
	}
}

void scry_buffer_init(scry_buffer* buffer)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(buffer->handle == 0U);
	ASSERT_FATAL(buffer->size == 0U);
	ASSERT_FATAL(buffer->storage_flags == 0U);
	ASSERT_FATAL(buffer->active_map_flags == 0U);
	ASSERT_FATAL(!buffer->immutable);
	ASSERT_FATAL(!buffer->mapped);

	glCreateBuffers(1, &buffer->handle);

	if (buffer->handle == 0U)
	{
		scry_buffer_fail("BUFFER CREATION FAILURE", "glCreateBuffers returned 0.");
	}
}

void scry_buffer_destroy(scry_buffer* buffer)
{
	scry_buffer_validate(buffer);
	ASSERT_FATAL(!buffer->mapped);

	glDeleteBuffers(1, &buffer->handle);

	buffer->handle			 = 0U;
	buffer->size			 = 0U;
	buffer->storage_flags	 = 0U;
	buffer->active_map_flags = 0U;
	buffer->immutable		 = false;
	buffer->mapped			 = false;
}

void scry_buffer_set_label(const scry_buffer* buffer, const char* label)
{
	scry_buffer_validate(buffer);
	ASSERT_FATAL(label);

	glObjectLabel(GL_BUFFER, buffer->handle, -1, label);
}

void scry_buffer_alloc(scry_buffer* buffer, size_t size, const void* data, scry_buffer_usage usage)
{
	scry_buffer_validate(buffer);
	scry_buffer_usage_validate(usage);
	ASSERT_FATAL(!buffer->immutable);
	ASSERT_FATAL(!buffer->mapped);
	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(data || size == 0U);

	glNamedBufferData(buffer->handle, (GLsizeiptr)size, data, scry_buffer_usage_gl_enum(usage));
	buffer->size = size;
}

void scry_buffer_storage(scry_buffer* buffer, size_t size, const void* data, uint32_t storage_flags)
{
	scry_buffer_validate(buffer);
	scry_buffer_validate_storage_flags(storage_flags);
	ASSERT_FATAL(buffer->size == 0U);
	ASSERT_FATAL(!buffer->immutable);
	ASSERT_FATAL(!buffer->mapped);
	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(data || size == 0U);

	glNamedBufferStorage(buffer->handle, (GLsizeiptr)size, data, scry_buffer_storage_gl_flags(storage_flags));

	buffer->size			 = size;
	buffer->storage_flags	 = storage_flags;
	buffer->active_map_flags = 0U;
	buffer->immutable		 = true;
}

void scry_buffer_write(const scry_buffer* buffer, size_t offset, size_t size, const void* data)
{
	scry_buffer_validate(buffer);
	scry_buffer_validate_range(buffer, offset, size);

	ASSERT_FATAL(data || size == 0U);

	if (size == 0U)
	{
		return;
	}

	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(offset <= (size_t)PTRDIFF_MAX);

	glNamedBufferSubData(buffer->handle, (GLintptr)offset, (GLsizeiptr)size, data);
}

void scry_buffer_read(const scry_buffer* buffer, size_t offset, size_t size, void* out_data)
{
	scry_buffer_validate(buffer);
	ASSERT_FATAL(out_data || size == 0U);

	if (size == 0U)
	{
		ASSERT_FATAL(offset <= buffer->size);
		return;
	}

	scry_buffer_validate_range(buffer, offset, size);
	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(offset <= (size_t)PTRDIFF_MAX);

	glGetNamedBufferSubData(buffer->handle, (GLintptr)offset, (GLsizeiptr)size, out_data);
}

void scry_buffer_copy(const scry_buffer* src_buffer, size_t src_offset, scry_buffer* dst_buffer, size_t dst_offset, size_t size)
{
	scry_buffer_validate(src_buffer);
	scry_buffer_validate(dst_buffer);

	if (size == 0U)
	{
		ASSERT_FATAL(src_offset <= src_buffer->size);
		ASSERT_FATAL(dst_offset <= dst_buffer->size);
		return;
	}

	scry_buffer_validate_range(src_buffer, src_offset, size);
	scry_buffer_validate_range(dst_buffer, dst_offset, size);
	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(src_offset <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(dst_offset <= (size_t)PTRDIFF_MAX);

	glCopyNamedBufferSubData(src_buffer->handle, dst_buffer->handle, (GLintptr)src_offset, (GLintptr)dst_offset, (GLsizeiptr)size);
}

void scry_buffer_clear_u32(scry_buffer* buffer, uint32_t value)
{
	scry_buffer_clear_value clear_value = { 0 };

	scry_buffer_validate(buffer);

	if (buffer->size == 0U)
	{
		return;
	}

	ASSERT_FATAL(buffer->size <= (size_t)PTRDIFF_MAX);
	clear_value.u32[0] = value;
	clear_value.u32[1] = value;
	clear_value.u32[2] = value;
	clear_value.u32[3] = value;

	glClearNamedBufferData(buffer->handle, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, clear_value.u32);
}

void scry_buffer_clear_range_u32(scry_buffer* buffer, size_t offset, size_t size, uint32_t value)
{
	scry_buffer_clear_value clear_value = { 0 };

	scry_buffer_validate(buffer);

	if (size == 0U)
	{
		ASSERT_FATAL(offset <= buffer->size);
		return;
	}

	scry_buffer_validate_range(buffer, offset, size);
	ASSERT_FATAL(offset <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);

	clear_value.u32[0] = value;
	clear_value.u32[1] = value;
	clear_value.u32[2] = value;
	clear_value.u32[3] = value;

	glClearNamedBufferSubData(buffer->handle, GL_R32UI, (GLintptr)offset, (GLsizeiptr)size, GL_RED_INTEGER, GL_UNSIGNED_INT, clear_value.u32);
}

void scry_buffer_clear_f32(scry_buffer* buffer, float value)
{
	scry_buffer_clear_value clear_value = { 0 };

	scry_buffer_validate(buffer);

	if (buffer->size == 0U)
	{
		return;
	}

	ASSERT_FATAL(buffer->size <= (size_t)PTRDIFF_MAX);
	clear_value.f32[0] = value;
	clear_value.f32[1] = value;
	clear_value.f32[2] = value;
	clear_value.f32[3] = value;

	glClearNamedBufferData(buffer->handle, GL_R32F, GL_RED, GL_FLOAT, clear_value.f32);
}

void scry_buffer_clear_range_f32(scry_buffer* buffer, size_t offset, size_t size, float value)
{
	scry_buffer_clear_value clear_value = { 0 };

	scry_buffer_validate(buffer);

	if (size == 0U)
	{
		ASSERT_FATAL(offset <= buffer->size);
		return;
	}

	scry_buffer_validate_range(buffer, offset, size);
	ASSERT_FATAL(offset <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);

	clear_value.f32[0] = value;
	clear_value.f32[1] = value;
	clear_value.f32[2] = value;
	clear_value.f32[3] = value;

	glClearNamedBufferSubData(buffer->handle, GL_R32F, (GLintptr)offset, (GLsizeiptr)size, GL_RED, GL_FLOAT, clear_value.f32);
}

void* scry_buffer_map(scry_buffer* buffer, size_t offset, size_t size, uint32_t map_flags)
{
	scry_buffer_validate(buffer);
	scry_buffer_validate_range(buffer, offset, size);

	ASSERT_FATAL(!buffer->mapped);

	if (size == 0U)
	{
		return NULL;
	}

	scry_buffer_validate_map_request(buffer, map_flags);

	ASSERT_FATAL(offset <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);

	void* const mapped = glMapNamedBufferRange(buffer->handle, (GLintptr)offset, (GLsizeiptr)size, scry_buffer_map_gl_flags(map_flags));
	ASSERT_FATAL(mapped);

	buffer->active_map_flags = map_flags;
	buffer->mapped			 = true;

	return mapped;
}

void scry_buffer_flush(const scry_buffer* buffer, size_t offset, size_t size)
{
	scry_buffer_validate(buffer);
	scry_buffer_validate_range(buffer, offset, size);

	ASSERT_FATAL(buffer->mapped);
	ASSERT_FATAL((buffer->active_map_flags & SCRY_BUFFER_MAP_FLAG_WRITE) != 0U);
	ASSERT_FATAL((buffer->active_map_flags & SCRY_BUFFER_MAP_FLAG_FLUSH_EXPLICIT) != 0U);

	if (size == 0U)
	{
		return;
	}

	ASSERT_FATAL(offset <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);

	glFlushMappedNamedBufferRange(buffer->handle, (GLintptr)offset, (GLsizeiptr)size);
}

void scry_buffer_unmap(scry_buffer* buffer)
{
	scry_buffer_validate(buffer);
	ASSERT_FATAL(buffer->mapped);

	if ((buffer->active_map_flags & SCRY_BUFFER_MAP_FLAG_PERSISTENT) != 0U)
	{
		ASSERT_FATAL(false && "Persistent buffers should not be unmapped.");
	}

	if (glUnmapNamedBuffer(buffer->handle) != GL_TRUE)
	{
		scry_buffer_fail("BUFFER MAP FAILURE", "glUnmapNamedBuffer reported buffer corruption.");
	}

	buffer->active_map_flags = 0U;
	buffer->mapped			 = false;
}

void scry_buffer_bind(const scry_buffer* buffer, scry_buffer_target target)
{
	scry_buffer_validate(buffer);
	scry_buffer_target_validate(target);

	glBindBuffer(scry_buffer_target_gl_enum(target), buffer->handle);
}

void scry_buffer_bind_base(const scry_buffer* buffer, scry_buffer_target target, unsigned int index)
{
	scry_buffer_validate(buffer);
	scry_buffer_target_validate(target);
	scry_buffer_validate_indexed_target(target);

	glBindBufferBase(scry_buffer_target_gl_enum(target), index, buffer->handle);
}

void scry_buffer_bind_range(const scry_buffer* buffer, scry_buffer_target target, unsigned int index, size_t offset, size_t size)
{
	scry_buffer_validate(buffer);
	scry_buffer_target_validate(target);
	scry_buffer_validate_indexed_target(target);

	ASSERT_FATAL(size > 0U);

	scry_buffer_validate_range(buffer, offset, size);

	ASSERT_FATAL(offset <= (size_t)PTRDIFF_MAX);
	ASSERT_FATAL(size <= (size_t)PTRDIFF_MAX);

	glBindBufferRange(scry_buffer_target_gl_enum(target), index, buffer->handle, (GLintptr)offset, (GLsizeiptr)size);
}

static GLenum scry_buffer_target_gl_enum(scry_buffer_target target)
{
	scry_buffer_target_validate(target);
	return scry_buffer_target_gl_enums[target];
}

static GLenum scry_buffer_usage_gl_enum(scry_buffer_usage usage)
{
	scry_buffer_usage_validate(usage);
	return scry_buffer_usage_gl_enums[usage];
}

static GLbitfield scry_buffer_storage_gl_flags(uint32_t storage_flags)
{
	scry_buffer_validate_storage_flags(storage_flags);

	GLbitfield gl_flags = 0U;

	if ((storage_flags & SCRY_BUFFER_STORAGE_FLAG_DYNAMIC) != 0U)
	{
		gl_flags |= GL_DYNAMIC_STORAGE_BIT;
	}

	if ((storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_READ) != 0U)
	{
		gl_flags |= GL_MAP_READ_BIT;
	}

	if ((storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_WRITE) != 0U)
	{
		gl_flags |= GL_MAP_WRITE_BIT;
	}

	if ((storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_PERSISTENT) != 0U)
	{
		gl_flags |= GL_MAP_PERSISTENT_BIT;
	}

	if ((storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_COHERENT) != 0U)
	{
		gl_flags |= GL_MAP_COHERENT_BIT;
	}

	if ((storage_flags & SCRY_BUFFER_STORAGE_FLAG_CLIENT) != 0U)
	{
		gl_flags |= GL_CLIENT_STORAGE_BIT;
	}

	return gl_flags;
}

static GLbitfield scry_buffer_map_gl_flags(uint32_t map_flags)
{
	scry_buffer_validate_map_flags(map_flags);

	GLbitfield gl_flags = 0U;

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_READ) != 0U)
	{
		gl_flags |= GL_MAP_READ_BIT;
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_WRITE) != 0U)
	{
		gl_flags |= GL_MAP_WRITE_BIT;
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_PERSISTENT) != 0U)
	{
		gl_flags |= GL_MAP_PERSISTENT_BIT;
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_COHERENT) != 0U)
	{
		gl_flags |= GL_MAP_COHERENT_BIT;
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_INVALIDATE_RANGE) != 0U)
	{
		gl_flags |= GL_MAP_INVALIDATE_RANGE_BIT;
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_INVALIDATE_BUFFER) != 0U)
	{
		gl_flags |= GL_MAP_INVALIDATE_BUFFER_BIT;
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_FLUSH_EXPLICIT) != 0U)
	{
		gl_flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_UNSYNCHRONIZED) != 0U)
	{
		gl_flags |= GL_MAP_UNSYNCHRONIZED_BIT;
	}

	return gl_flags;
}

static void scry_buffer_validate(const scry_buffer* buffer)
{
	ASSERT_FATAL(buffer);
	ASSERT_FATAL(buffer->handle != 0U);
}

static void scry_buffer_validate_range(const scry_buffer* buffer, size_t offset, size_t size)
{
	scry_buffer_validate(buffer);
	ASSERT_FATAL(offset <= buffer->size);

	if (size == 0U)
	{
		return;
	}

	ASSERT_FATAL(size <= buffer->size - offset);
}

static void scry_buffer_validate_indexed_target(scry_buffer_target target)
{
	switch (target)
	{
		case SCRY_BUFFER_TARGET_ATOMIC_COUNTER:
		case SCRY_BUFFER_TARGET_SHADER_STORAGE:
		case SCRY_BUFFER_TARGET_TRANSFORM_FEEDBACK:
		case SCRY_BUFFER_TARGET_UNIFORM:
			return;
		case SCRY_BUFFER_TARGET_ARRAY:
		case SCRY_BUFFER_TARGET_COPY_READ:
		case SCRY_BUFFER_TARGET_COPY_WRITE:
		case SCRY_BUFFER_TARGET_DISPATCH_INDIRECT:
		case SCRY_BUFFER_TARGET_DRAW_INDIRECT:
		case SCRY_BUFFER_TARGET_ELEMENT_ARRAY:
		case SCRY_BUFFER_TARGET_PIXEL_PACK:
		case SCRY_BUFFER_TARGET_PIXEL_UNPACK:
		case SCRY_BUFFER_TARGET_QUERY:
		case SCRY_BUFFER_TARGET_TEXTURE:
		case SCRY_BUFFER_TARGET_COUNT:
			break;
		default:
			break;
	}

	scry_buffer_fail("BUFFER BIND FAILURE", "Target does not support indexed buffer binding.");
}

static void scry_buffer_validate_map_request(const scry_buffer* buffer, uint32_t map_flags)
{
	scry_buffer_validate(buffer);
	scry_buffer_validate_map_flags(map_flags);

	if (!buffer->immutable)
	{
		return;
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_READ) != 0U && (buffer->storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_READ) == 0U)
	{
		scry_buffer_fail("BUFFER MAP FAILURE", "Immutable buffer was not created with read mapping support.");
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_WRITE) != 0U && (buffer->storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_WRITE) == 0U)
	{
		scry_buffer_fail("BUFFER MAP FAILURE", "Immutable buffer was not created with write mapping support.");
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_PERSISTENT) != 0U && (buffer->storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_PERSISTENT) == 0U)
	{
		scry_buffer_fail("BUFFER MAP FAILURE", "Immutable buffer was not created with persistent mapping support.");
	}

	if ((map_flags & SCRY_BUFFER_MAP_FLAG_COHERENT) != 0U && (buffer->storage_flags & SCRY_BUFFER_STORAGE_FLAG_MAP_COHERENT) == 0U)
	{
		scry_buffer_fail("BUFFER MAP FAILURE", "Immutable buffer was not created with coherent mapping support.");
	}
}

static NORETURN void scry_buffer_fail(const char* title, const char* detail)
{
	ASSERT_FATAL(title);
	ASSERT_FATAL(detail);

	flockfile(stderr);
	fputs("\n+====================================================================+\n", stderr);
	fprintf(stderr, "| %-66s |\n", title);
	fputs("+====================================================================+\n", stderr);
	fprintf(stderr, "| %s\n", detail);
	fputs("+====================================================================+\n", stderr);
	funlockfile(stderr);

	abort();
	UNREACHABLE;
}
