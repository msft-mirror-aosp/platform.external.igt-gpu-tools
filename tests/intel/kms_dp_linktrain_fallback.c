/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2024 Intel Corporation
 */

/**
 * TEST: kms dp linktrain fallback
 * Category: Display
 * Description: Test link training fallback for eDP/DP connectors
 * Driver requirement: i915, xe
 * Functionality: link training
 * Mega feature: General Display Features
 * Test category: functionality test
 */

#include <sys/types.h>

#include "igt.h"

/**
 * SUBTEST: dp-fallback
 * Description: Test fallback on DP connectors
 */

#define RETRAIN_COUNT 1
/*
 * Two consecutives link training failures
 * reduces link params (link rate, lane count)
 */
#define LT_FAILURE_REDUCED_CAPS 2
#define SPURIOUS_HPD_RETRY 3

static int traversed_mst_outputs[IGT_MAX_PIPES];
static int traversed_mst_output_count;
typedef struct {
	int drm_fd;
	igt_display_t display;
	drmModeModeInfo *mode;
	igt_output_t *output;
	enum pipe pipe;
	struct igt_fb fb;
	struct igt_plane *primary;
	int n_pipes;
} data_t;

typedef int (*condition_check_fn)(int drm_fd, igt_output_t *output);

IGT_TEST_DESCRIPTION("Test link training fallback");

static void find_mst_outputs(int drm_fd, data_t *data,
			     igt_output_t *output,
			     igt_output_t *mst_outputs[],
			     int *num_mst_outputs)
{
	int output_root_id, root_id;
	igt_output_t *connector_output;

	output_root_id = igt_get_dp_mst_connector_id(output);
	/*
	 * If output is MST check all other connected output which shares
	 * same path and fill mst_outputs and num_mst_outputs
	 */
	for_each_connected_output(&data->display, connector_output) {
		if (!igt_check_output_is_dp_mst(connector_output))
			continue;
		root_id = igt_get_dp_mst_connector_id(connector_output);
		if (((*num_mst_outputs) < IGT_MAX_PIPES) && root_id == output_root_id)
			mst_outputs[(*num_mst_outputs)++] = connector_output;
	}
}

static bool setup_mst_outputs(data_t *data, igt_output_t *mst_output[],
			      int *output_count)
{
	int i;
	igt_output_t *output;

	/*
	 * Check if this is already traversed
	 */
	for (i = 0; i < traversed_mst_output_count; i++)
		if (i < IGT_MAX_PIPES &&
		    traversed_mst_outputs[i] == data->output->config.connector->connector_id)
			return false;

	find_mst_outputs(data->drm_fd, data, data->output,
			 mst_output, output_count);

	for (i = 0; i < *output_count; i++) {
		output = mst_output[i];
		if (traversed_mst_output_count < IGT_MAX_PIPES) {
			traversed_mst_outputs[traversed_mst_output_count++] = output->config.connector->connector_id;
			igt_info("Output %s is in same topology as %s\n",
				 igt_output_name(output),
				 igt_output_name(data->output));
		} else {
			igt_assert_f(false, "Unable to save traversed output\n");
			return false;
		}
	}
	return true;
}

static void setup_pipe_on_outputs(data_t *data,
				      igt_output_t *outputs[],
				      int *output_count)
{
	int i = 0;

	igt_require_f(data->n_pipes >= *output_count,
		      "Need %d pipes to assign to %d outputs\n",
		      data->n_pipes, *output_count);

	for_each_pipe(&data->display, data->pipe) {
		if (i >= *output_count)
			break;
		/*
		 * TODO: add support for modes requiring joined pipes
		 */
		igt_info("Setting pipe %s on output %s\n",
			 kmstest_pipe_name(data->pipe),
			 igt_output_name(outputs[i]));
		igt_output_set_pipe(outputs[i++], data->pipe);
	}
}

static void setup_modeset_on_outputs(data_t *data,
				     igt_output_t *outputs[],
				     int *output_count,
				     drmModeModeInfo *mode[],
				     struct igt_fb fb[],
				     struct igt_plane *primary[])
{
	int i;

	for (i = 0; i < *output_count; i++) {
		mode[i] = igt_output_get_mode(outputs[i]);
		igt_info("Mode %dx%d@%d on output %s\n",
			 mode[i]->hdisplay, mode[i]->vdisplay,
			 mode[i]->vrefresh,
			 igt_output_name(outputs[i]));
		primary[i] = igt_output_get_plane_type(outputs[i],
						       DRM_PLANE_TYPE_PRIMARY);
		igt_create_color_fb(data->drm_fd,
				    mode[i]->hdisplay,
				    mode[i]->vdisplay,
				    DRM_FORMAT_XRGB8888,
				    DRM_FORMAT_MOD_LINEAR, 0.0, 1.0, 0.0,
				    &fb[i]);
		igt_plane_set_fb(primary[i], &fb[i]);
	}
}

