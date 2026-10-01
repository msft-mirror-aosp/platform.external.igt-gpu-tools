// SPDX-License-Identifier: MIT
/**
 * TEST: kms dp link training
 * Category: Display
 * Description: Test to validate link training on SST/MST with UHBR/NON_UHBR rates
 * Driver requirement: i915, xe
 * Mega feature: General Display Features
 */

/**
 * SUBTEST: uhbr-sst
 * Description: Test we can drive UHBR rates over SST.
 *
 * SUBTEST: uhbr-mst
 * Description: Test we can drive UHBR rates over MST.
 *
 * SUBTEST: non-uhbr-sst
 * Description: Test we can drive non-UHBR rates over SST.
 *
 * SUBTEST: non-uhbr-mst
 * Description: Test we can drive non-UHBR rates over MST.
 */

/**
 * SUBTEST: uhbr-sst-tbtalt-train
 * Description: Test we can drive UHBR rates over an SST link tunneled over
 *              USB4/Thunderbolt.
 *
 * SUBTEST: uhbr-mst-tbtalt-train
 * Description: Test we can drive UHBR rates over an MST link tunneled over
 *              USB4/Thunderbolt.
 *
 * SUBTEST: uhbr-sst-direct-train
 * Description: Test we can drive UHBR rates over a directly connected SST
 *              link.
 *
 * SUBTEST: uhbr-mst-direct-train
 * Description: Test we can drive UHBR rates over a directly connected MST
 *              link.
 *
 * SUBTEST: non-uhbr-sst-tbtalt-train
 * Description: Test we can drive non-UHBR rates over an SST link tunneled over
 *              USB4/Thunderbolt.
 *
 * SUBTEST: non-uhbr-mst-tbtalt-train
 * Description: Test we can drive non-UHBR rates over an MST link tunneled over
 *              USB4/Thunderbolt.
 *
 * SUBTEST: non-uhbr-sst-direct-train
 * Description: Test we can drive non-UHBR rates over a directly connected SST
 *              link.
 *
 * SUBTEST: non-uhbr-mst-direct-train
 * Description: Test we can drive non-UHBR rates over a directly connected MST
 *              link.
 */

#include <string.h>

#include "i915/i915_dp.h"
#include "igt.h"
#include "igt_dp.h"
#include "igt_kms.h"
#include "intel/kms_joiner_helper.h"
#include "intel/kms_mst_helper.h"

#define RETRAIN_COUNT	1

/* The driver allows at most 10 link rates over 3 lane counts. */
#define MAX_LINK_CONFIGS	32

/*
 * How long the driver's link recovery is given to reach a verdict, in seconds.
 * The automatic retrain is queued without a delay, so this only has to cover
 * one retrain and the fallback selection that follows it.
 */
#define LINK_RECOVERY_TIMEOUT	5.0

#define MAX_LINKS		16

typedef struct {
	int drm_fd;
	uint32_t devid;
	igt_display_t display;
	igt_output_t *output;
	int aux_fd;
} data_t;

/*
 * struct dp_link - One trainable DP link.
 *
 * @output is the output the link is driven through, which for MST is the first
 * of the topology's streams. The classification is discovered, never chosen.
 */
struct dp_link {
	igt_output_t *output;
	bool mst;
	enum i915_dp_tc_mode tc_mode;
};

/*
 * enum phy_filter - Which links a subtest selects.
 *
 * PHY_ANY is the PHY agnostic scope of the original subtests. The split
 * between PHY_TBTALT and PHY_DIRECT is where the link clock comes from: only a
 * tbt-alt link is clocked by the Thunderbolt PLL, native, DP alt mode and
 * legacy all use the PHY PLL, so folding the latter three together does not
 * change which driver code runs.
 */
enum phy_filter {
	PHY_ANY,
	PHY_TBTALT,
	PHY_DIRECT,
};

static const char *phy_filter_name(enum phy_filter phy)
{
	return phy == PHY_TBTALT ? "tbt-alt" : "direct";
}

