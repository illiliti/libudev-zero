#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <fnmatch.h>
#include <pthread.h>

#include "udev.h"
#include "udev_list.h"

struct udev_enumerate {
    struct udev_list_entry subsystem_nomatch;
    struct udev_list_entry subsystem_match;
    struct udev_list_entry sysattr_nomatch;
    struct udev_list_entry property_match;
    struct udev_list_entry sysattr_match;
    struct udev_list_entry sysname_match;
    struct udev_list_entry devices;
    struct udev *udev;
    int refcount;
};

struct udev_enumerate_thread {
    struct udev_enumerate *udev_enumerate;
    pthread_mutex_t *mutex;
    pthread_t thread;
    const char *name;
    const char *path;
};

int udev_enumerate_add_match_subsystem(struct udev_enumerate *udev_enumerate, const char *subsystem)
{
    if (!udev_enumerate || !subsystem) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->subsystem_match, subsystem, NULL, 0) ? 0 : -1;
}

int udev_enumerate_add_nomatch_subsystem(struct udev_enumerate *udev_enumerate, const char *subsystem)
{
    if (!udev_enumerate || !subsystem) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->subsystem_nomatch, subsystem, NULL, 0) ? 0 : -1;
}

int udev_enumerate_add_match_sysattr(struct udev_enumerate *udev_enumerate, const char *sysattr, const char *value)
{
    if (!udev_enumerate || !sysattr) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->sysattr_match, sysattr, value, 0) ? 0 : -1;
}

int udev_enumerate_add_nomatch_sysattr(struct udev_enumerate *udev_enumerate, const char *sysattr, const char *value)
{
    if (!udev_enumerate || !sysattr) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->sysattr_nomatch, sysattr, value, 0) ? 0 : -1;
}

int udev_enumerate_add_match_property(struct udev_enumerate *udev_enumerate, const char *property, const char *value)
{
    if (!udev_enumerate || !property) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->property_match, property, value, 0) ? 0 : -1;
}

int udev_enumerate_add_match_sysname(struct udev_enumerate *udev_enumerate, const char *sysname)
{
    if (!udev_enumerate || !sysname) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->sysname_match, sysname, NULL, 0) ? 0 : -1;
}

/* XXX NOT IMPLEMENTED */ int udev_enumerate_add_match_tag(struct udev_enumerate *udev_enumerate, const char *tag)
{
    return 0;
}

/* XXX NOT IMPLEMENTED */ int udev_enumerate_add_match_parent(struct udev_enumerate *udev_enumerate, struct udev_device *parent)
{
    return 0;
}

/* XXX NOT IMPLEMENTED */ int udev_enumerate_add_match_is_initialized(struct udev_enumerate *udev_enumerate)
{
    return 0;
}

