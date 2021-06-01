#include <poll.h>
#include <errno.h>
#include <fcntl.h>
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
    struct udev_list_entry subsystem_match;
    struct udev_list_entry devtype_match;
    pthread_t thread;
    struct udev *udev;
    int refcount;
    int signal_fd;
    int sfd[2];
    int ifd;
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
    char file[PATH_MAX + sizeof(UDEV_MONITOR_DIR)], data[4096];
    struct udev_device *udev_device;

    if (recv(udev_monitor->sfd[0], data, sizeof(data), 0) == -1) {
        return NULL;
    }

    snprintf(file, sizeof(file), "%s/%s", UDEV_MONITOR_DIR, data);
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

        len = read(udev_monitor->ifd, data, sizeof(data));

        if (len == -1) {
            continue;
        }

        // exit on ifd error or close
        if (!(poll_fds[0].revents & POLLIN) || len == 0) {
            return NULL;
        }

        for (i = 0; i < len; i += sizeof(struct inotify_event) + event->len) {
            event = (struct inotify_event *)&data[i];

            // TODO directory is removed
            if (event->mask & IN_IGNORED) {
                break;
            }

            send(udev_monitor->sfd[1], event->name, event->len, 0);
        }
    }

    // unreacheble
    return NULL;
}

int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor)
{
    if (!udev_monitor) {
        return -1;
    }

    return pthread_create(&udev_monitor->thread, NULL, handle_event, udev_monitor);
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
    struct stat st;

    if (!udev || !name) {
        return NULL;
    }

    if (lstat(UDEV_MONITOR_DIR, &st) != 0) {
        if (mkdir(UDEV_MONITOR_DIR, 0) == -1) {
            return NULL;
        }

        if (chmod(UDEV_MONITOR_DIR, 0777) == -1) {
            return NULL;
        }
    }
    else if (!S_ISDIR(st.st_mode)) {
        return NULL;
    }

    udev_monitor = calloc(1, sizeof(struct udev_monitor));

    if (!udev_monitor) {
        return NULL;
    }

    udev_monitor->signal_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

    if (udev_monitor->signal_fd == -1) {
        goto free_monitor;
    }

    udev_monitor->ifd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);

    if (udev_monitor->ifd == -1) {
        goto close_signal_fd;
    }

    if (inotify_add_watch(udev_monitor->ifd, UDEV_MONITOR_DIR, IN_CLOSE_WRITE | IN_EXCL_UNLINK) == -1) {
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

    udev_list_entry_free_all(&udev_monitor->devtype_match);
    udev_list_entry_free_all(&udev_monitor->subsystem_match);

    for (i = 0; i < 2; i++) {
        close(udev_monitor->sfd[i]);
    }

    close(udev_monitor->signal_fd);
    close(udev_monitor->ifd);
    free(udev_monitor);
    return NULL;
}