static bool link_matches_phy(const struct dp_link *link, enum phy_filter phy)
{
	switch (phy) {
	case PHY_TBTALT:
		return link->tc_mode == I915_DP_TC_TBT_ALT;
	case PHY_DIRECT:
		return link->tc_mode != I915_DP_TC_TBT_ALT;
	default:
		return true;
	}
}

/*
 * check_condition_with_timeout - Polls check_fn until it returns 0
 * or until 'timeout' seconds elapse.
 */
static int check_condition_with_timeout(int drm_fd, igt_output_t *output,
					int (*check_fn)(int, igt_output_t *),
					double interval, double timeout)
{
	struct timespec start_time, current_time;
	double elapsed_time;
	int ret;

	clock_gettime(CLOCK_MONOTONIC, &start_time);

	while (true) {
		ret = check_fn(drm_fd, output);
		if (ret == 0)
			return 0;

		clock_gettime(CLOCK_MONOTONIC, &current_time);
		elapsed_time = (current_time.tv_sec - start_time.tv_sec) +
			(current_time.tv_nsec - start_time.tv_nsec) / 1e9;
		if (elapsed_time >= timeout)
			return -1;

		usleep((useconds_t)(interval * 1e6));
	}
}

/*
 * assert_link_status_good - Verifies link-status == GOOD
 * for either a single SST output or all MST outputs in the topology.
 */
static void assert_link_status_good(data_t *data, bool mst)
{
	igt_output_t *outputs[IGT_MAX_PIPES];
	uint64_t link_status_value;
	int count = 0;
	int i;

	if (mst) {
		igt_assert_f(igt_find_all_mst_output_in_topology(data->drm_fd,
								 &data->display, data->output,
								 outputs, &count) == 0,
								 "Unable to find MST outputs\n");
	} else {
		outputs[0] = data->output;
		count = 1;
	}

	for (i = 0; i < count; i++) {
		igt_assert_f(kmstest_get_property(data->drm_fd,
						  outputs[i]->config.connector->connector_id,
						  DRM_MODE_OBJECT_CONNECTOR,
						  "link-status", NULL,
						  &link_status_value, NULL),
			     "No link-status property on %s\n",
			     igt_output_name(outputs[i]));

		igt_assert_eq(link_status_value, DRM_MODE_LINK_STATUS_GOOD);
	}
}

/*
 * set_link_status_good - Clear a latched BAD link-status.
 *
 * The property latches BAD when the driver falls back and only userspace can
 * clear it, so without clearing it before every attempt a single fallback
 * anywhere in the run poisons every later assertion.
 */
static void set_link_status_good(data_t *data, bool mst)
{
	igt_output_t *outputs[IGT_MAX_PIPES];
	int count = 0;
	int i;

	if (mst) {
		igt_assert_f(igt_find_all_mst_output_in_topology(data->drm_fd,
								 &data->display, data->output,
								 outputs, &count) == 0,
								 "Unable to find MST outputs\n");
	} else {
		outputs[0] = data->output;
		count = 1;
	}

	for (i = 0; i < count; i++)
		igt_output_set_prop_value(outputs[i], IGT_CONNECTOR_LINK_STATUS,
					  DRM_MODE_LINK_STATUS_GOOD);

	igt_display_commit2(&data->display, COMMIT_ATOMIC);
}

/*
 * link_config_data_rate - Data rate a link configuration carries, in
 * 10 kbit/s units.
 *
 * Channel coding efficiency is in 1ppm units, matching the kernel's
 * drm_dp_bw_channel_coding_efficiency(): 96.71% for 128b/132b and 80% for
 * 8b/10b. 8b/10b MST is 78.75% instead, because of the 1-in-64 MTPH overhead
 * that helper deliberately does not account for. Using 80% there overestimates
 * the available bandwidth and turns a correct driver rejection into a failure.
 */
static int link_config_data_rate(const struct i915_dp_link_config *config,
				 bool mst)
{
	uint64_t symbol_rate = (uint64_t)config->link_rate * config->lane_count;
	int efficiency;

	if (i915_dp_is_uhbr_rate(config->link_rate))
		efficiency = 967100;
	else
		efficiency = mst ? 787500 : 800000;

	return symbol_rate * efficiency / 1000000;
}

