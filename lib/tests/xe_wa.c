// SPDX-License-Identifier: MIT
/*
 * Copyright(c) 2026 Intel Corporation. All rights reserved.
 */

#include "igt_core.h"

#include "tests/xe_wa.h"

static char fake_dump[] =
	"Some workarounds\n"
	"	wa_a\n"
	"	wa_b\n"
	"Another set of workarounds\n"
	"	wa_c\n"
	"	wa_d\n"
	"Yet another, but empty section\n"
	"A final section\n"
	"	wa_x, wa_y, wa_z\n"
	"	wa_abcd\n";

int igt_simple_main()
{
	/* Match workaround that is the single occurrence in the line. */
	igt_assert_eq(xe_wa_test_debugfs_dump_has_wa(fake_dump, "wa_c"), 1);

	/* Match workarounds that are listed with others in the same line. */
	igt_assert_eq(xe_wa_test_debugfs_dump_has_wa(fake_dump, "wa_x"), 1);
	igt_assert_eq(xe_wa_test_debugfs_dump_has_wa(fake_dump, "wa_y"), 1);
	igt_assert_eq(xe_wa_test_debugfs_dump_has_wa(fake_dump, "wa_z"), 1);

	/* Do not match non-existing workarounds. */
	igt_assert_eq(xe_wa_test_debugfs_dump_has_wa(fake_dump, "non-existing"), 0);

	/* Match full workaround name and not substrings. */
	igt_assert_eq(xe_wa_test_debugfs_dump_has_wa(fake_dump, "wa_abc"), 0);
	igt_assert_eq(xe_wa_test_debugfs_dump_has_wa(fake_dump, "wa_abcd"), 1);

	/* Don't match section titles. */
	igt_assert_eq(xe_wa_test_debugfs_dump_has_wa(fake_dump, "Some workarounds"), 0);
	igt_assert_eq(xe_wa_test_debugfs_dump_has_wa(fake_dump, "Another set of workarounds"), 0);
}
