/*
 * Copyright © 2016 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Lyude Paul <lyude@redhat.com>
 */

/**
 * TEST: kms chamelium hpd
 * Category: Display
 * Description: Testing HPD with a Chamelium board
 * Driver requirement: i915, xe
 * Mega feature: General Display Features
 */

#include "kms_chamelium_helper.h"
#include "igt_device.h"

/**
 * SUBTEST: dp-hpd-fast
 * Description: Check that we get uevents and updated connector status on
 * 		hotplug and unplug
 *
 * SUBTEST: hdmi-hpd-fast
 * Description: Check that we get uevents and updated connector status on
 * 		hotplug and unplug
 *
 * SUBTEST: vga-hpd-fast
 * Description: Check that we get uevents and updated connector status on
 * 		hotplug and unplug
 *
 * SUBTEST: dp-hpd
 * Description: Check that we get uevents and updated connector status on
 *              hotplug and unplug
 *
 * SUBTEST: hdmi-hpd
 * Description: Check that we get uevents and updated connector status on
 *              hotplug and unplug
 *
 * SUBTEST: vga-hpd
 * Description: Check that we get uevents and updated connector status on
 *              hotplug and unplug
 *
 * SUBTEST: dp-hpd-%s
 * Description: Check that we get uevents and updated connector status on
 *              hotplug and unplug
 *
 * SUBTEST: hdmi-hpd-%s
 * Description: Check that we get uevents and updated connector status on
 *              hotplug and unplug
 *
 * SUBTEST: vga-hpd-%s
 * Description: Check that we get uevents and updated connector status on
 *              hotplug and unplug
 *
 * arg[1]:
 *
 * @enable-disable-mode:     Toggle the mode
 * @with-enabled-mode:       Enabling the mode
 */

/**
 * SUBTEST: dp-hpd-for-each-pipe
 * Description: Check that we get uevents and updated connector status on
 *              hotplug and unplug for each pipe with valid output
 *
 * SUBTEST: hdmi-hpd-for-each-pipe
 * Description: Check that we get uevents and updated connector status on
 *              hotplug and unplug for each pipe with valid output
 *
 * SUBTEST: vga-hpd-for-each-pipe
 * Description: Check that we get uevents and updated connector status on
 *              hotplug and unplug for each pipe with valid output
 *
 * SUBTEST: dp-hpd-after-hibernate
 * Description: Toggle HPD during Hibernation, check that uevents are sent and
 *              connector status is updated
 *
 * SUBTEST: hdmi-hpd-after-hibernate
 * Description: Toggle HPD during Hibernation, check that uevents are sent and
 *              connector status is updated
 *
 * SUBTEST: vga-hpd-after-hibernate
 * Description: Toggle HPD during Hibernation, check that uevents are sent and
 *              connector status is updated
 *
 * SUBTEST: dp-hpd-after-suspend
 * Description: Toggle HPD during Suspend, check that uevents are sent and
 *              connector status is updated
 *
 * SUBTEST: hdmi-hpd-after-suspend
 * Description: Toggle HPD during Suspend, check that uevents are sent and
 *              connector status is updated
 *
 * SUBTEST: vga-hpd-after-suspend
 * Description: Toggle HPD during Suspend, check that uevents are sent and
 *              connector status is updated
 *
 * SUBTEST: dp-hpd-after-runtime-suspend-d3hot
 * Description: Toggle HPD while runtime suspended in D3hot with PME signalling
 *              available, check that connector polling stayed disabled, that the
 *              device woke itself up and that uevents are sent and connector
 *              status is updated
 *
 * SUBTEST: hdmi-hpd-after-runtime-suspend-d3hot
 * Description: Toggle HPD while runtime suspended in D3hot with PME signalling
 *              available, check that connector polling stayed disabled, that the
 *              device woke itself up and that uevents are sent and connector
 *              status is updated
 *
 * SUBTEST: common-hpd-after-suspend
 * Description: Toggle HPD during suspend on all connectors, check that uevents
 * 		are sent and connector status is updated
 *
 * SUBTEST: common-hpd-after-hibernate
 * Description: Toggle HPD during suspend on all connectors, check that uevents
 *              are sent and connector status is updated
 *
 * SUBTEST: vga-hpd-without-ddc
 * Description: Disable DDC on a VGA connector, check we still get a uevent on
 *              hotplug
 *
 * SUBTEST: dp-hpd-storm
 * Description: Trigger a series of hotplugs in a very small timeframe to
 *              simulate abad cable, check the kernel falls back to polling
 *              to avoid a hotplug storm
 *
 * SUBTEST: hdmi-hpd-storm
 * Description: Trigger a series of hotplugs in a very small timeframe to
 *              simulate abad cable, check the kernel falls back to polling
 *              to avoid a hotplug storm
 *
 * SUBTEST: dp-hpd-storm-disable
 * Description: Disable HPD storm detection, trigger a storm and check the
 *              kernel doesn't detect one
 *
 * SUBTEST: hdmi-hpd-storm-disable
 * Description: Disable HPD storm detection, trigger a storm and check the
 *              kernel doesn't detect one
 */

#define HPD_STORM_PULSE_INTERVAL_DP 100 /* ms */
#define HPD_STORM_PULSE_INTERVAL_HDMI 200 /* ms */

#define HPD_TOGGLE_COUNT_VGA 5
#define HPD_TOGGLE_COUNT_DP_HDMI 15
#define HPD_TOGGLE_COUNT_FAST 3

