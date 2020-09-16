#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/inotify.h>

#include "udev.h"
#include "udev_list.h"

#ifndef UDEV_MONITOR_DIR
#define UDEV_MONITOR_DIR "/tmp/.libudev-zero"
#endif

enum { THREADS_MAX = 5 };

struct udev_monitor {
    struct udev_list_entry subsystem_match;
    struct udev_list_entry devtype_match;
    pthread_t thread[THREADS_MAX];
    pthread_barrier_t barrier;
    struct udev *udev;
    int refcount;
    int sfd[2];
    int ifd;
    int efd;
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

    if (!udev_monitor_filter_subsystem(udev_monitor, udev_device) ||
        !udev_monitor_filter_devtype(udev_monitor, udev_device)) {
        udev_device_unref(udev_device);
        return NULL;
    }

    return udev_device;
}

static void *udev_monitor_handle_event(void *ptr)
{
    struct udev_monitor *udev_monitor = ptr;
    struct inotify_event *event;
    struct epoll_event epoll;
    char data[4096];
    sigset_t mask;
    ssize_t len;
    int i;

    sigemptyset(&mask);

    while (epoll_pwait(udev_monitor->efd, &epoll, 1, -1, &mask) != -1) {
        len = read(udev_monitor->ifd, data, sizeof(data));

        if (len == -1) {
            continue;
        }

        for (i = 0; i < len; i += sizeof(struct inotify_event) + event->len) {
            event = (struct inotify_event *)&data[i];

            // TODO user deleted directory, what should we do ?
            if (event->mask & IN_IGNORED) {
                break;
            }

            send(udev_monitor->sfd[1], event->name, event->len, 0);
        }
    }

    pthread_barrier_wait(&udev_monitor->barrier);
    return NULL;
}

int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor)
{
    pthread_attr_t attr;
    int i;

    if (!udev_monitor) {
        return -1;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_barrier_init(&udev_monitor->barrier, NULL, THREADS_MAX + 1);

    for (i = 0; i < THREADS_MAX; i++) {
        pthread_create(&udev_monitor->thread[i], &attr, udev_monitor_handle_event, udev_monitor);
    }

    pthread_attr_destroy(&attr);
    return 0;
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
    struct epoll_event epoll;
    struct stat st;

    if (!udev || !name) {
        return NULL;
    }

    udev_monitor = calloc(1, sizeof(struct udev_monitor));

    if (!udev_monitor) {
        return NULL;
    }

    if (lstat(UDEV_MONITOR_DIR, &st) != 0) {
        if (mkdir(UDEV_MONITOR_DIR, 0) == -1 ||
            chmod(UDEV_MONITOR_DIR, 0777) == -1) {
            free(udev_monitor);
            return NULL;
        }
    }
    else if (!S_ISDIR(st.st_mode)) {
        free(udev_monitor);
        return NULL;
    }

    udev_monitor->efd = epoll_create1(EPOLL_CLOEXEC);

    if (udev_monitor->efd == -1) {
        free(udev_monitor);
        return NULL;
    }

    udev_monitor->ifd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);

    if (udev_monitor->ifd == -1) {
        close(udev_monitor->efd);
        free(udev_monitor);
        return NULL;
    }

    if (inotify_add_watch(udev_monitor->ifd, UDEV_MONITOR_DIR, IN_CLOSE_WRITE | IN_EXCL_UNLINK) == -1) {
        close(udev_monitor->ifd);
        close(udev_monitor->efd);
        free(udev_monitor);
        return NULL;
    }

    epoll.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(udev_monitor->efd, EPOLL_CTL_ADD, udev_monitor->ifd, &epoll) == -1) {
        close(udev_monitor->ifd);
        close(udev_monitor->efd);
        free(udev_monitor);
        return NULL;
    }

    if (socketpair(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, udev_monitor->sfd) == -1) {
        close(udev_monitor->ifd);
        close(udev_monitor->efd);
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
    int i;

    if (!udev_monitor) {
        return NULL;
    }

    if (--udev_monitor->refcount > 0) {
        return NULL;
    }

    udev_list_entry_free_all(&udev_monitor->subsystem_match);
    udev_list_entry_free_all(&udev_monitor->devtype_match);

    for (i = 0; i < THREADS_MAX; i++) {
        pthread_cancel(udev_monitor->thread[i]);
    }

    pthread_barrier_wait(&udev_monitor->barrier);
    pthread_barrier_destroy(&udev_monitor->barrier);

    for (i = 0; i < 2; i++) {
        close(udev_monitor->sfd[i]);
    }

    close(udev_monitor->ifd);
    close(udev_monitor->efd);
    free(udev_monitor);
    return NULL;
}
