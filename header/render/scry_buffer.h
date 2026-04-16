#pragma once

#include <glad/gl.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum scry_buffer_target
{
	SCRY_BUFFER_TARGET_ARRAY = 0,
	SCRY_BUFFER_TARGET_ATOMIC_COUNTER,
	SCRY_BUFFER_TARGET_COPY_READ,
	SCRY_BUFFER_TARGET_COPY_WRITE,
	SCRY_BUFFER_TARGET_DISPATCH_INDIRECT,
	SCRY_BUFFER_TARGET_DRAW_INDIRECT,
	SCRY_BUFFER_TARGET_ELEMENT_ARRAY,
	SCRY_BUFFER_TARGET_PIXEL_PACK,
	SCRY_BUFFER_TARGET_PIXEL_UNPACK,
	SCRY_BUFFER_TARGET_QUERY,
	SCRY_BUFFER_TARGET_SHADER_STORAGE,
	SCRY_BUFFER_TARGET_TEXTURE,
	SCRY_BUFFER_TARGET_TRANSFORM_FEEDBACK,
	SCRY_BUFFER_TARGET_UNIFORM,
	SCRY_BUFFER_TARGET_COUNT
} scry_buffer_target;

typedef enum scry_buffer_usage
{
	SCRY_BUFFER_USAGE_STREAM_DRAW = 0,
	SCRY_BUFFER_USAGE_STREAM_READ,
	SCRY_BUFFER_USAGE_STREAM_COPY,
	SCRY_BUFFER_USAGE_STATIC_DRAW,
	SCRY_BUFFER_USAGE_STATIC_READ,
	SCRY_BUFFER_USAGE_STATIC_COPY,
	SCRY_BUFFER_USAGE_DYNAMIC_DRAW,
	SCRY_BUFFER_USAGE_DYNAMIC_READ,
	SCRY_BUFFER_USAGE_DYNAMIC_COPY,
	SCRY_BUFFER_USAGE_COUNT
} scry_buffer_usage;

typedef enum scry_buffer_storage_flags
{
	SCRY_BUFFER_STORAGE_FLAGS_NONE			= 0,
	SCRY_BUFFER_STORAGE_FLAG_DYNAMIC		= 1 << 0,
	SCRY_BUFFER_STORAGE_FLAG_MAP_READ		= 1 << 1,
	SCRY_BUFFER_STORAGE_FLAG_MAP_WRITE		= 1 << 2,
	SCRY_BUFFER_STORAGE_FLAG_MAP_PERSISTENT = 1 << 3,
	SCRY_BUFFER_STORAGE_FLAG_MAP_COHERENT	= 1 << 4,
	SCRY_BUFFER_STORAGE_FLAG_CLIENT			= 1 << 5
} scry_buffer_storage_flags;

typedef enum scry_buffer_map_flags
{
	SCRY_BUFFER_MAP_FLAGS_NONE			   = 0,
	SCRY_BUFFER_MAP_FLAG_READ			   = 1 << 0,
	SCRY_BUFFER_MAP_FLAG_WRITE			   = 1 << 1,
	SCRY_BUFFER_MAP_FLAG_PERSISTENT		   = 1 << 2,
	SCRY_BUFFER_MAP_FLAG_COHERENT		   = 1 << 3,
	SCRY_BUFFER_MAP_FLAG_INVALIDATE_RANGE  = 1 << 4,
	SCRY_BUFFER_MAP_FLAG_INVALIDATE_BUFFER = 1 << 5,
	SCRY_BUFFER_MAP_FLAG_FLUSH_EXPLICIT	   = 1 << 6,
	SCRY_BUFFER_MAP_FLAG_UNSYNCHRONIZED	   = 1 << 7
} scry_buffer_map_flags;

typedef struct scry_buffer
{
	GLuint	 handle;
	size_t	 size;
	uint32_t storage_flags;
	uint32_t active_map_flags;
	bool	 immutable;
	bool	 mapped;
} scry_buffer;

const char* scry_buffer_target_name(scry_buffer_target target);
const char* scry_buffer_usage_name(scry_buffer_usage usage);
void		scry_buffer_target_validate(scry_buffer_target target);
void		scry_buffer_usage_validate(scry_buffer_usage usage);
void		scry_buffer_validate_storage_flags(uint32_t storage_flags);
void		scry_buffer_validate_map_flags(uint32_t map_flags);

void scry_buffer_init(scry_buffer* buffer);
void scry_buffer_destroy(scry_buffer* buffer);
void scry_buffer_set_label(const scry_buffer* buffer, const char* label);

void scry_buffer_alloc(scry_buffer* buffer, size_t size, const void* data, scry_buffer_usage usage);
void scry_buffer_storage(scry_buffer* buffer, size_t size, const void* data, uint32_t storage_flags);

void scry_buffer_write(const scry_buffer* buffer, size_t offset, size_t size, const void* data);
void scry_buffer_read(const scry_buffer* buffer, size_t offset, size_t size, void* out_data);
void scry_buffer_copy(const scry_buffer* src_buffer, size_t src_offset, scry_buffer* dst_buffer, size_t dst_offset, size_t size);
void scry_buffer_clear_u32(scry_buffer* buffer, uint32_t value);
void scry_buffer_clear_range_u32(scry_buffer* buffer, size_t offset, size_t size, uint32_t value);
void scry_buffer_clear_f32(scry_buffer* buffer, float value);
void scry_buffer_clear_range_f32(scry_buffer* buffer, size_t offset, size_t size, float value);

void* scry_buffer_map(scry_buffer* buffer, size_t offset, size_t size, uint32_t map_flags);
void  scry_buffer_flush(const scry_buffer* buffer, size_t offset, size_t size);
void  scry_buffer_unmap(scry_buffer* buffer);

void scry_buffer_bind(const scry_buffer* buffer, scry_buffer_target target);
void scry_buffer_bind_base(const scry_buffer* buffer, scry_buffer_target target, unsigned int index);
void scry_buffer_bind_range(const scry_buffer* buffer, scry_buffer_target target, unsigned int index, size_t offset, size_t size);
