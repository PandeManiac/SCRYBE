#include "utils/collections/scry_hash_map.h"

#include "utils/core/scry_assert.h"

#include <stdlib.h>
#include <string.h>

static size_t hash_u32(uint32_t key);
static bool	  scry_u32_hash_map_rehash(scry_u32_hash_map* map, size_t new_capacity);
static bool	  scry_u32_hash_map_insert_entry(scry_u32_hash_map_entry* entries, size_t capacity, uint32_t key, uint32_t value, bool* out_inserted_new);

void scry_u32_hash_map_init(scry_u32_hash_map* map)
{
	ASSERT_FATAL(map);
	memset(map, 0, sizeof(*map));
}

void scry_u32_hash_map_destroy(scry_u32_hash_map* map)
{
	ASSERT_FATAL(map);
	free(map->entries);
	memset(map, 0, sizeof(*map));
}

bool scry_u32_hash_map_put(scry_u32_hash_map* map, uint32_t key, uint32_t value)
{
	ASSERT_FATAL(map);

	if (map->capacity == 0U || ((map->count + 1U) * 10U) > (map->capacity * 7U))
	{
		const size_t new_capacity = (map->capacity == 0U) ? 16U : (map->capacity * 2U);

		if (!scry_u32_hash_map_rehash(map, new_capacity))
		{
			return false;
		}
	}

	bool inserted_new = false;

	if (!scry_u32_hash_map_insert_entry(map->entries, map->capacity, key, value, &inserted_new))
	{
		return false;
	}

	if (inserted_new)
	{
		map->count += 1U;
	}

	return true;
}

bool scry_u32_hash_map_get(const scry_u32_hash_map* map, uint32_t key, uint32_t* out_value)
{
	ASSERT_FATAL(map);
	ASSERT_FATAL(out_value);

	if (map->capacity == 0U)
	{
		return false;
	}

	const size_t mask  = map->capacity - 1U;
	size_t		 index = hash_u32(key) & mask;

	for (size_t probe = 0U; probe < map->capacity; ++probe)
	{
		const scry_u32_hash_map_entry* entry = &map->entries[index];

		if (!entry->occupied)
		{
			return false;
		}

		if (entry->key == key)
		{
			*out_value = entry->value;
			return true;
		}

		index = (index + 1U) & mask;
	}

	return false;
}

static size_t hash_u32(uint32_t key)
{
	return (size_t)(key * UINT32_C(2654435761));
}

static bool scry_u32_hash_map_rehash(scry_u32_hash_map* map, size_t new_capacity)
{
	ASSERT_FATAL(map);

	if (new_capacity == 0U || (new_capacity & (new_capacity - 1U)) != 0U)
	{
		return false;
	}

	scry_u32_hash_map_entry* new_entries = calloc(new_capacity, sizeof(*new_entries));

	if (!new_entries)
	{
		return false;
	}

	for (size_t i = 0U; i < map->capacity; ++i)
	{
		const scry_u32_hash_map_entry* entry = &map->entries[i];

		if (!entry->occupied)
		{
			continue;
		}

		bool inserted_new = false;

		if (!scry_u32_hash_map_insert_entry(new_entries, new_capacity, entry->key, entry->value, &inserted_new))
		{
			free(new_entries);
			return false;
		}
	}

	free(map->entries);

	map->entries  = new_entries;
	map->capacity = new_capacity;

	return true;
}

static bool scry_u32_hash_map_insert_entry(scry_u32_hash_map_entry* entries, size_t capacity, uint32_t key, uint32_t value, bool* out_inserted_new)
{
	ASSERT_FATAL(entries);
	ASSERT_FATAL(capacity > 0U);
	ASSERT_FATAL(out_inserted_new);

	const size_t mask  = capacity - 1U;
	size_t		 index = hash_u32(key) & mask;

	for (size_t probe = 0U; probe < capacity; ++probe)
	{
		scry_u32_hash_map_entry* entry = &entries[index];

		if (!entry->occupied)
		{
			entry->key		  = key;
			entry->value	  = value;
			entry->occupied	  = true;
			*out_inserted_new = true;
			return true;
		}

		if (entry->key == key)
		{
			entry->value	  = value;
			*out_inserted_new = false;
			return true;
		}

		index = (index + 1U) & mask;
	}

	return false;
}
