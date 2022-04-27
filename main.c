/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include "w80xprog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define DEFAULTS_PORT   "/dev/ttyUSB0"
#define DEFAULTS_SPEED  115200

#define FLAG_RESET  (1U << 0)
#define FLAG_INFO   (1U << 1)

static const struct option options[] = {
    {"help",    no_argument,        0,  'h'},
    {"port",    required_argument,  0,  'p'},
    {"info",    no_argument,        0,  'i'},
    {"speed",   required_argument,  0,  's'},
    {"flash",   required_argument,  0,  'f'},
    {"erase",   required_argument,  0,  'e'},
    {"bt",      required_argument,  0,  'b'},
    {"wifi",    required_argument,  0,  'w'},
    {"gain",    required_argument,  0,  'g'},
    {"reset",   no_argument,        0,  'r'},
    {"version", no_argument,        0,  'v'},
    { }, /* NULL */
};

static __noreturn void usage(void)
{
    printf("Usage: w80xprog [options]...\n");
    printf("\t-h, --help            display this message\n");
    printf("\t-p, --port <device>   set device path\n");
    printf("\t-i, --info            read the chip info\n");
    printf("\t-s, --speed <freq>    increase the download speed\n");
    printf("\t-f, --flash <file>    flash chip with data from filename\n");
    printf("\t-e, --erase <size>    erase the entire chip\n");
    printf("\t-b, --bt <mac>        set bluetooth mac address\n");
    printf("\t-w, --wifi <mac>      set wifi mac address\n");
    printf("\t-g, --gain <gain>     set power amplifier gain\n");
    printf("\t-r, --reset           reset chip after operate\n");
    printf("\t-v, --version         display version information\n");
    exit(1);
}

static __noreturn void version(void)
{
    printf("w80xprog 1.0\n");
    printf("Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>\n");
    printf("License GPLv2+: GNU GPL version 2 or later.\n");
    exit(1);
}

static void check_mac(const char *str)
{
    if (strlen(str) != 17 || str[2]  != ':' ||
        str[5]  != ':' || str[8]  != ':' ||
        str[11] != ':' || str[14] != ':')
        usage();
}

int main(int argc, char *const argv[])
{
    unsigned long flags = 0;
    unsigned long speed = 0, esize = 0;
    char *bmac = NULL, *wmac = NULL, *gain = NULL;
    char *file = NULL, *port = DEFAULTS_PORT;
    int optidx, ret;
    char arg;

    while ((arg = getopt_long(argc, argv, "p:is:f:e:b:w:g:rhv", options, &optidx)) != -1) {
        switch (arg) {
            case 'p':
                port = optarg;
                break;
            case 'i':
                flags |= FLAG_INFO;
                break;
            case 's':
                speed = strtoul(optarg, NULL, 0);
                break;
            case 'f':
                file = optarg;
                break;
            case 'e':
                esize = strtoul(optarg, NULL, 0);
                break;
            case 'b':
                check_mac(optarg);
                bmac = optarg;
                break;
            case 'w':
                check_mac(optarg);
                wmac = optarg;
                break;
            case 'g':
                if (strlen(optarg) != 168)
                    usage();
                gain = optarg;
                break;
            case 'r':
                flags |= FLAG_RESET;
                break;
            case 'v':
                version();
            case 'h': default:
                usage();
        }
    }

    if (argc < 2)
        usage();

    if ((ret = termios_open(port)))
        err(ret, "Failed to open port");

    if ((ret = termios_setup(115200, 8, 1, 'N')))
        err(ret, "Failed to setup port");

    if ((ret = entry_secboot()))
        err(ret, "Failed to entry secboot");

    if ((flags & FLAG_INFO) && (ret = chip_info()))
        err(ret, "Error while read info");

    if (esize && (ret = spinor_erase(esize)))
        err(ret, "Error while erase chip");

    if (bmac && (ret = flash_bmac(bmac)))
        err(ret, "Error while flash bt mac");

    if (wmac && (ret = flash_wmac(wmac)))
        err(ret, "Error while flash wifi mac");

    if (gain && (ret = flash_gain(gain)))
        err(ret, "Error while flash rf gain");

    if (speed) {
        if ((ret = serial_speed(speed)))
            err(ret, "Error while set chip speed");
        if ((ret = termios_set_speed(speed)))
            err(ret, "Error while set host speed");
    }

    if (file) {
        struct stat stat;
        void *map;
        int fd;

        if ((fd = open(file, O_RDONLY)) < 0)
            err(fd, "Failed to open file");

        if ((ret = fstat(fd, &stat)) < 0)
            err(ret, "fstat error");

        map = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED)
            err(1, "mmap error");

        if ((ret = spinor_flash(map, stat.st_size)))
            err(ret, "Error while flash chip");
    }

    if ((flags & FLAG_RESET) && (ret = chip_reset()))
        err(ret, "Error while reset chip");

    return 0;
}
