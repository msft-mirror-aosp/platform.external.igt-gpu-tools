/* SPDX-License-Identifier: MIT
 *
 * Originally developed by Jatin Kataria at Netflix.
 * Copyright (c) 2026 Netflix, Inc.
 *
 * Adapted for IGT by Advanced Micro Devices, Inc., 2026.
 */

#ifndef AMD_MES_RESET_H
#define AMD_MES_RESET_H

#include <stdint.h>
#include "amdgpu.h"

#define MES_HW_IP_GFX       0
#define MES_HW_IP_COMPUTE   1

#define MES_DEFAULT_IB_SIZE     (64 * 1024)
#define MES_DEFAULT_CHAIN_LEN   512
#define MES_VA_BASE             0x40000000ULL
#define MES_VA_GAP              0x20000ULL
#define MES_DST_BO_SIZE         (4 * 1024 * 1024)

struct mes_ib_node {
	amdgpu_bo_handle bo_handle;
	amdgpu_va_handle va_handle;
	uint64_t va;
	uint32_t *cpu_ptr;
	uint32_t size_bytes;
};

/* Buffer management */
int mes_create_ib_bo(amdgpu_device_handle device, uint32_t size, uint64_t va, struct mes_ib_node *node);
int mes_create_dst_bo(amdgpu_device_handle device, uint32_t size, uint64_t va, uint32_t flags, struct mes_ib_node *node);
void mes_free_bo(struct mes_ib_node *node);

/* Submission */
int mes_submit_ib(amdgpu_device_handle device, amdgpu_context_handle context,
                  amdgpu_bo_handle *bo_handles, int num_handles,
                  uint64_t ib_va, uint32_t ib_bytes, int ip_type);

/* VA management */
int mes_unmap_bo_va(struct mes_ib_node *node);

#endif /* AMD_MES_RESET_H */