/*
 * The KMS helper polls connectors every 10s, so watch for longer than two poll
 * periods: a single delayed or coalesced poll then cannot make the test pass.
 */
#define PME_POLL_PROOF_WINDOW_MS 25000
/*
 * How long the control run, with PME not armed, waits for a poll induced
 * resume: two poll periods.
 */
#define PME_POLL_CONTROL_WINDOW_MS 20000
/*
 * How often to sample the D state and runtime_active_time while watching a
 * suspended device. Config space reads do not resume a device in D3hot.
 */
#define PME_D_STATE_SAMPLE_MS 100
/*
 * Upper bound on the runtime_active_time the device may accrue while idle, to
 * allow for one unrelated wakeup rather than requiring exactly zero. The bound
 * actually applied is the lower of this and the cost of one poll induced
 * resume, as measured by the control run, so that the two or more polls a
 * still polling driver makes during the window always exceed it.
 */
#define PME_ACTIVE_TIME_SLACK_MS 100
/* Tolerance for accounting granularity and usleep() overshoot. */
#define PME_POLL_PROOF_SLACK_MS 500
/*
 * How long to leave the device suspended before measuring anything, so that
 * work left over from the previous step, such as the 1s HPD retry or a resume
 * of a linked device like HDA, is not blamed on polling.
 */
#define PME_SETTLE_MS 3000
/*
 * A PME reaches the PCI core either as an interrupt, through native PCIe PME
 * or an ACPI GPE, or, without either route, through the PCI core's own PME
 * poll, which runs every 1s. Either way the wake is expected within a couple of
 * seconds. The whole point of the feature is that this is not the ~10s a
 * connector poll would take.
 */
#define PME_WAKE_LATENCY_MAX_MS 3000
#define PME_HPD_TOGGLE_DELAY_MS 1000

enum test_modeset_mode {
	TEST_MODESET_ON,
	TEST_MODESET_ON_OFF,
	TEST_MODESET_OFF,
};

static void try_suspend_resume_hpd(chamelium_data_t *data,
				   struct chamelium_port *port,
				   enum igt_suspend_state state,
				   enum igt_suspend_test test,
				   struct udev_monitor *mon, bool connected)
{
	drmModeConnection target_state = connected ? DRM_MODE_DISCONNECTED :
						     DRM_MODE_CONNECTED;
	int timeout = CHAMELIUM_HOTPLUG_TIMEOUT;
	int delay;
	int p;

	igt_flush_uevents(mon);

	delay = igt_get_autoresume_delay(state) * 1000 / 2;

	if (port) {
		chamelium_schedule_hpd_toggle(data->chamelium, port, delay,
					      !connected);
	} else {
		for (p = 0; p < data->port_count; p++) {
			port = data->ports[p];
			chamelium_schedule_hpd_toggle(data->chamelium, port,
						      delay, !connected);
		}

		port = NULL;
	}

	igt_system_suspend_autoresume(state, test);
	igt_assert(chamelium_wait_for_hotplug(mon, &timeout));
	chamelium_assert_reachable(data->chamelium, ONLINE_TIMEOUT);

	if (port) {
		igt_assert_eq(chamelium_reprobe_connector(
				      &data->display, data->chamelium, port),
			      target_state);
	} else {
		for (p = 0; p < data->port_count; p++) {
			drmModeConnection current_state;

			port = data->ports[p];
			/*
			 * There could be as many hotplug events sent by
			 * driver as connectors we scheduled an HPD toggle on
			 * above, depending on timing. So if we're not seeing
			 * the expected connector state try to wait for an HPD
			 * event for each connector/port.
			 */
			current_state = chamelium_reprobe_connector(
				&data->display, data->chamelium, port);
			if (p > 0 && current_state != target_state) {
				igt_assert(chamelium_wait_for_hotplug(
					mon, &timeout));
				current_state = chamelium_reprobe_connector(
					&data->display, data->chamelium, port);
			}

			igt_assert_eq(current_state, target_state);
		}

		port = NULL;
	}
}

static const char test_basic_hotplug_desc[] =
	"Check that we get uevents and updated connector status on "
	"hotplug and unplug";
static void test_hotplug(chamelium_data_t *data, struct chamelium_port *port,
			 int toggle_count, enum test_modeset_mode modeset_mode)
{
	int i;
	igt_crtc_t *crtc;
	struct igt_fb fb = { 0 };
	drmModeModeInfo mode;
	struct udev_monitor *mon = igt_watch_uevents();
	igt_output_t *output = chamelium_get_output_for_port(data, port);

	igt_modeset_disable_all_outputs(&data->display);
	chamelium_reset_state(&data->display, data->chamelium, NULL,
			      data->ports, data->port_count);

	igt_hpd_storm_set_threshold(data->drm_fd, 0);

	for (i = 0; i < toggle_count; i++) {
		igt_flush_uevents(mon);

		/* Check if we get a sysfs hotplug event */
		chamelium_plug(data->chamelium, port);

		chamelium_wait_for_connector_after_hotplug(data, mon, port,
							   DRM_MODE_CONNECTED);
		igt_flush_uevents(mon);

		if (modeset_mode == TEST_MODESET_ON_OFF ||
		    (modeset_mode == TEST_MODESET_ON && i == 0)) {
			if (i == 0) {
				/* We can only get mode and pipe once we are
				 * connected */
				output = chamelium_get_output_for_port(data,
								       port);
				crtc = chamelium_get_pipe_for_output(&data->display,
								     output);
				mode = chamelium_get_mode_for_port(
					data->chamelium, port);
				chamelium_create_fb_for_mode(data, &fb, &mode);
			}

			igt_output_set_crtc(output,
					    crtc);
			chamelium_enable_output(data, port, output, &mode, &fb);
		}

		/* Now check if we get a hotplug from disconnection */
		chamelium_unplug(data->chamelium, port);

		chamelium_wait_for_connector_after_hotplug(
			data, mon, port, DRM_MODE_DISCONNECTED);

		igt_flush_uevents(mon);

		if (modeset_mode == TEST_MODESET_ON_OFF) {
			igt_output_set_crtc(output, NULL);
			igt_display_commit2(&data->display, COMMIT_ATOMIC);
		}
	}

	igt_cleanup_uevents(mon);
	igt_hpd_storm_reset(data->drm_fd);
	igt_remove_fb(data->drm_fd, &fb);
}

