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
 * Construct uevent message from environment and send it to 0x4 netlink group.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>

int main(int argc, char **argv)
{
    struct sockaddr_nl sa = {0};
    struct msghdr hdr = {0};
    struct iovec iov = {0};
    extern char **environ;
    char buf[8192];
    size_t len;
    int i, fd;

    iov.iov_base = buf;
    iov.iov_len = 0;

    for (i = 0; environ[i]; i++) {
        if (strncmp(environ[i], "PATH=", 5) == 0 ||
            strncmp(environ[i], "HOME=", 5) == 0) {
            continue;
        }

        len = strlen(environ[i]) + 1;

        if (iov.iov_len + len > sizeof(buf)) {
            fprintf(stderr, "%s: uevent exceeds buffer size", argv[0]);
            return 1;
        }

        memcpy(buf + iov.iov_len, environ[i], len);
        iov.iov_len += len;
    }

    sa.nl_family = AF_NETLINK;
    sa.nl_groups = 0x4; // XXX

    hdr.msg_name = &sa;
    hdr.msg_namelen = sizeof(sa);
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;

    fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);

    if (fd == -1) {
        perror("socket");
        return 1;
    }

    if (sendmsg(fd, &hdr, 0) == -1) {
        perror("sendmsg");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
