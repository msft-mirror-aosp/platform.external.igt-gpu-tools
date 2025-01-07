/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2023 Intel Corporation
 */

#ifndef XE_GT_H
#define XE_GT_H

#include "lib/igt_gt.h"

bool has_xe_gt_reset(int fd);
void xe_force_gt_reset_all(int fd);
igt_hang_t xe_hang_ring(int fd, uint64_t ahnd, uint32_t ctx, int ring,
				unsigned int flags);
void xe_post_hang_ring(int fd, igt_hang_t arg);
int xe_gt_stats_get_count(int fd, int gt, const char *stat);

#endif
