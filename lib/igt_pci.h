// SPDX-License-Identifier: MIT
/*
 * Copyright © 2022 Intel Corporation
 */

#ifndef __IGT_PCI_H__
#define __IGT_PCI_H__

#include <endian.h>
#include <stddef.h>
#include <stdint.h>

/* forward declaration */
struct pci_device;

#define PCI_TYPE0_1_HEADER_SIZE 0x40
#define PCI_CAPS_START 0x34
#define PCI_EXT_CAPS_START 0x100

enum pci_cap_id {
	PCI_EXPRESS_CAP_ID = 0x10
};

#define PCI_DEVICE_TYPE_OFFSET 0x2
#define PCI_DEVICE_TYPE_UPSTREAM_PORT	0x5
#define PCI_SLOT_CAP_OFFSET 0x14
#define  PCI_SLOT_PWR_CTRL_PRESENT (1 << 1)

/*
 * PCI Power Management capability registers, at the given offsets from the
 * start of the PM capability as returned by find_pci_cap_offset().
 */
#define PCI_PM_PMC_OFFSET 0x2			/* PM Capabilities */
#define  PCI_PM_PMC_PME_SUPPORT_SHIFT 11	/* PME_Support, bits [15:11] */
#define  PCI_PM_PMC_PME_SUPPORT_MASK (0x1f << PCI_PM_PMC_PME_SUPPORT_SHIFT)
#define PCI_PM_PMCS_OFFSET 0x4			/* PM Control/Status */
#define  PCI_PM_PMCS_PSTATE_MASK 0x3		/* PowerState, bits [1:0] */
#define  PCI_PM_PMCS_PME_EN (1 << 8)		/* PME_En */
#define  PCI_PM_PMCS_PME_STATUS (1 << 15)	/* PME_Status, write 1 to clear */

int find_pci_cap_offset(struct pci_device *dev, enum pci_cap_id cap_id);
int igt_pci_config_raw_read(const char *pci_slot, void *buffer, size_t length);
uint16_t igt_pci_config_raw_read_u16(const uint8_t *config, unsigned int offset);
uint32_t igt_pci_config_raw_read_u32(const uint8_t *config, unsigned int offset);
int igt_pci_config_raw_find_cap_offset(const uint8_t *config, uint8_t cap_id);
int igt_pci_config_raw_find_ext_cap_offset(const uint8_t *config, uint16_t cap_id);
int igt_pci_device_unbind(const char *pci_slot);
int igt_pci_driver_bind(const char *driver, const char *pci_slot);
int igt_pci_driver_unbind(const char *driver, const char *pci_slot);
int igt_pci_driver_unbind_all(const char *driver);
int igt_pci_set_driver_override(const char *pci_slot, const char *driver);
int igt_pci_probe_drivers(const char *pci_slot);
int igt_pci_get_bound_driver_name(const char *pci_slot, char *driver, size_t driver_len);
int igt_pci_bind_driver_override(const char *pci_slot, const char *driver,
				 unsigned int timeout_ms);
int igt_pci_unbind_driver_override(const char *pci_slot, unsigned int timeout_ms);

#endif
