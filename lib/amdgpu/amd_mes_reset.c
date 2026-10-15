/* SPDX-License-Identifier: MIT
 *
 * Originally developed by Jatin Kataria at Netflix.
 * Copyright (c) 2026 Netflix, Inc.
 *
 * Adapted for IGT by Advanced Micro Devices, Inc., 2026.
 */

#include <amdgpu_drm.h>
#include "igt.h"
#include "amd_mes_reset.h"
#include "amd_PM4.h"
#include <string.h>
#include <errno.h>

#define MES_NOP_PACKET PACKET3(PACKET3_NOP, 0)

/* ========================================================================
 * Buffer Object Management
 * ======================================================================== */

int mes_create_ib_bo(amdgpu_device_handle device, uint32_t size, uint64_t va,
		     struct mes_ib_node *node)
{
	struct amdgpu_bo_alloc_request req = {0};
	amdgpu_bo_handle buf_handle;
	void *cpu_ptr;
	int r;

	if (!node)
		return -1;

	memset(node, 0, sizeof(*node));

	req.alloc_size = size;
	req.phys_alignment = 4096;
	req.preferred_heap = AMDGPU_GEM_DOMAIN_GTT;
	req.flags = 0;

	r = amdgpu_bo_alloc(device, &req, &buf_handle);
	if (r)
		return r;

	r = amdgpu_va_range_alloc(device, amdgpu_gpu_va_range_general,
				  size, 4096, 0, &node->va, &node->va_handle,
				  0);
	if (r) {
		amdgpu_bo_free(buf_handle);
		return r;
	}

	r = amdgpu_bo_va_op_raw(device, buf_handle, 0, size, node->va,
				AMDGPU_VM_PAGE_READABLE |
				AMDGPU_VM_PAGE_WRITEABLE |
				AMDGPU_VM_PAGE_EXECUTABLE,
				AMDGPU_VA_OP_MAP);
	if (r) {
		amdgpu_va_range_free(node->va_handle);
		amdgpu_bo_free(buf_handle);
		return r;
	}

	r = amdgpu_bo_cpu_map(buf_handle, &cpu_ptr);
	if (r) {
		amdgpu_bo_va_op(buf_handle, 0, size, node->va, 0, AMDGPU_VA_OP_CLEAR);
		amdgpu_va_range_free(node->va_handle);
		amdgpu_bo_free(buf_handle);
		return r;
	}

	node->bo_handle = buf_handle;
	node->cpu_ptr = (uint32_t *)cpu_ptr;
	node->size_bytes = size;

	return 0;
}

int mes_create_dst_bo(amdgpu_device_handle device, uint32_t size, uint64_t va,
		      uint32_t flags, struct mes_ib_node *node)
{
	struct amdgpu_bo_alloc_request req = {0};
	amdgpu_bo_handle buf_handle;
	void *cpu_ptr = NULL;
	int r;

	if (!node)
		return -1;

	memset(node, 0, sizeof(*node));

	req.alloc_size = size;
	req.phys_alignment = 4096;
	req.preferred_heap = AMDGPU_GEM_DOMAIN_GTT;
	req.flags = 0;

	r = amdgpu_bo_alloc(device, &req, &buf_handle);
	if (r)
		return r;

	r = amdgpu_va_range_alloc(device, amdgpu_gpu_va_range_general,
				  size, 4096, 0, &node->va, &node->va_handle,
				  0);
	if (r) {
		amdgpu_bo_free(buf_handle);
		return r;
	}

	r = amdgpu_bo_va_op_raw(device, buf_handle, 0, size, node->va, flags,
				AMDGPU_VA_OP_MAP);
	if (r) {
		amdgpu_va_range_free(node->va_handle);
		amdgpu_bo_free(buf_handle);
		return r;
	}

	if (flags & AMDGPU_VM_PAGE_READABLE) {
		r = amdgpu_bo_cpu_map(buf_handle, &cpu_ptr);
		if (r) {
			amdgpu_bo_va_op(buf_handle, 0, size, node->va, 0,
					AMDGPU_VA_OP_CLEAR);
			amdgpu_va_range_free(node->va_handle);
			amdgpu_bo_free(buf_handle);
			return r;
		}
	}

	node->bo_handle = buf_handle;
	node->cpu_ptr = (uint32_t *)cpu_ptr;
	node->size_bytes = size;

	return 0;
}

void mes_free_bo(struct mes_ib_node *node)
{
	if (!node || !node->bo_handle)
		return;

	if (node->cpu_ptr)
		amdgpu_bo_cpu_unmap(node->bo_handle);

	if (node->va)
		amdgpu_bo_va_op(node->bo_handle, 0, node->size_bytes,
				node->va, 0, AMDGPU_VA_OP_CLEAR);

	if (node->va_handle)
		amdgpu_va_range_free(node->va_handle);

	amdgpu_bo_free(node->bo_handle);
	memset(node, 0, sizeof(*node));
}

/* ========================================================================
 * IB Filling Functions
 * ======================================================================== */

int mes_submit_ib(amdgpu_device_handle device, amdgpu_context_handle context,
		  amdgpu_bo_handle *bo_handles, int num_handles,
		  uint64_t ib_va, uint32_t ib_bytes, int ip_type)
{
	struct amdgpu_cs_ib_info ib_info = {0};
	struct amdgpu_cs_request cs_req = {0};
	amdgpu_bo_list_handle bo_list = NULL;
	int r;

	/* Create BO list */
	r = amdgpu_bo_list_create(device, num_handles, bo_handles, NULL,
				  &bo_list);
	if (r)
		return r;

	/* Setup IB */
	ib_info.ib_mc_address = ib_va;
	ib_info.size = ib_bytes / 4;

	/* Setup CS request */
	cs_req.ip_type = ip_type;
	cs_req.ring = 0;
	cs_req.number_of_ibs = 1;
	cs_req.ibs = &ib_info;
	cs_req.resources = bo_list;

	/* Submit */
	r = amdgpu_cs_submit(context, 0, &cs_req, 1);

	/* Cleanup */
	amdgpu_bo_list_destroy(bo_list);

	return r;
}

int mes_unmap_bo_va(struct mes_ib_node *node)
{
	int r;

	if (!node || !node->bo_handle || !node->va)
		return -EINVAL;

	r = amdgpu_bo_va_op(node->bo_handle, 0, node->size_bytes,
			    node->va, 0, AMDGPU_VA_OP_UNMAP);
	if (!r)
		node->va = 0;

	return r;
}
