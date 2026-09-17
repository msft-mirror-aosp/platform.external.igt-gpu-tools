// SPDX-License-Identifier: MIT
/*
 * Copyright © 2025 Google, Inc.
 */

/**
 * TEST: Tests for sync file functionality
 * Category: Core
 * Mega feature: General Core features
 * Sub-category: CMD submission
 * Functionality: fences
 */

#include "igt.h"
#include "sync_file.h"

#include "lib/igt_syncobj.h"
#include "lib/intel_reg.h"

#include "xe_drm.h"
#include "xe/xe_ioctl.h"
#include "xe/xe_query.h"

static int sync_file_get_status(int sync_file, char **driver_name)
{
	struct sync_fence_info fence = { };
	struct sync_file_info info = {
		.num_fences = 1,
		.sync_fence_info = to_user_pointer(&fence),
	};

	do_ioctl(sync_file, SYNC_IOC_FILE_INFO, &info);
	igt_assert_eq(info.num_fences, 1);

	igt_debug("'%s'/'%s' = %d\n",
		  fence.driver_name, fence.obj_name, fence.status);

	*driver_name = strdup(fence.driver_name);

	return fence.status;
}

/**
 * SUBTEST: sync_file_race
 * Description: Check that we can safely query an exported sync file fd
 * Test category: functionality test
 */
static void test_race(int xe, struct drm_xe_engine_class_instance *eci)
{
	uint32_t vm, bo, syncobj, bind_syncobj, *batch;
	struct drm_xe_sync sync[2] = {
		{ .type = DRM_XE_SYNC_TYPE_SYNCOBJ,
		  .flags = DRM_XE_SYNC_FLAG_SIGNAL,
		},
		{ .type = DRM_XE_SYNC_TYPE_SYNCOBJ,
		  .flags = DRM_XE_SYNC_FLAG_SIGNAL,
		},
	};
	const uint32_t bbend = MI_BATCH_BUFFER_END;
	struct drm_xe_exec exec = {
		.address = 0x100000,
		.num_batch_buffer = 1,
		.num_syncs = 2,
		.syncs = to_user_pointer(sync),
	};
	char *driver_name;
	int sync_fence;
	size_t size;

	vm = xe_vm_create(xe, 0, 0);

	size = xe_bb_size(xe, sizeof(bbend));
	bo = xe_bo_create(xe, vm, size, vram_if_possible(xe, eci->gt_id),
			  DRM_XE_GEM_CREATE_FLAG_NEEDS_VISIBLE_VRAM);

	batch = xe_bo_map(xe, bo, size);
	*batch = bbend;

	exec.exec_queue_id  = xe_exec_queue_create(xe, vm, eci, 0);

	syncobj = syncobj_create(xe, 0);
	bind_syncobj = syncobj_create(xe, 0);

	sync[0].handle = bind_syncobj;
	xe_vm_bind_async(xe, vm, 0, bo, 0, exec.address, size, sync, 1);

	sync[0].flags &= ~DRM_XE_SYNC_FLAG_SIGNAL;
	sync[0].handle = bind_syncobj;
	sync[1].flags |= DRM_XE_SYNC_FLAG_SIGNAL;
	sync[1].handle = syncobj;
	xe_exec(xe, &exec);

	/* Export a sync file fence. */
	sync_fence = syncobj_handle_to_fd(xe, sync[1].handle,
					  DRM_SYNCOBJ_HANDLE_TO_FD_FLAGS_EXPORT_SYNC_FILE);

	igt_assert(syncobj_wait(xe, &syncobj, 1, INT64_MAX, 0, NULL));
	igt_assert(syncobj_wait(xe, &bind_syncobj, 1, INT64_MAX, 0, NULL));

	driver_name = NULL;
	igt_assert_eq(sync_file_get_status(sync_fence, &driver_name), 1);
	igt_assert(driver_name);
	igt_assert(!strcmp(driver_name, "drm_sched"));
	free(driver_name);

	sync[0].flags |= DRM_XE_SYNC_FLAG_SIGNAL;
	syncobj_reset(xe, &sync[0].handle, 1);
	xe_vm_unbind_async(xe, vm, 0, 0, exec.address, size, sync, 1);
	igt_assert(syncobj_wait(xe, &sync[0].handle, 1, INT64_MAX, 0, NULL));

	syncobj_destroy(xe, syncobj);
	xe_exec_queue_destroy(xe, exec.exec_queue_id);

	munmap(batch, size);
	gem_close(xe, bo);

	syncobj_destroy(xe, bind_syncobj);
	xe_vm_destroy(xe, vm);

	/* Give any delayed freeing time to run. */
	sleep(1);

	/* This should still work and not crash the kernel. */
	driver_name = NULL;
	igt_assert_eq(sync_file_get_status(sync_fence, &driver_name), 1);

	/* And must not read the original timeline/driver name either! */
	igt_assert(driver_name);
	igt_assert(!strcmp(driver_name, "detached-driver"));
	free(driver_name);

	close(sync_fence);
}

int igt_main()
{
	struct drm_xe_engine_class_instance *eci;
	int xe;

	igt_fixture() {
		xe = drm_open_driver(DRIVER_XE);
	}

	igt_subtest("sync_file_race") {
		xe_for_each_engine(xe, eci) {
			test_race(xe, eci);
			break;
		}
	}

	igt_fixture() {
		drm_close_driver(xe);
	}
}
