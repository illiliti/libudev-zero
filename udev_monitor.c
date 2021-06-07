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

#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>

#include "udev.h"
#include "udev_list.h"

#ifndef UDEV_MONITOR_DIR
#define UDEV_MONITOR_DIR "/tmp/.libudev-zero"
#endif

struct udev_monitor {
    struct udev_list_table *subsystem_match;
    struct udev_list_table *devtype_match;
    struct udev *udev;
    pthread_t thread;
    const char *dir;
    int signal_fd;
    int refcount;
    int sfd[2];
    int ifd;
};

static int filter_devtype(struct udev_monitor *udev_monitor, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *devtype;

    list_entry = udev_list_table_get_head(udev_monitor->devtype_match);

    if (!list_entry) {
        return 1;
    }

    devtype = udev_device_get_devtype(udev_device);

    if (!devtype) {
        return 0;
    }

    return !!udev_list_entry_get_by_name(list_entry, devtype);
}

static int filter_subsystem(struct udev_monitor *udev_monitor, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *subsystem;

    list_entry = udev_list_table_get_head(udev_monitor->subsystem_match);

    if (!list_entry) {
        return 1;
    }

    subsystem = udev_device_get_subsystem(udev_device);

    if (!subsystem) {
        return 0;
    }

    return !!udev_list_entry_get_by_name(list_entry, subsystem);
}

struct udev_device *udev_monitor_receive_device(struct udev_monitor *udev_monitor)
{
    char file[PATH_MAX], data[4096];
    struct udev_device *udev_device;

    // TODO use recvmsg to detect truncated msgs?
    if (recv(udev_monitor->sfd[0], data, sizeof(data), 0) == -1) {
        return NULL;
    }

    // check truncation error to make gcc happy
    if ((unsigned)snprintf(file, sizeof(file), "%s/%s", udev_monitor->dir, data) >= sizeof(file)) {
        return NULL;
    }

    udev_device = udev_device_new_from_file(udev_monitor->udev, file);

    if (!udev_device) {
        return NULL;
    }

    if (!filter_subsystem(udev_monitor, udev_device) ||
        !filter_devtype(udev_monitor, udev_device)) {
        udev_device_unref(udev_device);
        return NULL;
    }

    return udev_device;
}

static void *handle_event(void *ptr)
{
    struct udev_monitor *udev_monitor = ptr;
    struct inotify_event *event;
    struct pollfd poll_fds[2];
    char data[4096];
    ssize_t len;
    int i;

    poll_fds[0].fd = udev_monitor->ifd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = udev_monitor->signal_fd;
    poll_fds[1].events = POLLIN;

    while (1) {
        if (poll(poll_fds, 2, -1) == -1) {
            if (errno == EINTR) {
                continue;
            }

            return NULL;
        }

        // exit on explicit signal
        if (poll_fds[1].revents & POLLIN) {
            return NULL;
        }

        // exit on poll error
        if (!(poll_fds[0].revents & POLLIN)) {
            return NULL;
        }

        len = read(udev_monitor->ifd, data, sizeof(data));

        if (len == -1) {
            return NULL;
        }

        for (i = 0; i < len; i += sizeof(struct inotify_event) + event->len) {
            event = (struct inotify_event *)&data[i];

            // TODO directory is removed
            if (event->mask & IN_IGNORED) {
                break;
            }

            if (event->mask & IN_ISDIR) {
                continue;
            }

            send(udev_monitor->sfd[1], event->name, event->len, 0);
        }
    }

    // unreachable
    return NULL;
}

int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor)
{
    return udev_monitor ? (pthread_create(&udev_monitor->thread, NULL, handle_event, udev_monitor) == 0) - 1 : -1;
}

/* XXX NOT IMPLEMENTED */ int udev_monitor_set_receive_buffer_size(struct udev_monitor *udev_monitor, int size)
{
    return 0;
}

int udev_monitor_get_fd(struct udev_monitor *udev_monitor)
{
    return udev_monitor ? udev_monitor->sfd[0] : -1;
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

    if (!udev_list_entry_add(udev_monitor->subsystem_match, subsystem, NULL)) {
        return -1;
    }

    if (!devtype) {
        return 0;
    }

    return !!udev_list_entry_add(udev_monitor->devtype_match, devtype, NULL) - 1;
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

    udev_monitor = calloc(1, sizeof(struct udev_monitor));

    if (!udev_monitor) {
        return NULL;
    }

    udev_monitor->subsystem_match = udev_list_table_init(10, strcmp);

    if (!udev_monitor->subsystem_match) {
        goto free_monitor;
    }

    udev_monitor->devtype_match = udev_list_table_init(10, strcmp);

    if (!udev_monitor->subsystem_match) {
        goto deinit_subsystem_match;
    }

    udev_monitor->signal_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

    if (udev_monitor->signal_fd == -1) {
        goto deinit_devtype_match;
    }

    udev_monitor->ifd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);

    if (udev_monitor->ifd == -1) {
        goto close_signal_fd;
    }

    // TODO docs
    udev_monitor->dir = getenv("UDEV_MONITOR_DIR");

    if (!udev_monitor->dir || udev_monitor->dir[0] == '\0') {
        udev_monitor->dir = UDEV_MONITOR_DIR;

        if (access(udev_monitor->dir, F_OK) == -1) {
            if (errno != ENOENT) {
                goto close_ifd;
            }

            if (mkdir(udev_monitor->dir, 0) == -1) {
                goto close_ifd;
            }

            if (chmod(udev_monitor->dir, 0777) == -1) {
                goto close_ifd;
            }
        }
    }

    if (inotify_add_watch(udev_monitor->ifd, udev_monitor->dir, IN_CLOSE_WRITE | IN_EXCL_UNLINK | IN_ONLYDIR) == -1) {
        goto close_ifd;
    }

    if (socketpair(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, udev_monitor->sfd) == -1) {
        goto close_ifd;
    }

    udev_monitor->refcount = 1;
    udev_monitor->udev = udev;
    return udev_monitor;

close_ifd:
    close(udev_monitor->ifd);

close_signal_fd:
    close(udev_monitor->signal_fd);

deinit_subsystem_match:
    udev_list_table_deinit(udev_monitor->subsystem_match);

deinit_devtype_match:
    udev_list_table_deinit(udev_monitor->devtype_match);

free_monitor:
    free(udev_monitor);
    return NULL;
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
    int i;

    if (!udev_monitor) {
        return NULL;
    }

    if (--udev_monitor->refcount > 0) {
        return NULL;
    }

    // Wake up the event thread
    eventfd_write(udev_monitor->signal_fd, 1);
    // waiting for event thread to end before freeing udev_monitor
    pthread_join(udev_monitor->thread, NULL);

    udev_list_table_deinit(udev_monitor->subsystem_match);
    udev_list_table_deinit(udev_monitor->devtype_match);

    for (i = 0; i < 2; i++) {
        close(udev_monitor->sfd[i]);
    }

    close(udev_monitor->signal_fd);
    close(udev_monitor->ifd);
    free(udev_monitor);
    return NULL;
}
