/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include "w80xprog.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

static char *return_errno[] = {
    [RETURN_NOMAL       - RETURN_NOMAL] = "Operation complete.",
    [RETURN_CANCEL      - RETURN_NOMAL] = "Host cancel.",
    [RETURN_ETIMEOUT    - RETURN_NOMAL] = "Timeout no data received.",
    [RETURN_EINDEX      - RETURN_NOMAL] = "Wrong package serial number.",
    [RETURN_ESIZE       - RETURN_NOMAL] = "Image too large.",
    [RETURN_EADDR       - RETURN_NOMAL] = "Illegal image flash address.",
    [RETURN_EALIGN      - RETURN_NOMAL] = "The image burning address page is not aligned.",
    [RETURN_EHCRC       - RETURN_NOMAL] = "Image header check error.",
    [RETURN_EDCRC       - RETURN_NOMAL] = "Image content verification error.",
    [RETURN_EDATA       - RETURN_NOMAL] = "The image content is incomplete or the signature is missing.",
    [RETURN_ECRC        - RETURN_NOMAL] = "Command check error.",
    [RETURN_EINVAL      - RETURN_NOMAL] = "Command parameter error.",
    [RETURN_EGETPARM    - RETURN_NOMAL] = "Failed to get ft parameters (MAC, gain, etc.).",
    [RETURN_ESETGAIN    - RETURN_NOMAL] = "Set gain failed.",
    [RETURN_ESETMAC     - RETURN_NOMAL] = "Failed to set mac.",
};
static inline void format_haddr(void *src) {
    char *str;
    char buff[
        ETH_HEX_ALEN + 
        5  +            /*  5 times':'  */
        1               /*  '\0'    */
    ];
    memset(buff, 0, sizeof(buff));
    //remove 'Mac:' and '\r\n'
    memcpy(buff, src + 4, 12);


    for (str = buff; *str; str++) {
        *str = tolower(*str);
    }

    //add ':'
    str = src;
    for (int i = 0 ; i < 6; i++) {
        memcpy(str, buff + i * 2, 2);
        str += 2;
        if (i != 5) {
            memcpy(str, ":", 1);
            str += 1;
        }
    }
    memset(str , 0 , 1);
}
static inline void replace_wrap(void *src)
{
    char *str;

    for (str = src; *str; str++) {
        if (*str == '\n' ||
            *str == '\r' ||
            *str == ':')
            *str  = ' ';
    }
}

static inline int atoh(const char *src, uint8_t *mac, unsigned int len)
{
    unsigned int count;
    uint8_t tmp;

    memset(mac, 0, len);

    for (count = 0; count < (len * 2); ++count) {
        if (*src == ':')
            src++;

        if (*src >= '0' && *src <= '9')
            tmp = *src++ - '0';
        else if (tolower(*src) >= 'a' && tolower(*src) <= 'f')
            tmp = tolower(*src++) - 'a' + 10;
        else
            return -EINVAL;

        if (count & 0x01)
            *mac++ |= tmp;
        else
            *mac |= tmp << 4;
    }

    return 0;
}

static int wait_busy(void)
{
    unsigned int count;
    uint8_t val;
    int ret;

    for (count = 0; count < 10000; ++count) {
        if ((ret = termios_read(&val, 1)) < 0)
            return ret;
        if (val == RETURN_NOMAL)
            return 0;
    }

    return -EBUSY;
}

