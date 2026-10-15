// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright (C) 2025 Collabora Ltd.

#include "igt.h"
#include "igt_core.h"
#include "igt_panthor.h"
#include "panthor_drm.h"
#include <stdint.h>

#define GPU_INFO_MIN_SIZE \
	(offsetof(struct drm_panthor_gpu_info, tiler_present) + \
	 sizeof(((struct drm_panthor_gpu_info *)0)->tiler_present))

int igt_main() {
	int fd = -1;

	igt_fixture() {
		igt_panthor_skip_on_big_endian();
		fd = drm_open_driver(DRIVER_PANTHOR);
	}

	igt_describe("Query GPU information from ROM.");
	igt_subtest("query") {
		struct drm_panthor_gpu_info gpu = {};

		igt_panthor_query(fd, DRM_PANTHOR_DEV_QUERY_GPU_INFO, &gpu, sizeof(gpu), 0);
		igt_assert_neq(gpu.gpu_id, 0);
	}

	igt_describe("Ensure kernel returns a GPU_INFO size.");
	igt_subtest("query-size-null-pointer") {
		uint32_t size;

		size = igt_panthor_query(fd, DRM_PANTHOR_DEV_QUERY_GPU_INFO, NULL, 0, 0);

		/*
		 * The kernel returns its output structure size, which may be out
		 * of sync with the header used to build these tests, so don't expect
		 * that the sizes will match necessarily.
		 */
		igt_assert(size >= GPU_INFO_MIN_SIZE);
	}

	igt_describe("Ensure kernel enforces GPU_INFO minimum size.");
	igt_subtest("query-size-too-small") {
		struct drm_panthor_gpu_info gpu = {};

		igt_panthor_query(fd, DRM_PANTHOR_DEV_QUERY_GPU_INFO,
				  &gpu, GPU_INFO_MIN_SIZE - 1, EINVAL);
	}

	igt_describe("Ensure kernel accepts GPU_INFO minimum size.");
	igt_subtest("query-min-size") {
		struct drm_panthor_gpu_info gpu = {};

		igt_panthor_query(fd, DRM_PANTHOR_DEV_QUERY_GPU_INFO,
				  &gpu, GPU_INFO_MIN_SIZE, 0);

		igt_assert_neq(gpu.gpu_id, 0);
	}

	igt_describe("Ensure kernel zeros out unused part of buffer.");
	igt_subtest("query-too-big") {
		uint32_t size;
		size_t buf_size;
		uint8_t *buf;

		/*
		 * Create a buffer that is larger than necessary.
		 * Start with the size returned by the kernel in case it is out of
		 * sync with these tests. Then add an extra 16 bytes and initialize
		 * the entire buffer to non-zero values.
		 */
		size = igt_panthor_query(fd, DRM_PANTHOR_DEV_QUERY_GPU_INFO,
					 NULL, 0, 0);
		buf_size = (size_t)size + 16;
		buf = malloc(buf_size);
		igt_assert(buf);
		memset(buf, 0xff, buf_size);

		igt_panthor_query(fd, DRM_PANTHOR_DEV_QUERY_GPU_INFO,
				  buf, buf_size, 0);

		/* Check that the extra bytes were zeroed. */
		for (size_t i = size; i < buf_size; i++)
			igt_assert_eq(buf[i], 0);

		free(buf);
	}

	igt_fixture() {
		drm_close_driver(fd);
	}
}
