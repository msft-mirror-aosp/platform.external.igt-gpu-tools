// SPDX-License-Identifier: MIT
/*
 * Copyright © 2026 Google
 *
 * Authors:
 *   Louis Chauvet <louis.chauvet@bootlin.com>
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "drmtest.h"
#include "i915/i915_dp.h"
#include "igt_core.h"
#include "igt_kms.h"
#include "igt_dp.h"

/**
 * igt_dp_get_current_link_rate: Get current link rate on a display port
 * @drm_fd: DRM file descriptor
 * @output: igt_output_t object representing the display port
 *
 * Returns:
 * The current link rate in kb/s, or a negative error code on failure.
 */
int igt_dp_get_current_link_rate(int drm_fd, igt_output_t *output)
{
	if (is_intel_device(drm_fd))
		/*
		 * i915_dp_get_current_link_rate returns the value in tens of kb/s because
		 * that what the kernel uses. Convert it to kb/s to have a sane unit...
		 */
		return i915_dp_get_current_link_rate(drm_fd, output) * 10;

	igt_assert_f(false, "Current drm device is not able to report used link rate\n");
	return -EINVAL;
}

/**
 * igt_dp_get_current_lane_count: Get current lane count on a display port
 * @drm_fd: DRM file descriptor
 * @output: igt_output_t object representing the display port
 *
 * Returns:
 * The number of active lanes, or a negative error code on failure.
 */
int igt_dp_get_current_lane_count(int drm_fd, igt_output_t *output)
{
	if (is_intel_device(drm_fd))
		return i915_dp_get_current_lane_count(drm_fd, output);

	igt_assert_f(false, "Current drm device is not able to report used lane count\n");
	return -EINVAL;
}

/**
 * igt_dp_get_max_link_rate: Get maximum link rate on a display port
 * @drm_fd:		DRM file descriptor
 * @output:		igt_output_t object representing the display port
 *
 * Returns:
 * The maximum link rate in kb/s, or a negative error code on failure.
 */
int igt_dp_get_max_link_rate(int drm_fd, igt_output_t *output)
{
	if (is_intel_device(drm_fd))
		/*
		 * i915_dp_get_max_link_rate returns the value in tens of kb/s because
		 * that what the kernel uses. Convert it to kb/s to have a sane unit...
		 */
		return i915_dp_get_max_link_rate(drm_fd, output) * 10;

	igt_assert_f(false, "Current drm device is not able to report max link rate\n");
	return -EINVAL;
}

/**
 * igt_dp_get_max_supported_rate: Get maximum supported link rate on a display port
 * @drm_fd: DRM file descriptor
 * @output: igt_output_t object representing the display port
 *
 * Returns:
 * The maximum supported link rate in kb/s, or a negative error code on failure.
 */
int igt_dp_get_max_supported_rate(int drm_fd, igt_output_t *output)
{
	if (is_intel_device(drm_fd))
		/*
		 * i915_dp_get_max_supported_rate returns the value in tens of kb/s because
		 * that what the kernel uses. Convert it to kb/s to have a sane unit...
		 */
		return i915_dp_get_max_supported_rate(drm_fd, output) * 10;

	igt_assert_f(false, "Current drm device is not able to report max link rate\n");
	return -EINVAL;
}

/**
 * igt_dp_get_max_lane_count: Get maximum lane count on a display port
 * @drm_fd: DRM file descriptor
 * @output: igt_output_t object representing the display port
 *
 * Returns:
 * The maximum number of lanes, or a negative error code on failure.
 */
int igt_dp_get_max_lane_count(int drm_fd, igt_output_t *output)
{
	if (is_intel_device(drm_fd))
		return i915_dp_get_max_lane_count(drm_fd, output);

	igt_assert_f(false, "Current drm device is not able to report max lane count\n");
	return -EINVAL;
}

/**
 * igt_dp_force_link_retrain: Force link retraining on a display port
 * @drm_fd:		DRM file descriptor
 * @output:		igt_output_t object representing the display port
 * @retrain_count:	Number of times to retrain the link
 */
void igt_dp_force_link_retrain(int drm_fd, igt_output_t *output, int retrain_count)
{
	if (is_intel_device(drm_fd)) {
		i915_dp_force_link_retrain(drm_fd, output, retrain_count);
		return;
	}

	igt_assert_f(false, "Current drm device does not support link retraining\n");
}