static int opcode_transfer(enum opcode_types opcode, void *param, void *buff, unsigned int len)
{
    struct opcode_transfer *transfer;
    unsigned int transfer_len;
    int ret;

    transfer_len = sizeof(struct opcode_head) + OPCODE_LEN(opcode);
    transfer = malloc(transfer_len);
    if (!transfer)
        return -ENOMEM;

    termios_flush();
    if ((ret = wait_busy()))
        return ret;

    transfer->head.sign = 0x21;
    transfer->head.length = OPCODE_LEN(opcode);
    transfer->head.reserved = 0x00;
    transfer->content.opcode = OPCODE_DATA(opcode);

    if (param)
        memcpy(transfer->content.param, param, OPCODE_LEN(opcode) - 3);
    else
        memset(transfer->content.param, 0, OPCODE_LEN(opcode) - 3);

    transfer->content.crc16 = crc16_ccitt(&transfer->content.opcode, OPCODE_LEN(opcode) - 2);

    ret = termios_write(transfer, transfer_len);
    if (ret < 0)
        goto error;

    if (buff)
        ret = termios_read(buff, len);

error:
    free(transfer);
    return ret;
}

int spinor_flash(uint8_t *src, unsigned long len)
{
    uint8_t *buffer = (uint8_t [1024]){};
    unsigned long xfer, retry;
    uint8_t count, val;
    int ret;

    printf("Chip Flash:\n");
    wait_busy();

    for (count = 1; (xfer = min(len, 1024)); len -= xfer, src += xfer, ++count) {
        uint16_t crc;
        retry = 20;

        memcpy(buffer, src, xfer);
        if (xfer < 1024)
            memset(buffer + xfer, 0x1a, 1024 - xfer);
        crc = crc16_xmodem(buffer, 1024);

retry:
        if (unlikely(!retry--)) {
            printf("\tAbort Transfer after twenty retries\n");
            goto retry;
        }

        if (((ret = termios_write(&(uint8_t){XMODEM_SOH}, 1)) < 0)
          ||((ret = termios_write(&count, 1)) < 0)
          ||((ret = termios_write(&(uint8_t){0xff - count}, 1)) < 0)
          ||((ret = termios_write(buffer, 1024)) < 0)
          ||((ret = termios_write((uint8_t [2]){crc >> 8, crc}, 2)) < 0))
            return ret;

        if ((ret = termios_read(&val, 1)) < 0)
            return ret;

        if (likely(val == XMODEM_ACK))
            continue;

        else if (val == XMODEM_NAK) {
            printf("\tTransfer Retry\n");
            goto retry;
        }

        printf("\tUnknow Retval 0x%x\n", val);
        goto error;
    }

    if ((ret = termios_write(&(uint8_t){XMODEM_EOT}, 1)) < 0)
        return ret;

    if ((ret = termios_read(&val, 1)) < 0)
        return ret;

    printf("\tFlash Done.\n");
    return val != XMODEM_ACK ? -ECOMM : 0;

error:
    termios_write(&(uint8_t){XMODEM_CAN}, 1);
    return ret;
}

int spinor_erase(unsigned long size)
{
    struct spinor_erase param = {
        .count = size / 4096,
        .index = 0x0002,
    };

    uint8_t state;
    int ret;

    printf("Chip Erase:\n");

    ret = opcode_transfer(OPCODE_ERASE_SPINOR, &param, &state, 1);
    if (ret < 0)
        return ret;

    printf("\t%s\n", return_errno[state - RETURN_NOMAL]);
    return state - RETURN_NOMAL;
}

int serial_speed(unsigned int speed)
{
    struct serial_speed param = {
        .speed = speed,
    };

    uint8_t state;
    int ret;

    printf("Serial Speed set to %d.\n", speed);

    ret = opcode_transfer(OPCODE_SET_FREQ, &param, &state, 1);
    if (ret < 0)
        return ret;

    if (state != 0x06)
        return -EBUSY;

    return 0;
}

int flash_bmac(const char *mac)
{
    struct mac_flash param = { };
    uint8_t state;
    int ret;

    printf("Flash BT MAC:\n");
    if (atoh(mac, &param.index[0], 6)) {
        printf("\tincorrect format\n");
        return -EINVAL;
    }

    ret = opcode_transfer(OPCODE_SET_BT_MAC, &param, &state, 1);
    if (ret < 0)
        return ret;

    printf("\t%s\n", return_errno[state - RETURN_NOMAL]);
    return state - RETURN_NOMAL;
}

