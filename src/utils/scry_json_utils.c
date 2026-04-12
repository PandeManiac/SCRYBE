#include "utils/scry_json_utils.h"

#include "utils/scry_assert.h"

bool scry_json_get_typed(json_object* obj, const char* key, json_type type, json_object** out_value)
{
	ASSERT_FATAL(obj);
	ASSERT_FATAL(key);
	ASSERT_FATAL(out_value);

	return json_object_object_get_ex(obj, key, out_value) && json_object_is_type(*out_value, type);
}

bool scry_json_get_string(json_object* obj, const char* key, const char** out_value)
{
	json_object* value = NULL;

	ASSERT_FATAL(out_value);

	if (!scry_json_get_typed(obj, key, json_type_string, &value))
	{
		return false;
	}

	*out_value = json_object_get_string(value);
	return *out_value != NULL;
}

bool scry_json_get_u32(json_object* obj, const char* key, uint32_t* out_value)
{
	json_object* value = NULL;
	int64_t		 raw   = 0;

	ASSERT_FATAL(out_value);

	if (!scry_json_get_typed(obj, key, json_type_int, &value))
	{
		return false;
	}

	raw = json_object_get_int64(value);

	if (raw < 0 || raw > (int64_t)UINT32_MAX)
	{
		return false;
	}

	*out_value = (uint32_t)raw;
	return true;
}

bool scry_json_get_f32(json_object* obj, const char* key, float* out_value)
{
	ASSERT_FATAL(out_value);
	json_object* value = NULL;

	if (!json_object_object_get_ex(obj, key, &value))
	{
		return false;
	}

	if (!json_object_is_type(value, json_type_double) && !json_object_is_type(value, json_type_int))
	{
		return false;
	}

	*out_value = (float)json_object_get_double(value);
	return true;
}

bool scry_json_get_bool(json_object* obj, const char* key, bool* out_value)
{
	ASSERT_FATAL(out_value);
	json_object* value = NULL;

	if (!scry_json_get_typed(obj, key, json_type_boolean, &value))
	{
		return false;
	}

	*out_value = json_object_get_boolean(value) != 0;
	return true;
}

bool scry_json_add_string(json_object* obj, const char* key, const char* value)
{
	ASSERT_FATAL(obj);
	ASSERT_FATAL(key);
	ASSERT_FATAL(value);

	json_object* json_value = NULL;

	json_value = json_object_new_string(value);
	return json_value != NULL && scry_json_object_add_owned(obj, key, json_value);
}

bool scry_json_add_string_if_nonempty(json_object* obj, const char* key, const char* value)
{
	ASSERT_FATAL(obj);
	ASSERT_FATAL(key);
	ASSERT_FATAL(value);

	return value[0] == '\0' ? true : scry_json_add_string(obj, key, value);
}

bool scry_json_add_i64(json_object* obj, const char* key, int64_t value)
{
	ASSERT_FATAL(obj);
	ASSERT_FATAL(key);

	json_object* json_value = NULL;

	json_value = json_object_new_int64(value);
	return json_value != NULL && scry_json_object_add_owned(obj, key, json_value);
}

bool scry_json_add_bool(json_object* obj, const char* key, bool value)
{
	ASSERT_FATAL(obj);
	ASSERT_FATAL(key);

	json_object* json_value = NULL;

	json_value = json_object_new_boolean(value);
	return json_value != NULL && scry_json_object_add_owned(obj, key, json_value);
}

bool scry_json_add_f64(json_object* obj, const char* key, double value)
{
	ASSERT_FATAL(obj);
	ASSERT_FATAL(key);

	json_object* json_value = NULL;

	json_value = json_object_new_double(value);
	return json_value != NULL && scry_json_object_add_owned(obj, key, json_value);
}

bool scry_json_object_add_owned(json_object* obj, const char* key, json_object* value)
{
	ASSERT_FATAL(obj);
	ASSERT_FATAL(key);
	ASSERT_FATAL(value);

	if (json_object_object_add(obj, key, value) == 0)
	{
		return true;
	}

	json_object_put(value);
	return false;
}

bool scry_json_array_add_owned(json_object* array, json_object* value)
{
	ASSERT_FATAL(array);
	ASSERT_FATAL(value);

	if (json_object_array_add(array, value) == 0)
	{
		return true;
	}

	json_object_put(value);
	return false;
}

bool scry_json_array_object_at(json_object* array, size_t index, json_object** out_object)
{
	ASSERT_FATAL(array);
	ASSERT_FATAL(out_object);

	*out_object = json_object_array_get_idx(array, index);
	return *out_object != NULL && json_object_is_type(*out_object, json_type_object);
}
