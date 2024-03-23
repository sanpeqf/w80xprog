/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2024 John Sanpe <sanpeqf@gmail.com>
 */

#include <stdio.h>
#include <sys/time.h>
#include <progress.h>

static double
gettime(void)
{
    struct timeval tv;
    double retval;

    gettimeofday(&tv, NULL);
    retval = tv.tv_sec + (double)tv.tv_usec / 1000000;

    return retval;
}

static const char *
format_eta(char *buff, double remaining)
{
    int seconds;

    seconds = remaining + 0.5;
    if (seconds >= 0 && seconds < 6000) {
        sprintf(buff, "%02d:%02d", seconds / 60, seconds % 60);
        return buff;
    }

    return "--:--";
}

static const char *
size_unit(char *buff, double size)
{
    const char *unit[] = {"B", "KB", "MB"};
    int count = 0;

    while ((size > 1024) && (count < BFDEV_ARRAY_SIZE(unit))) {
        size /= 1024;
        count++;
    }

    sprintf(buff, "%5.3f %s", size, unit[count]);
    return buff;
}

void
progress_update(struct progress *prog, size_t bytes)
{
    char buff1[32], buff2[32];
    double ratio, speed, eta;
    int index, pos;

    prog->done += bytes;
    ratio = (double)prog->done / prog->total;
    speed = (double)prog->done / (gettime() - prog->start);
    pos = 48 * ratio;

    eta = 0;
    if (speed)
        eta = (prog->total - prog->done) / speed;

    printf("\r%3.0f%% [", ratio * 100);
    for (index = 0; index < pos; index++)
        putchar('=');

    for (index = pos; index < 48; index++)
        putchar(' ');

    if (prog->done < prog->total) {
        printf("] %s/s, ETA %s\r",
               size_unit(buff1, speed), format_eta(buff2, eta));
    } else {
        printf("] %s, %s/s\r",
               size_unit(buff1, prog->done), size_unit(buff2, speed));
    }

    fflush(stdout);
}

void
progress_init(struct progress *prog, size_t total)
{
    prog->total = total;
    prog->done = 0;
    prog->start = gettime();
}
