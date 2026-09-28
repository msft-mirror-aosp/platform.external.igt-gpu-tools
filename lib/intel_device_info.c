#include "intel_chipset.h"
#include "pciids.h"
#include "i915_pciids_local.h"
#include "igt_core.h"
#include "igt_map.h"

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h> /* ffs() */

static const struct intel_device_info intel_generic_info = {
	.graphics_ver = 0,
	.display_ver = 0,
};

static const struct intel_device_info intel_i810_info = {
	.graphics_ver = 1,
	.display_ver = 1,
	.is_whitney = true,
	.codename = "solano" /* 815 == "whitney" ? or vice versa? */
};

static const struct intel_device_info intel_i815_info = {
	.graphics_ver = 1,
	.display_ver = 1,
	.is_whitney = true,
	.codename = "whitney"
};

static const struct intel_device_info intel_i830_info = {
	.graphics_ver = 2,
	.display_ver = 2,
	.is_almador = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "almador"
};
static const struct intel_device_info intel_i845_info = {
	.graphics_ver = 2,
	.display_ver = 2,
	.is_brookdale = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "brookdale"
};
static const struct intel_device_info intel_i855_info = {
	.graphics_ver = 2,
	.display_ver = 2,
	.is_mobile = true,
	.is_montara = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "montara"
};
static const struct intel_device_info intel_i865_info = {
	.graphics_ver = 2,
	.display_ver = 2,
	.is_springdale = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "spingdale"
};

static const struct intel_device_info intel_i915_info = {
	.graphics_ver = 3,
	.display_ver = 3,
	.is_grantsdale = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "grantsdale"
};
static const struct intel_device_info intel_i915m_info = {
	.graphics_ver = 3,
	.display_ver = 3,
	.is_mobile = true,
	.is_alviso = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "alviso"
};
static const struct intel_device_info intel_i945_info = {
	.graphics_ver = 3,
	.display_ver = 3,
	.is_lakeport = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "lakeport"
};
static const struct intel_device_info intel_i945m_info = {
	.graphics_ver = 3,
	.display_ver = 3,
	.is_mobile = true,
	.is_calistoga = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "calistoga"
};

static const struct intel_device_info intel_g33_info = {
	.graphics_ver = 3,
	.display_ver = 3,
	.is_bearlake = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "bearlake"
};

static const struct intel_device_info intel_pineview_g_info = {
	.graphics_ver = 3,
	.display_ver = 3,
	.is_pineview = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "pineview"
};

static const struct intel_device_info intel_pineview_m_info = {
	.graphics_ver = 3,
	.display_ver = 3,
	.is_mobile = true,
	.is_pineview = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "pineview"
};

static const struct intel_device_info intel_i965_info = {
	.graphics_ver = 4,
	.display_ver = 4,
	.is_broadwater = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "broadwater"
};

static const struct intel_device_info intel_i965m_info = {
	.graphics_ver = 4,
	.display_ver = 4,
	.is_mobile = true,
	.is_crestline = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "crestline"
};

static const struct intel_device_info intel_g45_info = {
	.graphics_ver = 4,
	.display_ver = 4,
	.is_eaglelake = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "eaglelake"
};
static const struct intel_device_info intel_gm45_info = {
	.graphics_ver = 4,
	.display_ver = 4,
	.is_mobile = true,
	.is_cantiga = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "cantiga"
};

static const struct intel_device_info intel_ironlake_info = {
	.graphics_ver = 5,
	.display_ver = 5,
	.is_ironlake = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "ironlake" /* clarkdale? */
};
static const struct intel_device_info intel_ironlake_m_info = {
	.graphics_ver = 5,
	.display_ver = 5,
	.is_mobile = true,
	.is_arrandale = true,
	.cmds_info = &pre_gen6_cmds_info,
	.codename = "arrandale"
};

static const struct intel_device_info intel_sandybridge_info = {
	.graphics_ver = 6,
	.display_ver = 6,
	.is_sandybridge = true,
	.cmds_info = &gen6_cmds_info,
	.codename = "sandybridge"
};
static const struct intel_device_info intel_sandybridge_m_info = {
	.graphics_ver = 6,
	.display_ver = 6,
	.is_mobile = true,
	.is_sandybridge = true,
	.cmds_info = &gen6_cmds_info,
	.codename = "sandybridge"
};

