#include "utils/io/scry_file.h"

#include "utils/core/scry_assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool scry_file_read_alloc(const char* path, void** out_data, size_t* out_size)
{
	ASSERT_FATAL(path);
	ASSERT_FATAL(out_data);
	ASSERT_FATAL(out_size);

	*out_data = NULL;
	*out_size = 0U;

	FILE* file = fopen(path, "rb");

	if (!file)
	{
		return false;
	}

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		return false;
	}

	const long size_long = ftell(file);

	if (size_long < 0)
	{
		fclose(file);
		return false;
	}

	if (fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		return false;
	}

	const size_t size = (size_t)size_long;
	void*		 data = (size > 0U) ? malloc(size) : NULL;

	if (size > 0U && !data)
	{
		fclose(file);
		return false;
	}

	const size_t bytes_read = fread(data, 1U, size, file);
	fclose(file);

	if (bytes_read != size)
	{
		free(data);
		return false;
	}

	*out_data = data;
	*out_size = size;
	return true;
}

bool scry_file_read(const char* path, char** out_text)
{
	char*  text = NULL;
	size_t size = 0U;

	ASSERT_FATAL(path);
	ASSERT_FATAL(out_text);

	*out_text = NULL;

	if (!scry_file_read_alloc(path, (void**)&text, &size))
	{
		return false;
	}

	char* nul_terminated = realloc(text, size + 1U);

	if (!nul_terminated)
	{
		free(text);
		return false;
	}

	nul_terminated[size] = '\0';
	*out_text			 = nul_terminated;

	return true;
}

bool scry_file_read_bytes(const char* path, uint8_t** out_data, size_t* out_size)
{
	ASSERT_FATAL(path);
	ASSERT_FATAL(out_data);
	ASSERT_FATAL(out_size);

	return scry_file_read_alloc(path, (void**)out_data, out_size);
}

bool scry_file_write(const char* path, const char* text)
{
	ASSERT_FATAL(path);
	ASSERT_FATAL(text);

	return scry_file_write_bytes(path, text, strlen(text));
}

bool scry_file_write_bytes(const char* path, const void* data, size_t size)
{
	ASSERT_FATAL(path);
	ASSERT_FATAL(data || size == 0U);

	FILE* file = fopen(path, "wb");

	if (!file)
	{
		return false;
	}

	const size_t written = (size > 0U) ? fwrite(data, 1U, size, file) : 0U;

	fclose(file);
	return written == size;
}
