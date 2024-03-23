/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2024 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _PROGGRESS_H_
#define _PROGGRESS_H_

#include <stdint.h>
#include <stddef.h>
#include <bfdev.h>

struct progress {
	uint64_t total;
	uint64_t done;
	double start;
};

extern void
progress_init(struct progress *prog, size_t total);

extern void
progress_update(struct progress *prog, size_t bytes);

#endif /* _PROGGRESS_H_ */