/**
 * igt_dp_get_pending_retrain: Get pending link retrain count
 * @drm_fd:		DRM file descriptor
 * @output:		igt_output_t object representing the display port
 *
 * Returns:
 * The number of pending retrain operations, or a negative error code on failure.
 */
int igt_dp_get_pending_retrain(int drm_fd, igt_output_t *output)
{
	if (is_intel_device(drm_fd))
		return i915_dp_get_pending_retrain(drm_fd, output);

	igt_assert_f(false, "Current drm device does not support pending retrain count checking\n");
	return -EINVAL;
}

/**
 * igt_dp_wait_pending_retrain: Wait for pending link retrain operations to complete
 * @drm_fd:		DRM file descriptor
 * @output:		igt_output_t object representing the display port
 *
 * This function waits for any pending link retrain operations to complete on the
 * specified display port. It polls the debugfs interface for the pending retrain
 * count until it reaches zero, indicating all retrain operations have completed.
 */
void igt_dp_wait_pending_retrain(int drm_fd, igt_output_t *output)
{
	double timeout = igt_default_display_detect_timeout();
	struct timespec start, now;

	igt_assert_eq(igt_gettime(&start), 0);

	while (1) {
		if (!igt_dp_get_pending_retrain(drm_fd, output))
			return;

		igt_assert_eq(igt_gettime(&now), 0);

		if (igt_time_elapsed(&start, &now) >= timeout)
			break;

		usleep(10000);
	}
	igt_assert_f(false, "Timeout waiting for pending retrain to complete\n");
}

/**
 * igt_dp_aux_open: Open the AUX channel device of a display port
 * @drm_fd: DRM file descriptor
 * @output: igt_output_t object representing the display port
 *
 * The AUX channel of a connector is exposed as a character device when the
 * kernel is built with CONFIG_DRM_DISPLAY_DP_AUX_CHARDEV. The device is
 * parented to the connector, so it is the connector's own sysfs directory
 * which is scanned here and not /dev, and there is at most one entry in it to
 * match. /dev is only where the matched entry is then opened from.
 *
 * Returns:
 * A file descriptor for the AUX channel device, or a negative error code on
 * failure.
 */
int igt_dp_aux_open(int drm_fd, igt_output_t *output)
{
	struct dirent *entry;
	int aux_fd = -ENOENT;
	int dir_fd;
	DIR *dir;

	dir_fd = igt_connector_sysfs_open(drm_fd, output->config.connector);
	if (dir_fd < 0)
		return -ENOENT;

	dir = fdopendir(dir_fd);
	if (!dir) {
		/* Save the error before close() gets a chance to overwrite it. */
		int err = -errno;

		close(dir_fd);
		return err;
	}

	errno = 0;

	while ((entry = readdir(dir))) {
		char path[NAME_MAX + sizeof("/dev/")];

		if (strncmp(entry->d_name, "drm_dp_aux", strlen("drm_dp_aux")))
			continue;

		snprintf(path, sizeof(path), "/dev/%s", entry->d_name);

		aux_fd = open(path, O_RDONLY);
		if (aux_fd < 0)
			aux_fd = -errno;

		break;
	}

	/* readdir() reports both the end of the directory and an error as NULL. */
	if (!entry && errno)
		aux_fd = -errno;

	closedir(dir);

	return aux_fd;
}

/**
 * igt_dp_dpcd_read: Read from the DPCD of a display port
 * @aux_fd:	AUX channel device file descriptor from igt_dp_aux_open()
 * @offset:	DPCD offset to read from
 * @buf:	Buffer to read into
 * @size:	Number of bytes to read
 *
 * Returns:
 * The number of bytes read, or a negative error code on failure.
 */
int igt_dp_dpcd_read(int aux_fd, unsigned int offset, void *buf, size_t size)
{
	ssize_t ret;

	ret = pread(aux_fd, buf, size, offset);
	if (ret < 0)
		return -errno;

	return ret;
}

/**
 * igt_dp_dpcd_read_byte: Read a single byte from the DPCD of a display port
 * @aux_fd:	AUX channel device file descriptor from igt_dp_aux_open()
 * @offset:	DPCD offset to read from
 * @val:	Location to store the value read
 *
 * Returns:
 * 0 on success, or a negative error code on failure.
 */
int igt_dp_dpcd_read_byte(int aux_fd, unsigned int offset, uint8_t *val)
{
	int ret;

	ret = igt_dp_dpcd_read(aux_fd, offset, val, sizeof(*val));
	if (ret < 0)
		return ret;

	return ret == 1 ? 0 : -EIO;
}

