/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include "w80xprog.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

static int ttys;

int termios_setspeed(unsigned int speed)
{
    struct termios term;
    int retval;

    retval = tcgetattr(ttys, &term);
    if (retval)
        return retval;

    retval = cfsetspeed(&term, speed);
    if (retval)
        return retval;

    tcflush(ttys, TCIOFLUSH);
    return tcsetattr(ttys, TCSANOW, &term);
}

int termios_setup(unsigned int speed, int databits, int stopbits, char parity)
{
    struct termios term;
    int retval;

    retval = termios_setspeed(speed);
    if (retval)
        return retval;

    retval = tcgetattr(ttys, &term);
    if (retval)
        return retval;

    term.c_cflag |= (CLOCAL | CREAD);

    term.c_cflag &= ~CSIZE;
    if (databits == 7)
        term.c_cflag |= CS7;
    else
        term.c_cflag |= CS8;

    if (stopbits == 2)
        term.c_cflag |= CSTOPB;
    else
        term.c_cflag &= ~CSTOPB;

    switch (parity){
        case 'N': case 'n':
            term.c_cflag &= ~PARENB;
            term.c_iflag &= ~INPCK;
            break;
        case 'O': case 'o':
            term.c_cflag |= (PARODD | PARENB);
            term.c_iflag |= INPCK;
            break;
        case 'E': case 'e':
            term.c_cflag |= PARENB;
            term.c_cflag &= ~PARODD;
            term.c_iflag |= INPCK;
            break;
        case 'S': case 's':
            term.c_cflag &= ~PARENB;
            term.c_cflag &= ~CSTOPB;
            break;
    }

    if (parity != 'n')
        term.c_iflag |= INPCK;

    term.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    term.c_oflag &= ~OPOST;
    term.c_cc[VTIME] = 255;
    term.c_cc[VMIN] = 0;

    tcflush(ttys, TCIOFLUSH);
    tcsetattr(ttys, TCSANOW, &term);
    return 0;
}

int termios_rts(bool enable)
{
    unsigned int state;
    int ret;

    ret = ioctl(ttys, TIOCMGET, &state);
    if (ret)
        return ret;

    if (enable)
        state |= TIOCM_RTS;
    else
        state &= ~TIOCM_RTS;

    return ioctl(ttys, TIOCMSET, &state);
}

int termios_flush(void)
{
    return tcflush(ttys, TCIFLUSH);
}

int termios_read(void *data, unsigned long len)
{
    int ret = read(ttys, data, len);
    tcflush(ttys, TCIFLUSH);
    return ret;
}

int termios_write(const void *data, unsigned long len)
{
    int ret = write(ttys, data, len);
    tcflush(ttys, TCOFLUSH);
    return ret;
}

int termios_print(const char *str)
{
    unsigned int len = strlen(str);
    return termios_write(str, len);
}

int termios_open(char *path)
{
    if (ttys)
        return -EALREADY;

    ttys = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
    fcntl(ttys, F_SETFL, 0);

    return ttys < 0 ? ttys : 0;
}

