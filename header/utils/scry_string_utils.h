#pragma once

#include <stdbool.h>
#include <stddef.h>

bool scry_string_is_ascii(const char* src);
bool scry_string_ascii_copy(char* dst, size_t dst_size, const char* src);
bool scry_string_ascii_copy_lower(char* dst, size_t dst_size, const char* src);
bool scry_string_ascii_copy_title(char* dst, size_t dst_size, const char* src);
bool scry_string_is_integer_literal(const char* src);
bool scry_string_is_numeric_literal(const char* src);
