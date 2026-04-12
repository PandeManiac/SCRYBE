#pragma once

#define COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))

#define SCRY_TRY(expr)                                                                                                                                         \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		if (!(expr))                                                                                                                                           \
		{                                                                                                                                                      \
			return false;                                                                                                                                      \
		}                                                                                                                                                      \
	} while (0)

#define SCRY_TRY_WITH(expr, cleanup_stmt)                                                                                                                      \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		if (!(expr))                                                                                                                                           \
		{                                                                                                                                                      \
			cleanup_stmt;                                                                                                                                      \
			return false;                                                                                                                                      \
		}                                                                                                                                                      \
	} while (0)

#define SCRY_APPLY_IF(cond, expr)                                                                                                                              \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		if ((cond))                                                                                                                                            \
		{                                                                                                                                                      \
			SCRY_TRY(expr);                                                                                                                                    \
		}                                                                                                                                                      \
	} while (0)

#define SCRY_TRY_PTR(ptr) SCRY_TRY((ptr) != NULL)