static const struct intel_device_info intel_ivybridge_info = {
	.graphics_ver = 7,
	.display_ver = 7,
	.is_ivybridge = true,
	.cmds_info = &gen6_cmds_info,
	.codename = "ivybridge"
};
static const struct intel_device_info intel_ivybridge_m_info = {
	.graphics_ver = 7,
	.display_ver = 7,
	.is_mobile = true,
	.is_ivybridge = true,
	.cmds_info = &gen6_cmds_info,
	.codename = "ivybridge"
};

static const struct intel_device_info intel_valleyview_info = {
	.graphics_ver = 7,
	.display_ver = 7,
	.is_valleyview = true,
	.cmds_info = &gen6_cmds_info,
	.codename = "valleyview"
};

#define HASWELL_FIELDS \
	.graphics_ver = 7, \
	.display_ver = 7, \
	.is_haswell = true, \
	.cmds_info = &gen6_cmds_info, \
	.codename = "haswell"

static const struct intel_device_info intel_haswell_gt1_info = {
	HASWELL_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_haswell_gt2_info = {
	HASWELL_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_haswell_gt3_info = {
	HASWELL_FIELDS,
	.gt = 3,
};

#define BROADWELL_FIELDS \
	.graphics_ver = 8, \
	.display_ver = 8, \
	.is_broadwell = true, \
	.cmds_info = &gen8_cmds_info, \
	.codename = "broadwell"

static const struct intel_device_info intel_broadwell_gt1_info = {
	BROADWELL_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_broadwell_gt2_info = {
	BROADWELL_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_broadwell_gt3_info = {
	BROADWELL_FIELDS,
	.gt = 3,
};

static const struct intel_device_info intel_broadwell_unknown_info = {
	BROADWELL_FIELDS,
};

static const struct intel_device_info intel_cherryview_info = {
	.graphics_ver = 8,
	.display_ver = 8,
	.is_cherryview = true,
	.cmds_info = &gen8_cmds_info,
	.codename = "cherryview"
};

#define SKYLAKE_FIELDS \
	.graphics_ver = 9, \
	.display_ver = 9, \
	.cmds_info = &gen11_cmds_info, \
	.codename = "skylake", \
	.is_skylake = true

static const struct intel_device_info intel_skylake_gt1_info = {
	SKYLAKE_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_skylake_gt2_info = {
	SKYLAKE_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_skylake_gt3_info = {
	SKYLAKE_FIELDS,
	.gt = 3,
};

static const struct intel_device_info intel_skylake_gt4_info = {
	SKYLAKE_FIELDS,
	.gt = 4,
};

static const struct intel_device_info intel_broxton_info = {
	.graphics_ver = 9,
	.display_ver = 9,
	.is_broxton = true,
	.cmds_info = &gen11_cmds_info,
	.codename = "broxton"
};

#define KABYLAKE_FIELDS \
	.graphics_ver = 9, \
	.display_ver = 9, \
	.is_kabylake = true, \
	.cmds_info = &gen11_cmds_info, \
	.codename = "kabylake"

static const struct intel_device_info intel_kabylake_gt1_info = {
	KABYLAKE_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_kabylake_gt2_info = {
	KABYLAKE_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_kabylake_gt3_info = {
	KABYLAKE_FIELDS,
	.gt = 3,
};

static const struct intel_device_info intel_kabylake_gt4_info = {
	KABYLAKE_FIELDS,
	.gt = 4,
};

static const struct intel_device_info intel_geminilake_info = {
	.graphics_ver = 9,
	.display_ver = 9,
	.is_geminilake = true,
	.cmds_info = &gen11_cmds_info,
	.codename = "geminilake"
};

#define COFFEELAKE_FIELDS \
	.graphics_ver = 9, \
	.display_ver = 9, \
	.is_coffeelake = true, \
	.cmds_info = &gen11_cmds_info, \
	.codename = "coffeelake"

static const struct intel_device_info intel_coffeelake_gt1_info = {
	COFFEELAKE_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_coffeelake_gt2_info = {
	COFFEELAKE_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_coffeelake_gt3_info = {
	COFFEELAKE_FIELDS,
	.gt = 3,
};

#define COMETLAKE_FIELDS \
	.graphics_ver = 9, \
	.display_ver = 9, \
	.is_cometlake = true, \
	.cmds_info = &gen11_cmds_info, \
	.codename = "cometlake"

static const struct intel_device_info intel_cometlake_gt1_info = {
	COMETLAKE_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_cometlake_gt2_info = {
	COMETLAKE_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_cannonlake_info = {
	.graphics_ver = 10,
	.display_ver = 10,
	.is_cannonlake = true,
	.cmds_info = &gen11_cmds_info,
	.codename = "cannonlake"
};

static const struct intel_device_info intel_icelake_info = {
	.graphics_ver = 11,
	.display_ver = 11,
	.is_icelake = true,
	.cmds_info = &gen11_cmds_info,
	.codename = "icelake"
};

static const struct intel_device_info intel_elkhartlake_info = {
	.graphics_ver = 11,
	.display_ver = 11,
	.is_elkhartlake = true,
	.cmds_info = &gen11_cmds_info,
	.codename = "elkhartlake"
};

static const struct intel_device_info intel_jasperlake_info = {
	.graphics_ver = 11,
	.display_ver = 11,
	.is_jasperlake = true,
	.cmds_info = &gen11_cmds_info,
	.codename = "jasperlake"
};

static const struct intel_device_info intel_tigerlake_gt1_info = {
	.graphics_ver = 12,
	.display_ver = 12,
	.is_tigerlake = true,
	.cmds_info = &gen12_cmds_info,
	.codename = "tigerlake",
	.gt = 1,
};

static const struct intel_device_info intel_tigerlake_gt2_info = {
	.graphics_ver = 12,
	.display_ver = 12,
	.is_tigerlake = true,
	.cmds_info = &gen12_cmds_info,
	.codename = "tigerlake",
	.gt = 2,
};

static const struct intel_device_info intel_rocketlake_info = {
	.graphics_ver = 12,
	.display_ver = 12,
	.is_rocketlake = true,
	.cmds_info = &gen12_cmds_info,
	.codename = "rocketlake"
};

static const struct intel_device_info intel_dg1_info = {
	.graphics_ver = 12,
	.graphics_rel = 10,
	.display_ver = 12,
	.is_dg1 = true,
	.cmds_info = &gen12_cmds_info,
	.codename = "dg1"
};

static const struct intel_device_info intel_dg2_info = {
	.graphics_ver = 12,
	.graphics_rel = 55,
	.display_ver = 13,
	.has_4tile = true,
	.is_dg2 = true,
	.codename = "dg2",
	.cmds_info = &gen12_dg2_cmds_info,
	.has_flatccs = true,
};

static const struct intel_device_info intel_alderlake_s_info = {
	.graphics_ver = 12,
	.display_ver = 12,
	.is_alderlake_s = true,
	.cmds_info = &gen12_cmds_info,
	.codename = "alderlake_s"
};

static const struct intel_device_info intel_raptorlake_s_info = {
	.graphics_ver = 12,
	.display_ver = 12,
	.is_raptorlake_s = true,
	.cmds_info = &gen12_cmds_info,
	.codename = "raptorlake_s"
};

static const struct intel_device_info intel_alderlake_p_info = {
	.graphics_ver = 12,
	.display_ver = 13,
	.is_alderlake_p = true,
	.cmds_info = &gen12_cmds_info,
	.codename = "alderlake_p"
};

static const struct intel_device_info intel_alderlake_n_info = {
	.graphics_ver = 12,
	.display_ver = 13,
	.is_alderlake_n = true,
	.cmds_info = &gen12_cmds_info,
	.codename = "alderlake_n"
};

static const struct intel_device_info intel_ats_m_info = {
	.graphics_ver = 12,
	.graphics_rel = 55,
	.display_ver = 0, /* no display support */
	.is_dg2 = true,
	.has_4tile = true,
	.codename = "ats_m",
	.cmds_info = &gen12_dg2_cmds_info,
	.has_flatccs = true,
};

static const struct intel_device_info intel_meteorlake_info = {
	.graphics_ver = 12,
	.graphics_rel = 70,
	.display_ver = 14,
	.has_4tile = true,
	.has_oam = true,
	.is_meteorlake = true,
	.codename = "meteorlake",
	.cmds_info = &gen12_mtl_cmds_info,
};

static const struct intel_device_info intel_pontevecchio_info = {
	.graphics_ver = 12,
	.graphics_rel = 60,
	.is_pontevecchio = true,
	.codename = "pontevecchio",
	.cmds_info = &gen12_pvc_cmds_info,
};

static const struct intel_device_info intel_lunarlake_info = {
	.graphics_ver = 20,
	.graphics_rel = 4,
	.display_ver = 20,
	.has_4tile = true,
	.has_flatccs = true,
	.has_oam = true,
	.is_lunarlake = true,
	.codename = "lunarlake",
	.cmds_info = &xe2_cmds_info,
};

static const struct intel_device_info intel_battlemage_info = {
	.graphics_ver = 20,
	.graphics_rel = 1,
	.display_ver = 14,
	.has_4tile = true,
	.has_flatccs = true,
	.has_oam = true,
	.is_battlemage = true,
	.codename = "battlemage",
	.cmds_info = &xe2_cmds_info,
};

static const struct intel_device_info intel_pantherlake_info = {
	.graphics_ver = 30,
	.graphics_rel = 0,
	.display_ver = 30,
	.has_4tile = true,
	.has_flatccs = true,
	.has_oam = true,
	.is_pantherlake = true,
	.codename = "pantherlake",
	.cmds_info = &xe2_cmds_info,
};

static const struct intel_device_info intel_wildcatlake_info = {
	.graphics_ver = 30,
	.graphics_rel = 3,
	.display_ver = 30,
	.has_4tile = true,
	.has_flatccs = true,
	.has_oam = true,
	.is_wildcatlake = true,
	.codename = "wildcatlake",
	.cmds_info = &xe2_cmds_info,
};

static const struct intel_device_info intel_novalake_s_info = {
	.graphics_ver = 30,
	.graphics_rel = 04,
	.display_ver = 35,
	.has_4tile = true,
	.has_oam = true,
	.has_flatccs = true,
	.is_novalake_s = true,
	.codename = "novalake_s",
	.cmds_info = &xe2_cmds_info,
};

static const struct intel_device_info intel_novalake_p_info = {
	.graphics_ver = 35,
	.graphics_rel = 10,
	.display_ver = 35,
	.has_4tile = true,
	.has_oam = true,
	.has_flatccs = true,
	.is_novalake_p = true,
	.codename = "novalake_p",
	.cmds_info = &xe2_cmds_info,
};

static const struct intel_device_info intel_crescentisland_info = {
	.graphics_ver = 35,
	.graphics_rel = 11,
	.display_ver = 0,
	.has_oam = true,
	.is_crescentisland = true,
	.codename = "crescentisland",
	.cmds_info = &xe2_cmds_info,
};

#define INTEL_PCI_ID_INIT(_id, _info) { \
	.vendor_id = 0x8086, .device_id = (_id), \
	.subvendor_id = PCI_MATCH_ANY, .subdevice_id = PCI_MATCH_ANY, \
	.device_class = 0x03 << 16, .device_class_mask = 0xff << 16, \
	.match_data = (intptr_t)(_info), \
}

static const struct pci_id_match intel_device_match[] = {
	INTEL_I810_IDS(INTEL_PCI_ID_INIT, &intel_i810_info),
	INTEL_I815_IDS(INTEL_PCI_ID_INIT, &intel_i815_info),

	INTEL_I830_IDS(INTEL_PCI_ID_INIT, &intel_i830_info),
	INTEL_I845G_IDS(INTEL_PCI_ID_INIT, &intel_i845_info),
	INTEL_I85X_IDS(INTEL_PCI_ID_INIT, &intel_i855_info),
	INTEL_I865G_IDS(INTEL_PCI_ID_INIT, &intel_i865_info),

	INTEL_I915G_IDS(INTEL_PCI_ID_INIT, &intel_i915_info),
	INTEL_I915GM_IDS(INTEL_PCI_ID_INIT, &intel_i915m_info),
	INTEL_I945G_IDS(INTEL_PCI_ID_INIT, &intel_i945_info),
	INTEL_I945GM_IDS(INTEL_PCI_ID_INIT, &intel_i945m_info),

	INTEL_G33_IDS(INTEL_PCI_ID_INIT, &intel_g33_info),
	INTEL_PNV_G_IDS(INTEL_PCI_ID_INIT, &intel_pineview_g_info),
	INTEL_PNV_M_IDS(INTEL_PCI_ID_INIT, &intel_pineview_m_info),

	INTEL_I965G_IDS(INTEL_PCI_ID_INIT, &intel_i965_info),
	INTEL_I965GM_IDS(INTEL_PCI_ID_INIT, &intel_i965m_info),

	INTEL_G45_IDS(INTEL_PCI_ID_INIT, &intel_g45_info),
	INTEL_GM45_IDS(INTEL_PCI_ID_INIT, &intel_gm45_info),

	INTEL_ILK_D_IDS(INTEL_PCI_ID_INIT, &intel_ironlake_info),
	INTEL_ILK_M_IDS(INTEL_PCI_ID_INIT, &intel_ironlake_m_info),

	INTEL_SNB_D_IDS(INTEL_PCI_ID_INIT, &intel_sandybridge_info),
	INTEL_SNB_M_IDS(INTEL_PCI_ID_INIT, &intel_sandybridge_m_info),

	INTEL_IVB_D_IDS(INTEL_PCI_ID_INIT, &intel_ivybridge_info),
	INTEL_IVB_M_IDS(INTEL_PCI_ID_INIT, &intel_ivybridge_m_info),

	INTEL_HSW_GT1_IDS(INTEL_PCI_ID_INIT, &intel_haswell_gt1_info),
	INTEL_HSW_GT2_IDS(INTEL_PCI_ID_INIT, &intel_haswell_gt2_info),
	INTEL_HSW_GT3_IDS(INTEL_PCI_ID_INIT, &intel_haswell_gt3_info),

	INTEL_VLV_IDS(INTEL_PCI_ID_INIT, &intel_valleyview_info),

	INTEL_BDW_GT1_IDS(INTEL_PCI_ID_INIT, &intel_broadwell_gt1_info),
	INTEL_BDW_GT2_IDS(INTEL_PCI_ID_INIT, &intel_broadwell_gt2_info),
	INTEL_BDW_GT3_IDS(INTEL_PCI_ID_INIT, &intel_broadwell_gt3_info),
	INTEL_BDW_RSVD_IDS(INTEL_PCI_ID_INIT, &intel_broadwell_unknown_info),

	INTEL_CHV_IDS(INTEL_PCI_ID_INIT, &intel_cherryview_info),

	INTEL_SKL_GT1_IDS(INTEL_PCI_ID_INIT, &intel_skylake_gt1_info),
	INTEL_SKL_GT2_IDS(INTEL_PCI_ID_INIT, &intel_skylake_gt2_info),
	INTEL_SKL_GT3_IDS(INTEL_PCI_ID_INIT, &intel_skylake_gt3_info),
	INTEL_SKL_GT4_IDS(INTEL_PCI_ID_INIT, &intel_skylake_gt4_info),

	INTEL_BXT_IDS(INTEL_PCI_ID_INIT, &intel_broxton_info),

	INTEL_KBL_GT1_IDS(INTEL_PCI_ID_INIT, &intel_kabylake_gt1_info),
	INTEL_KBL_GT2_IDS(INTEL_PCI_ID_INIT, &intel_kabylake_gt2_info),
	INTEL_KBL_GT3_IDS(INTEL_PCI_ID_INIT, &intel_kabylake_gt3_info),
	INTEL_KBL_GT4_IDS(INTEL_PCI_ID_INIT, &intel_kabylake_gt4_info),
	INTEL_AML_KBL_GT2_IDS(INTEL_PCI_ID_INIT, &intel_kabylake_gt2_info),

	INTEL_GLK_IDS(INTEL_PCI_ID_INIT, &intel_geminilake_info),

	INTEL_CFL_S_GT1_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt1_info),
	INTEL_CFL_S_GT2_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt2_info),
	INTEL_CFL_H_GT1_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt1_info),
	INTEL_CFL_H_GT2_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt2_info),
	INTEL_CFL_U_GT2_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt2_info),
	INTEL_CFL_U_GT3_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt3_info),
	INTEL_WHL_U_GT1_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt1_info),
	INTEL_WHL_U_GT2_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt2_info),
	INTEL_WHL_U_GT3_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt3_info),
	INTEL_AML_CFL_GT2_IDS(INTEL_PCI_ID_INIT, &intel_coffeelake_gt2_info),

	INTEL_CML_GT1_IDS(INTEL_PCI_ID_INIT, &intel_cometlake_gt1_info),
	INTEL_CML_GT2_IDS(INTEL_PCI_ID_INIT, &intel_cometlake_gt2_info),
	INTEL_CML_U_GT1_IDS(INTEL_PCI_ID_INIT, &intel_cometlake_gt1_info),
	INTEL_CML_U_GT2_IDS(INTEL_PCI_ID_INIT, &intel_cometlake_gt2_info),

	INTEL_CNL_IDS(INTEL_PCI_ID_INIT, &intel_cannonlake_info),

	INTEL_ICL_IDS(INTEL_PCI_ID_INIT, &intel_icelake_info),

	INTEL_EHL_IDS(INTEL_PCI_ID_INIT, &intel_elkhartlake_info),
	INTEL_JSL_IDS(INTEL_PCI_ID_INIT, &intel_jasperlake_info),

	INTEL_TGL_GT1_IDS(INTEL_PCI_ID_INIT, &intel_tigerlake_gt1_info),
	INTEL_TGL_GT2_IDS(INTEL_PCI_ID_INIT, &intel_tigerlake_gt2_info),
	INTEL_RKL_IDS(INTEL_PCI_ID_INIT, &intel_rocketlake_info),

	INTEL_DG1_IDS(INTEL_PCI_ID_INIT, &intel_dg1_info),
	INTEL_DG2_IDS(INTEL_PCI_ID_INIT, &intel_dg2_info),

	INTEL_ADLS_IDS(INTEL_PCI_ID_INIT, &intel_alderlake_s_info),
	INTEL_RPLS_IDS(INTEL_PCI_ID_INIT, &intel_raptorlake_s_info),
	INTEL_ADLP_IDS(INTEL_PCI_ID_INIT, &intel_alderlake_p_info),
	INTEL_RPLU_IDS(INTEL_PCI_ID_INIT, &intel_alderlake_p_info),
	INTEL_RPLP_IDS(INTEL_PCI_ID_INIT, &intel_alderlake_p_info),
	INTEL_ADLN_IDS(INTEL_PCI_ID_INIT, &intel_alderlake_n_info),

	INTEL_ATS_M_IDS(INTEL_PCI_ID_INIT, &intel_ats_m_info),

	INTEL_MTL_IDS(INTEL_PCI_ID_INIT, &intel_meteorlake_info),
	INTEL_ARL_IDS(INTEL_PCI_ID_INIT, &intel_meteorlake_info),

	INTEL_PVC_IDS(INTEL_PCI_ID_INIT, &intel_pontevecchio_info),

	INTEL_LNL_IDS(INTEL_PCI_ID_INIT, &intel_lunarlake_info),

	INTEL_BMG_IDS(INTEL_PCI_ID_INIT, &intel_battlemage_info),

	INTEL_PTL_IDS(INTEL_PCI_ID_INIT, &intel_pantherlake_info),
	INTEL_WCL_IDS(INTEL_PCI_ID_INIT, &intel_wildcatlake_info),

	INTEL_NVLS_IDS(INTEL_PCI_ID_INIT, &intel_novalake_s_info),

	INTEL_NVLP_IDS(INTEL_PCI_ID_INIT, &intel_novalake_p_info),

	INTEL_CRI_IDS(INTEL_PCI_ID_INIT, &intel_crescentisland_info),

	INTEL_PCI_ID_INIT(PCI_MATCH_ANY, &intel_generic_info),
};

#undef INTEL_PCI_ID_INIT

/*
 * Process-wide cache of per-devid copies of the static PCI-ID table entries.
 * Entries can be updated at runtime, e.g. by xe_device_get() with the graphics
 * IP version reported by GMD_ID. The cache is keyed by PCI device ID, so all
 * devices sharing a device ID share one entry.
 *
 * The map is created lazily on first lookup, so lookups do not depend on
 * constructor ordering.
 */
static struct {
	pthread_mutex_t mutex;
	struct igt_map *map;
} devinfo_cache = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
};

static void free_device_info(struct igt_map_entry *entry)
{
	free(entry->data);
	free((void *)entry->key);
}

igt_destructor
{
	pthread_mutex_lock(&devinfo_cache.mutex);
	igt_map_destroy(devinfo_cache.map, free_device_info);
	devinfo_cache.map = NULL;
	pthread_mutex_unlock(&devinfo_cache.mutex);
}

/* Caller must hold devinfo_cache.mutex. */
static struct intel_device_info *devinfo_cache_search(uint16_t devid)
{
	uint32_t key = devid;

	if (!devinfo_cache.map)
		return NULL;

	return igt_map_search(devinfo_cache.map, &key);
}

static const struct intel_device_info *devinfo_table_lookup(uint16_t devid)
{
	int i;

	for (i = 0; intel_device_match[i].device_id != PCI_MATCH_ANY; i++) {
		if (devid == intel_device_match[i].device_id)
			break;
	}

	return (const struct intel_device_info *)intel_device_match[i].match_data;
}

/**
 * xe_dev_update_graphics_ver:
 * @devid: pci device id
 * @major: new major graphics version
 * @minor: new minor graphics version
 *
 * Updates the cached graphics version of the device with the specified @devid.
 * On Xe platforms with GMD_ID, the graphics major and minor versions are read
 * from the hardware and override the values from the static PCI-ID table.
 *
 * The cache is keyed by @devid, so all devices sharing a PCI device ID share
 * one entry; with several such devices reporting different IP versions, the
 * last update wins.
 *
 * Readers of intel_get_device_info() do not take the cache lock. This is fine
 * because the entry is only written when a device is first probed by
 * xe_device_get(), and only if the version actually changes.
 *
 * Returns:
 * 0 on success, -ENOENT if there is no cache entry for @devid.
 */
int xe_dev_update_graphics_ver(uint16_t devid, unsigned int major, unsigned int minor)
{
	struct intel_device_info *info;
	int ret = 0;

	if (intel_graphics_ver(devid) == IP_VER(major, minor))
		return 0;

	pthread_mutex_lock(&devinfo_cache.mutex);
	info = devinfo_cache_search(devid);
	if (info) {
		info->graphics_ver = major;
		info->graphics_rel = minor;
	} else {
		ret = -ENOENT;
	}
	pthread_mutex_unlock(&devinfo_cache.mutex);

	return ret;
}

/**
 * intel_get_device_info:
 * @devid: pci device id
 *
 * Looks up the Intel GFX device info for the given device id.
 *
 * Returns:
 * The associated intel_get_device_info
 */
const struct intel_device_info *intel_get_device_info(uint16_t devid)
{
	const struct intel_device_info *table_info;
	struct intel_device_info *info, *new_info;
	uint32_t *new_key;

	pthread_mutex_lock(&devinfo_cache.mutex);
	info = devinfo_cache_search(devid);
	pthread_mutex_unlock(&devinfo_cache.mutex);
	if (info)
		return info;

	table_info = devinfo_table_lookup(devid);

	new_key = malloc(sizeof(*new_key));
	new_info = malloc(sizeof(*new_info));
	if (!new_key || !new_info) {
		free(new_key);
		free(new_info);
		fprintf(stderr, "Failed to allocate device info for devid 0x%04x\n", devid);
		exit(EXIT_FAILURE);
	}

	*new_key = devid;
	*new_info = *table_info;

	pthread_mutex_lock(&devinfo_cache.mutex);
	if (!devinfo_cache.map)
		devinfo_cache.map = igt_map_create(igt_map_hash_32,
						   igt_map_equal_32);
	/* Another thread may have inserted while we were allocating. */
	info = devinfo_cache_search(devid);
	if (!info && devinfo_cache.map &&
	    igt_map_insert(devinfo_cache.map, new_key, new_info)) {
		info = new_info;
		new_key = NULL;
		new_info = NULL;
	}
	pthread_mutex_unlock(&devinfo_cache.mutex);

	free(new_info);
	free(new_key);
	if (!info) {
		fprintf(stderr, "Failed to cache device info for devid 0x%04x\n", devid);
		exit(EXIT_FAILURE);
	}

	return info;
}

static bool char_eq(char c1, char c2)
{
	c1 = isalnum(c1) ? tolower(c1) : '-';
	c2 = isalnum(c2) ? tolower(c2) : '-';

	return c1 == c2;
}

/*
 * Return true if the codenames s1 and s2 match, with fuzziness.
 *
 * Case insensitive matching, ignoring differences in non-alnum characters. With
 * non-zero fuzziness, accept matches up to the first non-alnum character.
 */
static bool codename_match(const char *s1, const char *s2, int fuzziness)
{
	while (*s1 && *s2 && char_eq(*s1, *s2)) {
		s1++;
		s2++;
	}

	/* full match */
	if (!*s1 && !*s2)
		return true;

	/* sub-string match up to a non-alnum char */
	if (fuzziness >=1) {
		if (!*s1 && !isalnum(*s2))
			return true;
		if (!*s2 && !isalnum(*s1))
			return true;
	}

	return false;
}

static uint16_t lookup_device_id(const char *codename, int fuzziness)
{
	int i;

	for (i = 0; intel_device_match[i].device_id != PCI_MATCH_ANY; i++) {
		const struct intel_device_info *info = (void *)intel_device_match[i].match_data;

		if (codename_match(info->codename, codename, fuzziness))
			return intel_device_match[i].device_id;
	}

	return 0;
}

/**
 * intel_guess_device_id:
 * @codenameish: something resembling a codename
 *
 * Based on something resembling a codename, try to fuzzy find the first PCI
 * device ID matching the codename.
 *
 * Returns:
 * PCI device ID fuzzy matching the @codenameish, or 0 if no match was found.
 */
uint16_t intel_guess_device_id(const char *codenameish)
{
	uint16_t devid;
	int fuzziness;

	for (fuzziness = 0; fuzziness < 2; fuzziness++) {
		devid = lookup_device_id(codenameish, fuzziness);
		if (devid)
			return devid;
	}

	return 0;
}

/**
 * intel_get_cmds_info:
 * @devid: pci device id
 *
 * Looks up information on copy commands and tiling formats supported
 * by the device.
 *
 * Returns:
 * The associated intel_cmds_info, NULL if no such information is found
 */
const struct intel_cmds_info *intel_get_cmds_info(uint16_t devid)
{
	const struct intel_device_info *dev_info;

	dev_info = intel_get_device_info(devid);

	return dev_info->cmds_info;
}

/**
 * intel_gen:
 * @devid: pci device id
 *
 * Computes the Intel GFX generation for the given device id.
 *
 * Returns:
 * The GFX generation on successful lookup, -1u on failure.
 */
unsigned intel_gen(uint16_t devid)
{
	return intel_get_device_info(devid)->graphics_ver ?: -1u;
}

unsigned intel_graphics_ver(uint16_t devid)
{
	const struct intel_device_info *info = intel_get_device_info(devid);

	return IP_VER(info->graphics_ver, info->graphics_rel);
}

/**
 * intel_display_ver:
 * @devid: pci device id
 *
 * Computes the Intel GFX display version for the given device id.
 *
 * Returns:
 * The display version on successful lookup, -1u on failure.
 */
unsigned intel_display_ver(uint16_t devid)
{
	return intel_get_device_info(devid)->display_ver ?: -1u;
}
