/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 John Sanpe <sanpeqf@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <w80xprog.h>
#include <term.h>

#define DEFAULTS_PORT "/dev/ttyUSB0"
#define DEFAULTS_SPEED 115200

enum {
    __FLAG_SECBOOT = 0,
    __FLAG_RESET,
    __FLAG_INFO,

    FLAG_SECBOOT = 1UL << __FLAG_SECBOOT,
    FLAG_RESET = 1UL << __FLAG_RESET,
    FLAG_INFO = 1UL << __FLAG_INFO,
};

static const struct option
options[] = {
    {"help",    no_argument,        0,  'h'},
    {"port",    required_argument,  0,  'p'},
    {"secboot", required_argument,  0,  'o'},
    {"info",    no_argument,        0,  'i'},
    {"speed",   required_argument,  0,  's'},
    {"flash",   required_argument,  0,  'f'},
    {"erase",   required_argument,  0,  'e'},
    {"bt",      required_argument,  0,  'b'},
    {"wifi",    required_argument,  0,  'w'},
    {"gain",    required_argument,  0,  'g'},
    {"reset",   no_argument,        0,  'r'},
    { }, /* NULL */
};

static __bfdev_noreturn void
usage(void)
{
    printf("Usage: w80xprog [options]...\n");
    printf("\t-h, --help                display this message\n");
    printf("\t-p, --port <device>       set device path\n");
    printf("\t-o, --secboot             entry secboot mode\n");
    printf("\t-i, --info                read the chip info\n");
    printf("\t-s, --speed <freq>        increase the download speed\n");
    printf("\t-f, --flash <file>        flash chip with data from filename\n");
    printf("\t-e, --erase <offset:size> erase the entire chip\n");
    printf("\t-b, --bt <mac>            set bluetooth mac address\n");
    printf("\t-w, --wifi <mac>          set wifi mac address\n");
    printf("\t-g, --gain <gain>         set power amplifier gain\n");
    printf("\t-r, --reset               reset chip after operate\n");
    exit(1);
}

static void
check_mac(const char *str)
{
    if (strlen(str) != 17 || str[2]  != ':' ||
        str[5]  != ':' || str[8]  != ':' ||
        str[11] != ':' || str[14] != ':')
        usage();
}

int main(int argc, char *const argv[])
{
    unsigned int speed, flags, eidx, esize;
    const char *bmac, *wmac, *gain;
    const char *file, *port, *errname;
    int optidx, retval;
    char *endp;
    char arg;

    port = DEFAULTS_PORT;
    file = NULL;

    bmac = NULL;
    wmac = NULL;
    gain = NULL;

    speed = 0;
    flags = 0;
    esize = 0;

    printf("w80xprog v" __bfdev_stringify(PROJECT_VERSION) "\n");
    printf("Copyright(c) 2021-2024 John Sanpe <sanpeqf@gmail.com>\n");
    printf("License GPLv2+: GNU GPL version 2 or later.\n\n");

    for (;;) {
        arg = getopt_long(argc, argv, "p:ois:f:e:b:w:g:rh", options, &optidx);
        if (arg == -1)
            break;

        switch (arg) {
            case 'p':
                port = optarg;
                break;

            case 'o':
                flags |= FLAG_SECBOOT;
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
                eidx = strtoul(optarg, &endp, 0);
                if (*endp++ != ':')
                    usage();

                esize = strtoul(endp, NULL, 0);
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

            case 'h': default:
                usage();
        }
    }

    if (argc < 2)
        usage();

    retval = term_open(port);
    if (retval) {
        bfdev_errname(retval, &errname);
        printf("Failed to open port: %s\n", errname);
        return retval;
    }

    retval = term_setup(DEFAULTS_SPEED, 8, 1, 'N');
    if (retval) {
        bfdev_errname(retval, &errname);
        printf("Failed to setup port: %s\n", errname);
        return retval;
    }

    term_rts(false);

    if (flags & FLAG_SECBOOT) {
        retval = entry_secboot();
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to entry secboot: %s\n", errname);
            return retval;
        }
    }

    if (flags & FLAG_INFO) {
        retval = chip_info();
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to read info: %s\n", errname);
            return retval;
        }
    }

    if (esize) {
        retval = spinor_erase(eidx, esize);
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to erase chip: %s\n", errname);
            return retval;
        }
    }

    if (bmac) {
        retval = flash_bmac(bmac);
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to flash bt mac: %s\n", errname);
            return retval;
        }
    }

    if (wmac) {
        retval = flash_wmac(wmac);
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to flash wifi mac: %s\n", errname);
            return retval;
        }
    }

    if (gain) {
        retval = flash_gain(gain);
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to flash rf gain: %s\n", errname);
            return retval;
        }
    }

    if (speed) {
        retval = serial_speed(speed);
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to set chip speed: %s\n", errname);
            return retval;
        }

        retval = term_setspeed(speed);
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to set host speed: %s\n", errname);
            return retval;
        }
    }

    if (file) {
        struct stat stat;
        void *map;
        int fd;

        fd = open(file, O_RDONLY);
        if (fd < 0) {
            printf("Failed to open file\n");
            return fd;
        }

        retval = fstat(fd, &stat);
        if (retval) {
            printf("Failed to fstat file\n");
            return retval;
        }

        map = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            printf("Failed to mmap file\n");
            return -BFDEV_ENOMEM;
        }

        retval = spinor_flash(map, stat.st_size);
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to flash chip: %s\n", errname);
            return retval;
        }

        munmap(map, stat.st_size);
        close(fd);
    }

    if (flags & FLAG_RESET) {
        retval = chip_reset();
        if (retval) {
            bfdev_errname(retval, &errname);
            printf("Failed to reset chip: %s\n", errname);
            return retval;
        }
    }

    term_close();

    return 0;
}
