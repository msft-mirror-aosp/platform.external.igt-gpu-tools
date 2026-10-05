// SPDX-License-Identifier: MIT
/*
 * Copyright © 2026 Intel Corporation
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "igt.h"
#include "igt_sysfs.h"

/**
 * TEST: Test Xe RAS (Reliability, Availability, Serviceability) functionality
 * Category: Core
 * Mega feature: RAS
 * Sub-category: RAS tests
 * Functionality: ras
 * Test category: Functional tests
 *
 * SUBTEST: gpu-health
 * Description: Verify the gpu_health sysfs attribute accepts each valid state
 *		and rejects invalid writes with -EINVAL.
 *		The valid states are:
 *		ok	 - The gpu is healthy and operating within normal
 *			   parameters.
 *		warning	 - The gpu is experiencing minor issues but remains
 *			   operational.
 *		critical - The gpu is in a critical state and may not be
 *			   operational.
 */

IGT_TEST_DESCRIPTION("Tests for Xe RAS");

#define GPU_HEALTH_ATTR	"device/gpu_health"

static const char * const gpu_health_states[] = {
	"ok",
	"warning",
	"critical",
	"invalid-input",
};

#define GPU_HEALTH_VALID_STATE_COUNT (ARRAY_SIZE(gpu_health_states) - 1)

static struct {
	int sys_fd;
	int orig_health;
} gpu_health_ctx = { .sys_fd = -1, .orig_health = -1 };

static void restore_gpu_health(int sig)
{
	if (gpu_health_ctx.sys_fd < 0)
		return;

	if (gpu_health_ctx.orig_health >= 0) {
		igt_sysfs_set(gpu_health_ctx.sys_fd, GPU_HEALTH_ATTR,
			      gpu_health_states[gpu_health_ctx.orig_health]);
		igt_info("Restored initial gpu_health to '%s'\n",
			 gpu_health_states[gpu_health_ctx.orig_health]);
		gpu_health_ctx.orig_health = -1;
	}
	close(gpu_health_ctx.sys_fd);
	gpu_health_ctx.sys_fd = -1;
}

static bool valid_gpu_health(const char *s)
{
	int i;

	if (!s)
		return false;

	for (i = 0; i < GPU_HEALTH_VALID_STATE_COUNT; i++)
		if (!strcmp(s, gpu_health_states[i]))
			return true;

	return false;
}

static void setup_gpu_health(int xe)
{
	char *health;
	int i;

	gpu_health_ctx.sys_fd = igt_sysfs_open(xe);
	igt_assert(gpu_health_ctx.sys_fd >= 0);

	igt_skip_on_f(!igt_sysfs_has_attr(gpu_health_ctx.sys_fd, GPU_HEALTH_ATTR),
		      "gpu_health sysfs attribute not exposed by driver\n");

	health = igt_sysfs_get(gpu_health_ctx.sys_fd, GPU_HEALTH_ATTR);
	igt_assert_f(valid_gpu_health(health),
		     "Unexpected initial gpu_health value: '%s'\n", health);

	for (i = 0; i < GPU_HEALTH_VALID_STATE_COUNT; i++)
		if (!strcmp(health, gpu_health_states[i]))
			gpu_health_ctx.orig_health = i;

	igt_info("Initial gpu_health: %s\n", health);
	free(health);
}

static void test_gpu_health_state(const char *state)
{
	char *health;
	int ret;

	if (!valid_gpu_health(state)) {
		ret = igt_sysfs_write(gpu_health_ctx.sys_fd, GPU_HEALTH_ATTR,
				      state, strlen(state));
		igt_assert_f(ret == -EINVAL,
			     "Writing invalid value to %s returned %d, expected -EINVAL\n",
			     GPU_HEALTH_ATTR, ret);
		return;
	}

	igt_info("Setting gpu_health to '%s'\n", state);

	igt_assert_f(igt_sysfs_set(gpu_health_ctx.sys_fd, GPU_HEALTH_ATTR, state),
		     "Failed to write '%s' to %s\n",
		     state, GPU_HEALTH_ATTR);

	health = igt_sysfs_get(gpu_health_ctx.sys_fd, GPU_HEALTH_ATTR);
	igt_assert(health);
	igt_assert_f(!strcmp(health, state),
		     "gpu_health readback mismatch: wrote '%s', read '%s'\n",
		     state, health);
	igt_info("Verified gpu_health state readback for '%s' successfully\n",
		 state);
	free(health);
}

int igt_main()
{
	int xe;
	int state;

	igt_fixture() {
		xe = drm_open_driver(DRIVER_XE);
	}

	igt_subtest_with_dynamic("gpu-health") {
		igt_install_exit_handler(restore_gpu_health);
		setup_gpu_health(xe);

		for (state = 0; state < ARRAY_SIZE(gpu_health_states); state++) {
			igt_dynamic_f("%s", gpu_health_states[state]) {
				test_gpu_health_state(gpu_health_states[state]);
			}
		}

		restore_gpu_health(0);
	}

	igt_fixture()
		drm_close_driver(xe);
}
