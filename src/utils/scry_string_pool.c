#include "utils/scry_string_pool.h"

#include "utils/scry_assert.h"
#include "utils/scry_storage.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

void scry_string_pool_builder_init(scry_string_pool_builder* pool)
{
	ASSERT_FATAL(pool);

	pool->data	   = NULL;
	pool->size	   = 0U;
	pool->capacity = 0U;
}

void scry_string_pool_builder_destroy(scry_string_pool_builder* pool)
{
	ASSERT_FATAL(pool);

	free(pool->data);

	pool->data	   = NULL;
	pool->size	   = 0U;
	pool->capacity = 0U;
}

bool scry_string_pool_builder_intern(scry_string_pool_builder* pool, const char* text, uint32_t* out_offset)
{
	ASSERT_FATAL(pool);
	ASSERT_FATAL(text);
	ASSERT_FATAL(out_offset);

	const size_t text_len = strlen(text);

	for (size_t offset = 0U; offset < pool->size;)
	{
		const char* existing = &pool->data[offset];

		if (strcmp(existing, text) == 0)
		{
			*out_offset = (uint32_t)offset;
			return true;
		}

		offset += strlen(existing) + 1U;
	}

	if (pool->size > UINT32_MAX - (text_len + 1U) || !SCRY_STORAGE_RESERVE(pool->data, pool->capacity, pool->size + text_len + 1U, 64U))
	{
		return false;
	}

	*out_offset = (uint32_t)pool->size;

	memcpy(&pool->data[pool->size], text, text_len + 1U);
	pool->size += text_len + 1U;

	return true;
}

char* scry_string_pool_builder_release(scry_string_pool_builder* pool, size_t* out_size)
{
	char* data = NULL;

	ASSERT_FATAL(pool);
	ASSERT_FATAL(out_size);

	data		   = pool->data;
	*out_size	   = pool->size;
	pool->data	   = NULL;
	pool->size	   = 0U;
	pool->capacity = 0U;
	return data;
}
