#pragma once

#include "vm/scry_sdlg.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct scry_sdlg_span
{
	const uint8_t* data;
	size_t		   size;
} scry_sdlg_span;

typedef struct scry_sdlg_string
{
	const char* data;
	size_t		length;
} scry_sdlg_string;

typedef struct scry_sdlg_view
{
	scry_sdlg_header header;
	const uint8_t*	 file_data;
	size_t			 file_size;
	scry_sdlg_span	 instructions;
	scry_sdlg_span	 table_directory;
	scry_sdlg_span	 string_index_data;
	scry_sdlg_span	 string_data;
} scry_sdlg_view;

typedef struct scry_sdlg_document
{
	uint8_t*	   buffer;
	size_t		   size;
	scry_sdlg_view view;
} scry_sdlg_document;

bool scry_sdlg_view_init(scry_sdlg_view* view, const uint8_t* data, size_t size);
void scry_sdlg_document_destroy(scry_sdlg_document* document);
bool scry_sdlg_document_load_file(scry_sdlg_document* document, const char* path);
bool scry_sdlg_table_at(const scry_sdlg_view* view, uint32_t table_index, scry_sdlg_table_entry* out_table);
bool scry_sdlg_table_payload(const scry_sdlg_view* view, const scry_sdlg_table_entry* table, scry_sdlg_span* out_payload);
bool scry_sdlg_string_at(const scry_sdlg_view* view, uint32_t string_id, scry_sdlg_string* out_string);
bool scry_sdlg_debug_dump(const scry_sdlg_view* view);
