#include "render/scry_buffer.h"
#include "utils/core/scry_assert.h"
#include "window/scry_window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <stdint.h>
#include <stdlib.h>

static const scry_window_config window_config = {
	.title = "SCRYBE WINDOW",
	.pos   = { { 0, 0 } },
	.size  = { { 1920, 1080 } },

	.major	 = 4,
	.minor	 = 6,
	.profile = GLFW_OPENGL_CORE_PROFILE,

	.samples = 4,

	.fullscreen = true,
	.vsync		= true,
};

static scry_window window = { 0 };

static void scry_buffer_smoke_test(void);

int main(void)
{
	ASSERT_FATAL(glfwInit());
	scry_window_init(&window, &window_config);
	ASSERT_FATAL(gladLoaderLoadGL());
	scry_buffer_smoke_test();

	while (!scry_window_should_close(&window))
	{
		scry_window_update_input(&window, 1.0 / 60.0);
		scry_window_swap_buffers(&window);
	}

	scry_window_destroy(&window);
	glfwTerminate();

	return 0;
}

static void scry_buffer_smoke_test(void)
{
	static const uint32_t src_data[16] = {
		0x01020304U,
		0x11121314U,
		0x21222324U,
		0x31323334U,
		0x41424344U,
		0x51525354U,
		0x61626364U,
		0x71727374U,
		0x81828384U,
		0x91929394U,
		0xA1A2A3A4U,
		0xB1B2B3B4U,
		0xC1C2C3C4U,
		0xD1D2D3D4U,
		0xE1E2E3E4U,
		0xF1F2F3F4U,
	};

	scry_buffer mutable_buffer	 = { 0 };
	scry_buffer immutable_buffer = { 0 };
	scry_buffer zero_buffer		 = { 0 };
	uint32_t	readback[16]	 = { 0 };
	uint32_t*	mapped_words	 = NULL;

	scry_buffer_init(&mutable_buffer);
	scry_buffer_set_label(&mutable_buffer, "mutable_buffer");
	scry_buffer_alloc(&mutable_buffer, sizeof(src_data), src_data, SCRY_BUFFER_USAGE_DYNAMIC_DRAW);
	ASSERT_FATAL(mutable_buffer.size == sizeof(src_data));
	ASSERT_FATAL(!mutable_buffer.immutable);
	ASSERT_FATAL(!mutable_buffer.mapped);
	scry_buffer_bind(&mutable_buffer, SCRY_BUFFER_TARGET_ARRAY);
	scry_buffer_bind(&mutable_buffer, SCRY_BUFFER_TARGET_COPY_READ);
	scry_buffer_write(&mutable_buffer, 0U, 0U, NULL);
	scry_buffer_write(&mutable_buffer, sizeof(uint32_t) * 4U, sizeof(uint32_t) * 2U, &src_data[8]);
	scry_buffer_read(&mutable_buffer, 0U, sizeof(readback), readback);
	ASSERT_FATAL(readback[0] == src_data[0]);
	ASSERT_FATAL(readback[4] == src_data[8]);
	ASSERT_FATAL(readback[5] == src_data[9]);
	scry_buffer_clear_range_u32(&mutable_buffer, sizeof(uint32_t) * 8U, sizeof(uint32_t) * 4U, 0U);
	scry_buffer_clear_u32(&mutable_buffer, 0U);

	scry_buffer_init(&immutable_buffer);
	scry_buffer_set_label(&immutable_buffer, "immutable_buffer");
	scry_buffer_storage(&immutable_buffer,
						sizeof(src_data),
						src_data,
						SCRY_BUFFER_STORAGE_FLAG_DYNAMIC | SCRY_BUFFER_STORAGE_FLAG_MAP_READ | SCRY_BUFFER_STORAGE_FLAG_MAP_WRITE);
	ASSERT_FATAL(immutable_buffer.size == sizeof(src_data));
	ASSERT_FATAL(immutable_buffer.immutable);
	ASSERT_FATAL(!immutable_buffer.mapped);
	ASSERT_FATAL(immutable_buffer.storage_flags != 0U);
	scry_buffer_bind(&immutable_buffer, SCRY_BUFFER_TARGET_COPY_WRITE);
	scry_buffer_bind_base(&immutable_buffer, SCRY_BUFFER_TARGET_SHADER_STORAGE, 0U);
	scry_buffer_bind_range(&immutable_buffer, SCRY_BUFFER_TARGET_UNIFORM, 1U, 0U, sizeof(uint32_t) * 4U);
	scry_buffer_copy(&mutable_buffer, 0U, &immutable_buffer, 0U, 0U);
	scry_buffer_copy(&mutable_buffer, 0U, &immutable_buffer, 0U, sizeof(src_data));

	mapped_words = scry_buffer_map(&immutable_buffer, 0U, sizeof(src_data), SCRY_BUFFER_MAP_FLAG_READ | SCRY_BUFFER_MAP_FLAG_WRITE);
	ASSERT_FATAL(mapped_words);
	ASSERT_FATAL(immutable_buffer.mapped);
	ASSERT_FATAL(immutable_buffer.active_map_flags == (SCRY_BUFFER_MAP_FLAG_READ | SCRY_BUFFER_MAP_FLAG_WRITE));
	mapped_words[0] = 0xDEADBEEFU;
	mapped_words[1] = 0xFACECAFEU;
	scry_buffer_unmap(&immutable_buffer);
	ASSERT_FATAL(!immutable_buffer.mapped);
	ASSERT_FATAL(immutable_buffer.active_map_flags == 0U);

	scry_buffer_read(&immutable_buffer, 0U, sizeof(readback), readback);
	ASSERT_FATAL(readback[0] == 0xDEADBEEFU);
	ASSERT_FATAL(readback[1] == 0xFACECAFEU);
	scry_buffer_clear_f32(&immutable_buffer, 0.0F);
	scry_buffer_clear_range_f32(&immutable_buffer, 0U, sizeof(uint32_t) * 4U, 1.0F);

	scry_buffer_init(&zero_buffer);
	scry_buffer_set_label(&zero_buffer, "zero_buffer");
	scry_buffer_alloc(&zero_buffer, 0U, NULL, SCRY_BUFFER_USAGE_STATIC_DRAW);
	ASSERT_FATAL(zero_buffer.size == 0U);
	ASSERT_FATAL(!zero_buffer.immutable);
	scry_buffer_destroy(&zero_buffer);

	scry_buffer_destroy(&immutable_buffer);
	scry_buffer_destroy(&mutable_buffer);
}