static const char test_hotplug_for_each_pipe_desc[] =
	"Check that we get uevents and updated connector status on "
	"hotplug and unplug for each pipe with valid output";
static void test_hotplug_for_each_pipe(chamelium_data_t *data,
				       struct chamelium_port *port)
{
	igt_output_t *output;
	igt_crtc_t *crtc;
	struct udev_monitor *mon = igt_watch_uevents();

	chamelium_reset_state(&data->display, data->chamelium, port,
			      data->ports, data->port_count);

	igt_hpd_storm_set_threshold(data->drm_fd, 0);

	for_each_crtc(&data->display, crtc) {
		igt_modeset_disable_all_outputs(&data->display);
		igt_flush_uevents(mon);
		/* Check if we get a sysfs hotplug event */
		chamelium_plug(data->chamelium, port);
		chamelium_wait_for_connector_after_hotplug(data, mon, port,
							   DRM_MODE_CONNECTED);
		igt_flush_uevents(mon);
		output = chamelium_get_output_for_port(data, port);

		/* If pipe is valid for output then set it */
		igt_output_set_crtc(output,
				    crtc);
		if (intel_pipe_output_combo_valid(&data->display))
			igt_display_commit2(&data->display, COMMIT_ATOMIC);

		chamelium_unplug(data->chamelium, port);
		chamelium_wait_for_connector_after_hotplug(
			data, mon, port, DRM_MODE_DISCONNECTED);
		igt_flush_uevents(mon);
	}

	igt_cleanup_uevents(mon);
	igt_hpd_storm_reset(data->drm_fd);
}

static const char test_suspend_resume_hpd_desc[] =
	"Toggle HPD during suspend, check that uevents are sent and connector "
	"status is updated";
static void test_suspend_resume_hpd(chamelium_data_t *data,
				    struct chamelium_port *port,
				    enum igt_suspend_state state,
				    enum igt_suspend_test test)
{
	struct udev_monitor *mon = igt_watch_uevents();

	igt_modeset_disable_all_outputs(&data->display);
	chamelium_reset_state(&data->display, data->chamelium, port,
			      data->ports, data->port_count);

	/* Make sure we notice new connectors after resuming */
	try_suspend_resume_hpd(data, port, state, test, mon, false);

	/* Now make sure we notice disconnected connectors after resuming */
	try_suspend_resume_hpd(data, port, state, test, mon, true);

	igt_cleanup_uevents(mon);
}

static const char test_suspend_resume_hpd_common_desc[] =
	"Toggle HPD during suspend on all connectors, check that uevents are "
	"sent and connector status is updated";
static void test_suspend_resume_hpd_common(chamelium_data_t *data,
					   enum igt_suspend_state state,
					   enum igt_suspend_test test)
{
	struct udev_monitor *mon = igt_watch_uevents();
	struct chamelium_port *port;
	int p;

	for (p = 0; p < data->port_count; p++) {
		port = data->ports[p];
		igt_debug("Testing port %s\n", chamelium_port_get_name(port));
	}

	igt_modeset_disable_all_outputs(&data->display);
	chamelium_reset_state(&data->display, data->chamelium, NULL,
			      data->ports, data->port_count);

	/* Make sure we notice new connectors after resuming */
	try_suspend_resume_hpd(data, NULL, state, test, mon, false);

	/* Now make sure we notice disconnected connectors after resuming */
	try_suspend_resume_hpd(data, NULL, state, test, mon, true);

	igt_cleanup_uevents(mon);
}

/*
 * Name a D state for failure messages. igt_pm_pci_get_d_state() reports D3cold
 * from the kernel's power_state without touching config space, so an unknown
 * state here means the config space read failed, i.e. the device has gone away.
 */
static const char *pme_d_state_name(enum igt_acpi_d_state state)
{
	switch (state) {
	case IGT_ACPI_D0:
		return "D0";
	case IGT_ACPI_D1:
		return "D1";
	case IGT_ACPI_D2:
		return "D2";
	case IGT_ACPI_D3Hot:
		return "D3hot";
	case IGT_ACPI_D3Cold:
		return "D3cold";
	default:
		return "an unreadable state";
	}
}

/*
 * State changed by the PME subtest that has no exit handler of its own.
 * pme_hpd_cleanup() puts it back, together with power/wakeup and
 * drm_kms_helper.poll, both at the end of the subtest and from a fixture after
 * it, so that a failure or skip does not leave the next subtest running with it.
 */
static struct {
	struct udev_monitor *mon;
	bool storm_set;
} pme_hpd;

