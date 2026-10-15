/* SPDX-License-Identifier: MIT
 *
 * Originally developed by Jatin Kataria at Netflix.
 * Copyright (c) 2026 Netflix, Inc.
 *
 * Adapted for IGT by Advanced Micro Devices, Inc., 2026.
 */

#include "igt.h"
#include "lib/amdgpu/amd_mes_reset.h"
#include "lib/amdgpu/amd_ip_blocks.h"
#include <signal.h>
#include <amdgpu_drm.h>
#include <pthread.h>
#include <time.h>

#define MES_CHAIN_WARMUP_MS 2000
#define MES_WRITEDATA_WARMUP_MS 500
#define MES_SELF_CHAIN_WARMUP_MS 500
#define MES_WAIT_CHAIN_DELAY_MS 1000
#define MES_SIGNAL_RELEASE_DELAY_MS 100
#define MES_WRITEDATA_TEARDOWN_DELAY_MS 2000
#define MES_MULTITENANT_TEARDOWN_DELAY_MS 500

static volatile sig_atomic_t mes_reset_stop;
static struct amdgpu_cmd_base *mes_reset_cmd;

static void mes_reset_stop_child(int signal_number)
{
	(void)signal_number;
	mes_reset_stop = 1;
}

/* All test functions defined before igt_main() */

static void test_mes_ib_chain_teardown(amdgpu_device_handle device)
{
	uint32_t chain_len = MES_DEFAULT_CHAIN_LEN;
	uint32_t ib_size = MES_DEFAULT_IB_SIZE;
	struct mes_ib_node *nodes;
	amdgpu_context_handle context;
	amdgpu_bo_handle *bo_handles;
	uint32_t ib_dwords;
	int i, r;

	igt_info("Chain: %u x %uKiB IBs, warmup %dms\n",
		 chain_len, ib_size / 1024, MES_CHAIN_WARMUP_MS);

	r = amdgpu_cs_ctx_create(device, &context);
	igt_assert_eq(r, 0);

	nodes = calloc(chain_len, sizeof(*nodes));
	bo_handles = calloc(chain_len, sizeof(*bo_handles));
	igt_assert(nodes && bo_handles);

	ib_dwords = ib_size / 4;

	for (i = 0; i < (int)chain_len; i++) {
		uint64_t va = MES_VA_BASE + (uint64_t)i * (ib_size + MES_VA_GAP);

		r = mes_create_ib_bo(device, ib_size, va, &nodes[i]);
		igt_assert_eq(r, 0);
		bo_handles[i] = nodes[i].bo_handle;
	}

	for (i = 0; i < (int)chain_len - 1; i++)
		mes_reset_cmd->ib_fill_chain(mes_reset_cmd, nodes[i].cpu_ptr, ib_dwords, nodes[i + 1].va, ib_dwords);
	mes_reset_cmd->ib_fill_nop(mes_reset_cmd, nodes[chain_len - 1].cpu_ptr, ib_dwords);

	igt_info("Chain built: IB[0]=0x%lx\n", (unsigned long)nodes[0].va);

	igt_fork(child, 1) {
		mes_reset_stop = 0;
		signal(SIGTERM, mes_reset_stop_child);
		while (!mes_reset_stop) {
			mes_submit_ib(device, context, bo_handles, chain_len,
				      nodes[0].va, ib_size, MES_HW_IP_GFX);
		}
	}

	usleep(MES_CHAIN_WARMUP_MS * 1000);
	igt_kill_children(SIGTERM);
	igt_waitchildren();

	for (i = 0; i < (int)chain_len; i++)
		mes_free_bo(&nodes[i]);

	amdgpu_cs_ctx_free(context);
	free(nodes);
	free(bo_handles);
}

static void test_mes_writedata_teardown(amdgpu_device_handle device)
{
	int num_children = 4;
	uint32_t ib_size = 1024 * 1024;
	struct mes_ib_node ib_node, dst_node;
	amdgpu_context_handle contexts[4];
	amdgpu_bo_handle bo_handles[2];
	int i, r;

	r = mes_create_ib_bo(device, ib_size, MES_VA_BASE, &ib_node);
	igt_assert_eq(r, 0);

	r = mes_create_dst_bo(device, MES_DST_BO_SIZE, MES_VA_BASE + 0x800000,
			      AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_WRITEABLE,
			      &dst_node);
	igt_assert_eq(r, 0);

	mes_reset_cmd->ib_fill_writedata(mes_reset_cmd, ib_node.cpu_ptr, ib_size / 4, dst_node.va, MES_DST_BO_SIZE);

	bo_handles[0] = ib_node.bo_handle;
	bo_handles[1] = dst_node.bo_handle;

	for (i = 0; i < 4; i++) {
		r = amdgpu_cs_ctx_create(device, &contexts[i]);
		igt_assert_eq(r, 0);
	}

	igt_fork(child, num_children) {
		int c = 0;

		mes_reset_stop = 0;
		signal(SIGTERM, mes_reset_stop_child);
		while (!mes_reset_stop) {
			mes_submit_ib(device, contexts[c % 4], bo_handles, 2,
				      ib_node.va, ib_size, MES_HW_IP_GFX);
			c++;
		}
	}

	usleep(MES_WRITEDATA_WARMUP_MS * 1000);
	igt_kill_children(SIGTERM);
	igt_waitchildren();

	for (i = 0; i < 4; i++)
		amdgpu_cs_ctx_free(contexts[i]);

	mes_free_bo(&ib_node);
	mes_free_bo(&dst_node);
}

