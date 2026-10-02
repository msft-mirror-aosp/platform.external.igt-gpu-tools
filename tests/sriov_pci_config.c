// SPDX-License-Identifier: MIT
/*
 * Copyright(c) 2026 Intel Corporation. All rights reserved.
 */

#include "drmtest.h"
#include "igt.h"
#include "igt_pci.h"
#include "igt_pci_regs.h"
#include "igt_sriov_device.h"

#define VF_PCI_EXP_DEVSTA_EPRD		(1 << 6)
#define VF_PCI_EXP_LNKSTA2_DEEMPHASIS	(1 << 0)

/**
 * TEST: SR-IOV PCI configuration
 * Category: Core
 * Mega feature: SR-IOV
 * Sub-category: PCI configuration
 * Functionality: SR-IOV-related PCI configuration space
 * Description: Validate PCI configuration fields required for SR-IOV,
 * including Virtual Functions.
 *
 * SUBTEST: vf-common-header
 * Description: Validate that the VF Vendor ID and Device ID read as 0xffff,
 * and that the I/O, Memory, and INTx Disable bits in Command, the Immediate
 * Readiness and Interrupt Status bits in Status, the Header Layout bits in
 * Header Type, and the BIST Completion Code are all zero.
 *
 * SUBTEST: vf-bars
 * Description: Validate that VF BAR address bits read as zero. The BAR type
 * and attribute bits are static and may remain set; the upper dword of a
 * 64-bit BAR must read as zero.
 *
 * SUBTEST: vf-pcie-capabilities
 * Description: Validate that FLR is supported, and Phantom Functions Supported,
 * Auxiliary Power Detected, Emergency Power Reduction Detected, Current Link
 * Speed, Negotiated Link Width, and Link Autonomous Bandwidth Status are all
 * zero.
 *
 * SUBTEST: vf-pcie-capabilities-2
 * Description: Validate that the Current De-emphasis Level field in Link
 * Status 2 is RsvdZ and therefore reads as zero for PCIe Capability Version 2
 * and later.
 *
 * SUBTEST: vf-power-management-capability
 * Description: Validate that Data Select in Power Management Control and
 * Power Management Data are ROZ and therefore read as zero when the optional
 * Power Management capability is present.
 *
 * SUBTEST: vf-advanced-error-reporting
 * Description: Validate that the specified ROZ bits in AER Uncorrectable
 * Error Status, Mask, and Severity, and Correctable Error Status read as zero
 * when the optional AER capability is present.
 *
 * SUBTEST: vf-ats-capability
 * Description: Validate that ATS Queue Depth and the Smallest Translation Unit
 * (STU) are ROZ and therefore read as zero when the optional ATS capability
 * is present.
 */

IGT_TEST_DESCRIPTION("Validate PCI configuration fields required for SR-IOV, "
		     "including Virtual Functions");

static void read_vf_config(int pf_fd, uint8_t *config)
{
	char *vf_slot;
	int ret;

	vf_slot = igt_sriov_get_vf_pci_slot_alloc(pf_fd, 1);
	igt_assert(vf_slot);
	ret = igt_pci_config_raw_read(vf_slot, config, PCI_CFG_SPACE_EXP_SIZE);
	free(vf_slot);
	igt_assert_f(!ret, "Failed to read VF config space (%d)\n", ret);
}