static void pme_hpd_cleanup(int drm_fd)
{
	if (pme_hpd.mon) {
		igt_cleanup_uevents(pme_hpd.mon);
		pme_hpd.mon = NULL;
	}

	if (pme_hpd.storm_set) {
		igt_hpd_storm_reset(drm_fd);
		pme_hpd.storm_set = false;
	}

	igt_kms_poll_restore();
	igt_pm_restore_wakeup();
}

/*
 * Read power/wakeup_active_count. It only exists while power/wakeup is enabled,
 * which the test keeps it from before it takes any baseline, so a missing count
 * means something else disabled it and no comparison would mean anything.
 */
static int64_t pme_wakeup_count(struct pci_device *pci_dev)
{
	int64_t count = igt_pm_get_wakeup_active_count(pci_dev);

	igt_assert_f(count >= 0,
		     "power/wakeup_active_count is unavailable, so power/wakeup "
		     "was disabled during the test\n");

	return count;
}

/*
 * Wait for the device to runtime suspend, leave it alone for PME_SETTLE_MS so
 * that work left over from the previous step can finish, and wait again in case
 * that work resumed it.
 */
static void pme_wait_settled(void)
{
	igt_assert_f(igt_wait_for_pm_status(IGT_RUNTIME_PM_STATUS_SUSPENDED),
		     "Device did not runtime suspend\n");
	usleep(PME_SETTLE_MS * 1000);
	igt_assert_f(igt_wait_for_pm_status(IGT_RUNTIME_PM_STATUS_SUSPENDED),
		     "Device did not runtime suspend\n");
}

/*
 * Whether PME is armed, and so whether the driver polls connectors, is decided
 * on every runtime suspend. After changing power/wakeup, resume the device once
 * so that the next suspend picks up the new value. Probing the connector takes
 * a display power reference, which does exactly that.
 */
static void pme_cycle_runtime_pm(chamelium_data_t *data,
				 struct chamelium_port *port)
{
	chamelium_reprobe_connector(&data->display, data->chamelium, port);
	pme_wait_settled();
}

/*
 * Watch a runtime suspended device for up to @window_ms, sampling every
 * PME_D_STATE_SAMPLE_MS. The device counts as woken once it is seen outside
 * D3hot, or once it has accrued more than @active_limit_ms of
 * runtime_active_time, which also catches resumes too brief to be sampled.
 *
 * Returns the time in ms at which the device was seen to wake, or -1 if it did
 * not. @d_state and @active_ms hold the last sample.
 */
static int pme_watch_for_wake(struct pci_device *pci_dev, int window_ms,
			      uint64_t active_limit_ms,
			      enum igt_acpi_d_state *d_state,
			      uint64_t *active_ms)
{
	uint64_t active_before = igt_pm_get_runtime_active_time(pci_dev);
	struct timespec start, now;
	int elapsed;

	igt_assert_eq(igt_gettime(&start), 0);
	for (;;) {
		*d_state = igt_pm_pci_get_d_state(pci_dev);
		*active_ms = igt_pm_get_runtime_active_time(pci_dev) -
			     active_before;

		igt_assert_eq(igt_gettime(&now), 0);
		elapsed = igt_time_elapsed(&start, &now) * 1000;

		if (*d_state != IGT_ACPI_D3Hot || *active_ms > active_limit_ms)
			return elapsed;
		if (elapsed >= window_ms)
			return -1;

		usleep(PME_D_STATE_SAMPLE_MS * 1000);
	}
}

/*
 * Control run, with PME not armed. The driver then falls back to polling
 * connectors while runtime suspended, so the device has to be seen to wake
 * within two poll periods. That proves polling is live on this setup and that
 * pme_watch_for_wake() catches a poll induced resume, so that the same check
 * passing with PME armed means something. The only difference between the two
 * runs is whether PME is armed.
 *
 * Returns the runtime_active_time in ms that one poll induced resume costs.
 */
static uint64_t pme_measure_poll_wake(chamelium_data_t *data,
				      struct chamelium_port *port,
				      struct pci_device *pci_dev)
{
	enum igt_acpi_d_state d_state;
	uint64_t active_before, active_ms;
	int pme_en, woke_ms;

	/*
	 * xe arms PME from its runtime suspend hook with pci_enable_wake(), which
	 * refuses unless device_may_wakeup(), i.e. unless power/wakeup is
	 * enabled. xe also saves the PCI state itself, so the PCI/PM core does
	 * not arm PME on its behalf either. Disabling power/wakeup is therefore
	 * enough to runtime suspend without PME.
	 */
	igt_pm_set_wakeup_enabled(pci_dev, false);
	pme_cycle_runtime_pm(data, port);

	pme_en = igt_pm_pci_pme_enabled(pci_dev);
	igt_assert_f(pme_en == 0, "PME_En is %s with power/wakeup disabled\n",
		     pme_en < 0 ? "unreadable" : "set");

	active_before = igt_pm_get_runtime_active_time(pci_dev);
	woke_ms = pme_watch_for_wake(pci_dev, PME_POLL_CONTROL_WINDOW_MS, 0,
				     &d_state, &active_ms);
	igt_skip_on_f(woke_ms < 0,
		      "Device stayed suspended for %dms with PME not armed, so "
		      "connector polling is not running and cannot be told apart "
		      "from PME\n", PME_POLL_CONTROL_WINDOW_MS);

	/* Let the resume complete, so that its whole cost is accounted. */
	igt_assert_f(igt_wait_for_pm_status(IGT_RUNTIME_PM_STATUS_SUSPENDED),
		     "Device did not runtime suspend again after a poll\n");
	active_ms = igt_pm_get_runtime_active_time(pci_dev) - active_before;