static void set_connector_link_status_good(data_t *data, igt_output_t *outputs[],
					   int *output_count)
{
	int i;
	igt_output_t *output;

        /*
         * update the link status to good for all outputs
         */
        for_each_connected_output(&data->display, output)
		for(i = 0; i < *output_count; i++)
			if (output->id == outputs[i]->id)
				igt_output_set_prop_value(output,
							  IGT_CONNECTOR_LINK_STATUS,
							  DRM_MODE_LINK_STATUS_GOOD);
}

static bool fit_modes_in_bw(data_t *data)
{
	bool found;
	int ret;

	ret = igt_display_try_commit_atomic(&data->display,
					    DRM_MODE_ATOMIC_TEST_ONLY |
					    DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
	if (ret != 0) {
		found = igt_override_all_active_output_modes_to_fit_bw(&data->display);
		igt_require_f(found,
			      "No valid mode combo found for modeset\n");
	}
	return true;
}

static bool validate_modeset_for_outputs(data_t *data,
					igt_output_t *outputs[],
					int *output_count,
					drmModeModeInfo *mode[],
					struct igt_fb fb[],
					struct igt_plane *primary[])
{
	igt_require_f(*output_count > 0, "Require at least 1 output\n");
	setup_pipe_on_outputs(data, outputs, output_count);
	igt_assert_f(fit_modes_in_bw(data), "Unable to fit modes in bw\n");
	setup_modeset_on_outputs(data, outputs,
				 output_count,
				 mode, fb, primary);
	return true;
}

static bool setup_outputs(data_t *data, bool is_mst,
		      igt_output_t *outputs[],
		      int *output_count, drmModeModeInfo *mode[],
		      struct igt_fb fb[], struct igt_plane *primary[])
{
	bool ret;

	*output_count = 0;

	if (is_mst) {
		ret = setup_mst_outputs(data, outputs, output_count);
		if (!ret) {
			igt_info("Skipping MST output %s as already tested\n",
				 igt_output_name(data->output));
			return false;
		}
	} else
		if ((*output_count) < IGT_MAX_PIPES)
			outputs[(*output_count)++] = data->output;

	ret = validate_modeset_for_outputs(data, outputs,
					   output_count, mode,
					   fb, primary);

	if (!ret) {
		igt_info("Skipping output %s as valid pipe/output combo not found\n",
			 igt_output_name(data->output));
		return false;
	}

	igt_display_commit2(&data->display, COMMIT_ATOMIC);
	return true;
}

static int check_condition_with_timeout(int drm_fd, igt_output_t *output,
					condition_check_fn check_fn,
					double interval, double timeout)
{
	struct timespec start_time, current_time;
	double elapsed_time;

	clock_gettime(CLOCK_MONOTONIC, &start_time);

	while (1) {
		if (check_fn(drm_fd, output) == 0)
			return 0;

		clock_gettime(CLOCK_MONOTONIC, &current_time);
		elapsed_time = (current_time.tv_sec - start_time.tv_sec) +
			(current_time.tv_nsec - start_time.tv_nsec) / 1e9;

		if (elapsed_time >= timeout)
			return -1;

		usleep((useconds_t)(interval * 1000000));
	}
}

static void test_fallback(data_t *data, bool is_mst)
{
	int output_count, retries;
	int max_link_rate, curr_link_rate, prev_link_rate;
	int max_lane_count, curr_lane_count, prev_lane_count;
	igt_output_t *outputs[IGT_MAX_PIPES];
	uint32_t link_status_prop_id;
	uint64_t link_status_value;
	drmModeModeInfo *modes[IGT_MAX_PIPES];
	drmModePropertyPtr link_status_prop;
	struct igt_fb fbs[IGT_MAX_PIPES];
	struct igt_plane *primarys[IGT_MAX_PIPES];
	struct udev_monitor *mon;

	retries = SPURIOUS_HPD_RETRY;

	igt_display_reset(&data->display);
	igt_reset_link_params(data->drm_fd, data->output);
	if (!setup_outputs(data, is_mst, outputs,
			   &output_count, modes, fbs,
			   primarys))
		return;

	igt_info("Testing link training fallback on %s\n",
		 igt_output_name(data->output));
	max_link_rate = igt_get_max_link_rate(data->drm_fd, data->output);
	max_lane_count = igt_get_max_lane_count(data->drm_fd, data->output);
	prev_link_rate = igt_get_current_link_rate(data->drm_fd, data->output);
	prev_lane_count = igt_get_current_lane_count(data->drm_fd, data->output);

	while (!igt_get_dp_link_retrain_disabled(data->drm_fd,
						 data->output)) {
		igt_info("Current link rate: %d, Current lane count: %d\n",
			 prev_link_rate,
			 prev_lane_count);
		mon = igt_watch_uevents();
		igt_force_lt_failure(data->drm_fd, data->output,
				     LT_FAILURE_REDUCED_CAPS);
		igt_force_link_retrain(data->drm_fd, data->output,
				       RETRAIN_COUNT);

		igt_assert_eq(check_condition_with_timeout(data->drm_fd,
							   data->output,
							   igt_get_dp_pending_retrain,
							   1.0, 20.0), 0);
		igt_assert_eq(check_condition_with_timeout(data->drm_fd,
							   data->output,
							   igt_get_dp_pending_lt_failures,
							   1.0, 20.0), 0);

		if (igt_get_dp_link_retrain_disabled(data->drm_fd,
						     data->output)) {
			igt_reset_connectors();
			return;
		}

		igt_assert_f(igt_hotplug_detected(mon, 20),
			     "Didn't get hotplug for force link training failure\n");

		kmstest_get_property(data->drm_fd,
				data->output->config.connector->connector_id,
				DRM_MODE_OBJECT_CONNECTOR, "link-status",
				&link_status_prop_id, &link_status_value,
				&link_status_prop);
		igt_assert_eq(link_status_value, DRM_MODE_LINK_STATUS_BAD);
		igt_flush_uevents(mon);
		set_connector_link_status_good(data, outputs, &output_count);
		igt_assert_f(validate_modeset_for_outputs(data,
							  outputs,
							  &output_count,
							  modes,
							  fbs,
							  primarys),
			     "modeset failed\n");
		igt_display_commit2(&data->display, COMMIT_ATOMIC);

		igt_assert_eq(data->output->values[IGT_CONNECTOR_LINK_STATUS], DRM_MODE_LINK_STATUS_GOOD);
		curr_link_rate = igt_get_current_link_rate(data->drm_fd, data->output);
		curr_lane_count = igt_get_current_lane_count(data->drm_fd, data->output);

		igt_assert_f((curr_link_rate < prev_link_rate ||
			     curr_lane_count < prev_lane_count) ||
			     ((curr_link_rate == max_link_rate && curr_lane_count == max_lane_count) && --retries),
			     "Fallback unsuccessful\n");

		prev_link_rate = curr_link_rate;
		prev_lane_count = curr_lane_count;
	}
}

static bool run_test(data_t *data)
{
	bool ran = false;
	igt_output_t *output;

	for_each_connected_output(&data->display, output) {
		data->output = output;

		if (!igt_has_force_link_training_failure_debugfs(data->drm_fd,
								 data->output)) {
			igt_info("Output %s doesn't support forcing link training failure\n",
				 igt_output_name(data->output));
			continue;
		}

		if (output->config.connector->connector_type != DRM_MODE_CONNECTOR_DisplayPort) {
			igt_info("Skipping output %s as it's not DP\n", output->name);
				continue;
		}

		ran = true;

		/*
		 * Check output is MST
		 */
		if (igt_check_output_is_dp_mst(data->output)) {
			igt_info("Testing MST output %s\n",
				 igt_output_name(data->output));
			test_fallback(data, true);
		} else {
			igt_info("Testing DP output %s\n",
				 igt_output_name(data->output));
			test_fallback(data, false);
		}
	}
	return ran;
}

igt_main
{
	data_t data = {};

	igt_fixture {
		data.drm_fd = drm_open_driver_master(DRIVER_INTEL |
						     DRIVER_XE);
		kmstest_set_vt_graphics_mode();
		igt_display_require(&data.display, data.drm_fd);
		igt_display_require_output(&data.display);
		for_each_pipe(&data.display, data.pipe)
			data.n_pipes++;
	}

	igt_subtest("dp-fallback") {
		igt_require_f(run_test(&data),
			      "Skipping test as no output found or none supports fallback\n");
	}

	igt_fixture {
		igt_remove_fb(data.drm_fd, &data.fb);
		igt_display_fini(&data.display);
		close(data.drm_fd);
	}
}