/*
 * mode_data_rate - Data rate a mode needs, in 10 kbit/s units.
 *
 * Uncompressed 8 bpc, which is what the smallest mode of a DP sink is driven
 * at. Deliberately pessimistic: overestimating what the mode needs makes a
 * borderline configuration skip rather than fail.
 */
static int mode_data_rate(const drmModeModeInfo *mode)
{
	return DIV_ROUND_UP(mode->clock * 24, 10);
}

/*
 * link_min_data_rate - Data rate the link has to carry, in 10 kbit/s units.
 *
 * do_modeset() drives every stream of an MST topology, so the link carries all
 * of them at once. Summing them is what tells a configuration that cannot
 * carry the whole payload apart from one that can, which a single stream's
 * requirement would let through and turn into a false link training failure.
 */
static int link_min_data_rate(data_t *data, bool mst)
{
	igt_output_t *outputs[IGT_MAX_PIPES];
	int count = 0;
	int rate = 0;
	int i;

	if (mst) {
		igt_assert_f(igt_find_all_mst_output_in_topology(data->drm_fd,
								 &data->display, data->output,
								 outputs, &count) == 0,
								 "Unable to find MST outputs\n");
	} else {
		outputs[0] = data->output;
		count = 1;
	}

	for (i = 0; i < count; i++)
		rate += mode_data_rate(igt_output_get_mode(outputs[i]));

	return rate;
}

/*
 * assert_link_retrain_not_disabled - Let the driver's link recovery reach a
 * verdict and check it did not give up on the link.
 *
 * A failed training is not visible the moment the forced retrain flag clears:
 * the driver clears that flag when the retrain modeset starts and only then
 * queues its automatic retrain, so the first failure is still in flight. Once
 * the automatic retrain is used up the driver looks for a configuration to
 * fall back to, and with both the rate and the lane count forced there is
 * none. It then marks retraining disabled, which is the one place a link that
 * failed for good becomes visible.
 *
 * Poll for that verdict rather than reading it once, and give recovery the
 * full timeout to reach it before calling the link trained: sleep before each
 * read rather than after it, so that the last read is taken once the whole
 * timeout has elapsed rather than one poll interval short of it.
 */
static void assert_link_retrain_not_disabled(data_t *data,
					     const struct i915_dp_link_config *config)
{
	struct timespec start, now;
	double elapsed;

	clock_gettime(CLOCK_MONOTONIC, &start);

	do {
		usleep(200 * 1000);

		clock_gettime(CLOCK_MONOTONIC, &now);
		elapsed = (now.tv_sec - start.tv_sec) +
			(now.tv_nsec - start.tv_nsec) / 1e9;

		igt_assert_f(!i915_dp_get_link_retrain_disabled(data->drm_fd,
								data->output),
			     "Link training at %d lanes, rate %d was given up on.\n",
			     config->lane_count, config->link_rate);
	} while (elapsed < LINK_RECOVERY_TIMEOUT);
}

/*
 * open_link_aux - Open the AUX device at the near end of the link.
 *
 * An MST stream connector's AUX reaches the far sink over sideband rather than
 * the link partner, which is the end of the link being trained, so for MST it
 * is the topology's root connector that has to be asked. The root reads
 * disconnected while MST is active, so it is found by id rather than by
 * walking the connected outputs.
 */
static int open_link_aux(data_t *data, bool mst)
{
	igt_output_t *output = data->output;

	if (mst) {
		int root_id = igt_get_dp_mst_connector_id(data->output);
		int i;

		output = NULL;

		for (i = 0; i < data->display.n_outputs; i++) {
			igt_output_t *root = &data->display.outputs[i];

			if (root->config.connector &&
			    root->config.connector->connector_id == root_id) {
				output = root;
				break;
			}
		}

		if (!output)
			return -ENOENT;
	}

	return igt_dp_aux_open(data->drm_fd, output);
}

/*
 * assert_sink_agrees - Ask the sink whether the link is really up.
 *
 * Everything else the test checks is read back from the driver. This is the
 * only check that the sink agrees, and the only direct evidence of which
 * channel coding reached the wire.
 *
 * A DPCD the test could open but cannot read leaves the case unverified, so
 * skip rather than report it as trained. A missing AUX device does not: AUX is
 * out of band and answers whether or not the link trained, so a read that
 * fails once the device is open is a result in itself, while
 * CONFIG_DRM_DISPLAY_DP_AUX_CHARDEV not being enabled - it is not by default -
 * says nothing about the link and would take every driver side check down with
 * it.
 */
