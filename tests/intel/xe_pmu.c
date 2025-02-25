// SPDX-License-Identifier: MIT
/*
 * Copyright © 2025 Intel Corporation
 */

/**
 * TEST: Test Xe PMU(Performance Monitoring Unit) functionality
 * Category: Metrics
 * Functionality: Power/Perf
 * Mega feature: Performance Monitoring Unit
 * Sub-category: Telemetry
 * Test category: Functional tests
 */

#include "igt.h"
#include "igt_perf.h"
#include "igt_sysfs.h"

#include "xe/xe_gt.h"
#include "xe/xe_ioctl.h"
#include "xe/xe_spin.h"

#define SLEEP_DURATION 2 /* in seconds */
/* flag masks */
#define TEST_LOAD		BIT(0)
#define TEST_TRAILING_IDLE	BIT(1)

const double tolerance = 0.1;

#define test_each_engine(test, fd, hwe) \
	igt_subtest_with_dynamic(test) \
		xe_for_each_engine(fd, hwe) \
			igt_dynamic_f("engine-%s%d", xe_engine_class_string(hwe->engine_class), \
				      hwe->engine_instance)

static int open_pmu(int xe, uint64_t config)
{
	int fd;

	fd = perf_xe_open(xe, config);
	igt_skip_on(fd < 0 && errno == ENODEV);
	igt_assert(fd >= 0);

	return fd;
}

static int open_group(int xe, uint64_t config, int group)
{
	int fd;

	fd = igt_perf_open_group(xe_perf_type_id(xe), config, group);
	igt_skip_on(fd < 0 && errno == ENODEV);
	igt_assert(fd >= 0);

	return fd;
}

static uint64_t __pmu_read_single(int fd, uint64_t *ts)
{
	uint64_t data[2];

	igt_assert_eq(read(fd, data, sizeof(data)), sizeof(data));
	if (ts)
		*ts = data[1];

	return data[0];
}

static uint64_t pmu_read_multi(int fd, unsigned int num, uint64_t *val)
{
	uint64_t buf[2 + num];
	unsigned int i;

	igt_assert_eq(read(fd, buf, sizeof(buf)), sizeof(buf));

	for (i = 0; i < num; i++)
		val[i] = buf[2 + i];

	return buf[1];
}

static unsigned long read_idle_residency(int fd, int gt)
{
	unsigned long residency = 0;
	int gt_fd;

	gt_fd = xe_sysfs_gt_open(fd, gt);
	igt_assert(gt_fd >= 0);
	igt_assert(igt_sysfs_scanf(gt_fd, "gtidle/idle_residency_ms", "%lu", &residency) == 1);
	close(gt_fd);

	return residency;
}

static uint64_t add_format_config(const char *xe_device, const char *format, uint64_t val)
{
	int ret;
	uint32_t shift;
	uint64_t config;

	ret = perf_event_format(xe_device, format, &shift);
	igt_assert(ret >= 0);
	config = val << shift;

	return config;
}

static uint64_t get_event_config(int xe, unsigned int gt, struct drm_xe_engine_class_instance *eci,
				 const char *event)
{
	char xe_device[100];
	uint64_t pmu_config = 0;
	int ret;

	xe_perf_device(xe, xe_device, sizeof(xe_device));
	ret = perf_event_config(xe_device, event, &pmu_config);
	igt_assert(ret >= 0);
	pmu_config |= add_format_config(xe_device, "gt", gt);

	if (eci) {
		pmu_config |= add_format_config(xe_device, "engine_class", eci->engine_class);
		pmu_config |= add_format_config(xe_device, "engine_instance", eci->engine_instance);
	}

	return pmu_config;
}

/**
 * SUBTEST: engine-activity-idle
 * Description: Test to validate engine activity shows no load when idle
 *
 * SUBTEST: engine-activity-load-idle
 * Description: Test to validate engine activity with full load followed by trailing idle
 *
 * SUBTEST: engine-activity-load
 * Description: Test to validate engine activity stats by running a workload and
 *              reading engine active ticks and engine total ticks PMU counters
 */
