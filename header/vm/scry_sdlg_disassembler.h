#pragma once

#include "vm/scry_sdlg_loader.h"

#include <stdbool.h>
#include <stdio.h>

bool scry_sdlg_disassemble(const scry_sdlg_view* view, FILE* output);
bool scry_sdlg_disassemble_file(const char* path, FILE* output);