static void assert_sink_agrees(data_t *data,
			       const struct i915_dp_link_config *config)
{
	bool uhbr = i915_dp_is_uhbr_rate(config->link_rate);
	int ret;

	if (data->aux_fd < 0)
		return;

	ret = igt_dp_channel_coding_ok(data->aux_fd, uhbr);
	igt_skip_on_f(ret < 0, "Unable to read the sink's channel coding: %s\n",
		      strerror(-ret));
	igt_assert_f(ret, "Sink is not set to %s at rate %d\n",
		     uhbr ? "128b/132b" : "8b/10b", config->link_rate);

	ret = igt_dp_link_status_ok(data->aux_fd, config->lane_count, uhbr);
	igt_skip_on_f(ret < 0, "Unable to read the sink's link status: %s\n",
		      strerror(-ret));
	igt_assert_f(ret, "Sink does not report %d lanes locked at rate %d\n",
		     config->lane_count, config->link_rate);
}

/*
 * train_link_config - Force one link configuration, re-establish the link and
 * check that the configuration took effect and survived training.
 */
static void train_link_config(data_t *data, bool mst,
			      const struct i915_dp_link_config *config)
{
	int current_link_rate;
	char rate_str[32];
	char lane_str[32];

	snprintf(rate_str, sizeof(rate_str), "%d", config->link_rate);
	snprintf(lane_str, sizeof(lane_str), "%d", config->lane_count);
	igt_info("Training %s at %d lanes, rate %d\n",
		 igt_output_name(data->output), config->lane_count,
		 config->link_rate);

	set_link_status_good(data, mst);

	i915_dp_set_link_params(data->drm_fd, data->output, rate_str, lane_str);
	i915_dp_force_link_retrain(data->drm_fd, data->output, RETRAIN_COUNT);
	igt_assert_eq(check_condition_with_timeout(data->drm_fd, data->output,
						   i915_dp_get_pending_retrain,
						   1.0, 20.0), 0);
	assert_link_status_good(data, mst);

	current_link_rate = i915_dp_get_current_link_rate(data->drm_fd, data->output);
	igt_info("Current link rate is %d\n", current_link_rate);
	igt_assert_f(current_link_rate == config->link_rate,
		     "Link training did not succeed at the forced link rate.\n");
	igt_assert_f(i915_dp_get_current_lane_count(data->drm_fd, data->output) ==
		     config->lane_count,
		     "Link training did not succeed at the forced lane count.\n");

	/*
	 * The link parameters read back above are the ones the driver asked
	 * the sink for, not the ones the link ended up running at, so ask the
	 * driver whether it gave up on the link after training it.
	 *
	 * This has to happen before the next force: forcing a rate or a lane
	 * count resets the recovery state, which would mask the failure.
	 */
	assert_link_retrain_not_disabled(data, config);

	assert_sink_agrees(data, config);
}

/*
 * log_link_inventory - Describe every connected DP link once.
 *
 * With the configuration in the subtest name, most subtests skip on any one
 * machine. This is what tells a missing monitor apart from a monitor behind a
 * dock that offers no UHBR configuration, without having to re-run by hand.
 */
