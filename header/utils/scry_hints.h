#pragma once

#define LIKELY(expr) __builtin_expect(!(!(expr)), 1)
#define UNLIKELY(expr) __builtin_expect(!(!(expr)), 0)

#define UNUSED(x)                                                                                                                                              \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		(void)(x);                                                                                                                                             \
	} while (0)

#define NORETURN __attribute__((noreturn))
#define UNREACHABLE __builtin_unreachable()
