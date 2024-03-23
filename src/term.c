/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <term.h>

static int ttys;

int
term_setspeed(unsigned int speed)
{
    struct termios term;
    int retval;

    retval = tcgetattr(ttys, &term);
    if (retval)
        return retval;

    retval = cfsetspeed(&term, speed);
    if (retval)
        return retval;

    retval = tcsetattr(ttys, TCSANOW, &term);
    if (retval)
        return retval;

    return 0;
}

int
term_setup(unsigned int speed, int databits, int stopbits, char parity)
{
    struct termios term;
    int retval;

    retval = term_setspeed(speed);
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

    term.c_cc[VTIME] = 0;
    term.c_cc[VMIN] = 0;

    retval = tcsetattr(ttys, TCSANOW, &term);
    if (retval)
        return retval;

    return 0;
}

int
term_rts(bool enable)
{
    unsigned int state;
    int retval;

    retval = ioctl(ttys, TIOCMGET, &state);
    if (retval)
        return retval;

    if (enable)
        state |= TIOCM_RTS;
    else
        state &= ~TIOCM_RTS;

    retval = ioctl(ttys, TIOCMSET, &state);
    if (retval)
        return retval;

    return 0;
}

int
term_read(void *data, size_t size)
{
    return read(ttys, data, size);
}

int
term_write(const void *data, size_t size)
{
    return write(ttys, data, size);
}

int
term_print(const char *str)
{
    return write(ttys, str, strlen(str));
}

int term_flush(void)
{
    return tcflush(ttys, TCIFLUSH);
}

int
term_open(const char *path)
{
    int retval;

    ttys = open(path, O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
    if (ttys < 0)
        return ttys;

    retval = fcntl(ttys, F_SETFL, 0);
    if (retval < 0)
        return retval;

    return 0;
}

void
term_close(void)
{
    close(ttys);
}
