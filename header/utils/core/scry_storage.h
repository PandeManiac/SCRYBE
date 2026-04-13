#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static inline bool scry_storage_reserve_impl(void** ptr, size_t element_size, size_t* capacity, size_t required, size_t initial_capacity)
{
	if (required <= *capacity)
	{
		return true;
	}

	size_t new_capacity = (*capacity > 0U) ? *capacity : initial_capacity;

	while (new_capacity < required)
	{
		if (new_capacity > SIZE_MAX / 2U)
		{
			return false;
		}

		new_capacity *= 2U;
	}

	void* new_block = realloc(*ptr, new_capacity * element_size);

	if (!new_block)
	{
		return false;
	}

	*ptr	  = new_block;
	*capacity = new_capacity;

	return true;
}

#define SCRY_STORAGE_RESERVE(ptr, capacity, required, initial_capacity)                                                                                        \
	scry_storage_reserve_impl((void**)&(ptr), sizeof(*(ptr)), &(capacity), (required), (initial_capacity))
