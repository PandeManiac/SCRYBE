#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool scry_file_read(const char* path, char** out_text);
bool scry_file_read_bytes(const char* path, uint8_t** out_data, size_t* out_size);
bool scry_file_write(const char* path, const char* text);
bool scry_file_write_bytes(const char* path, const void* data, size_t size);
