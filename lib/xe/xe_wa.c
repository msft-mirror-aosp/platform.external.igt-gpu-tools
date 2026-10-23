// SPDX-License-Identifier: MIT
/*
 * Copyright © 2026 Intel Corporation
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

#include "igt_debugfs.h"
#include "igt_sysfs.h"

#include "xe/xe_query.h"
#include "xe/xe_wa.h"

static int debugfs_file_has_wa(int drm_fd, int debugfs_fd,
			       const char *debugfs_name, const char *wa)
{
	char *debugfs_dump;

	if (!igt_debugfs_exists(drm_fd, debugfs_name, O_RDONLY))
		return -1;

	debugfs_dump = igt_sysfs_get(debugfs_fd, debugfs_name);
	if (debugfs_dump) {
		char *has_wa = strstr(debugfs_dump, wa);

		free(debugfs_dump);

		if (has_wa)
			return 1;
	}

	return 0;
}

/**
 * xe_wa: Check if a workaround is enabled for the device.
 * @fd: A drm file descriptor.
 * @wa: Name of the workaround to be checked.
 *
 * Returns 1 if enabled, 0 if disabled, -1 on error.
 */
int xe_wa(int fd, const char *wa)
{
	int ret = 0;
	int debugfs_fd;
	unsigned int xe;
	char name[256];

	debugfs_fd = igt_debugfs_dir(fd);
	if (debugfs_fd == -1)
		return -1;

	xe_for_each_gt(fd, xe) {
		sprintf(name, "gt%d/workarounds", xe);
		ret = debugfs_file_has_wa(fd, debugfs_fd, name, wa);
		if (ret)
			break;
	}

	if (!ret)
		ret = debugfs_file_has_wa(fd, debugfs_fd, "workarounds", wa);

	close(debugfs_fd);
	return ret;
}
