#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct scry_string_pool_builder
{
	char*  data;
	size_t size;
	size_t capacity;
} scry_string_pool_builder;

void  scry_string_pool_builder_init(scry_string_pool_builder* pool);
void  scry_string_pool_builder_destroy(scry_string_pool_builder* pool);
bool  scry_string_pool_builder_intern(scry_string_pool_builder* pool, const char* text, uint32_t* out_offset);
char* scry_string_pool_builder_release(scry_string_pool_builder* pool, size_t* out_size);