static void log_link_inventory(data_t *data)
{
	igt_output_t *output;

	for_each_connected_output(&data->display, output) {
		struct i915_dp_link_config configs[MAX_LINK_CONFIGS];
		enum i915_dp_tc_mode tc_mode;
		char configs_str[512];
		char pin_assignment;
		int tc_max_lanes;
		int num_configs;
		int len = 0;
		int i;

		if (output->config.connector->connector_type !=
		    DRM_MODE_CONNECTOR_DisplayPort)
			continue;

		tc_mode = i915_dp_get_tc_mode(data->drm_fd, output,
					      &pin_assignment, &tc_max_lanes);

		if (tc_mode == I915_DP_TC_NONE)
			igt_info("%s: %s, %s\n", igt_output_name(output),
				 igt_check_output_is_dp_mst(output) ? "MST" : "SST",
				 i915_dp_tc_mode_name(tc_mode));
		else
			igt_info("%s: %s, %s, pin assignment %c, TC max lanes %d\n",
				 igt_output_name(output),
				 igt_check_output_is_dp_mst(output) ? "MST" : "SST",
				 i915_dp_tc_mode_name(tc_mode),
				 pin_assignment, tc_max_lanes);

		if (!i915_dp_has_allowed_link_configs_debugfs(data->drm_fd, output)) {
			igt_info("%s: no allowed link configs debugfs\n",
				 igt_output_name(output));
			continue;
		}

		/*
		 * Enumerate with the forced parameters reset, or the set read
		 * back is the forced one rather than the real one.
		 */
		i915_dp_reset_link_params(data->drm_fd, output);

		num_configs = i915_dp_get_allowed_link_configs(data->drm_fd, output,
							       configs,
							       ARRAY_SIZE(configs));

		for (i = 0; i < num_configs && len < (int)sizeof(configs_str); i++)
			len += snprintf(configs_str + len, sizeof(configs_str) - len,
					" %dx%d", configs[i].lane_count,
					configs[i].link_rate);

		igt_info("%s: allowed configs:%s\n", igt_output_name(output),
			 num_configs ? configs_str : " none");
	}
}

/*
 * override_lowest_mode - Drive the mode with the lowest pixel clock, so that
 * the largest number of link configurations can carry it.
 */
static void override_lowest_mode(data_t *data, igt_output_t *output)
{
	drmModeConnector *connector = output->config.connector;
	drmModeModeInfo *mode = NULL;
	int i;

	for (i = 0; i < connector->count_modes; i++)
		if (!mode || connector->modes[i].clock < mode->clock)
			mode = &connector->modes[i];

	igt_assert_f(mode, "No mode on output %s\n", igt_output_name(output));

	igt_output_override_mode(output, mode);
}

/*
 * setup_planes_fbs - Create solid-color FBs and attach them to the primary plane.
 */
static void setup_planes_fbs(data_t *data, igt_output_t *outs[],
			     int count, drmModeModeInfo *modes[],
			     struct igt_fb fbs[], igt_plane_t *planes[])
{
	int i;

	for (i = 0; i < count; i++) {
		modes[i] = igt_output_get_mode(outs[i]);
		igt_info("Mode %dx%d@%d on output %s\n",
			 modes[i]->hdisplay, modes[i]->vdisplay,
			 modes[i]->vrefresh, igt_output_name(outs[i]));

		planes[i] = igt_output_get_plane_type(outs[i], DRM_PLANE_TYPE_PRIMARY);

		igt_create_color_fb(data->drm_fd, modes[i]->hdisplay,
				    modes[i]->vdisplay,
				    DRM_FORMAT_XRGB8888,
				    DRM_FORMAT_MOD_LINEAR,
				    0.0, 1.0, 0.0, &fbs[i]);

		igt_plane_set_fb(planes[i], &fbs[i]);
	}
}

static void do_modeset(data_t *data, bool mst)
{
	uint32_t master_pipes_mask = 0;
	uint32_t valid_pipes_mask = 0;
	uint32_t used_pipes_mask = 0;
	igt_output_t *outs[IGT_MAX_PIPES];
	drmModeModeInfo *modes[IGT_MAX_PIPES];
	struct igt_fb fbs[IGT_MAX_PIPES];
	igt_plane_t *planes[IGT_MAX_PIPES];
	int n_pipes = 0;
	int out_count = 0;
	igt_crtc_t *crtc;
	int i;

	for_each_crtc(&data->display, crtc) {
		valid_pipes_mask |= BIT(crtc->hardware_pipe);
		n_pipes++;
	}

	if (mst) {
		igt_assert_f(igt_find_all_mst_output_in_topology(data->drm_fd,
								 &data->display,
								 data->output, outs,
								 &out_count) == 0,
								 "Unable to find MST outputs\n");
	} else {
		outs[0] = data->output;
		out_count = 1;
	}

	igt_assert_f(out_count > 0, "Require at least one output\n");

	for (i = 0; i < out_count; i++)
		override_lowest_mode(data, outs[i]);

	igt_set_all_master_pipes_for_platform(&data->display, &master_pipes_mask);

	igt_assert_f(igt_assign_pipes_for_outputs(data->drm_fd,
						  outs, out_count,
						  n_pipes,
						  &used_pipes_mask,
						  master_pipes_mask,
						  valid_pipes_mask),
						  "Unable to assign pipes for outputs\n");

	setup_planes_fbs(data, outs, out_count, modes, fbs, planes);
	igt_assert_f(igt_fit_modes_in_bw(&data->display), "Unable to fit modes in bw\n");
	igt_display_commit2(&data->display, COMMIT_ATOMIC);
}

