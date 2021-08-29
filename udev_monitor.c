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
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "udev.h"
#include "udev_list.h"

#ifndef UDEV_MONITOR_NLGRP
#define UDEV_MONITOR_NLGRP 0x4
#endif

struct udev_monitor {
    struct udev_list_entry subsystem_match;
    struct udev_list_entry devtype_match;
    struct udev *udev;
    unsigned nlgrp;
    int refcount;
    int fd;
};

static int filter_devtype(struct udev_monitor *udev_monitor, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *devtype;

    devtype = udev_device_get_devtype(udev_device);
    list_entry = udev_list_entry_get_next(&udev_monitor->devtype_match);

    if (!list_entry) {
        return 1;
    }

    if (!devtype) {
        return 0;
    }

    while (list_entry) {
        if (strcmp(devtype, udev_list_entry_get_name(list_entry)) == 0) {
            return 1;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    return 0;
}

static int filter_subsystem(struct udev_monitor *udev_monitor, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *subsystem;

    subsystem = udev_device_get_subsystem(udev_device);
    list_entry = udev_list_entry_get_next(&udev_monitor->subsystem_match);

    if (!list_entry) {
        return 1;
    }

    if (!subsystem) {
        return 0;
    }

    while (list_entry) {
        if (strcmp(subsystem, udev_list_entry_get_name(list_entry)) == 0) {
            return 1;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    return 0;
}

struct udev_device *udev_monitor_receive_device(struct udev_monitor *udev_monitor)
{
    struct udev_device *udev_device;
    struct sockaddr_nl sa = {0};
    struct msghdr hdr = {0};
    struct iovec iov = {0};
    char buf[8192];
    ssize_t len;

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);

    hdr.msg_name = &sa;
    hdr.msg_namelen = sizeof(sa);
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;

    while (1) {
        len = recvmsg(udev_monitor->fd, &hdr, 0);

        if (len <= 0) {
            break;
        }

        if (hdr.msg_flags & MSG_TRUNC) {
            continue;
        }

        if (sa.nl_groups == 0x0 || (sa.nl_groups == 0x1 && sa.nl_pid)) {
            continue;
        }

        udev_device = udev_device_new_from_uevent(udev_monitor->udev, buf, len);

        if (!udev_device) {
            continue;
        }

        if (!filter_subsystem(udev_monitor, udev_device) ||
            !filter_devtype(udev_monitor, udev_device)) {
            udev_device_unref(udev_device);
            continue;
        }

        return udev_device;
    }

    return NULL;
}

int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor)
{
    struct sockaddr_nl sa = {0};

    if (!udev_monitor) {
        return -1;
    }

    sa.nl_family = AF_NETLINK;
    sa.nl_groups = udev_monitor->nlgrp;
    return bind(udev_monitor->fd, (struct sockaddr *)&sa, sizeof(sa));
}

int udev_monitor_set_receive_buffer_size(struct udev_monitor *udev_monitor, int size)
{
    return udev_monitor ? setsockopt(udev_monitor->fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) : -1;
}

int udev_monitor_get_fd(struct udev_monitor *udev_monitor)
{
    return udev_monitor ? udev_monitor->fd : -1;
}

struct udev *udev_monitor_get_udev(struct udev_monitor *udev_monitor)
{
    return udev_monitor ? udev_monitor->udev : NULL;
}

/* XXX NOT IMPLEMENTED */ int udev_monitor_filter_update(struct udev_monitor *udev_monitor)
{
    return 0;
}

/* XXX NOT IMPLEMENTED */ int udev_monitor_filter_remove(struct udev_monitor *udev_monitor)
{
    return 0;
}

int udev_monitor_filter_add_match_subsystem_devtype(struct udev_monitor *udev_monitor, const char *subsystem, const char *devtype)
{
    if (!udev_monitor || !subsystem) {
        return -1;
    }

    udev_list_entry_add(&udev_monitor->subsystem_match, subsystem, NULL, 0);

    if (devtype) {
        udev_list_entry_add(&udev_monitor->devtype_match, devtype, NULL, 0);
    }

    return 0;
}

/* XXX NOT IMPLEMENTED */ int udev_monitor_filter_add_match_tag(struct udev_monitor *udev_monitor, const char *tag)
{
    return 0;
}

struct udev_monitor *udev_monitor_new_from_netlink(struct udev *udev, const char *name)
{
    struct udev_monitor *udev_monitor;

    if (!udev || !name) {
        return NULL;
    }

    udev_monitor = calloc(1, sizeof(*udev_monitor));

    if (!udev_monitor) {
        return NULL;
    }

    if (strcmp(name, "udev") == 0) {
        udev_monitor->nlgrp = UDEV_MONITOR_NLGRP;
    }
    else if (strcmp(name, "kernel") == 0) {
        udev_monitor->nlgrp = 0x1;
    }
    else {
        free(udev_monitor);
        return NULL;
    }

    udev_monitor->fd = socket(AF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, NETLINK_KOBJECT_UEVENT);

    if (udev_monitor->fd == -1) {
        free(udev_monitor);
        return NULL;
    }

    udev_monitor->refcount = 1;
    udev_monitor->udev = udev;
    return udev_monitor;
}

struct udev_monitor *udev_monitor_ref(struct udev_monitor *udev_monitor)
{
    if (!udev_monitor) {
        return NULL;
    }

    udev_monitor->refcount++;
    return udev_monitor;
}

struct udev_monitor *udev_monitor_unref(struct udev_monitor *udev_monitor)
{
    if (!udev_monitor) {
        return NULL;
    }

    if (--udev_monitor->refcount > 0) {
        return NULL;
    }

    udev_list_entry_free_all(&udev_monitor->devtype_match);
    udev_list_entry_free_all(&udev_monitor->subsystem_match);

    close(udev_monitor->fd);
    free(udev_monitor);
    return NULL;
}
