/* SPDX-License-Identifier: MIT
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
 * Copyright 2022 Advanced Micro Devices, Inc.
 * Copyright 2023 Advanced Micro Devices, Inc.
 */

#ifndef AMD_FAMILY_H
#define AMD_FAMILY_H

#ifdef __cplusplus
extern "C" {
#endif

enum radeon_family {
	CHIP_UNKNOWN	= 0,
	CHIP_R300,		/* R3xx-based cores. (GFX2) */
	CHIP_R350,
	CHIP_RV350,
	CHIP_RV370,
	CHIP_RV380,
	CHIP_RS400,
	CHIP_RC410,
	CHIP_RS480,
	CHIP_R420, /* R4xx-based cores. (GFX2) */
	CHIP_R423,
	CHIP_R430,
	CHIP_R480,
	CHIP_R481,
	CHIP_RV410,
	CHIP_RS600,
	CHIP_RS690,
	CHIP_RS740,
	CHIP_RV515, /* R5xx-based cores. (GFX2) */
	CHIP_R520,
	CHIP_RV530,
	CHIP_R580,
	CHIP_RV560,
	CHIP_RV570,
	CHIP_R600, /* GFX3 (R6xx) */
	CHIP_RV610,
	CHIP_RV630,
	CHIP_RV670,
	CHIP_RV620,
	CHIP_RV635,
	CHIP_RS780,
	CHIP_RS880,
	CHIP_RV770, /* GFX3 (R7xx) */
	CHIP_RV730,
	CHIP_RV710,
	CHIP_RV740,
	CHIP_CEDAR, /* GFX4 (Evergreen) */
	CHIP_REDWOOD,
	CHIP_JUNIPER,
	CHIP_CYPRESS,
	CHIP_HEMLOCK,
	CHIP_PALM,
	CHIP_SUMO,
	CHIP_SUMO2,
	CHIP_BARTS,
	CHIP_TURKS,
	CHIP_CAICOS,
	CHIP_CAYMAN, /* GFX5 (Northern Islands) */
	CHIP_ARUBA,
	CHIP_TAHITI, /* GFX6 (Southern Islands) */
	CHIP_PITCAIRN,
	CHIP_VERDE,
	CHIP_OLAND,
	CHIP_HAINAN,
	CHIP_BONAIRE, /* GFX7 (Sea Islands) */
	CHIP_KAVERI,
	CHIP_KABINI,
	CHIP_HAWAII,
	CHIP_TONGA, /* GFX8 (Volcanic Islands & Polaris) */
	CHIP_ICELAND,
	CHIP_CARRIZO,
	CHIP_FIJI,
	CHIP_STONEY,
	CHIP_POLARIS10,
	CHIP_POLARIS11,
	CHIP_POLARIS12,
	CHIP_VEGAM,
	CHIP_VEGA10, /* GFX9 (Vega) */
	CHIP_VEGA12,
	CHIP_VEGA20,
	CHIP_RAVEN,
	CHIP_RAVEN2,
	CHIP_RENOIR,
	CHIP_ARCTURUS,
	CHIP_ALDEBARAN,
	CHIP_NAVI10,
	CHIP_NAVI12,
	CHIP_NAVI14,
	CHIP_SIENNA_CICHLID,
	CHIP_NAVY_FLOUNDER,
	CHIP_VANGOGH,
	CHIP_DIMGREY_CAVEFISH,
	CHIP_BEIGE_GOBY,
	CHIP_YELLOW_CARP,
	CHIP_GFX1036,
	CHIP_GFX1037,
	CHIP_GFX1100,
	CHIP_GFX1101,
	CHIP_GFX1102,
	CHIP_GFX1103_R1,
	CHIP_GFX1103_R2,
	CHIP_GFX1150,
	CHIP_GFX1151,
	CHIP_GFX1200,
	CHIP_LAST,
};

enum chip_class {
	CLASS_UNKNOWN = 0,
	R300,
	R400,
	R500,
	R600,
	R700,
	EVERGREEN,
	CAYMAN,
	GFX6,
	GFX7,
	GFX8,
	GFX9,
	GFX10,
	GFX10_3,
	GFX11,
	NUM_GFX_VERSIONS
};

enum ring_type {
	RING_GFX = 0,
	RING_COMPUTE,
	RING_DMA,
	RING_UVD,
	RING_VCE,
	RING_UVD_ENC,
	RING_VCN_DEC,
	RING_VCN_ENC,
	RING_VCN_JPEG,
	NUM_RING_TYPES,
};

const char *ac_get_family_name(enum radeon_family family);

#ifdef __cplusplus
}
#endif

#endif
