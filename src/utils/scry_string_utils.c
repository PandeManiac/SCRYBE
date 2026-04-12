#include "utils/scry_string_utils.h"

#include <ctype.h>
#include <string.h>

bool scry_string_is_ascii(const char* src)
{
	if (!src)
	{
		return false;
	}

	for (size_t i = 0U; src[i] != '\0'; ++i)
	{
		if ((unsigned char)src[i] > 0x7FU)
		{
			return false;
		}
	}

	return true;
}

bool scry_string_ascii_copy(char* dst, size_t dst_size, const char* src)
{
	size_t length = 0U;

	if (!dst || dst_size == 0U || !src || !scry_string_is_ascii(src))
	{
		return false;
	}

	length = strlen(src);

	if (length + 1U > dst_size)
	{
		return false;
	}

	memcpy(dst, src, length + 1U);
	return true;
}

bool scry_string_ascii_copy_lower(char* dst, size_t dst_size, const char* src)
{
	if (!scry_string_ascii_copy(dst, dst_size, src))
	{
		return false;
	}

	for (size_t i = 0U; dst[i] != '\0'; ++i)
	{
		dst[i] = (char)tolower((unsigned char)dst[i]);
	}

	return true;
}

bool scry_string_ascii_copy_title(char* dst, size_t dst_size, const char* src)
{
	bool capitalize_next = true;

	if (!scry_string_ascii_copy(dst, dst_size, src))
	{
		return false;
	}

	for (size_t i = 0U; dst[i] != '\0'; ++i)
	{
		unsigned char ch = (unsigned char)dst[i];

		if (isalpha(ch) == 0)
		{
			capitalize_next = ch == ' ' || ch == '_' || ch == '-';
			continue;
		}

		dst[i]			= (char)(capitalize_next ? toupper(ch) : tolower(ch));
		capitalize_next = false;
	}

	return true;
}

bool scry_string_is_integer_literal(const char* src)
{
	size_t index	 = 0U;
	bool   saw_digit = false;

	if (!src)
	{
		return false;
	}

	if (src[0] == '+' || src[0] == '-')
	{
		index = 1U;
	}

	for (; src[index] != '\0'; ++index)
	{
		if (isdigit((unsigned char)src[index]) == 0)
		{
			return false;
		}

		saw_digit = true;
	}

	return saw_digit;
}

bool scry_string_is_numeric_literal(const char* src)
{
	size_t index	 = 0U;
	bool   saw_digit = false;
	bool   saw_dot	 = false;

	if (!src)
	{
		return false;
	}

	if (src[0] == '+' || src[0] == '-')
	{
		index = 1U;
	}

	for (; src[index] != '\0'; ++index)
	{
		const unsigned char ch = (unsigned char)src[index];

		if (isdigit(ch) != 0)
		{
			saw_digit = true;
			continue;
		}

		if (ch == '.' && !saw_dot)
		{
			saw_dot = true;
			continue;
		}

		return false;
	}

	return saw_digit;
}
