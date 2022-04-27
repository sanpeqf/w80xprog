/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#ifndef _W80xHW_H_
#define _W80xHW_H_

#include "macro.h"

#define OPCODE(len, opcode) (((len) << 8)| ((opcode) << 0))
#define OPCODE_DATA(opcode) (((opcode) >> 0) & 0xff)
#define OPCODE_LEN(opcode) (((opcode) >> 8) & 0xff)
#define ETH_ALEN     6
#define ETH_HEX_ALEN 12

enum xmodem_types {
    XMODEM_SOH  = 0x02,
    XMODEM_EOT  = 0x04,
    XMODEM_ACK  = 0x06,
    XMODEM_NAK  = 0x15,
    XMODEM_CAN  = 0x18,
};

enum opcode_types {
    OPCODE_SET_FREQ     = OPCODE(0x0a, 0x31), /* Set UART Speed */
    OPCODE_ERASE_SPINOR = OPCODE(0x0a, 0x32), /* Erase Spinor */
    OPCODE_SET_BT_MAC   = OPCODE(0x0e, 0x33), /* Set Bluetooth MAC */
    OPCODE_GET_BT_MAC   = OPCODE(0x06, 0x34), /* Set Bluetooth MAC  */
    OPCODE_SET_GAIN     = OPCODE(0x5a, 0x35), /* Set Gain param */
    OPCODE_GET_GAIN     = OPCODE(0x06, 0x36), /* Get Gain param */
    OPCODE_SET_NET_MAC  = OPCODE(0x0e, 0x37), /* Set Network MAC */
    OPCODE_GET_NET_MAC  = OPCODE(0x06, 0x38), /* Get Network MAC */
    OPCODE_GET_ERROR    = OPCODE(0x06, 0x3b), /* Get last Error code */
    OPCODE_GET_SPINOR   = OPCODE(0x06, 0x3c), /* Get Spinor Version */
    OPCODE_GET_VERSION  = OPCODE(0x06, 0x3e), /* Get Rom Version */
    OPCODE_REBOOT       = OPCODE(0x06, 0x3f), /* System Reboot */
};

enum return_types {
    RETURN_NOMAL        = 'C',  /* Operation complete */

    /* during upgrade (xmodem) */
    RETURN_CANCEL       = 'D',  /* Host cancel */
    RETURN_ETIMEOUT     = 'F',  /* Timeout no data received */
    RETURN_EINDEX       = 'G',  /* Wrong package serial number */
    RETURN_ESIZE        = 'I',  /* Image too large */
    RETURN_EADDR        = 'J',  /* Illegal image flash address */
    RETURN_EALIGN       = 'K',  /* The image burning address page is not aligned */
    RETURN_EHCRC        = 'L',  /* Image header check error */
    RETURN_EDCRC        = 'M',  /* Image content verification error */
    RETURN_EDATA        = 'P',  /* The image content is incomplete or the signature is missing */

    /* during startup */
    RETURN_EFLASHID     = 'N',  /* Flash ID self test failed */
    RETURN_EFIRMWARE    = 'Q',  /* Firmware type error */
    RETURN_ESECHEAD     = 'L',  /* Secboot header check error */
    RETURN_ESECCHECK    = 'M',  /* Secboot check error */
    RETURN_EDECRYPT     = 'Y',  /* Failed to decrypt and read secboot */
    RETURN_ESIGN        = 'Z',  /* Signature verification failed */

    /* functional module */
    RETURN_ECRC         = 'R',  /* Command check error */
    RETURN_EINVAL       = 'S',  /* Command parameter error */
    RETURN_EGETPARM     = 'T',  /* Failed to get ft parameters (MAC, gain, etc.) */
    RETURN_ESETGAIN     = 'U',  /* Set gain failed */
    RETURN_ESETMAC      = 'V',  /* Failed to set mac */
};

struct opcode_head {
    uint8_t sign;
    uint8_t length;
    uint8_t reserved;
} __packed;

struct opcode_content {
    uint16_t crc16;
    uint8_t opcode;
    uint8_t param[0];
} __packed;

struct opcode_transfer {
    struct opcode_head head;
    struct opcode_content content;
} __packed;

struct serial_speed {
    uint8_t reserved[3];
    uint32_t speed;
} __packed;

struct mac_flash {
    uint8_t reserved[3];
    uint8_t index[8];
} __packed;

struct gain_flash {
    uint8_t reserved[3];
    uint8_t index[84];
} __packed;

struct spinor_erase {
    uint8_t reserved[3];
    uint16_t index;
    uint16_t count;
} __packed;

#endif  /* _W80xHW_H_ */
