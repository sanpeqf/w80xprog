/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _W80XPROG_H_
#define _W80XPROG_H_

#include <config.h>
#include <errno.h>
#include <bfdev.h>

#define ETH_ALEN 6
#define ETH_HEX_ALEN 12

extern int
flash_gain(const char *bmac);

extern int
spinor_flash(uint8_t *src, size_t size);

extern int
spinor_erase(uint16_t index, uint16_t size);

extern int
serial_speed(uint32_t speed);

extern int
flash_bmac(const char *bmac);

extern int
flash_wmac(const char *wmac);

extern int
chip_info(void);

extern int
chip_reset(void);

extern int
entry_secboot(void);

#endif /* _W80XPROG_H_ */