static void test_gfx_chain_close_fault(amdgpu_device_handle device)
{
	uint32_t ib_size = 64 * 1024;
	struct mes_ib_node ib_node;
	amdgpu_context_handle context;
	amdgpu_bo_handle bo_handles[1];
	int r;

	r = amdgpu_cs_ctx_create(device, &context);
	igt_assert_eq(r, 0);

	r = mes_create_ib_bo(device, ib_size, MES_VA_BASE, &ib_node);
	igt_assert_eq(r, 0);

	mes_reset_cmd->ib_fill_chain(mes_reset_cmd, ib_node.cpu_ptr, ib_size / 4, ib_node.va, ib_size / 4);
	bo_handles[0] = ib_node.bo_handle;

	igt_fork(child, 1) {
		mes_reset_stop = 0;
		signal(SIGTERM, mes_reset_stop_child);
		while (!mes_reset_stop) {
			mes_submit_ib(device, context, bo_handles, 1,
				      ib_node.va, ib_size, MES_HW_IP_GFX);
		}
	}

	usleep(MES_SELF_CHAIN_WARMUP_MS * 1000);
	igt_kill_children(SIGTERM);
	igt_waitchildren();

	mes_free_bo(&ib_node);
	amdgpu_cs_ctx_free(context);
}

static void test_gfx_wait_chain_fault(amdgpu_device_handle device)
{
	struct mes_ib_node ib0, ib1, signal;
	amdgpu_context_handle context;
	amdgpu_bo_handle bo_handles[3];
	int r;

	r = amdgpu_cs_ctx_create(device, &context);
	igt_assert_eq(r, 0);

	r = mes_create_ib_bo(device, 64 * 1024, MES_VA_BASE, &ib0);
	igt_assert_eq(r, 0);
	r = mes_create_ib_bo(device, 64 * 1024, MES_VA_BASE + 0x30000, &ib1);
	igt_assert_eq(r, 0);
	r = mes_create_dst_bo(device, 4096, MES_VA_BASE + 0x10000000,
			      AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_WRITEABLE, &signal);
	igt_assert_eq(r, 0);

	*signal.cpu_ptr = 0;
	mes_reset_cmd->ib_fill_wait(mes_reset_cmd, ib0.cpu_ptr, ib0.size_bytes / 4,
			signal.va, ib1.va, ib1.size_bytes / 4);
	mes_reset_cmd->ib_fill_nop(mes_reset_cmd, ib1.cpu_ptr, 16384);

	bo_handles[0] = ib0.bo_handle;
	bo_handles[1] = ib1.bo_handle;
	bo_handles[2] = signal.bo_handle;

	mes_submit_ib(device, context, bo_handles, 3, ib0.va, 64 * 1024, MES_HW_IP_GFX);
	r = mes_unmap_bo_va(&ib1);
	igt_assert_eq(r, 0);

	usleep(MES_WAIT_CHAIN_DELAY_MS * 1000);
	*signal.cpu_ptr = 1;
	usleep(MES_SIGNAL_RELEASE_DELAY_MS * 1000);

	mes_free_bo(&ib0);
	mes_free_bo(&ib1);
	mes_free_bo(&signal);
	amdgpu_cs_ctx_free(context);
}