static void check_vf_common_header(const uint8_t *config)
{
	uint16_t command = igt_pci_config_raw_read_u16(config, PCI_COMMAND);
	uint16_t status = igt_pci_config_raw_read_u16(config, PCI_STATUS);
	uint16_t vendor_id = igt_pci_config_raw_read_u16(config, PCI_VENDOR_ID);
	uint16_t device_id = igt_pci_config_raw_read_u16(config, PCI_DEVICE_ID);

	igt_assert_f(vendor_id == 0xffff, "Vendor ID: expected 0xffff, got 0x%04x\n", vendor_id);
	igt_assert_f(device_id == 0xffff, "Device ID: expected 0xffff, got 0x%04x\n", device_id);
	igt_assert_f(!(command & (PCI_COMMAND_IO | PCI_COMMAND_MEMORY |
				  PCI_COMMAND_INTX_DISABLE)),
		     "Command: expected I/O, Memory, and INTx Disable bits clear, got 0x%04x\n",
		     command);
	igt_assert_f(!(status & (PCI_STATUS_IMM_READY | PCI_STATUS_INTERRUPT)),
		     "Status: expected Immediate Readiness and Interrupt Status bits clear, got 0x%04x\n",
		     status);
	igt_assert_f(!(config[PCI_HEADER_TYPE] & PCI_HEADER_TYPE_MASK),
		     "Header Type: expected Header Layout 0, got 0x%02x\n",
		     config[PCI_HEADER_TYPE]);
	igt_assert_f(!(config[PCI_BIST] & PCI_BIST_CODE_MASK),
		     "BIST: expected Completion Code 0, got 0x%02x\n", config[PCI_BIST]);
}

static void check_vf_bars(const uint8_t *config)
{
	for (int bar = 0; bar < 6; bar++) {
		uint32_t value =
			igt_pci_config_raw_read_u32(config,
						    PCI_BASE_ADDRESS_0 + bar * sizeof(uint32_t));
		bool is_io = value & PCI_BASE_ADDRESS_SPACE;
		uint32_t attr_mask = is_io ? PCI_BASE_ADDRESS_SPACE : 0xfu;
		bool is_mem64 = !is_io &&
				(value & PCI_BASE_ADDRESS_MEM_TYPE_MASK) ==
				PCI_BASE_ADDRESS_MEM_TYPE_64;

		/* Only the address bits are hardwired to zero; attributes are static. */
		igt_assert_f(!(value & ~attr_mask), "BAR%d: expected address 0x00000000, got 0x%08x\n",
			     bar, value);

		/* The upper dword of a 64-bit BAR holds pure address bits, no type bits. */
		if (is_mem64) {
			unsigned int upper_offset = PCI_BASE_ADDRESS_0 +
						    (bar + 1) * sizeof(uint32_t);
			uint32_t upper =
				igt_pci_config_raw_read_u32(config, upper_offset);

			igt_assert_f(bar < 5, "BAR%d: 64-bit memory type in the last BAR (0x%08x)\n",
				     bar, value);
			igt_assert_f(!upper, "BAR%d (upper): expected 0x00000000, got 0x%08x\n",
				     bar + 1, upper);
			bar++;
		}
	}
}

static int get_pcie_cap(const uint8_t *config)
{
	int pcie_cap = igt_pci_config_raw_find_cap_offset(config, PCI_CAP_ID_EXP);

	igt_assert_f(pcie_cap > 0, "VF has no PCI Express capability (%d)\n", pcie_cap);

	return pcie_cap;
}

static void check_vf_pcie_capabilities(const uint8_t *config)
{
	int pcie_cap = get_pcie_cap(config);
	uint32_t pcie_devcap = igt_pci_config_raw_read_u32(config, pcie_cap + PCI_EXP_DEVCAP);
	uint16_t pcie_devsta = igt_pci_config_raw_read_u16(config, pcie_cap + PCI_EXP_DEVSTA);
	uint16_t pcie_lnksta = igt_pci_config_raw_read_u16(config, pcie_cap + PCI_EXP_LNKSTA);

	igt_assert_f(pcie_devcap & PCI_EXP_DEVCAP_FLR,
		     "Device Capabilities: FLR bit is clear (0x%08x)\n", pcie_devcap);
	igt_assert_f(!(pcie_devcap & PCI_EXP_DEVCAP_PHANTOM),
		     "Device Capabilities: Phantom Functions Supported must be zero (0x%08x)\n",
		     pcie_devcap);
	igt_assert_f(!(pcie_devsta & (PCI_EXP_DEVSTA_AUXPD |
					  VF_PCI_EXP_DEVSTA_EPRD)),
		     "Device Status: Aux Power Detected or Emergency Power Reduction Detected is set (0x%04x)\n",
		     pcie_devsta);
	igt_assert_f(!(pcie_lnksta & (PCI_EXP_LNKSTA_CLS | PCI_EXP_LNKSTA_NLW |
					 PCI_EXP_LNKSTA_LABS)),
		     "Link Status: Current Link Speed, Negotiated Link Width, or Link Autonomous Bandwidth Status is set (0x%04x)\n",
		     pcie_lnksta);
}

