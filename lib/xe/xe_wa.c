// SPDX-License-Identifier: MIT
/*
 * Copyright © 2026 Intel Corporation
 */

#include <fcntl.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>

#include "igt_core.h"
#include "igt_debugfs.h"
#include "igt_sysfs.h"

#include "xe/xe_query.h"
#include "xe/xe_wa.h"

#include "tests/xe_wa.h"

struct xe_wa_cache {
	int num_dumps;
	char *dumps[];
};

static void free_wa_debugfs_dumps(struct xe_wa_cache *wa_cache)
{
	for (int i = 0; i < wa_cache->num_dumps && wa_cache->dumps[i]; i++) {
		free(wa_cache->dumps[i]);
		wa_cache->dumps[i] = NULL;
	}
}

static int get_wa_debugfs_count(int fd, uint64_t gt_mask)
{
	int gt;
	int count = 1; /* Device workarounds */

	xe_for_each_gt_from_mask(gt_mask, gt)
		count++;

	return count;
}

static int get_wa_debugfs_dumps(int fd, uint64_t gt_mask, struct xe_wa_cache *wa_cache)
{
	int gt;
	int debugfs_fd;
	int count;
	int ret = 0;

	debugfs_fd = igt_debugfs_dir(fd);
	if (debugfs_fd == -1)
		goto err;

	count = 0;

	if (!(wa_cache->dumps[count++] = igt_sysfs_get(debugfs_fd, "workarounds")))
		goto err;

	xe_for_each_gt_from_mask(gt_mask, gt) {
		char name[32];

		snprintf(name, sizeof(name), "gt%d/workarounds", gt);

		if (!(wa_cache->dumps[count++] = igt_sysfs_get(debugfs_fd, name)))
			goto err;
	}

	if (igt_warn_on(count != wa_cache->num_dumps))
		goto err;

	goto out;

err:
	free_wa_debugfs_dumps(wa_cache);
	ret = 1;

out:
	close(debugfs_fd);

	return ret;
}

static void free_cache(struct xe_wa_cache *wa_cache)
{
	if (!wa_cache)
		return;

	free_wa_debugfs_dumps(wa_cache);
	free(wa_cache);
}

static struct xe_wa_cache *build_cache(int fd, uint64_t gt_mask)
{
	struct xe_wa_cache *wa_cache;
	int num_dumps;

	num_dumps = get_wa_debugfs_count(fd, gt_mask);
	wa_cache = calloc(1, sizeof(*wa_cache) + num_dumps * sizeof(wa_cache->dumps[0]));

	if (igt_warn_on(!wa_cache))
		return NULL;

	wa_cache->num_dumps = num_dumps;

	/*
	 * Some vendor-neutral tests (e.g. core_setmaster) drop root
	 * privileges on purpose and it can cause
	 * get_wa_debugfs_dumps() to fail. Since those are not
	 * xe-specific and will not require information about
	 * workarounds, let's avoid creating noise with igt_warn_on()
	 * and limit ourselves to use igt_debug_on() here.
	 */
	if (igt_debug_on(get_wa_debugfs_dumps(fd, gt_mask, wa_cache))) {
		free_cache(wa_cache);

		return NULL;
	}

	return wa_cache;
}

static int debugfs_dump_has_wa(char *dump, const char *wa)
{
	size_t wa_len;
	regex_t regex;
	regmatch_t match[2];
	int ret;

	if (regcomp(&regex, "[ \t,]*([^ \t,]+)", REG_EXTENDED | REG_NEWLINE)) {
		igt_critical("Failed to compile regex, workaround %s will not be checked.\n",
			     wa);

		return -1;
	}

	wa_len = strlen(wa);
	ret = 0;

	while (*dump) {
		/*
		 * Each workaround name is indented by one tab
		 * character; unindented lines are used as "section
		 * names" identifying the type of workarounds that
		 * follow (e.g. "GT Workarounds", "Engine Workarounds"
		 * etc) and should be ignored.
		 */
		if (*dump != '\t')
			goto next_line;

		while (*dump && regexec(&regex, dump, 2, match, 0) == 0) {
			char *other_wa = dump + match[1].rm_so;
			size_t other_wa_len = match[1].rm_eo - match[1].rm_so;

			dump += match[0].rm_eo;

			if (wa_len == other_wa_len && !strncmp(wa, other_wa, wa_len)) {
				ret = 1;

				goto done;
			}
		}

	next_line:
		while (*dump && *dump != '\n')
			dump++;

		if (*dump)
			dump++;
	}

done:
	regfree(&regex);

	return ret;
}

int xe_wa_test_debugfs_dump_has_wa(char *dump, const char *wa)
{
	return debugfs_dump_has_wa(dump, wa);
}

/**
 * xe_wa: Check if a workaround is enabled for the device.
 * @fd: A drm file descriptor.
 * @wa: Name of the workaround to be checked.
 *
 * Return a boolean indicating whether the workaround is enabled.
 */
bool xe_wa(int fd, const char *wa)
{
	struct xe_wa_cache *wa_cache = xe_device_get(fd)->wa_cache;

	igt_assert(wa_cache);

	for (int i = 0; i < wa_cache->num_dumps; i++)
		if (debugfs_dump_has_wa(wa_cache->dumps[i], wa))
			return true;

	return false;
}

/**
 * xe_wa_build_cache: Build cached data for xe_wa().
 * @xe_dev: Xe device where the cache will be stashed.
 */
void xe_wa_build_cache(struct xe_device *xe_dev)
{
	xe_dev->wa_cache = build_cache(xe_dev->fd, xe_dev->gt_mask);
}

/**
 * xe_wa_free_cache: Free cached data that was built with xe_wa_build_cache().
 * @xe_dev: Xe device where the cached data is stashed.
 */
void xe_wa_free_cache(struct xe_device *xe_dev)
{
	free_cache(xe_dev->wa_cache);
	xe_dev->wa_cache = NULL;
}
