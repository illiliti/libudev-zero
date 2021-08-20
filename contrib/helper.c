/*
 * Copyright (c) 2020-2021 illiliti <illiliti@protonmail.com>
 * SPDX-License-Identifier: ISC
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * NOTE: you don't need this if you have mdev/mdevd, refer to mdev.conf
 * NOTE: you need this if you want to use bare-bones CONFIG_UEVENT_HELPER
 *
 * build:
 * cc helper.c -o helper
 *
 * usage:
 * echo /full/path/to/helper > /proc/sys/kernel/hotplug
 * echo "/full/path/to/helper UDEV_MONITOR_DIR" > /proc/sys/kernel/hotplug
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
    extern char **environ;
    char path[PATH_MAX];
    char *dir;
    int fd, i;

    switch (argc) {
    case 1:
        dir = "/tmp/.libudev-zero";
        break;
    case 2:
        dir = argv[1];
        break;
    default:
        fprintf(stderr, "usage: %s [dir]\n", argv[0]);
        return 2;
    }

    snprintf(path, sizeof(path), "%s/uevent.XXXXXX", dir);
    fd = mkstemp(path);

    if (fd == -1) {
        perror("mkstemp");
        return 1;
    }

    for (i = 0; environ[i]; i++) {
        if (strncmp(environ[i], "PATH=", 5) == 0 ||
            strncmp(environ[i], "HOME=", 5) == 0) {
            continue;
        }

        if (write(fd, environ[i], strlen(environ[i])) == -1 ||
            write(fd, "\n", 1) == -1) {
            perror("write");
            close(fd);
            unlink(path);
            return 1;
        }
    }

    fchmod(fd, 0444);
    close(fd);
    return 0;
}
