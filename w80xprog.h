/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#ifndef _W80xPROG_H_
#define _W80xPROG_H_

#include "w80xhw.h"
#include <stdbool.h>

/* crc16.c */
uint16_t crc16_xmodem(const uint8_t *src, unsigned int len);
uint16_t crc16_ccitt(const uint8_t *src, unsigned int len);

/* termios.c */
extern int termios_open(char *path);
extern int termios_set_speed(unsigned int speed);
extern int termios_flush(void);
extern int termios_setup(unsigned int speed, int databits, int stopbits, char parity);
extern int termios_rts(bool enable);
extern int termios_read(void *data, unsigned long len);
extern int termios_write(const void *data, unsigned long len);
extern int termios_print(const char *str);

/* w80xprog.c */
extern int entry_secboot(void);
extern int serial_speed(unsigned int speed);
extern int flash_bmac(const char *bmac);
extern int flash_wmac(const char *wmac);
extern int flash_gain(const char *bmac);
extern int spinor_flash(uint8_t *src, unsigned long len);
extern int spinor_erase(unsigned long size);
extern int chip_info(void);
extern int chip_reset(void);

#endif  /* _W80xPROG_H_ */
