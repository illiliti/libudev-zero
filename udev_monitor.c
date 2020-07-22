#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>

#include "udev.h"
#include "udev_list.h"
#include "udev_device.h"

#ifndef UDEV_MONITOR_FIFO
#define UDEV_MONITOR_FIFO "/tmp/.libudev-zero"
#endif

struct udev_monitor {
    struct udev_list_entry subsystem_match;
    struct udev_list_entry devtype_match;
    struct udev *udev;
    int refcount;
    int fd;
};

static int udev_monitor_filter_devtype(struct udev_monitor *udev_monitor, struct udev_device *udev_device)
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

static int udev_monitor_filter_subsystem(struct udev_monitor *udev_monitor, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *subsystem;

    subsystem = udev_device_get_subsystem(udev_device);
    list_entry = udev_list_entry_get_next(&udev_monitor->devtype_match);

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
    char data[PIPE_BUF];
    ssize_t len;

    len = read(udev_monitor->fd, data, sizeof(data));

    if (len == -1) {
        return NULL;
    }

    data[len] = '\0';
    udev_device = udev_device_new_from_uevent(udev_monitor->udev, data);

    if (!udev_device) {
        return NULL;
    }

    if (!udev_monitor_filter_subsystem(udev_monitor, udev_device) ||
        !udev_monitor_filter_devtype(udev_monitor, udev_device)) {
        udev_device_unref(udev_device);
        return NULL;
    }

    return udev_device;
}

int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

int udev_monitor_set_receive_buffer_size(struct udev_monitor *udev_monitor, int size)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

int udev_monitor_get_fd(struct udev_monitor *udev_monitor)
{
    return udev_monitor ? udev_monitor->fd : -1;
}

struct udev *udev_monitor_get_udev(struct udev_monitor *udev_monitor)
{
    return udev_monitor ? udev_monitor->udev : NULL;
}

int udev_monitor_filter_update(struct udev_monitor *udev_monitor)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

int udev_monitor_filter_remove(struct udev_monitor *udev_monitor)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

int udev_monitor_filter_add_match_subsystem_devtype(struct udev_monitor *udev_monitor, const char *subsystem, const char *devtype)
{
    if (!udev_monitor || !subsystem) {
        return -1;
    }

    udev_list_entry_add(&udev_monitor->subsystem_match, subsystem, NULL);

    if (devtype) {
        udev_list_entry_add(&udev_monitor->devtype_match, devtype, NULL);
    }

    return 0;
}

int udev_monitor_filter_add_match_tag(struct udev_monitor *udev_monitor, const char *tag)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

struct udev_monitor *udev_monitor_new_from_netlink(struct udev *udev, const char *name)
{
    struct udev_monitor *udev_monitor;
    struct stat st;

    if (!udev || !name) {
        return NULL;
    }

    udev_monitor = calloc(1, sizeof(struct udev_monitor));

    if (!udev_monitor) {
        return NULL;
    }

    if (lstat(UDEV_MONITOR_FIFO, &st) != 0) {
        umask(0);
        mkfifo(UDEV_MONITOR_FIFO, 0666);
    }
    else if (!S_ISFIFO(st.st_mode)) {
        // someone trying to do something bad. haxor ?
        free(udev_monitor);
        return NULL;
    }

    udev_monitor->fd = open(UDEV_MONITOR_FIFO, O_RDONLY | O_CLOEXEC | O_NONBLOCK);

    if (udev_monitor->fd == -1) {
        free(udev_monitor);
        return NULL;
    }

    udev_monitor->refcount = 1;
    udev_monitor->udev = udev;

    udev_list_entry_init(&udev_monitor->devtype_match);
    udev_list_entry_init(&udev_monitor->subsystem_match);

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