static void check_vf_pcie_capabilities_2(const uint8_t *config)
{
	int pcie_cap = get_pcie_cap(config);
	uint16_t flags = igt_pci_config_raw_read_u16(config, pcie_cap + PCI_EXP_FLAGS);
	uint16_t lnksta2;

	if ((flags & PCI_EXP_FLAGS_VERS) < 2)
		igt_skip("PCI Express Capability Version 2 is not present\n");

	lnksta2 = igt_pci_config_raw_read_u16(config, pcie_cap + PCI_EXP_LNKSTA2);
	igt_assert_f(!(lnksta2 & VF_PCI_EXP_LNKSTA2_DEEMPHASIS),
		     "Link Status 2: Current De-emphasis Level must be zero (0x%04x)\n",
		     lnksta2);
}

static int get_optional_cap(const uint8_t *config, uint8_t cap_id, const char *name)
{
	int cap = igt_pci_config_raw_find_cap_offset(config, cap_id);

	igt_assert_f(cap >= 0, "Invalid VF PCI capability list (%d)\n", cap);
	if (!cap)
		igt_skip("%s\n", name);

	return cap;
}

static int get_optional_ext_cap(const uint8_t *config, uint16_t cap_id, const char *name)
{
	int cap = igt_pci_config_raw_find_ext_cap_offset(config, cap_id);

	igt_assert_f(cap >= 0, "Invalid VF extended capability list (%d)\n", cap);
	if (!cap)
		igt_skip("%s\n", name);

	return cap;
}

static void check_vf_power_management_capability(const uint8_t *config)
{
	int pm_cap = get_optional_cap(config, PCI_CAP_ID_PM,
				     "Power Management capability is not present");
	uint16_t pm_ctrl = igt_pci_config_raw_read_u16(config, pm_cap + PCI_PM_CTRL);
	uint8_t pm_data = config[pm_cap + PCI_PM_DATA_REGISTER];

	igt_assert_f(!(pm_ctrl & PCI_PM_CTRL_DATA_SEL_MASK),
		     "Power Management Control: Data Select must be zero (0x%04x)\n", pm_ctrl);
	igt_assert_f(!pm_data, "Power Management Data: expected 0x00, got 0x%02x\n", pm_data);
}

static void check_vf_advanced_error_reporting(const uint8_t *config)
{
	const uint32_t uncor_roz = PCI_ERR_UNC_DLP | PCI_ERR_UNC_SURPDN |
				   PCI_ERR_UNC_FCP | PCI_ERR_UNC_RX_OVER |
				   PCI_ERR_UNC_MALF_TLP | PCI_ERR_UNC_ECRC;
	const uint32_t cor_roz = PCI_ERR_COR_RCVR | PCI_ERR_COR_BAD_TLP |
					 PCI_ERR_COR_REP_ROLL | PCI_ERR_COR_REP_TIMER;
	int aer_cap = get_optional_ext_cap(config, PCI_EXT_CAP_ID_ERR,
					   "Advanced Error Reporting capability is not present");
	uint32_t uncor_status = igt_pci_config_raw_read_u32(config,
							     aer_cap + PCI_ERR_UNCOR_STATUS);
	uint32_t uncor_mask = igt_pci_config_raw_read_u32(config,
							   aer_cap + PCI_ERR_UNCOR_MASK);
	uint32_t uncor_severity = igt_pci_config_raw_read_u32(config,
							       aer_cap + PCI_ERR_UNCOR_SEVER);
	uint32_t cor_status = igt_pci_config_raw_read_u32(config,
							   aer_cap + PCI_ERR_COR_STATUS);

	igt_assert_f(!(uncor_status & uncor_roz),
		     "AER Uncorrectable Error Status: VF ROZ bits are set (0x%08x)\n",
		     uncor_status);
	igt_assert_f(!(uncor_mask & uncor_roz),
		     "AER Uncorrectable Error Mask: VF ROZ bits are set (0x%08x)\n",
		     uncor_mask);
	igt_assert_f(!(uncor_severity & uncor_roz),
		     "AER Uncorrectable Error Severity: VF ROZ bits are set (0x%08x)\n",
		     uncor_severity);
	igt_assert_f(!(cor_status & cor_roz),
		     "AER Correctable Error Status: VF ROZ bits are set (0x%08x)\n",
		     cor_status);
}