static void engine_activity(int fd, struct drm_xe_engine_class_instance *eci, unsigned int flags)
{
	uint64_t config, engine_active_ticks, engine_total_ticks, before[2], after[2];
	struct xe_cork *cork = NULL;
	uint32_t vm;
	int pmu_fd[2];

	config = get_event_config(fd, eci->gt_id, eci, "engine-active-ticks");
	pmu_fd[0] = open_group(fd, config, -1);

	config = get_event_config(fd, eci->gt_id, eci, "engine-total-ticks");
	pmu_fd[1] = open_group(fd, config, pmu_fd[0]);

	vm = xe_vm_create(fd, 0, 0);

	if (flags & TEST_LOAD) {
		cork = xe_cork_create_opts(fd, eci, vm, 1, 1);
		xe_cork_sync_start(fd, cork);
	}

	pmu_read_multi(pmu_fd[0], 2, before);
	usleep(SLEEP_DURATION * USEC_PER_SEC);
	if (flags & TEST_TRAILING_IDLE)
		xe_cork_sync_end(fd, cork);
	pmu_read_multi(pmu_fd[0], 2, after);

	if ((flags & TEST_LOAD) && !cork->ended)
		xe_cork_sync_end(fd, cork);

	engine_active_ticks = after[0] - before[0];
	engine_total_ticks = after[1] - before[1];

	igt_debug("Engine active ticks:  after %ld, before %ld delta %ld\n", after[0], before[0],
		  engine_active_ticks);
	igt_debug("Engine total ticks: after %ld, before %ld delta %ld\n", after[1], before[1],
		  engine_total_ticks);

	if (cork)
		xe_cork_destroy(fd, cork);

	xe_vm_destroy(fd, vm);

	close(pmu_fd[0]);
	close(pmu_fd[1]);

	if (flags & TEST_LOAD)
		assert_within_epsilon(engine_active_ticks, engine_total_ticks, tolerance);
	else
		igt_assert(!engine_active_ticks);
}

/**
 * SUBTEST: gt-c6-idle
 * Description: Basic residency test to validate idle residency
 *		measured over a time interval is within the tolerance
 */
static void test_gt_c6_idle(int xe, unsigned int gt)
{
	int pmu_fd;
	uint64_t pmu_config;
	uint64_t ts[2];
	unsigned long slept, start, end;
	uint64_t val;

	/* Get the PMU config for the gt-c6 event */
	pmu_config = get_event_config(xe, gt, NULL, "gt-c6-residency");

	pmu_fd = open_pmu(xe, pmu_config);

	igt_require_f(igt_wait(xe_gt_is_in_c6(xe, gt), 1000, 10), "GT %d should be in C6\n", gt);

	/* While idle check full RC6. */
	start = read_idle_residency(xe, gt);
	val = __pmu_read_single(pmu_fd, &ts[0]);
	slept = igt_measured_usleep(SLEEP_DURATION * USEC_PER_SEC) / 1000;
	end = read_idle_residency(xe, gt);
	val = __pmu_read_single(pmu_fd, &ts[1]) - val;

	igt_debug("gt%u: slept=%lu, perf=%"PRIu64"\n",
		  gt, slept,  val);

	igt_debug("Start res: %lu, end_res: %lu", start, end);

	assert_within_epsilon(val,
			      (ts[1] - ts[0])/USEC_PER_SEC,
			      tolerance);
	close(pmu_fd);
}

igt_main
{
	int fd, gt;
	struct drm_xe_engine_class_instance *eci;

	igt_fixture {
		fd = drm_open_driver(DRIVER_XE);
	}

	igt_describe("Validate PMU gt-c6 residency counters when idle");
	igt_subtest("gt-c6-idle") {
		igt_require(!IS_PONTEVECCHIO(xe_dev_id(fd)));
		xe_for_each_gt(fd, gt)
			test_gt_c6_idle(fd, gt);
	}

	igt_describe("Validate there is no engine activity when idle");
	test_each_engine("engine-activity-idle", fd, eci)
		engine_activity(fd, eci, 0);

	igt_describe("Validate engine activity with load and trailing idle");
	test_each_engine("engine-activity-load-idle", fd, eci)
		engine_activity(fd, eci, TEST_LOAD | TEST_TRAILING_IDLE);

	igt_describe("Validate engine activity with workload");
	test_each_engine("engine-activity-load", fd, eci)
		engine_activity(fd, eci, TEST_LOAD);

	igt_fixture {
		close(fd);
	}
}