/*
 * setup_link - Bring the link up at the parameters the driver picks itself.
 *
 * Runs per dynamic subtest rather than once per link: everything it does can
 * fail, and a failure in a igt_subtest_with_dynamic() container aborts the
 * whole test instead of yielding a dynamic subtest result. For the same
 * reason the reset here is the only one between configurations, and what the
 * last configuration forced is left to the exit handler that
 * i915_dp_set_link_params() installs.
 */
static void setup_link(data_t *data, bool mst)
{
	igt_display_reset(&data->display);
	i915_dp_reset_link_params(data->drm_fd, data->output);
	do_modeset(data, mst);

	/* Retrain at default/driver parameters */
	i915_dp_force_link_retrain(data->drm_fd, data->output, RETRAIN_COUNT);
	igt_assert_eq(check_condition_with_timeout(data->drm_fd, data->output,
						   i915_dp_get_pending_retrain,
						   1.0, 20.0), 0);
	assert_link_status_good(data, mst);
}

/*
 * run_link_rate_test - Main link training routine. Expects the MST vs. SST check
 * to be done beforehand. Returns true if tested at the correct rate.
 */
static bool run_link_rate_test(data_t *data, bool mst, bool uhbr)
{
	struct i915_dp_link_config configs[MAX_LINK_CONFIGS];
	int num_configs, num_dynamics = 0;
	int i;

	igt_require_f(i915_dp_has_allowed_link_configs_debugfs(data->drm_fd,
							       data->output),
		      "Kernel has no intel_dp_allowed_link_configs debugfs\n");

	data->aux_fd = open_link_aux(data, mst);
	if (data->aux_fd < 0)
		igt_info("%s: no AUX device (%s), sink side checks are skipped\n",
			 igt_output_name(data->output), strerror(-data->aux_fd));

	/*
	 * Enumerate with the forced parameters reset, or the set being read is
	 * the forced one rather than the one the driver would pick from.
	 */
	i915_dp_reset_link_params(data->drm_fd, data->output);
	num_configs = i915_dp_get_allowed_link_configs(data->drm_fd, data->output,
						       configs, ARRAY_SIZE(configs));

	for (i = 0; i < num_configs; i++) {
		char name[64];

		if (i915_dp_is_uhbr_rate(configs[i].link_rate) != uhbr)
			continue;

		snprintf(name, sizeof(name), "%s-%dx%d",
			 igt_output_name(data->output),
			 configs[i].lane_count, configs[i].link_rate);

		num_dynamics++;

		igt_dynamic(name) {
			setup_link(data, mst);

			igt_require_f(link_config_data_rate(&configs[i], mst) >=
				      link_min_data_rate(data, mst),
				      "%d lanes, rate %d is too narrow for the mode\n",
				      configs[i].lane_count,
				      configs[i].link_rate);

			train_link_config(data, mst, &configs[i]);
		}
	}

	if (!num_dynamics)
		igt_info("Output %s allows no %sUHBR link config\n",
			 igt_output_name(data->output), uhbr ? "" : "non-");

	if (data->aux_fd >= 0) {
		close(data->aux_fd);
		data->aux_fd = -1;
	}

	igt_info("----------------------------------------------------\n");
	return num_dynamics > 0;
}

/*
 * test_link_rate - Iterates over connected DP outputs. Checks MST vs. SST
 * early, then calls run_link_rate_test(). Returns true if it ran on at
 * least one matching output.
 */