static void check_vf_ats_capability(const uint8_t *config)
{
	int ats_cap = get_optional_ext_cap(config, PCI_EXT_CAP_ID_ATS,
					   "ATS capability is not present");
	uint16_t ats_capability = igt_pci_config_raw_read_u16(config, ats_cap + PCI_ATS_CAP);
	uint16_t ats_control = igt_pci_config_raw_read_u16(config, ats_cap + PCI_ATS_CTRL);

	igt_assert_f(!PCI_ATS_CAP_QDEP(ats_capability),
		     "ATS Capability: Queue Depth must be zero (0x%04x)\n", ats_capability);
	igt_assert_f(!PCI_ATS_CTRL_STU(ats_control),
		     "ATS Control: STU must be zero (0x%04x)\n", ats_control);
}

int igt_main()
{
	uint8_t vf_config[PCI_CFG_SPACE_EXP_SIZE];
	int pf_fd = -1;
	bool autoprobe = false;

	igt_fixture() {
		pf_fd = drm_open_driver(DRIVER_ANY);
		igt_require(igt_sriov_is_pf(pf_fd));
		igt_require(igt_sriov_vfs_supported(pf_fd));
		igt_require(igt_sriov_get_enabled_vfs(pf_fd) == 0);
		autoprobe = igt_sriov_is_driver_autoprobe_enabled(pf_fd);
		igt_sriov_install_exit_handler(pf_fd, NULL, NULL);

		igt_sriov_disable_driver_autoprobe(pf_fd);
		igt_sriov_enable_vfs(pf_fd, 1);
		igt_require_f(!igt_pci_system_reinit(), "Failed to refresh PCI state\n");
		read_vf_config(pf_fd, vf_config);
	}

	igt_describe("Validate the VF PCI common header");
	igt_subtest("vf-common-header") {
		check_vf_common_header(vf_config);
	}

	igt_describe("Validate the VF BARs");
	igt_subtest("vf-bars") {
		check_vf_bars(vf_config);
	}

	igt_describe("Validate the VF PCI Express capability");
	igt_subtest("vf-pcie-capabilities") {
		check_vf_pcie_capabilities(vf_config);
	}

	igt_describe("Validate the VF PCI Express Capability Version 2 fields");
	igt_subtest("vf-pcie-capabilities-2") {
		check_vf_pcie_capabilities_2(vf_config);
	}

	igt_describe("Validate the optional VF Power Management capability");
	igt_subtest("vf-power-management-capability") {
		check_vf_power_management_capability(vf_config);
	}

	igt_describe("Validate the optional VF AER capability");
	igt_subtest("vf-advanced-error-reporting") {
		check_vf_advanced_error_reporting(vf_config);
	}

	igt_describe("Validate the optional VF ATS capability");
	igt_subtest("vf-ats-capability") {
		check_vf_ats_capability(vf_config);
	}

	igt_fixture() {
		igt_abort_on_f(pf_fd < 0, "Device is not accessible\n");
		igt_sriov_disable_vfs(pf_fd);
		igt_abort_on_f(igt_sriov_get_enabled_vfs(pf_fd) > 0,
			       "Failed to disable VF(s)\n");
		autoprobe ? igt_sriov_enable_driver_autoprobe(pf_fd) :
			    igt_sriov_disable_driver_autoprobe(pf_fd);
		igt_abort_on_f(autoprobe != igt_sriov_is_driver_autoprobe_enabled(pf_fd),
			       "Failed to restore sriov_drivers_autoprobe value\n");
		igt_sriov_clear_exit_handler();
		close(pf_fd);
	}
}