	igt_info("Without PME the device woke after %dms, %s, and one poll "
		 "costs %" PRIu64 "ms active\n", woke_ms,
		 d_state != IGT_ACPI_D3Hot ? "seen outside D3hot" :
					     "seen as active time",
		 active_ms);

	return active_ms;
}

static void try_hpd_runtime_suspend_pme(chamelium_data_t *data,
					struct chamelium_port *port,
					struct pci_device *pci_dev,
					struct udev_monitor *mon,
					bool connected, bool check_idle,
					uint64_t active_limit_ms)
{
	drmModeConnection target_state = connected ? DRM_MODE_DISCONNECTED :
						     DRM_MODE_CONNECTED;
	int timeout = CHAMELIUM_HOTPLUG_TIMEOUT;
	enum igt_acpi_d_state d_state;
	int64_t wakeups_before, wakeups_after;
	uint64_t susp_before, susp_delta, active_ms;
	struct timespec start, end;
	int pme_en, pending, woke_ms, latency;
	const char *cause = NULL;
	bool hotplugged;

	pme_wait_settled();

	/*
	 * Whether the device suspends into D3hot at all was decided up front, so
	 * finding it anywhere else now means something woke it, which is a
	 * failure rather than a reason to skip.
	 */
	d_state = igt_pm_pci_get_d_state(pci_dev);
	igt_assert_f(d_state == IGT_ACPI_D3Hot,
		     "Device is in %s rather than D3hot after runtime suspending\n",
		     pme_d_state_name(d_state));

	/*
	 * PME_En is the direct sign that the feature engaged; without it, the test
	 * would only be measuring the polling fallback.
	 */
	pme_en = igt_pm_pci_pme_enabled(pci_dev);
	igt_assert_f(pme_en == 1, "PME_En is %s after suspending into D3hot\n",
		     pme_en < 0 ? "unreadable" : "not set");

	if (check_idle) {
		bool self_woke;

		/*
		 * Phase 1: ensure the device stays in D3hot while idle. If polling
		 * remains on, the KMS helper will resume it to probe connectors.
		 */
		wakeups_before = pme_wakeup_count(pci_dev);
		susp_before = igt_pm_get_runtime_suspended_time(pci_dev);

		woke_ms = pme_watch_for_wake(pci_dev, PME_POLL_PROOF_WINDOW_MS,
					     active_limit_ms, &d_state,
					     &active_ms);

		/*
		 * Distinguish self-woken resumes from host-driven ones. A
		 * self-wakeup bumps wakeup_active_count; a host-driven resume
		 * does not.
		 */
		wakeups_after = pme_wakeup_count(pci_dev);
		self_woke = wakeups_after > wakeups_before;

		igt_fail_on_f(woke_ms >= 0,
			      "Device woke after %dms of a %dms idle window: it "
			      "is in %s and was active for %" PRIu64 "ms, %" PRIu64
			      "ms allowed.\n"
			      "wakeup_active_count %" PRId64 " -> %" PRId64
			      ", so the resume was %s.\n%s\n",
			      woke_ms, PME_POLL_PROOF_WINDOW_MS,
			      pme_d_state_name(d_state), active_ms,
			      active_limit_ms, wakeups_before, wakeups_after,
			      self_woke ? "signalled by the device" :
					  "initiated by the host",
			      self_woke ?
			      "No hotplug was requested yet, so this is a "
			      "spurious or still pending PME rather than the "
			      "polling fallback." :
			      "The polling fallback resumes on a 10s period, or "
			      "1s while a delayed event is pending, so compare "
			      "the elapsed time above: a wake at an unrelated "
			      "delay is some other client or driver taking a "
			      "display power reference rather than connector "
			      "polling.");

		susp_delta = igt_pm_get_runtime_suspended_time(pci_dev) -
			     susp_before;

		igt_info("Idle for %dms in D3hot: suspended +%" PRIu64
			 "ms, active +%" PRIu64 "ms\n",
			 PME_POLL_PROOF_WINDOW_MS, susp_delta, active_ms);

		igt_assert_lte_u64(PME_POLL_PROOF_WINDOW_MS -
				   PME_POLL_PROOF_SLACK_MS, susp_delta);
	}

	/*
	 * Phase 2: fire the HPD and check the device wakes up promptly. The
	 * toggle is scheduled over XMLRPC, which travels over the network and so
	 * does not disturb the GPU while we wait for the uevent.
	 *
	 * Take a fresh wakeup baseline rather than reusing the one from phase 1,
	 * which is a whole idle window old by now. Phase 1 only requires that
	 * the device stayed in D3hot, which a PME that resumed nothing at all
	 * would not violate, so the counter is not guaranteed to be unchanged.
	 *
	 * Flush uevents only now, right before the toggle, so that one sent
	 * during the steps above cannot satisfy the wait, and start the clock
	 * before the XMLRPC call, so that the latency cannot come out negative.
	 */
	wakeups_before = pme_wakeup_count(pci_dev);
	igt_flush_uevents(mon);

	igt_assert_eq(igt_gettime(&start), 0);
	chamelium_schedule_hpd_toggle(data->chamelium, port,
				      PME_HPD_TOGGLE_DELAY_MS, !connected);
	hotplugged = chamelium_wait_for_hotplug(mon, &timeout);
	igt_assert_eq(igt_gettime(&end), 0);

	d_state = IGT_ACPI_UNKNOWN_STATE;
	pending = -1;
	wakeups_after = -1;
	if (!hotplugged) {
		d_state = igt_pm_pci_get_d_state(pci_dev);
		pending = igt_pm_pci_pme_status(pci_dev);
		wakeups_after = igt_pm_get_wakeup_active_count(pci_dev);

		/*
		 * If the device stays in D3hot, the wake is from PME. If it
		 * returns to D0, the HPD was handled by the driver resume path.
		 */
		if (d_state != IGT_ACPI_D3Hot)
			cause = "The device resumed but sent no uevent, so the "
				"HPD was dropped on the resume path.";
		else if (pending < 0)
			cause = "PME_Status could not be read, so whether the "
				"device signalled a PME is unknown.";
		else if (pending)
			cause = "The device signalled a PME that was never "
				"delivered, so the PME is not reaching the OS.";
		else
			cause = "The device never signalled a PME, so the HPD "
				"is not reaching the PME logic.";
	}
	igt_fail_on_f(!hotplugged,
		      "No hotplug uevent %ds after an HPD toggle in D3hot.\n"
		      "Device is in %s, PME_Status is %s, "
		      "wakeup_active_count %" PRId64 " -> %" PRId64 ".\n%s\n",
		      CHAMELIUM_HOTPLUG_TIMEOUT, pme_d_state_name(d_state),
		      pending < 0 ? "unreadable" : pending ? "set" : "clear",
		      wakeups_before, wakeups_after, cause);

	latency = igt_time_elapsed(&start, &end) * 1000 -
		  PME_HPD_TOGGLE_DELAY_MS;

	/*
	 * wakeup_active_count incrementing means the resume was signalled by the
	 * device itself, i.e. by a PME. Unlike PME_Status, which the PCI/PM core
	 * clears on the way back to D0, this counter survives the resume and so
	 * is not racy to sample here.
	 */
	wakeups_after = pme_wakeup_count(pci_dev);

	igt_info("HPD to uevent latency %dms, wakeup_active_count %" PRId64
		 " -> %" PRId64 "\n", latency, wakeups_before, wakeups_after);

	igt_assert_f(wakeups_after > wakeups_before,
		     "power/wakeup_active_count did not increment, so the resume "
		     "was not triggered by a PME from the device\n");
	igt_assert_lte(0, latency);
	igt_assert_lt(latency, PME_WAKE_LATENCY_MAX_MS);

	chamelium_assert_reachable(data->chamelium, ONLINE_TIMEOUT);
	igt_assert_eq(chamelium_reprobe_connector(&data->display,
						  data->chamelium, port),
		      target_state);
}

