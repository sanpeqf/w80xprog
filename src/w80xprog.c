/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 John Sanpe <sanpeqf@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <w80xprog.h>
#include <w80xhw.h>
#include <term.h>
#include <progress.h>

struct status_info {
    char code;
    const char *info;
};

static struct status_info
error_table[] = {
    {
        RETURN_NOMAL,
        "Operation complete",
    }, {
        RETURN_CANCEL,
        "Host cancel.",
    }, {
        RETURN_ETIMEOUT,
        "Timeout no data received",
    }, {
        RETURN_EINDEX,
        "Wrong package serial number",
    }, {
        RETURN_ESIZE,
        "Image too large",
    }, {
        RETURN_EADDR,
        "Illegal image flash address",
    }, {
        RETURN_EALIGN,
        "The image burning address page is not aligned",
    }, {
        RETURN_EHCRC,
        "Image header check error",
    }, {
        RETURN_EDCRC,
        "Image content verification error",
    }, {
        RETURN_EDATA,
        "The image content is incomplete or the signature is missing",
    }, {
        RETURN_ECRC,
        "Command check error",
    }, {
        RETURN_EINVAL,
        "Command parameter error",
    }, {
        RETURN_EGETPARM,
        "Failed to get ft parameters (MAC, gain, etc.)",
    }, {
        RETURN_ESETGAIN,
        "Set gain failed",
    }, {
        RETURN_ESETMAC,
        "Failed to set mac",
    },
    { }, /* NULL */
};

static const char *
status_info(char error)
{
    unsigned int index;

    for (index = 0; error_table[index].code; ++index) {
        if (error_table[index].code == error)
            return error_table[index].info;
    }

    return "Unknow error";
}

static inline void
format_haddr(void *src)
{
    char buff[ETH_HEX_ALEN + 6], *str;
    unsigned int count;

    /*
     * The raw haddr format is "Mac:ABABABABABAB",
     * we need format it.
     */
    memset(buff, 0, sizeof(buff));
    memcpy(buff, src + 4, 12);

    for (str = buff; *str; str++)
        *str = tolower(*str);

    for (str = src, count = 0; count < 6; ++count) {
        memcpy(str, buff + count * 2, 2);
        str += 2;
        if (count != 5) {
            memcpy(str, ":", 1);
            str += 1;
        }
    }

    memset(str, 0, 1);
}

static inline int
atoh(const char *src, uint8_t *mac, unsigned int len)
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
            return -BFDEV_EINVAL;

        if (count & 0x01)
            *mac++ |= tmp;
        else
            *mac |= tmp << 4;
    }

    return -BFDEV_ENOERR;
}

static int
wait_busy(void)
{
    unsigned int count;
    uint8_t value;
    int retval;

    for (count = 0; count < WAIT_TIMES; ++count) {
        retval = term_read(&value, 1);
        if (retval < 0)
            return retval;

        BFDEV_BUG_ON(retval > 1);
        if (retval && value == RETURN_NOMAL)
            return -BFDEV_ENOERR;

        /* Sending interval: 120ms */
        usleep(120000);
    }

    return -BFDEV_EBUSY;
}

static int
wait_read(void *buffer, unsigned int length)
{
    unsigned int count, index;
    int retval;

    index = 0;
    for (count = 0; count < WAIT_TIMES; ++count) {
        retval = term_read(buffer + index, length - index);
        if (retval < 0)
            return retval;

        if (retval) {
            index += retval;
            count = 0;
        }

        BFDEV_BUG_ON(index > length);
        if (index == length)
            return -BFDEV_ENOERR;

        /* Sending interval: 100ms */
        usleep(100000);
    }

    return -BFDEV_EBUSY;
}

static int
opcode_transfer(enum opcode_types opcode, void *param,
                void *buffer, unsigned int length)
{
    struct opcode_transfer *trans;
    unsigned int tsize, psize;
    uint16_t cksum;
    int retval;

    tsize = sizeof(struct opcode_head) + OPCODE_LEN(opcode);
    trans = malloc(tsize);
    if (!trans)
        return -BFDEV_ENOMEM;

    term_flush();
    retval = wait_busy();
    if (retval)
        return retval;

    trans->head.sign = 0x21;
    trans->head.reserved = 0x00;
    trans->head.length = OPCODE_LEN(opcode);
    trans->content.opcode = bfdev_cpu_to_le32(OPCODE_DATA(opcode));

    psize = OPCODE_LEN(opcode) - sizeof(trans->content);
    if (psize)
        memcpy(trans->content.param, param, psize);

    /* Checksum should skip itself */
    cksum = bfdev_crc_itut(&trans->content.opcode, OPCODE_LEN(opcode) - 2, 0xffff);
    trans->content.checksum = bfdev_cpu_to_le16(cksum);

    term_flush();
    retval = term_write(trans, tsize);
    if (retval < 0)
        return retval;

    if (buffer) {
        retval = wait_read(buffer, length);
        if (retval)
            return retval;
    }

    free(trans);
    return -BFDEV_ENOERR;
}

