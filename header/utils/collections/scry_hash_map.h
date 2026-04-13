#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct scry_u32_hash_map_entry
{
	uint32_t key;
	uint32_t value;
	bool	 occupied;
} scry_u32_hash_map_entry;

typedef struct scry_u32_hash_map
{
	scry_u32_hash_map_entry* entries;
	size_t					 capacity;
	size_t					 count;
} scry_u32_hash_map;

void scry_u32_hash_map_init(scry_u32_hash_map* map);
void scry_u32_hash_map_destroy(scry_u32_hash_map* map);

bool scry_u32_hash_map_put(scry_u32_hash_map* map, uint32_t key, uint32_t value);
bool scry_u32_hash_map_get(const scry_u32_hash_map* map, uint32_t key, uint32_t* out_value);