/*
 * Mirror of HAS_PM_PME_SUPPORT() in xe_pm.c: only xe reports PME capability to
 * the display code, and only from graphics IP 35.00 on.
 */
static bool pme_hpd_supported(int fd)
{
	return is_xe_device(fd) &&
	       intel_graphics_ver(intel_get_drm_devid(fd)) >= IP_VER(35, 0);
}

static const char test_hpd_runtime_suspend_pme_desc[] =
	"Toggle HPD while runtime suspended in D3hot with PME signalling "
	"available, check that connector polling stayed disabled, that the "
	"device woke itself up and that uevents are sent and connector status "
	"is updated";
static void test_hpd_runtime_suspend_pme(chamelium_data_t *data,
					 struct chamelium_port *port)
{
	struct pci_device *pci_dev;
	enum igt_acpi_d_state d_state;
	uint64_t active_limit_ms;

	pci_dev = igt_device_get_pci_device(data->drm_fd);

	/*
	 * Check everything that can make the subtest skip before changing any
	 * state. Without driver support the device keeps HPD polling enabled
	 * and phase 1 would fail rather than skip, so gate on the platform
	 * before looking at the hardware capability at all.
	 */
	igt_require_f(pme_hpd_supported(data->drm_fd),
		      "Driver does not use PME for HPD on this platform\n");
	igt_require_f(igt_pm_pci_pme_supported(pci_dev, IGT_ACPI_D3Hot),
		      "Device does not advertise PME support in D3hot\n");
	igt_require_f(igt_pm_has_wakeup_support(pci_dev),
		      "Device does not expose power/wakeup\n");
	igt_require_hpd_storm_ctl(data->drm_fd);
	igt_require(igt_kms_poll_save());
	igt_require(igt_setup_runtime_pm(data->drm_fd));

	igt_modeset_disable_all_outputs(&data->display);
	chamelium_reset_state(&data->display, data->chamelium, port,
			      data->ports, data->port_count);

	/*
	 * xe only arms PME for D3hot, and only when D3cold is not allowed. An
	 * integrated device is never D3cold capable, since xe requires a PCIe
	 * root port above the device and an RCiEP has none. A discrete device
	 * can drop to D3cold unless its vram_d3cold_threshold is set to 0, which
	 * this test does not do. Decide once, here, whether the device suspends
	 * into D3hot: in D3cold there is no PME to observe, so skip, and from
	 * here on finding it in any other state is a failure.
	 * igt_pm_pci_get_d_state() reports D3cold without resuming the device.
	 */
	pme_wait_settled();
	d_state = igt_pm_pci_get_d_state(pci_dev);
	igt_require_f(d_state == IGT_ACPI_D3Hot,
		      "Device runtime suspends into %s rather than D3hot, so "
		      "there is no PME to observe\n", pme_d_state_name(d_state));

	/*
	 * Polling must be enabled, so that a previous test cannot leave it
	 * disabled and fake a pass: the test checks that the driver does not
	 * use it when PME is available. Setting the parameter does not restart
	 * a poll worker that a previous test already stopped, which is why the
	 * control run below checks that polling actually happens.
	 */
	igt_kms_poll_set(true);

