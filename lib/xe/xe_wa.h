/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2025 Intel Corporation
 */

#ifndef XE_WA_H
#define XE_WA_H

#include <stdbool.h>

struct xe_device;

bool xe_wa(int fd, const char *wa);

void xe_wa_build_cache(struct xe_device *xe_dev);
void xe_wa_free_cache(struct xe_device *xe_dev);

#endif /* XE_WA_H */