int flash_wmac(const char *mac)
{
    struct mac_flash param = { };
    uint8_t state;
    int ret;

    printf("Flash WIFI MAC:\n");
    if (atoh(mac, &param.index[0], 6)) {
        printf("\tincorrect format\n");
        return -EINVAL;
    }

    ret = opcode_transfer(OPCODE_SET_NET_MAC, &param, &state, 1);
    if (ret < 0)
        return ret;

    printf("\t%s\n", return_errno[state - RETURN_NOMAL]);
    return state - RETURN_NOMAL;
}

int flash_gain(const char *mac)
{
    struct gain_flash param = { };
    uint8_t state;
    int ret;

    printf("Flash RF GAIN:\n");
    if (atoh(mac, &param.index[0], 84)) {
        printf("\tincorrect format\n");
        return -EINVAL;
    }

    ret = opcode_transfer(OPCODE_SET_GAIN, &param, &state, 1);
    if (ret < 0)
        return ret;

    printf("\t%s\n", return_errno[state - RETURN_NOMAL]);
    return state - RETURN_NOMAL;
}

int chip_info(void)
{
    uint8_t buff[256];
    int ret;

    printf("Chip info:\n");

    printf("\tBT");
    ret = opcode_transfer(OPCODE_GET_BT_MAC, NULL, buff, sizeof(buff) -1);
    if (ret < 0)
        return ret;
    else if (ret == 1)
        printf(":\t%s\n", return_errno[buff[0] - RETURN_NOMAL]);
    else {
        format_haddr(buff);
        printf(":\t%s\n", buff);
    }

    memset(buff, 0, sizeof(buff));

    printf("\tWifi");
    ret = opcode_transfer(OPCODE_GET_NET_MAC, NULL, buff, sizeof(buff) -1);
    if (ret < 0)
        return ret;
    else if (ret == 1)
        printf(":\t%s\n", return_errno[buff[0] - RETURN_NOMAL]);
    else {
        format_haddr(buff);
        printf(":\t%s\n", buff);
    }

    memset(buff, 0, sizeof(buff));

    printf("\tFlash");
    ret = opcode_transfer(OPCODE_GET_SPINOR, NULL, buff, sizeof(buff) -1);
    if (ret < 0)
        return ret;
    replace_wrap(buff);
    printf(":\t%s\n", buff);

    memset(buff, 0, sizeof(buff));

    printf("\tROM");
    ret = opcode_transfer(OPCODE_GET_VERSION, NULL, buff, sizeof(buff) -1);
    if (ret < 0)
        return ret;
    replace_wrap(buff);
    printf(":\t%s\n", buff);

    memset(buff, 0, sizeof(buff));

    printf("\tRF");
    ret = opcode_transfer(OPCODE_GET_GAIN, NULL, buff, sizeof(buff) -1);
    if (ret < 0)
        return ret;
    replace_wrap(buff);
    printf(":\t%s\n", buff);

    return 0;
}

int chip_reset(void)
{
    int ret;

    printf("Chip reset...\n");
    ret = opcode_transfer(OPCODE_REBOOT, NULL, NULL, 0);
    return ret < 0 ? ret : 0;
}

int entry_secboot(void)
{
    uint8_t buff[256];
    unsigned int count;
    int ret;

    termios_rts(1);
    usleep(5000);
    termios_print("AT+Z\r\n");
    termios_rts(0);

    for (count = 0; count < 50; ++count) {
        termios_write(&(uint8_t){0x1b}, 1);
        termios_write(&(uint8_t){0x1b}, 1);
        termios_write(&(uint8_t){0x1b}, 1);
        usleep(2000);
    }

    if ((ret = termios_read(buff, sizeof(buff) -1)) < 0)
        return ret;

    return 0;
}
