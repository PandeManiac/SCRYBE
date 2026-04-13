#pragma once

// #define SCRY_ASSERT_QUIET

#include "utils/core/scry_hints.h"

NORETURN void scry_assert_fail(const char* expr, const char* file, int line);

#define ASSERT_FATAL(expr)                                                                                                                                     \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		if (UNLIKELY(!(expr)))                                                                                                                                 \
		{                                                                                                                                                      \
			scry_assert_fail(#expr, __FILE__, __LINE__);                                                                                                       \
		}                                                                                                                                                      \
	} while (0)

#define STATIC_ASSERT_GLUE(a, b) a##b
#define STATIC_ASSERT_XGLUE(a, b) STATIC_ASSERT_GLUE(a, b)

#define STATIC_ASSERT(cond, msg)                                                                                                                               \
	extern int scry_static_assert(void) __attribute__((diagnose_if(!(cond), msg, "error")));                                                                   \
                                                                                                                                                               \
	enum                                                                                                                                                       \
	{                                                                                                                                                          \
		STATIC_ASSERT_XGLUE(scry_static_assert_trigger_, __LINE__) = sizeof(scry_static_assert())                                                              \
	}
