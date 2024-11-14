// SPDX-License-Identifier: MIT
/*
 * Copyright(c) 2024 Intel Corporation. All rights reserved.
 */

#include <stdlib.h>

#include "igt_core.h"
#include "intel_chipset.h"
#include "linux_scaffold.h"
#include "xe/xe_mmio.h"
#include "xe/xe_sriov_provisioning.h"

/**
 * xe_sriov_shared_res_to_string:
 * @key: The shared resource of type enum xe_sriov_shared_res
 *
 * Converts a shared resource enum to its corresponding string
 * representation. It is useful for logging and debugging purposes.
 *
 * Return: A string representing the shared resource key.
 */
const char *xe_sriov_shared_res_to_string(enum xe_sriov_shared_res res)
{
	switch (res) {
	case XE_SRIOV_SHARED_RES_CONTEXTS:
		return "contexts";
	case XE_SRIOV_SHARED_RES_DOORBELLS:
		return "doorbells";
	case XE_SRIOV_SHARED_RES_GGTT:
		return "ggtt";
	case XE_SRIOV_SHARED_RES_LMEM:
		return "lmem";
	}

	return NULL;
}

#define PRE_1250_IP_VER_GGTT_PTE_VFID_MASK	GENMASK_ULL(4, 2)
#define GGTT_PTE_VFID_MASK			GENMASK_ULL(11, 2)
#define GGTT_PTE_VFID_SHIFT			2

static uint64_t get_vfid_mask(int fd)
{
	uint16_t dev_id = intel_get_drm_devid(fd);

	return (intel_graphics_ver(dev_id) >= IP_VER(12, 50)) ?
		GGTT_PTE_VFID_MASK : PRE_1250_IP_VER_GGTT_PTE_VFID_MASK;
}

/**
 * xe_sriov_find_ggtt_provisioned_pte_offsets - Find GGTT provisioned PTE offsets
 * @pf_fd: File descriptor for the Physical Function
 * @gt: GT identifier
 * @mmio: Pointer to the MMIO structure
 * @ranges: Pointer to the array of provisioned ranges
 * @nr_ranges: Pointer to the number of provisioned ranges
 *
 * Searches for GGTT provisioned PTE ranges for each VF and populates
 * the provided ranges array with the start and end offsets of each range.
 * The number of ranges found is stored in nr_ranges.
 *
 * Reads the GGTT PTEs and identifies the VF ID associated with each PTE.
 * It then groups contiguous PTEs with the same VF ID into ranges.
 * The ranges are dynamically allocated and must be freed by the caller.
 * The start and end offsets in each range are inclusive.
 *
 * Returns 0 on success, or a negative error code on failure.
 */
int xe_sriov_find_ggtt_provisioned_pte_offsets(int pf_fd, int gt, struct xe_mmio *mmio,
					       struct xe_sriov_provisioned_range **ranges,
					       unsigned int *nr_ranges)
{
	uint64_t vfid_mask = get_vfid_mask(pf_fd);
	unsigned int vf_id, current_vf_id = -1;
	uint32_t current_start = 0;
	uint32_t current_end = 0;
	xe_ggtt_pte_t pte;

	*ranges = NULL;
	*nr_ranges = 0;

	for (uint32_t offset = 0; offset < SZ_8M; offset += sizeof(xe_ggtt_pte_t)) {
		pte = xe_mmio_ggtt_read(mmio, gt, offset);
		vf_id = (pte & vfid_mask) >> GGTT_PTE_VFID_SHIFT;

		if (vf_id != current_vf_id) {
			if (current_vf_id != -1) {
				/* End the current range */
				*ranges = realloc(*ranges, (*nr_ranges + 1) *
						  sizeof(struct xe_sriov_provisioned_range));
				igt_assert(*ranges);
				igt_debug("Found VF%u ggtt range [%#x-%#x] num_ptes=%ld\n",
					  current_vf_id, current_start, current_end,
					  (current_end - current_start + sizeof(xe_ggtt_pte_t)) /
					  sizeof(xe_ggtt_pte_t));
				(*ranges)[*nr_ranges].vf_id = current_vf_id;
				(*ranges)[*nr_ranges].start = current_start;
				(*ranges)[*nr_ranges].end = current_end;
				(*nr_ranges)++;
			}
			/* Start a new range */
			current_vf_id = vf_id;
			current_start = offset;
		}
		current_end = offset;
	}

	if (current_vf_id != -1) {
		*ranges = realloc(*ranges, (*nr_ranges + 1) *
				  sizeof(struct xe_sriov_provisioned_range));
		igt_assert(*ranges);
		igt_debug("Found VF%u ggtt range [%#x-%#x] num_ptes=%ld\n",
			  current_vf_id, current_start, current_end,
			  (current_end - current_start + sizeof(xe_ggtt_pte_t)) /
			  sizeof(xe_ggtt_pte_t));
		(*ranges)[*nr_ranges].vf_id = current_vf_id;
		(*ranges)[*nr_ranges].start = current_start;
		(*ranges)[*nr_ranges].end = current_end;
		(*nr_ranges)++;
	}

	return 0;
}