// SPDX-License-Identifier: MIT
/*
 * Copyright © 2026 Intel Corporation
 */

#include <fcntl.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>

#include "igt_debugfs.h"
#include "igt_sysfs.h"

#include "xe/xe_query.h"
#include "xe/xe_wa.h"

#include "tests/xe_wa.h"

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

static int debugfs_file_has_wa(int drm_fd, int debugfs_fd,
			       const char *debugfs_name, const char *wa)
{
	char *debugfs_dump;

	if (!igt_debugfs_exists(drm_fd, debugfs_name, O_RDONLY))
		return -1;

	debugfs_dump = igt_sysfs_get(debugfs_fd, debugfs_name);
	if (debugfs_dump) {
		int ret = debugfs_dump_has_wa(debugfs_dump, wa);

		free(debugfs_dump);

		if (ret)
			return ret;
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
