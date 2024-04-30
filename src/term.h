/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2024 Sanpe <sanpeqf@gmail.com>
 */

#ifndef _TERM_H_
#define _TERM_H_

#include <config.h>
#include <errno.h>
#include <bfdev.h>

extern int
term_setspeed(unsigned int speed);

extern int
term_setup(unsigned int speed, int databits, int stopbits, char parity);

extern int
term_reset(bool enable);

extern int
term_read(void *data, size_t len);

extern int
term_write(const void *data, size_t len);

extern int
term_print(const char *str);

extern int
term_flush(void);

extern int
term_open(const char *path);

extern void
term_close(void);

#endif /* _TERM_H_ */