/*
 * discover_links - Collapse the connected DP outputs into links.
 *
 * One link is one trainable DP link, so a whole MST topology becomes a single
 * entry, represented by the first of its streams. Forcing link parameters on
 * one stream affects every stream in the topology, so training a topology once
 * per stream would train the same link repeatedly.
 */
static int discover_links(data_t *data, struct dp_link *links, int max_links)
{
	igt_output_t *output;
	int num_links = 0;
	int i;

	for_each_connected_output(&data->display, output) {
		bool mst, seen = false;

		if (output->config.connector->connector_type !=
		    DRM_MODE_CONNECTOR_DisplayPort) {
			igt_info("Skipping non-DisplayPort output %s\n",
				 igt_output_name(output));
			continue;
		}

		mst = igt_check_output_is_dp_mst(output);

		if (mst) {
			int root = igt_get_dp_mst_connector_id(output);

			for (i = 0; i < num_links; i++)
				if (links[i].mst &&
				    igt_get_dp_mst_connector_id(links[i].output) == root) {
					seen = true;
					break;
				}
		}

		if (seen) {
			igt_info("Skipping %s: same MST topology as %s\n",
				 igt_output_name(output),
				 igt_output_name(links[i].output));
			continue;
		}

		if (num_links == max_links) {
			igt_info("Skipping %s: more than %d DP links connected\n",
				 igt_output_name(output), max_links);
			continue;
		}

		links[num_links].output = output;
		links[num_links].mst = mst;
		links[num_links].tc_mode = i915_dp_get_tc_mode(data->drm_fd,
							       output, NULL, NULL);
		num_links++;
	}

	return num_links;
}

static bool test_link_rate(data_t *data, bool mst, bool uhbr,
			   enum phy_filter phy)
{
	struct dp_link links[MAX_LINKS];
	bool ran_any_link = false;
	int num_links;
	int i;

	igt_skip_on_f(!is_intel_device(data->drm_fd),
		      "Test supported only on Intel platforms.\n");

	num_links = discover_links(data, links, ARRAY_SIZE(links));

	for (i = 0; i < num_links; i++) {
		if (links[i].mst != mst) {
			igt_info("Skipping %s: %s requested but it's %s.\n",
				 igt_output_name(links[i].output),
				 mst ? "MST" : "SST",
				 links[i].mst ? "MST" : "SST");
			igt_info("----------------------------------------------------\n");
			continue;
		}

		if (!link_matches_phy(&links[i], phy)) {
			igt_info("Skipping %s: %s requested but it's %s.\n",
				 igt_output_name(links[i].output),
				 phy_filter_name(phy),
				 i915_dp_tc_mode_name(links[i].tc_mode));
			igt_info("----------------------------------------------------\n");
			continue;
		}

		data->output = links[i].output;
		igt_info("Running link training test for %s\n",
			 igt_output_name(data->output));
		ran_any_link |= run_link_rate_test(data, mst, uhbr);
	}

	return ran_any_link;
}

IGT_TEST_DESCRIPTION("Test to validate link training on SST/MST with "
		     "UHBR/NON_UHBR rates");

