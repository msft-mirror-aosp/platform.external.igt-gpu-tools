/* SPDX-License-Identifier: MIT */
/* Copyright © 2024 Intel Corporation */

/* Header used during pre-process phase of iga64 assembly.
 * WARNING: changing this file causes rebuild of all shaders.
 * Do not touch without current version of iga64 compiler.
 */

#ifndef IGA64_MACROS_H
#define IGA64_MACROS_H

/* send instruction for DG2+ requires 0 length in case src1 is null, BSpec: 47443 */
#if GEN_VER <= 1250
#define src1_null null
#else
#define src1_null null:0
#endif

#endif