	/* Don't let our own toggles trip the storm detection into polling. */
	igt_hpd_storm_set_threshold(data->drm_fd, 0);
	pme_hpd.storm_set = true;

	pme_hpd.mon = igt_watch_uevents();

	active_limit_ms = min_t(uint64_t, PME_ACTIVE_TIME_SLACK_MS,
				pme_measure_poll_wake(data, port, pci_dev));

	/*
	 * Now arm PME. Need both: PME is only armed when device_may_wakeup() is
	 * true, and without a wakeup source the PM core never records the PME
	 * wakeup. Enabling power/wakeup creates a new wakeup source, so
	 * wakeup_active_count restarts from 0; every baseline is taken after
	 * this.
	 */
	igt_pm_set_wakeup_enabled(pci_dev, true);
	pme_cycle_runtime_pm(data, port);

	/*
	 * Ports are left disconnected by the reset, so plug first. The idle
	 * window only needs to be watched once.
	 */
	try_hpd_runtime_suspend_pme(data, port, pci_dev, pme_hpd.mon, false,
				    true, active_limit_ms);

	/* Now check we notice a disconnect signalled from D3hot too. */
	try_hpd_runtime_suspend_pme(data, port, pci_dev, pme_hpd.mon, true,
				    false, active_limit_ms);

	pme_hpd_cleanup(data->drm_fd);
}

static const char test_hpd_without_ddc_desc[] =
	"Disable DDC on a VGA connector, check we still get a uevent on hotplug";
static void test_hpd_without_ddc(chamelium_data_t *data,
				 struct chamelium_port *port)
{
	struct udev_monitor *mon = igt_watch_uevents();

	igt_modeset_disable_all_outputs(&data->display);
	chamelium_reset_state(&data->display, data->chamelium, port,
			      data->ports, data->port_count);
	igt_flush_uevents(mon);

	/* Disable the DDC on the connector and make sure we still get a
	 * hotplug
	 */
	chamelium_port_set_ddc_state(data->chamelium, port, false);
	chamelium_plug(data->chamelium, port);

	igt_assert(igt_hotplug_detected(mon, CHAMELIUM_HOTPLUG_TIMEOUT));
	igt_assert_eq(chamelium_reprobe_connector(&data->display,
						  data->chamelium, port),
		      DRM_MODE_CONNECTED);

	igt_cleanup_uevents(mon);
}

static const char test_hpd_storm_detect_desc[] =
	"Trigger a series of hotplugs in a very small timeframe to simulate a"
	"bad cable, check the kernel falls back to polling to avoid a hotplug "
	"storm";
static void test_hpd_storm_detect(chamelium_data_t *data,
				  struct chamelium_port *port, int width)
{
	struct udev_monitor *mon;
	int count = 0;

	igt_require_hpd_storm_ctl(data->drm_fd);
	igt_modeset_disable_all_outputs(&data->display);
	chamelium_reset_state(&data->display, data->chamelium, port,
			      data->ports, data->port_count);

	igt_hpd_storm_set_threshold(data->drm_fd, 1);
	chamelium_fire_hpd_pulses(data->chamelium, port, width, 10);
	igt_assert(igt_hpd_storm_detected(data->drm_fd));

	mon = igt_watch_uevents();
	chamelium_fire_hpd_pulses(data->chamelium, port, width, 10);

	/*
	 * Polling should have been enabled by the HPD storm at this point,
	 * so we should only get at most 1 hotplug event
	 */
	igt_until_timeout(5)
		count += igt_hotplug_detected(mon, 1);
	igt_assert_lt(count, 2);

	igt_cleanup_uevents(mon);
	igt_hpd_storm_reset(data->drm_fd);
}

static const char test_hpd_storm_disable_desc[] =
	"Disable HPD storm detection, trigger a storm and check the kernel "
	"doesn't detect one";
static void test_hpd_storm_disable(chamelium_data_t *data,
				   struct chamelium_port *port, int width)
{
	igt_require_hpd_storm_ctl(data->drm_fd);
	igt_modeset_disable_all_outputs(&data->display);
	chamelium_reset_state(&data->display, data->chamelium, port,
			      data->ports, data->port_count);

	igt_hpd_storm_set_threshold(data->drm_fd, 0);
	chamelium_fire_hpd_pulses(data->chamelium, port, width, 10);
	igt_assert(!igt_hpd_storm_detected(data->drm_fd));

	igt_hpd_storm_reset(data->drm_fd);
}