static int udev_enumerate_filter_subsystem(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *subsystem;

    subsystem = udev_device_get_subsystem(udev_device);
    list_entry = udev_list_entry_get_next(&udev_enumerate->subsystem_nomatch);

    if (!subsystem) {
        return 0;
    }

    while (list_entry) {
        if (fnmatch(udev_list_entry_get_name(list_entry), subsystem, 0) == 0) {
            return 0;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    list_entry = udev_list_entry_get_next(&udev_enumerate->subsystem_match);

    if (list_entry) {
        while (list_entry) {
            if (fnmatch(udev_list_entry_get_name(list_entry), subsystem, 0) == 0) {
                return 1;
            }

            list_entry = udev_list_entry_get_next(list_entry);
        }

        return 0;
    }

    return 1;
}

static int udev_enumerate_filter_sysname(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *sysname;

    sysname = udev_device_get_sysname(udev_device);
    list_entry = udev_list_entry_get_next(&udev_enumerate->sysname_match);

    if (!list_entry) {
        return 1;
    }

    while (list_entry) {
        if (fnmatch(udev_list_entry_get_name(list_entry), sysname, 0) == 0) {
            return 1;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    return 0;
}

static int udev_enumerate_filter_property(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    const char *property, *property2, *value, *value2;
    struct udev_list_entry *list_entry, *list_entry2;

    list_entry = udev_list_entry_get_next(&udev_enumerate->property_match);

    if (!list_entry) {
        return 1;
    }

    while (list_entry) {
        property = udev_list_entry_get_name(list_entry);
        value = udev_list_entry_get_value(list_entry);

        list_entry2 = udev_device_get_properties_list_entry(udev_device);

        if (!list_entry2) {
            return 0;
        }

        while (list_entry2) {
            property2 = udev_list_entry_get_name(list_entry2);
            value2 = udev_list_entry_get_value(list_entry2);

            if (!value || !value2) {
                continue;
            }

            if (fnmatch(property, property2, 0) == 0 &&
                fnmatch(value, value2, 0) == 0) {
                return 1;
            }

            list_entry2 = udev_list_entry_get_next(list_entry2);
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    return 0;
}

static int udev_enumerate_filter_sysattr(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *sysattr, *value;

    list_entry = udev_list_entry_get_next(&udev_enumerate->sysattr_nomatch);

    while (list_entry) {
        sysattr = udev_list_entry_get_name(list_entry);
        value = udev_device_get_sysattr_value(udev_device, sysattr);

        if (!value) {
            return 1;
        }

        if (fnmatch(udev_list_entry_get_value(list_entry), value, 0) == 0) {
            return 0;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    list_entry = udev_list_entry_get_next(&udev_enumerate->sysattr_match);

    if (list_entry) {
        while (list_entry) {
            sysattr = udev_list_entry_get_name(list_entry);
            value = udev_device_get_sysattr_value(udev_device, sysattr);

            if (!value) {
                return 0;
            }

            if (fnmatch(udev_list_entry_get_value(list_entry), value, 0) == 0) {
                return 1;
            }

            list_entry = udev_list_entry_get_next(list_entry);
        }

        return 0;
    }

    return 1;
}

static void *udev_enumerate_add_device(void *ptr)
{
    struct udev_enumerate_thread *data = ptr;
    struct udev_device *udev_device;
    char path[PATH_MAX];

    snprintf(path, sizeof(path), "%s/%s", data->path, data->name);
    udev_device = udev_device_new_from_syspath(data->udev_enumerate->udev, path);

    if (!udev_device) {
        return NULL;
    }

    if (!udev_enumerate_filter_subsystem(data->udev_enumerate, udev_device) ||
        !udev_enumerate_filter_sysname(data->udev_enumerate, udev_device) ||
        !udev_enumerate_filter_property(data->udev_enumerate, udev_device) ||
        !udev_enumerate_filter_sysattr(data->udev_enumerate, udev_device)) {
        udev_device_unref(udev_device);
        return NULL;
    }

    pthread_mutex_lock(data->mutex);
    udev_list_entry_add(&data->udev_enumerate->devices, udev_device_get_syspath(udev_device), NULL, 0);
    pthread_mutex_unlock(data->mutex);

    udev_device_unref(udev_device);
    return NULL;
}

static int udev_enumerate_filter_dots(const struct dirent *de)
{
    if (strcmp(de->d_name, ".") == 0 ||
        strcmp(de->d_name, "..") == 0) {
        return 0;
    }

    return 1;
}

int udev_enumerate_scan_devices(struct udev_enumerate *udev_enumerate)
{
    const char *path[] = { "/sys/dev/block", "/sys/dev/char", NULL };
    struct udev_enumerate_thread *data;
    pthread_mutex_t mutex;
    struct dirent **de;
    int cnt, i, u;

    pthread_mutex_init(&mutex, NULL);

    for (i = 0; path[i]; i++) {
        cnt = scandir(path[i], &de, udev_enumerate_filter_dots, NULL);

        if (cnt == -1) {
            continue;
        }

        data = calloc(cnt, sizeof(struct udev_enumerate_thread));

        if (!data) {
            for (u = 0; u < cnt; u++) {
                free(de[u]);
            }

            free(de);
            continue;
        }

        // TODO do we really need structure for every thread ?
        for (u = 0; u < cnt; u++) {
            data[u].path = path[i];
            data[u].name = de[u]->d_name;
            data[u].mutex = &mutex;
            data[u].udev_enumerate = udev_enumerate;

            pthread_create(&data[u].thread, NULL, udev_enumerate_add_device, &data[u]);
        }

        for (u = 0; u < cnt; u++) {
            pthread_join(data[u].thread, NULL);
        }

        for (u = 0; u < cnt; u++) {
            free(de[u]);
        }

        free(de);
        free(data);
    }

    pthread_mutex_destroy(&mutex);
    return 0;
}

/* XXX NOT IMPLEMENTED */ int udev_enumerate_scan_subsystems(struct udev_enumerate *udev_enumerate)
{
    return 0;
}

struct udev_list_entry *udev_enumerate_get_list_entry(struct udev_enumerate *udev_enumerate)
{
    return udev_enumerate ? udev_list_entry_get_next(&udev_enumerate->devices) : NULL;
}

/* XXX NOT IMPLEMENTED */ int udev_enumerate_add_syspath(struct udev_enumerate *udev_enumerate, const char *syspath)
{
    return 0;
}

struct udev *udev_enumerate_get_udev(struct udev_enumerate *udev_enumerate)
{
    return udev_enumerate ? udev_enumerate->udev : NULL;
}

struct udev_enumerate *udev_enumerate_new(struct udev *udev)
{
    struct udev_enumerate *udev_enumerate;

    if (!udev) {
        return NULL;
    }

    udev_enumerate = calloc(1, sizeof(struct udev_enumerate));

    if (!udev_enumerate) {
        return NULL;
    }

    udev_enumerate->refcount = 1;
    udev_enumerate->udev = udev;

    udev_list_entry_init(&udev_enumerate->subsystem_nomatch);
    udev_list_entry_init(&udev_enumerate->subsystem_match);
    udev_list_entry_init(&udev_enumerate->sysattr_nomatch);
    udev_list_entry_init(&udev_enumerate->property_match);
    udev_list_entry_init(&udev_enumerate->sysattr_match);
    udev_list_entry_init(&udev_enumerate->sysname_match);
    udev_list_entry_init(&udev_enumerate->devices);

    return udev_enumerate;
}

struct udev_enumerate *udev_enumerate_ref(struct udev_enumerate *udev_enumerate)
{
    if (!udev_enumerate) {
        return NULL;
    }

    udev_enumerate->refcount++;
    return udev_enumerate;
}

struct udev_enumerate *udev_enumerate_unref(struct udev_enumerate *udev_enumerate)
{
    if (!udev_enumerate) {
        return NULL;
    }

    if (--udev_enumerate->refcount > 0) {
        return NULL;
    }

    udev_list_entry_free_all(&udev_enumerate->subsystem_nomatch);
    udev_list_entry_free_all(&udev_enumerate->subsystem_match);
    udev_list_entry_free_all(&udev_enumerate->sysattr_nomatch);
    udev_list_entry_free_all(&udev_enumerate->property_match);
    udev_list_entry_free_all(&udev_enumerate->sysattr_match);
    udev_list_entry_free_all(&udev_enumerate->sysname_match);
    udev_list_entry_free_all(&udev_enumerate->devices);

    free(udev_enumerate);
    return NULL;
}
