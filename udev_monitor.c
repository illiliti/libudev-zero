#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "udev.h"

struct udev_monitor {
    struct udev *udev;
    int refcount;
    int fd[2];
};

UDEV_EXPORT struct udev_device *udev_monitor_receive_device(struct udev_monitor *udev_monitor)
{
    return (void *)1;
}

UDEV_EXPORT int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor)
{
    return 0;
}

UDEV_EXPORT int udev_monitor_set_receive_buffer_size(struct udev_monitor *udev_monitor, int size)
{
    return 0;
}

UDEV_EXPORT int udev_monitor_get_fd(struct udev_monitor *udev_monitor)
{
    return udev_monitor ? udev_monitor->fd[0] : -1;
}

UDEV_EXPORT struct udev *udev_monitor_get_udev(struct udev_monitor *udev_monitor)
{
    return udev_monitor ? udev_monitor->udev : NULL;
}

UDEV_EXPORT int udev_monitor_filter_update(struct udev_monitor *udev_monitor)
{
    return 0;
}

UDEV_EXPORT int udev_monitor_filter_remove(struct udev_monitor *udev_monitor)
{
    return 0;
}

UDEV_EXPORT int udev_monitor_filter_add_match_subsystem_devtype(struct udev_monitor *udev_monitor, const char *subsystem, const char *devtype)
{
    return 0;
}

UDEV_EXPORT int udev_monitor_filter_add_match_tag(struct udev_monitor *udev_monitor, const char *tag)
{
    return 0;
}

UDEV_EXPORT struct udev_monitor *udev_monitor_new_from_netlink(struct udev *udev, const char *name)
{
    struct udev_monitor *udev_monitor;
    int i;

    if (!udev || !name) {
        return NULL;
    }

    udev_monitor = calloc(1, sizeof(struct udev_monitor));

    if (!udev_monitor) {
        return NULL;
    }

    // TODO better no-op, HELP WANTED !
    if (pipe(udev_monitor->fd) == -1) {
        free(udev_monitor);
        return NULL;
    }

    for (i = 0; i < 2; i++) {
        fcntl(udev_monitor->fd[i], F_SETFD, FD_CLOEXEC | O_NONBLOCK);
    }

    udev_monitor->refcount = 1;
    udev_monitor->udev = udev;
    return udev_monitor;
}

UDEV_EXPORT struct udev_monitor *udev_monitor_ref(struct udev_monitor *udev_monitor)
{
    if (!udev_monitor) {
        return NULL;
    }

    udev_monitor->refcount++;
    return udev_monitor;
}

UDEV_EXPORT struct udev_monitor *udev_monitor_unref(struct udev_monitor *udev_monitor)
{
    int i;

    if (!udev_monitor) {
        return NULL;
    }

    if (--udev_monitor->refcount > 0) {
        return NULL;
    }

    for (i = 0; i < 2; i++) {
        close(udev_monitor->fd[i]);
    }

    free(udev_monitor);
    return NULL;
}
