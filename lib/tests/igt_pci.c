// SPDX-License-Identifier: MIT
/*
 * Copyright © 2026 Intel Corporation
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "igt_core.h"
#include "igt_pci.h"
#include "igt_pci_regs.h"

static void write_u32(uint8_t *config, unsigned int offset, uint32_t value)
{
	config[offset] = value;
	config[offset + 1] = value >> 8;
	config[offset + 2] = value >> 16;
	config[offset + 3] = value >> 24;
}

static void test_cap(void)
{
	uint8_t config[PCI_CFG_SPACE_SIZE] = {};

	config[PCI_CAPS_START] = PCI_TYPE0_1_HEADER_SIZE;
	config[PCI_TYPE0_1_HEADER_SIZE] = 0xaa;
	igt_assert_eq(igt_pci_config_raw_find_cap_offset(config, 0xaa),
		      PCI_TYPE0_1_HEADER_SIZE);

	config[PCI_CAPS_START] = 0xff;
	igt_assert_eq(igt_pci_config_raw_find_cap_offset(config, 0xaa), -ENODEV);

	config[PCI_CAPS_START] = PCI_TYPE0_1_HEADER_SIZE - 4;
	igt_assert_eq(igt_pci_config_raw_find_cap_offset(config, 0xaa), -EINVAL);

	config[PCI_CAPS_START] = PCI_TYPE0_1_HEADER_SIZE | 3;
	igt_assert_eq(igt_pci_config_raw_find_cap_offset(config, 0xaa),
		      PCI_TYPE0_1_HEADER_SIZE);

	config[PCI_CAPS_START] = PCI_TYPE0_1_HEADER_SIZE;
	config[PCI_TYPE0_1_HEADER_SIZE] = 0xbb;
	config[PCI_TYPE0_1_HEADER_SIZE + 1] = PCI_TYPE0_1_HEADER_SIZE;
	igt_assert_eq(igt_pci_config_raw_find_cap_offset(config, 0xaa), -ELOOP);
}

static void test_ext_cap(void)
{
	uint8_t config[PCI_CFG_SPACE_EXP_SIZE] = {};
	unsigned int offset;

	write_u32(config, PCI_EXT_CAPS_START, 0xaa);
	igt_assert_eq(igt_pci_config_raw_find_ext_cap_offset(config, 0xaa),
		      PCI_EXT_CAPS_START);

	write_u32(config, PCI_EXT_CAPS_START,
		  0xbb | (PCI_EXT_CAPS_START << 20));
	igt_assert_eq(igt_pci_config_raw_find_ext_cap_offset(config, 0xaa), -ELOOP);

	memset(config, 0xff, sizeof(config));
	igt_assert_eq(igt_pci_config_raw_find_ext_cap_offset(config, 0xaa), 0);

	memset(config, 0, sizeof(config));
	for (offset = PCI_EXT_CAPS_START;
	     offset < PCI_CFG_SPACE_EXP_SIZE - sizeof(uint32_t);
	     offset += sizeof(uint32_t))
		write_u32(config, offset, 0xbb | ((offset + sizeof(uint32_t)) << 20));

	igt_assert_eq(igt_pci_config_raw_find_ext_cap_offset(config, 0xaa), 0);
}

int igt_simple_main()
{
	test_cap();
	test_ext_cap();
}
