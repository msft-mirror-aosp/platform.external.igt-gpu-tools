/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2026 Intel Corporation
 */

#ifndef __IGT_PCI_REGS_H__
#define __IGT_PCI_REGS_H__

/*
 * PCI register definitions, following <linux/pci_regs.h> naming and values.
 * Defined here as the system header may be too old to provide all of them.
 */

#define PCI_CFG_SPACE_SIZE	256
#define PCI_CFG_SPACE_EXP_SIZE	4096

#define PCI_VENDOR_ID		0x00	/* 16 bits */
#define PCI_DEVICE_ID		0x02	/* 16 bits */
#define PCI_COMMAND		0x04	/* 16 bits */
#define  PCI_COMMAND_IO		0x1	/* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY	0x2	/* Enable response in Memory space */
#define  PCI_COMMAND_INTX_DISABLE 0x400 /* INTx Emulation Disable */
#define PCI_STATUS		0x06	/* 16 bits */
#define  PCI_STATUS_IMM_READY	0x01	/* Immediate Readiness */
#define  PCI_STATUS_INTERRUPT	0x08	/* Interrupt status */
#define PCI_HEADER_TYPE		0x0e	/* 8 bits */
#define  PCI_HEADER_TYPE_MASK		0x7f
#define PCI_BIST		0x0f	/* 8 bits */
#define  PCI_BIST_CODE_MASK	0x0f	/* Return result */
#define PCI_BASE_ADDRESS_0	0x10	/* 32 bits */
#define  PCI_BASE_ADDRESS_SPACE		0x01	/* 0 = memory, 1 = I/O */
#define  PCI_BASE_ADDRESS_MEM_TYPE_MASK	0x06
#define  PCI_BASE_ADDRESS_MEM_TYPE_64	0x04	/* 64 bit address */

/* Capability IDs */
#define  PCI_CAP_ID_EXP		0x10	/* PCI Express */

/* PCI Express capability */
#define PCI_EXP_FLAGS		2	/* Capabilities register */
#define  PCI_EXP_FLAGS_VERS	0x000f	/* Capability version */
#define PCI_EXP_DEVCAP		4	/* Device capabilities */
#define  PCI_EXP_DEVCAP_PHANTOM	0x00000018 /* Phantom functions */
#define  PCI_EXP_DEVCAP_FLR     0x10000000 /* Function Level Reset */
#define PCI_EXP_DEVSTA		10	/* Device Status */
#define  PCI_EXP_DEVSTA_AUXPD	0x0010	/* AUX Power Detected */
#define PCI_EXP_LNKSTA		18	/* Link Status */
#define  PCI_EXP_LNKSTA_CLS	0x000f	/* Current Link Speed */
#define  PCI_EXP_LNKSTA_NLW	0x03f0	/* Negotiated Link Width */
#define  PCI_EXP_LNKSTA_LABS	0x8000	/* Link Autonomous Bandwidth Status */
#define PCI_EXP_LNKSTA2		50	/* Link Status 2 */

#endif /* __IGT_PCI_REGS_H__ */
