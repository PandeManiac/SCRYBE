#include "utils/scry_assert.h"
#include "utils/scry_hints.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UPDATE_WIDTH(s)                                                                                                                                        \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		int len = (int)strlen(s);                                                                                                                              \
                                                                                                                                                               \
		if (len > width)                                                                                                                                       \
		{                                                                                                                                                      \
			width = len;                                                                                                                                       \
		}                                                                                                                                                      \
	} while (0)

#define PRINT_BORDER()                                                                                                                                         \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		fputc('+', stderr);                                                                                                                                    \
                                                                                                                                                               \
		for (int i = 0; i < width + 2; ++i)                                                                                                                    \
		{                                                                                                                                                      \
			fputc('-', stderr);                                                                                                                                \
		}                                                                                                                                                      \
                                                                                                                                                               \
		fputs("+\n", stderr);                                                                                                                                  \
	} while (0)

NORETURN void scry_assert_fail(const char* expr, const char* file, int line)
{
#ifndef SCRY_ASSERT_QUIET
	char expr_buf[1024];
	char file_buf[1024];
	char line_buf[64];

	snprintf(expr_buf, sizeof(expr_buf), "                %s", expr);
	snprintf(file_buf, sizeof(file_buf), "                %s", file);
	snprintf(line_buf, sizeof(line_buf), "                %d", line);

	int width = 0;

	UPDATE_WIDTH("    ASSERTION FAILED:");
	UPDATE_WIDTH("        EXPR:");
	UPDATE_WIDTH(expr_buf);
	UPDATE_WIDTH("        FILE:");
	UPDATE_WIDTH(file_buf);
	UPDATE_WIDTH("        LINE:");
	UPDATE_WIDTH(line_buf);

	width += 2;

	flockfile(stderr);

	fputc('\n', stderr);
	PRINT_BORDER();

	fprintf(stderr, "| %-*s |\n", width, "    ASSERTION FAILED:");

	fprintf(stderr, "| %-*s |\n", width, "");
	fprintf(stderr, "| %-*s |\n", width, "        EXPR:");
	fprintf(stderr, "| %-*s |\n", width, expr_buf);

	fprintf(stderr, "| %-*s |\n", width, "");
	fprintf(stderr, "| %-*s |\n", width, "        FILE:");
	fprintf(stderr, "| %-*s |\n", width, file_buf);

	fprintf(stderr, "| %-*s |\n", width, "");
	fprintf(stderr, "| %-*s |\n", width, "        LINE:");
	fprintf(stderr, "| %-*s |\n", width, line_buf);

	PRINT_BORDER();

	funlockfile(stderr);
#endif

	abort();
	UNREACHABLE;
}