static int
xmodem_transfer(uint8_t *src, unsigned int size)
{
    struct progress prog;
    struct xmodem_packet packet;
    unsigned int xfer, retry;
    uint8_t count, value;
    uint16_t cksum;
    int retval;

    term_flush();
    retval = wait_busy();
    if (retval)
        return retval;

    progress_init(&prog, size);
    for (count = 1; (xfer = bfdev_min(size, PAYLOAD_SIZE)); size -= xfer) {
        memcpy(packet.payload, src, xfer);
        if (xfer < PAYLOAD_SIZE)
            memset(packet.payload + xfer, 0x1a, PAYLOAD_SIZE - xfer);

        cksum = bfdev_crc_itut(packet.payload, PAYLOAD_SIZE, 0);
        retry = XMODEM_RETRANS;

        packet.types = XMODEM_SOH;
        packet.checksum = bfdev_cpu_to_be16(cksum);

retry:
        if (bfdev_unlikely(!retry--)) {
            bfdev_log_err("\tAbort Transfer after twenty retries\n");
            retval = -BFDEV_ETIMEDOUT;
            goto abort;
        }

        packet.count = count;
        packet.verify = ~(uint8_t)count;

        retval = term_write(&packet, sizeof(packet));
        if (retval < 0)
            return retval;

        retval = wait_read(&value, 1);
        if (retval)
            return retval;

        if (bfdev_unlikely(value != XMODEM_ACK)) {
            if (value == XMODEM_NAK) {
                bfdev_log_err("\tTransfer Retry\n");
                goto retry;
            }

            if (value == XMODEM_CAN) {
                bfdev_log_err("\tTransfer Cancelled\n");
                retval = -BFDEV_ECANCELED;
            } else {
                bfdev_log_err("\tUnknow Retval %#04x\n", value);
                retval = -BFDEV_EREMOTEIO;
            }

            goto abort;
        }

        progress_update(&prog, xfer);
        src += xfer;
        count++;
    }

    printf("\n");
    value = XMODEM_EOT;
    retval = term_write(&value, 1);
    if (retval < 0)
        return retval;

    retval = wait_read(&value, 1);
    if (retval)
        return retval;

    if (value != XMODEM_ACK)
        return -BFDEV_ECOMM;

    return -BFDEV_ENOERR;

abort:
    value = XMODEM_EOT;
    term_write(&value, 1);
    return retval;
}

int
spinor_flash(uint8_t *src, size_t size)
{
    int retval;

    bfdev_log_info("Chip Flash:\n");
    retval = xmodem_transfer(src, size);
    if (retval)
        return retval;

    return -BFDEV_ENOERR;
}

int
spinor_erase(uint16_t index, uint16_t size)
{
    struct spinor_erase param = {};
    uint8_t state;
    int retval;

    bfdev_log_info("Chip Erase:\n");
    param.index = bfdev_cpu_to_le16(index & 0x7fff);
    param.count = bfdev_cpu_to_le16(BFDEV_DIV_ROUND_UP(size, 4096));

    retval = opcode_transfer(OPCODE_ERASE_SPINOR, &param, &state, 1);
    if (retval)
        return retval;

    bfdev_log_info("\t[%#04x]: %s\n", state, status_info(state));
    if (state != RETURN_NOMAL)
        return -BFDEV_ECONNABORTED;

    return -BFDEV_ENOERR;
}

int
serial_speed(uint32_t speed)
{
    struct serial_speed param = {};
    uint8_t state;
    int retval;

    bfdev_log_info("Setting speed:\n");
    param.speed = bfdev_cpu_to_le32(speed),

    retval = opcode_transfer(OPCODE_SET_FREQ, &param, &state, 1);
    if (retval)
        return retval;

    bfdev_log_info("\t[%#04x]: %s\n", state, state == 6 ? "OK" : "Failed");
    if (state != 6)
        return -BFDEV_EBUSY;

    return -BFDEV_ENOERR;
}

int
flash_bmac(const char *mac)
{
    struct mac_flash param = {};
    uint8_t state;
    int retval;

    bfdev_log_info("Flash BT MAC:\n");
    if (atoh(mac, &param.index[0], 6)) {
        bfdev_log_err("\tIncorrect format\n");
        return -BFDEV_EINVAL;
    }

    retval = opcode_transfer(OPCODE_SET_BT_MAC, &param, &state, 1);
    if (retval)
        return retval;

    bfdev_log_info("\t[%#04x]: %s\n", state, status_info(state));
    if (state != RETURN_NOMAL)
        return -BFDEV_ECONNABORTED;

    return -BFDEV_ENOERR;
}