int igt_main()
{
	data_t data = { .aux_fd = -1 };

	igt_fixture() {
		data.drm_fd = drm_open_driver_master(DRIVER_INTEL | DRIVER_XE);
		data.devid = intel_get_drm_devid(data.drm_fd);
		kmstest_set_vt_graphics_mode();
		igt_display_require(&data.display, data.drm_fd);
		igt_display_require_output(&data.display);
		/*
		 * Some environments may have environment
		 * variable set to ignore long hpd, disable it for this test
		 */
		igt_assert_f(igt_ignore_long_hpd(data.drm_fd, false),
			     "Unable to disable ignore long hpd\n");

		if (is_intel_device(data.drm_fd))
			log_link_inventory(&data);
	}

	igt_describe("Test we can drive UHBR rates over SST");
	igt_subtest_with_dynamic("uhbr-sst") {
		igt_require_f(intel_display_ver(data.devid) > 13,
			      "UHBR not supported on platform\n");
		igt_require_f(test_link_rate(&data, false, true, PHY_ANY),
			      "Didn't find any SST output with UHBR rates.\n");
	}

	igt_describe("Test we can drive UHBR rates over MST");
	igt_subtest_with_dynamic("uhbr-mst") {
                igt_require_f(intel_display_ver(data.devid) > 13,
                              "UHBR not supported on platform\n");
		igt_require_f(test_link_rate(&data, true, true, PHY_ANY),
			      "Didn't find any MST output with UHBR rates.\n");
	}

	igt_describe("Test we can drive NON-UHBR rates over SST");
	igt_subtest_with_dynamic("non-uhbr-sst") {
		igt_require_f(test_link_rate(&data, false, false, PHY_ANY),
			      "Didn't find any SST output with NON-UHBR rates.\n");
	}

	igt_describe("Test we can drive NON-UHBR rates over MST");
	igt_subtest_with_dynamic("non-uhbr-mst") {
		igt_require_f(test_link_rate(&data, true, false, PHY_ANY),
			      "Didn't find any MST output with NON-UHBR rates.\n");
	}

	igt_describe("Test we can drive UHBR rates over a tunneled SST link");
	igt_subtest_with_dynamic("uhbr-sst-tbtalt-train") {
		igt_require_f(intel_display_ver(data.devid) > 13,
			      "UHBR not supported on platform\n");
		igt_require_f(test_link_rate(&data, false, true, PHY_TBTALT),
			      "No tbt-alt DP SST link allows a UHBR config\n");
	}

	igt_describe("Test we can drive UHBR rates over a tunneled MST link");
	igt_subtest_with_dynamic("uhbr-mst-tbtalt-train") {
		igt_require_f(intel_display_ver(data.devid) > 13,
			      "UHBR not supported on platform\n");
		igt_require_f(test_link_rate(&data, true, true, PHY_TBTALT),
			      "No tbt-alt DP MST link allows a UHBR config\n");
	}

	igt_describe("Test we can drive UHBR rates over a direct SST link");
	igt_subtest_with_dynamic("uhbr-sst-direct-train") {
		igt_require_f(intel_display_ver(data.devid) > 13,
			      "UHBR not supported on platform\n");
		igt_require_f(test_link_rate(&data, false, true, PHY_DIRECT),
			      "No direct DP SST link allows a UHBR config\n");
	}

	igt_describe("Test we can drive UHBR rates over a direct MST link");
	igt_subtest_with_dynamic("uhbr-mst-direct-train") {
		igt_require_f(intel_display_ver(data.devid) > 13,
			      "UHBR not supported on platform\n");
		igt_require_f(test_link_rate(&data, true, true, PHY_DIRECT),
			      "No direct DP MST link allows a UHBR config\n");
	}

	igt_describe("Test we can drive non-UHBR rates over a tunneled SST link");
	igt_subtest_with_dynamic("non-uhbr-sst-tbtalt-train") {
		igt_require_f(test_link_rate(&data, false, false, PHY_TBTALT),
			      "No tbt-alt DP SST link allows a non-UHBR config\n");
	}

	igt_describe("Test we can drive non-UHBR rates over a tunneled MST link");
	igt_subtest_with_dynamic("non-uhbr-mst-tbtalt-train") {
		igt_require_f(test_link_rate(&data, true, false, PHY_TBTALT),
			      "No tbt-alt DP MST link allows a non-UHBR config\n");
	}

	igt_describe("Test we can drive non-UHBR rates over a direct SST link");
	igt_subtest_with_dynamic("non-uhbr-sst-direct-train") {
		igt_require_f(test_link_rate(&data, false, false, PHY_DIRECT),
			      "No direct DP SST link allows a non-UHBR config\n");
	}

	igt_describe("Test we can drive non-UHBR rates over a direct MST link");
	igt_subtest_with_dynamic("non-uhbr-mst-direct-train") {
		igt_require_f(test_link_rate(&data, true, false, PHY_DIRECT),
			      "No direct DP MST link allows a non-UHBR config\n");
	}

	igt_fixture() {
		igt_reset_connectors();
		igt_display_fini(&data.display);
		close(data.drm_fd);
	}
}