IGT_TEST_DESCRIPTION("Testing HPD with a Chamelium board");
int igt_main()
{
	chamelium_data_t data;

	igt_fixture() {
		chamelium_init_test(&data);
	}

	igt_describe("DisplayPort tests");

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("dp-hpd", DisplayPort, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_DP_HDMI, TEST_MODESET_OFF);

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("dp-hpd-fast", DisplayPort, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_FAST, TEST_MODESET_OFF);

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("dp-hpd-enable-disable-mode", DisplayPort, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_FAST, TEST_MODESET_ON_OFF);

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("dp-hpd-with-enabled-mode", DisplayPort, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_FAST, TEST_MODESET_ON);

	igt_describe(test_hotplug_for_each_pipe_desc);
	connector_subtest("dp-hpd-for-each-pipe", DisplayPort, &data,
			  test_hotplug_for_each_pipe);

	igt_describe(test_suspend_resume_hpd_desc);
	connector_subtest("dp-hpd-after-suspend", DisplayPort, &data, test_suspend_resume_hpd,
			  SUSPEND_STATE_MEM, SUSPEND_TEST_NONE);

	igt_describe(test_suspend_resume_hpd_desc);
	connector_subtest("dp-hpd-after-hibernate", DisplayPort, &data, test_suspend_resume_hpd,
			  SUSPEND_STATE_DISK, SUSPEND_TEST_DEVICES);

	igt_describe(test_hpd_runtime_suspend_pme_desc);
	connector_subtest("dp-hpd-after-runtime-suspend-d3hot", DisplayPort,
			  &data, test_hpd_runtime_suspend_pme);

	igt_fixture()
		pme_hpd_cleanup(data.drm_fd);

	igt_describe(test_hpd_storm_detect_desc);
	connector_subtest("dp-hpd-storm", DisplayPort, &data, test_hpd_storm_detect,
			  HPD_STORM_PULSE_INTERVAL_DP);

	igt_describe(test_hpd_storm_disable_desc);
	connector_subtest("dp-hpd-storm-disable", DisplayPort, &data, test_hpd_storm_disable,
			  HPD_STORM_PULSE_INTERVAL_DP);

	igt_describe("HDMI tests");

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("hdmi-hpd", HDMIA, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_DP_HDMI, TEST_MODESET_OFF);

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("hdmi-hpd-fast", HDMIA, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_FAST, TEST_MODESET_OFF);

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("hdmi-hpd-enable-disable-mode", HDMIA, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_FAST, TEST_MODESET_ON_OFF);

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("hdmi-hpd-with-enabled-mode", HDMIA, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_FAST, TEST_MODESET_ON);

	igt_describe(test_hotplug_for_each_pipe_desc);
	connector_subtest("hdmi-hpd-for-each-pipe", HDMIA, &data,
			  test_hotplug_for_each_pipe);

	igt_describe(test_suspend_resume_hpd_desc);
	connector_subtest("hdmi-hpd-after-suspend", HDMIA, &data, test_suspend_resume_hpd,
			  SUSPEND_STATE_MEM, SUSPEND_TEST_NONE);

	igt_describe(test_suspend_resume_hpd_desc);
	connector_subtest("hdmi-hpd-after-hibernate", HDMIA, &data, test_suspend_resume_hpd,
			  SUSPEND_STATE_DISK, SUSPEND_TEST_DEVICES);

	igt_describe(test_hpd_runtime_suspend_pme_desc);
	connector_subtest("hdmi-hpd-after-runtime-suspend-d3hot", HDMIA,
			  &data, test_hpd_runtime_suspend_pme);

	igt_fixture()
		pme_hpd_cleanup(data.drm_fd);

	igt_describe(test_hpd_storm_detect_desc);
	connector_subtest("hdmi-hpd-storm", HDMIA, &data, test_hpd_storm_detect,
			  HPD_STORM_PULSE_INTERVAL_HDMI);

	igt_describe(test_hpd_storm_disable_desc);
	connector_subtest("hdmi-hpd-storm-disable", HDMIA, &data, test_hpd_storm_disable,
			  HPD_STORM_PULSE_INTERVAL_HDMI);

	igt_describe("VGA tests");

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("vga-hpd", VGA, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_VGA, TEST_MODESET_OFF);

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("vga-hpd-fast", VGA, &data, test_hotplug,
			  HPD_TOGGLE_COUNT_FAST, TEST_MODESET_OFF);

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("vga-hpd-enable-disable-mode", VGA, &data,
			  test_hotplug, HPD_TOGGLE_COUNT_FAST, TEST_MODESET_ON_OFF);

	igt_describe(test_basic_hotplug_desc);
	connector_subtest("vga-hpd-with-enabled-mode", VGA, &data,
			  test_hotplug, HPD_TOGGLE_COUNT_FAST, TEST_MODESET_ON);

	igt_describe(test_suspend_resume_hpd_desc);
	connector_subtest("vga-hpd-after-suspend", VGA, &data,
			  test_suspend_resume_hpd, SUSPEND_STATE_MEM, SUSPEND_TEST_NONE);

	igt_describe(test_suspend_resume_hpd_desc);
	connector_subtest("vga-hpd-after-hibernate", VGA, &data,
			  test_suspend_resume_hpd, SUSPEND_STATE_DISK, SUSPEND_TEST_DEVICES);

	igt_describe(test_hpd_without_ddc_desc);
	connector_subtest("vga-hpd-without-ddc", VGA, &data, test_hpd_without_ddc);

	igt_describe(test_hotplug_for_each_pipe_desc);
	connector_subtest("vga-hpd-for-each-pipe", VGA, &data, test_hotplug_for_each_pipe);

	igt_describe("Tests that operate on all connectors");
	igt_subtest_group() {
		igt_fixture() {
			igt_require(data.port_count);
		}

		igt_describe(test_suspend_resume_hpd_common_desc);
		igt_subtest("common-hpd-after-suspend")
			test_suspend_resume_hpd_common(&data, SUSPEND_STATE_MEM,
						       SUSPEND_TEST_NONE);

		igt_describe(test_suspend_resume_hpd_common_desc);
		igt_subtest("common-hpd-after-hibernate")
			test_suspend_resume_hpd_common(&data,
						       SUSPEND_STATE_DISK,
						       SUSPEND_TEST_DEVICES);
	}


	igt_fixture() {
		igt_display_fini(&data.display);
		drm_close_driver(data.drm_fd);
	}
}
