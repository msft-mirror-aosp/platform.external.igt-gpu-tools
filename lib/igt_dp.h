/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2026 Google
 *
 * Authors:
 *   Louis Chauvet <louis.chauvet@bootlin.com>
 */

#ifndef _IGT_DP_H_
#define _IGT_DP_H_

#include <stddef.h>
#include <stdint.h>

#include "igt_kms.h"

int igt_dp_get_current_link_rate(int drm_fd, igt_output_t *output);

int igt_dp_get_current_lane_count(int drm_fd, igt_output_t *output);

int igt_dp_get_max_link_rate(int drm_fd, igt_output_t *output);

int igt_dp_get_max_supported_rate(int drm_fd, igt_output_t *output);

int igt_dp_get_max_lane_count(int drm_fd, igt_output_t *output);

void igt_dp_force_link_retrain(int drm_fd, igt_output_t *output, int retrain_count);

int igt_dp_get_pending_retrain(int drm_fd, igt_output_t *output);

void igt_dp_wait_pending_retrain(int drm_fd, igt_output_t *output);

int igt_dp_aux_open(int drm_fd, igt_output_t *output);

int igt_dp_dpcd_read(int aux_fd, unsigned int offset, void *buf, size_t size);

int igt_dp_dpcd_read_byte(int aux_fd, unsigned int offset, uint8_t *val);

#endif
