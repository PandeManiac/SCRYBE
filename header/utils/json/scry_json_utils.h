#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <json-c/json.h>

bool scry_json_get_typed(json_object* obj, const char* key, json_type type, json_object** out_value);
bool scry_json_get_string(json_object* obj, const char* key, const char** out_value);
bool scry_json_get_u32(json_object* obj, const char* key, uint32_t* out_value);
bool scry_json_get_f32(json_object* obj, const char* key, float* out_value);
bool scry_json_get_bool(json_object* obj, const char* key, bool* out_value);

bool scry_json_add_string(json_object* obj, const char* key, const char* value);
bool scry_json_add_string_if_nonempty(json_object* obj, const char* key, const char* value);
bool scry_json_add_i64(json_object* obj, const char* key, int64_t value);
bool scry_json_add_bool(json_object* obj, const char* key, bool value);
bool scry_json_add_f64(json_object* obj, const char* key, double value);

bool scry_json_object_add_owned(json_object* obj, const char* key, json_object* value);
bool scry_json_array_add_owned(json_object* array, json_object* value);
bool scry_json_array_object_at(json_object* array, size_t index, json_object** out_object);