int
flash_wmac(const char *mac)
{
    struct mac_flash param = {};
    uint8_t state;
    int retval;

    bfdev_log_info("Flash WIFI MAC:\n");
    if (atoh(mac, &param.index[0], 6)) {
        bfdev_log_err("\tincorrect format\n");
        return -BFDEV_EINVAL;
    }

    retval = opcode_transfer(OPCODE_SET_NET_MAC, &param, &state, 1);
    if (retval)
        return retval;

    bfdev_log_info("\t[%#04x]: %s\n", state, status_info(state));
    if (state != RETURN_NOMAL)
        return -BFDEV_ECONNABORTED;

    return -BFDEV_ENOERR;
}

int
flash_gain(const char *gain)
{
    struct gain_flash param = {};
    uint8_t state;
    int retval;

    bfdev_log_info("Flash RF GAIN:\n");
    if (atoh(gain, &param.index[0], 84)) {
        bfdev_log_err("\tincorrect format\n");
        return -BFDEV_EINVAL;
    }

    retval = opcode_transfer(OPCODE_SET_GAIN, &param, &state, 1);
    if (retval)
        return retval;

    bfdev_log_info("\t[%#04x]: %s\n", state, status_info(state));
    if (state != RETURN_NOMAL)
        return -BFDEV_ECONNABORTED;

    return -BFDEV_ENOERR;
}

int
chip_info(void)
{
    uint8_t buff[256];
    int retval;

    bfdev_log_info("Chip information:\n");
    retval = opcode_transfer(OPCODE_GET_BT_MAC, NULL, buff, REPLY_MAC_LEN);
    if (retval)
        return retval;

    buff[REPLY_MAC_LEN] = '\0';
    format_haddr(buff);
    bfdev_log_info("\tBT MAC: %s\n", buff);

    retval = opcode_transfer(OPCODE_GET_NET_MAC, NULL, buff, REPLY_MAC_LEN);
    if (retval)
        return retval;

    buff[REPLY_MAC_LEN] = '\0';
    format_haddr(buff);
    bfdev_log_info("\tWIFI MAC: %s\n", buff);

    retval = opcode_transfer(OPCODE_GET_SPINOR, NULL, buff, REPLY_FLASH_LEN);
    if (retval)
        return retval;

    buff[REPLY_FLASH_LEN] = '\0';
    bfdev_log_info("\tFlash: %s\n", buff);

    retval = opcode_transfer(OPCODE_GET_VERSION, NULL, buff, REPLY_ROM_LEN);
    if (retval)
        return retval;

    buff[REPLY_ROM_LEN] = '\0';
    bfdev_log_info("\tROM: %s\n", buff);

    retval = opcode_transfer(OPCODE_GET_GAIN, NULL, buff, REPLY_GAIN_LEN);
    if (retval)
        return retval;

    buff[REPLY_GAIN_LEN] = '\0';
    bfdev_log_info("\tRF GAIN: %s\n", buff);

    return -BFDEV_ENOERR;
}

int
chip_reset(void)
{
    int retval;

    bfdev_log_info("Chip reset...\n");
    retval = opcode_transfer(OPCODE_REBOOT, NULL, NULL, 0);
    if (retval)
        return retval;

    return -BFDEV_ENOERR;
}

int
entry_secboot(void)
{
    uint8_t buff[3], version[256];
    unsigned int count, index;
    int retval;

    bfdev_log_info("Entry secboot:\n");
    term_reset(true);
    usleep(5000);

    term_flush();
    term_print("AT+Z\r\n");
    term_reset(false);

    buff[0] = 0x1b;
    buff[1] = 0x1b;
    buff[2] = 0x1b;

    index = 0;
    memset(version, 0, sizeof(version));

    for (count = 0; count < SECBOOT_RETRANS; ++count) {
        retval = term_write(buff, 3);
        if (retval < 0)
            return retval;

        retval = term_read(version + index, REPLY_SECBOOT_LEN - index);
        if (retval < 0)
            return retval;

        index += retval;
        BFDEV_BUG_ON(index > REPLY_SECBOOT_LEN);

        if (index == REPLY_SECBOOT_LEN)
            break;

        usleep(2000);
    }

    if (strncmp((void *)version, "Secboot", 7)) {
        bfdev_log_err("\tChip error\n");
        return -BFDEV_EPERM;
    }

    bfdev_log_info("\tVersion: %s\n", version);
    sleep(1);

    return -BFDEV_ENOERR;
}