static void test_gfx_waitmem_timeout(amdgpu_device_handle device)
{
	struct mes_ib_node ib_node, signal;
	amdgpu_context_handle context;
	amdgpu_bo_handle bo_handles[2];
	int r;

	r = amdgpu_cs_ctx_create(device, &context);
	igt_assert_eq(r, 0);

	r = mes_create_ib_bo(device, 64 * 1024, MES_VA_BASE, &ib_node);
	igt_assert_eq(r, 0);
	r = mes_create_dst_bo(device, 4096, MES_VA_BASE + 0x10000000,
			      AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_WRITEABLE, &signal);
	igt_assert_eq(r, 0);

	*signal.cpu_ptr = 0;
	mes_reset_cmd->ib_fill_wait(mes_reset_cmd, ib_node.cpu_ptr, ib_node.size_bytes / 4,
			signal.va, 0, 0);

	bo_handles[0] = ib_node.bo_handle;
	bo_handles[1] = signal.bo_handle;

	igt_fork(child, 1) {
		mes_submit_ib(device, context, bo_handles, 2,
			      ib_node.va, 64 * 1024, MES_HW_IP_GFX);
	}

	usleep(MES_SIGNAL_RELEASE_DELAY_MS * 1000);
	*signal.cpu_ptr = 1;
	igt_waitchildren();

	mes_free_bo(&ib_node);
	mes_free_bo(&signal);
	amdgpu_cs_ctx_free(context);
}

static void test_gfx_writedata_chain_close_fault(amdgpu_device_handle device)
{
	uint32_t ib_size = 2 * 1024 * 1024;
	struct mes_ib_node ib_nodes[2], dst_node;
	amdgpu_context_handle context;
	amdgpu_bo_handle bo_handles[3];
	int r;

	r = amdgpu_cs_ctx_create(device, &context);
	igt_assert_eq(r, 0);

	r = mes_create_ib_bo(device, ib_size, MES_VA_BASE, &ib_nodes[0]);
	igt_assert_eq(r, 0);
	r = mes_create_ib_bo(device, ib_size, MES_VA_BASE + 0x400000, &ib_nodes[1]);
	igt_assert_eq(r, 0);
	r = mes_create_dst_bo(device, MES_DST_BO_SIZE, MES_VA_BASE + 0x1000000,
			      AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_WRITEABLE, &dst_node);
	igt_assert_eq(r, 0);

	mes_reset_cmd->ib_fill_writedata_chain(mes_reset_cmd, ib_nodes[0].cpu_ptr,
					      ib_nodes[0].size_bytes / 4,
					      dst_node.va + ib_size, MES_DST_BO_SIZE - ib_size,
					      ib_nodes[1].va, ib_nodes[1].size_bytes / 4);
	mes_reset_cmd->ib_fill_writedata_chain(mes_reset_cmd, ib_nodes[1].cpu_ptr,
					      ib_nodes[1].size_bytes / 4,
					      dst_node.va, ib_size,
					      ib_nodes[0].va, ib_nodes[0].size_bytes / 4);

	bo_handles[0] = ib_nodes[0].bo_handle;
	bo_handles[1] = ib_nodes[1].bo_handle;
	bo_handles[2] = dst_node.bo_handle;

	igt_fork(child, 1) {
		mes_submit_ib(device, context, bo_handles, 3,
			      ib_nodes[0].va, ib_nodes[0].size_bytes, MES_HW_IP_GFX);
	}

	igt_waitchildren();
	usleep(MES_WRITEDATA_TEARDOWN_DELAY_MS * 1000);

	mes_free_bo(&ib_nodes[0]);
	mes_free_bo(&ib_nodes[1]);
	mes_free_bo(&dst_node);
	amdgpu_cs_ctx_free(context);
}

static void test_gsl_frame_teardown(amdgpu_device_handle device)
{
	int num_tenants = 2;
	struct mes_ib_node gfx_ib, comp_ib, dst;
	amdgpu_context_handle gfx_ctx, comp_ctx;
	amdgpu_bo_handle bo_handles[3];
	int r;

	r = amdgpu_cs_ctx_create(device, &gfx_ctx);
	igt_assert_eq(r, 0);
	r = amdgpu_cs_ctx_create(device, &comp_ctx);
	igt_assert_eq(r, 0);

	r = mes_create_ib_bo(device, 64 * 1024, MES_VA_BASE, &gfx_ib);
	igt_assert_eq(r, 0);
	r = mes_create_ib_bo(device, 64 * 1024, MES_VA_BASE + 0x100000, &comp_ib);
	igt_assert_eq(r, 0);
	r = mes_create_dst_bo(device, MES_DST_BO_SIZE, MES_VA_BASE + 0x800000,
			      AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_WRITEABLE, &dst);
	igt_assert_eq(r, 0);

	mes_reset_cmd->ib_fill_writedata(mes_reset_cmd, gfx_ib.cpu_ptr, 16384, dst.va, MES_DST_BO_SIZE);
	mes_reset_cmd->ib_fill_writedata(mes_reset_cmd, comp_ib.cpu_ptr, 16384, dst.va, MES_DST_BO_SIZE);

	bo_handles[0] = gfx_ib.bo_handle;
	bo_handles[1] = comp_ib.bo_handle;
	bo_handles[2] = dst.bo_handle;

	igt_fork(tenant, num_tenants) {
		int i;

		for (i = 0; i < 50; i++) {
			mes_submit_ib(device, gfx_ctx, bo_handles, 3,
				      gfx_ib.va, 64 * 1024, MES_HW_IP_GFX);
			mes_submit_ib(device, comp_ctx, bo_handles, 3,
				      comp_ib.va, 64 * 1024, MES_HW_IP_COMPUTE);
		}
	}

	igt_waitchildren();
	usleep(MES_MULTITENANT_TEARDOWN_DELAY_MS * 1000);

	mes_free_bo(&gfx_ib);
	mes_free_bo(&comp_ib);
	mes_free_bo(&dst);
	amdgpu_cs_ctx_free(gfx_ctx);
	amdgpu_cs_ctx_free(comp_ctx);
}

