#pragma once

#include "vm/scry_ir.h"
#include "vm/scry_sdlg_loader.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct scry_sdlg_binary
{
	uint8_t* data;
	size_t	 size;
} scry_sdlg_binary;

void scry_sdlg_binary_destroy(scry_sdlg_binary* binary);
bool scry_sdlg_compile_ir(const scry_ir_program* program, const scry_sdlg_string* strings, scry_sdlg_binary* out_binary);
bool scry_sdlg_write_ir_file(const char* path, const scry_ir_program* program, const scry_sdlg_string* strings);