/*
 * DPCD registers and bits used by the sink side link checks, mirroring the
 * kernel's include/drm/display/drm_dp.h.
 */
#define DPCD_MAIN_LINK_CHANNEL_CODING_SET	0x108
#define  DPCD_SET_ANSI_8B10B			(1 << 0)
#define  DPCD_SET_ANSI_128B132B			(1 << 1)

#define DPCD_LANE0_1_STATUS			0x202
#define DPCD_LINK_STATUS_SIZE			6
#define  DPCD_LANE_CR_DONE			(1 << 0)
#define  DPCD_LANE_CHANNEL_EQ_DONE		(1 << 1)
#define  DPCD_LANE_SYMBOL_LOCKED		(1 << 2)

/* DP_LANE_ALIGN_STATUS_UPDATED, the third byte of the status block. */
#define  DPCD_INTERLANE_ALIGN_DONE		(1 << 0)
#define  DPCD_128B132B_LT_FAILED		(1 << 4)

static uint8_t dpcd_lane_status(const uint8_t link_status[DPCD_LINK_STATUS_SIZE],
				int lane)
{
	return (link_status[lane / 2] >> (4 * (lane % 2))) & 0xf;
}

/**
 * igt_dp_channel_coding_ok: Check the channel coding the sink was told to use
 * @aux_fd:	AUX channel device file descriptor from igt_dp_aux_open()
 * @uhbr:	Whether 128b/132b is expected
 *
 * Read MAIN_LINK_CHANNEL_CODING_SET and check that it selects the encoding the
 * link rate implies. This is the only direct evidence that the coding a UHBR
 * rate implies is the coding that reached the wire.
 *
 * Returns:
 * 1 when the expected coding is selected, 0 when it is not, or a negative
 * error code when the DPCD could not be read.
 */
int igt_dp_channel_coding_ok(int aux_fd, bool uhbr)
{
	uint8_t coding;
	int ret;

	ret = igt_dp_dpcd_read_byte(aux_fd, DPCD_MAIN_LINK_CHANNEL_CODING_SET,
				    &coding);
	if (ret < 0)
		return ret;

	return !!(coding & (uhbr ? DPCD_SET_ANSI_128B132B : DPCD_SET_ANSI_8B10B));
}

/**
 * igt_dp_link_status_ok: Check the sink considers the link trained
 * @aux_fd:	AUX channel device file descriptor from igt_dp_aux_open()
 * @lane_count:	Number of lanes in use
 * @uhbr:	Whether the link runs at a UHBR rate, i.e. uses 128b/132b
 *
 * Read the link status block at DP_LANE0_1_STATUS and check that every lane in
 * use is locked and the lanes are aligned.
 *
 * 128b/132b reports this differently from 8b/10b: there is no clock recovery
 * bit, and a training failure has its own bit in
 * DP_LANE_ALIGN_STATUS_UPDATED. Mirrors drm_dp_channel_eq_ok() and the
 * drm_dp_128b132b_* predicates.
 *
 * Returns:
 * 1 when the sink considers the link trained, 0 when it does not, or a
 * negative error code when the DPCD could not be read.
 */
int igt_dp_link_status_ok(int aux_fd, int lane_count, bool uhbr)
{
	uint8_t link_status[DPCD_LINK_STATUS_SIZE];
	uint8_t lane_mask;
	uint8_t align;
	int ret;
	int lane;

	ret = igt_dp_dpcd_read(aux_fd, DPCD_LANE0_1_STATUS, link_status,
			       sizeof(link_status));
	if (ret < 0)
		return ret;
	if (ret != sizeof(link_status))
		return -EIO;

	align = link_status[2];

	if (uhbr && (align & DPCD_128B132B_LT_FAILED))
		return 0;

	if (!(align & DPCD_INTERLANE_ALIGN_DONE))
		return 0;

	/* 128b/132b has no clock recovery phase, so no CR_DONE bit. */
	lane_mask = DPCD_LANE_CHANNEL_EQ_DONE | DPCD_LANE_SYMBOL_LOCKED;
	if (!uhbr)
		lane_mask |= DPCD_LANE_CR_DONE;

	for (lane = 0; lane < lane_count; lane++)
		if ((dpcd_lane_status(link_status, lane) & lane_mask) != lane_mask)
			return 0;

	return 1;
}
