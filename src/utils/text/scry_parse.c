#include "utils/text/scry_parse.h"

#include "utils/core/scry_assert.h"

#include <limits.h>
#include <stddef.h>

bool scry_parse_u32_decimal(const char* text, uint32_t* out_value)
{
	uint64_t value = 0U;

	ASSERT_FATAL(text);
	ASSERT_FATAL(out_value);

	if (text[0] == '\0')
	{
		return false;
	}

	for (size_t i = 0U; text[i] != '\0'; ++i)
	{
		const unsigned char ch = (unsigned char)text[i];

		if (ch < '0' || ch > '9')
		{
			return false;
		}

		value = value * 10U + (uint64_t)(ch - '0');

		if (value > UINT32_MAX)
		{
			return false;
		}
	}

	*out_value = (uint32_t)value;
	return true;
}