static void test_multitenant_writedata_close(amdgpu_device_handle device)
{
	int num_tenants = 2;
	struct mes_ib_node ib_nodes[2], dst_nodes[2];
	amdgpu_context_handle contexts[2];
	amdgpu_bo_handle bo_handles[2][2];
	int i, r;

	for (i = 0; i < num_tenants; i++) {
		uint64_t ib_va = MES_VA_BASE + i * 0x10000000ULL;

		r = amdgpu_cs_ctx_create(device, &contexts[i]);
		igt_assert_eq(r, 0);

		r = mes_create_ib_bo(device, 1024 * 1024, ib_va, &ib_nodes[i]);
		igt_assert_eq(r, 0);

		r = mes_create_dst_bo(device, MES_DST_BO_SIZE, ib_va + 0x800000,
				      AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_WRITEABLE,
				      &dst_nodes[i]);
		igt_assert_eq(r, 0);

		mes_reset_cmd->ib_fill_writedata(mes_reset_cmd, ib_nodes[i].cpu_ptr, 256 * 1024,
				      dst_nodes[i].va, MES_DST_BO_SIZE);

		bo_handles[i][0] = ib_nodes[i].bo_handle;
		bo_handles[i][1] = dst_nodes[i].bo_handle;
	}

	igt_fork(tenant, num_tenants) {
		int j;

		for (j = 0; j < 100; j++) {
			mes_submit_ib(device, contexts[tenant], bo_handles[tenant], 2,
				      ib_nodes[tenant].va, 1024 * 1024, MES_HW_IP_GFX);
		}
	}

	igt_waitchildren();
	usleep(MES_MULTITENANT_TEARDOWN_DELAY_MS * 1000);

	for (i = 0; i < num_tenants; i++) {
		mes_free_bo(&ib_nodes[i]);
		mes_free_bo(&dst_nodes[i]);
		amdgpu_cs_ctx_free(contexts[i]);
	}
}

int igt_main()
{
	amdgpu_device_handle device;
	int fd = -1;

	igt_fixture() {
		uint32_t major, minor;
		int err;

		mes_reset_cmd = get_cmd_base();
		igt_assert(mes_reset_cmd);
		fd = drm_open_driver(DRIVER_AMDGPU);
		err = amdgpu_device_initialize(fd, &major, &minor, &device);
		igt_require(err == 0);
	}

	igt_describe("IB chain teardown race");
	igt_subtest("mes-ib-chain-teardown")
		test_mes_ib_chain_teardown(device);

	igt_describe("Multi-context WRITE_DATA teardown");
	igt_subtest("mes-writedata-teardown")
		test_mes_writedata_teardown(device);

	igt_describe("Self-looping IB teardown");
	igt_subtest("gfx-chain-close-fault")
		test_gfx_chain_close_fault(device);

	igt_describe("WAIT_REG_MEM + unmap fault");
	igt_subtest("gfx-wait-chain-fault")
		test_gfx_wait_chain_fault(device);

	igt_describe("WAIT_REG_MEM timeout");
	igt_subtest("gfx-waitmem-timeout")
		test_gfx_waitmem_timeout(device);

	igt_describe("WRITE_DATA chain close");
	igt_subtest("gfx-writedata-chain-close-fault")
		test_gfx_writedata_chain_close_fault(device);

	igt_describe("Multi-tenant GFX+compute");
	igt_subtest("gsl-frame-teardown")
		test_gsl_frame_teardown(device);

	igt_describe("Multi-tenant WRITE_DATA");
	igt_subtest("multitenant-writedata-close")
		test_multitenant_writedata_close(device);


	igt_fixture() {
		free_cmd_base(mes_reset_cmd);
		mes_reset_cmd = NULL;
		amdgpu_device_deinitialize(device);
		drm_close_driver(fd);
	}
}
