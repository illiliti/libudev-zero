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
    struct udev_list_table *subsystem_nomatch;
    struct udev_list_table *subsystem_match;
    struct udev_list_table *sysattr_nomatch;
    struct udev_list_table *property_match;
    struct udev_list_table *sysattr_match;
    struct udev_list_table *sysname_match;
    struct udev_list_table *devices;
    struct udev *udev;
    int refcount;
};

struct udev_enumerate_thread {
    struct udev_enumerate *udev_enumerate;
    pthread_mutex_t *mutex;
    char path[PATH_MAX];
    pthread_t thread;
};

int udev_enumerate_add_match_subsystem(struct udev_enumerate *udev_enumerate, const char *subsystem)
{
    return udev_enumerate ? !!udev_list_entry_add(udev_enumerate->subsystem_match, subsystem, NULL) - 1 : -1;
}

int udev_enumerate_add_nomatch_subsystem(struct udev_enumerate *udev_enumerate, const char *subsystem)
{
    return udev_enumerate ? !!udev_list_entry_add(udev_enumerate->subsystem_nomatch, subsystem, NULL) - 1 : -1;
}

int udev_enumerate_add_match_sysattr(struct udev_enumerate *udev_enumerate, const char *sysattr, const char *value)
{
    return udev_enumerate ? !!udev_list_entry_add(udev_enumerate->sysattr_match, sysattr, value) - 1 : -1;
}

int udev_enumerate_add_nomatch_sysattr(struct udev_enumerate *udev_enumerate, const char *sysattr, const char *value)
{
    return udev_enumerate ? !!udev_list_entry_add(udev_enumerate->sysattr_nomatch, sysattr, value) - 1 : -1;
}

int udev_enumerate_add_match_property(struct udev_enumerate *udev_enumerate, const char *property, const char *value)
{
    return udev_enumerate ? !!udev_list_entry_add(udev_enumerate->property_match, property, value) - 1 : -1;
}

int udev_enumerate_add_match_sysname(struct udev_enumerate *udev_enumerate, const char *sysname)
{
    return udev_enumerate ? !!udev_list_entry_add(udev_enumerate->sysname_match, sysname, NULL) - 1 : -1;
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

static int filter_subsystem(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *subsystem;

    subsystem = udev_device_get_subsystem(udev_device);

    if (!subsystem) {
        return 0;
    }

    list_entry = udev_list_table_get_head(udev_enumerate->subsystem_nomatch);

    if (udev_list_entry_get_by_name(list_entry, subsystem)) {
        return 0;
    }

    list_entry = udev_list_table_get_head(udev_enumerate->subsystem_match);

    if (!list_entry) {
        return 0;
    }

    return !!udev_list_entry_get_by_name(list_entry, subsystem);
}

static int filter_sysname(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;

    list_entry = udev_list_table_get_head(udev_enumerate->sysname_match);

    if (!list_entry) {
        return 1;
    }

    return !!udev_list_entry_get_by_name(list_entry, udev_device_get_sysname(udev_device));
}

static int strmatch(const char *pat, const char *str)
{
    return fnmatch(pat, str, 0);
}

static int filter_property(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry_match, *list_entry_properties, *list_entry_by_name;
    const char *property, *value, *value_by_name;

    list_entry_match = udev_list_table_get_head(udev_enumerate->property_match);

    if (!list_entry_match) {
        return 1;
    }

    list_entry_properties = udev_device_get_properties_list_entry(udev_device);

    if (!list_entry_properties) {
        return 0;
    }

    while (list_entry_properties) {
        property = udev_list_entry_get_name(list_entry_properties);
        value = udev_list_entry_get_value(list_entry_properties);

        list_entry_by_name = udev_list_entry_get_by_name(list_entry_match, property);

        if (list_entry_by_name) {
            value_by_name = udev_list_entry_get_value(list_entry_by_name);

            if (value_by_name && value) {
                if (strmatch(value_by_name, value) == 0) {
                    return 1;
                }
            }
        }

        list_entry_properties = udev_list_entry_get_next(list_entry_properties);
    }

    return 0;
}

static int filter_sysattr(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *sysattr, *value;

    list_entry = udev_list_table_get_head(udev_enumerate->sysattr_nomatch);

    while (list_entry) {
        sysattr = udev_list_entry_get_name(list_entry);
        value = udev_device_get_sysattr_value(udev_device, sysattr);

        if (!value) {
            return 1;
        }

        if (strmatch(udev_list_entry_get_value(list_entry), value) == 0) {
            return 0;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    list_entry = udev_list_table_get_head(udev_enumerate->sysattr_match);

    if (!list_entry) {
        return 1;
    }

    while (list_entry) {
        sysattr = udev_list_entry_get_name(list_entry);
        value = udev_device_get_sysattr_value(udev_device, sysattr);

        if (!value) {
            return 0;
        }

        if (strmatch(udev_list_entry_get_value(list_entry), value) == 0) {
            return 1;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    return 0;
}

static void *add_device(void *ptr)
{
    struct udev_enumerate_thread *thread = ptr;
    struct udev_device *udev_device;

    udev_device = udev_device_new_from_syspath(thread->udev_enumerate->udev, thread->path);

    if (!udev_device) {
        return NULL;
    }

    if (!filter_subsystem(thread->udev_enumerate, udev_device) ||
        !filter_sysname(thread->udev_enumerate, udev_device) ||
        !filter_property(thread->udev_enumerate, udev_device) ||
        !filter_sysattr(thread->udev_enumerate, udev_device)) {
        udev_device_unref(udev_device);
        return NULL;
    }

    pthread_mutex_lock(thread->mutex);
    udev_list_entry_add(thread->udev_enumerate->devices, udev_device_get_syspath(udev_device), NULL);
    pthread_mutex_unlock(thread->mutex);

    udev_device_unref(udev_device);
    return NULL;
}

static int filter_dot(const struct dirent *de)
{
    return de->d_name[0] != '.';
}

static int scan_devices(struct udev_enumerate *udev_enumerate, const char *path)
{
    struct udev_enumerate_thread *thread;
    pthread_mutex_t mutex;
    struct dirent **de;
    int cnt, i;

    cnt = scandir(path, &de, filter_dot, NULL);

    if (cnt == -1) {
        return 0;
    }

    thread = calloc(cnt, sizeof(struct udev_enumerate_thread));

    if (!thread) {
        for (i = 0; i < cnt; i++) {
            free(de[i]);
        }

        free(de);
        return 0;
    }

    pthread_mutex_init(&mutex, NULL);

    for (i = 0; i < cnt; i++) {
        thread[i].mutex = &mutex;
        thread[i].udev_enumerate = udev_enumerate;

        snprintf(thread[i].path, sizeof(thread[i].path), "%s/%s", path, de[i]->d_name);

        if (pthread_create(&thread[i].thread, NULL, add_device, &thread[i]) != 0) {
            return 0;
        }
    }

    for (i = 0; i < cnt; i++) {
        pthread_join(thread[i].thread, NULL);
    }

    for (i = 0; i < cnt; i++) {
        free(de[i]);
    }

    free(de);
    free(thread);
    pthread_mutex_destroy(&mutex);
    return 1;
}

int udev_enumerate_scan_devices(struct udev_enumerate *udev_enumerate)
{
    const char *path[] = { "/sys/dev/block", "/sys/dev/char", NULL };
    int i;

    if (!udev_enumerate) {
        return -1;
    }

    for (i = 0; path[i]; i++) {
        if (!scan_devices(udev_enumerate, path[i])) {
            return -1;
        }
    }

    return 0;
}

/* XXX NOT IMPLEMENTED */ int udev_enumerate_scan_subsystems(struct udev_enumerate *udev_enumerate)
{
    return 0;
}

struct udev_list_entry *udev_enumerate_get_list_entry(struct udev_enumerate *udev_enumerate)
{
    return udev_enumerate ? udev_list_table_get_head(udev_enumerate->devices) : NULL;
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

    udev_enumerate->subsystem_nomatch = udev_list_table_init(10, strmatch);

    if (!udev_enumerate->subsystem_nomatch) {
        goto free_enumerate;
    }

    udev_enumerate->subsystem_match = udev_list_table_init(10, strmatch);

    if (!udev_enumerate->subsystem_match) {
        goto deinit_subsystem_nomatch;
    }

    udev_enumerate->sysattr_nomatch = udev_list_table_init(10, strmatch);

    if (!udev_enumerate->sysattr_nomatch) {
        goto deinit_subsystem_match;
    }

    udev_enumerate->property_match = udev_list_table_init(10, strmatch);

    if (!udev_enumerate->property_match) {
        goto deinit_sysattr_nomatch;
    }

    udev_enumerate->sysattr_match = udev_list_table_init(10, strmatch);

    if (!udev_enumerate->sysattr_match) {
        goto deinit_property_match;
    }

    udev_enumerate->sysname_match = udev_list_table_init(10, strmatch);

    if (!udev_enumerate->sysname_match) {
        goto deinit_sysattr_match;
    }

    udev_enumerate->devices = udev_list_table_init(100, strmatch);

    if (!udev_enumerate->devices) {
        goto deinit_sysname_match;
    }

    return udev_enumerate;

deinit_sysname_match:
    udev_list_table_deinit(udev_enumerate->sysname_match);

deinit_sysattr_match:
    udev_list_table_deinit(udev_enumerate->sysattr_match);

deinit_property_match:
    udev_list_table_deinit(udev_enumerate->property_match);

deinit_sysattr_nomatch:
    udev_list_table_deinit(udev_enumerate->sysattr_nomatch);

deinit_subsystem_match:
    udev_list_table_deinit(udev_enumerate->subsystem_match);

deinit_subsystem_nomatch:
    udev_list_table_deinit(udev_enumerate->subsystem_nomatch);

free_enumerate:
    free(udev_enumerate);
    return NULL;
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

    udev_list_table_deinit(udev_enumerate->subsystem_nomatch);
    udev_list_table_deinit(udev_enumerate->subsystem_match);
    udev_list_table_deinit(udev_enumerate->sysattr_nomatch);
    udev_list_table_deinit(udev_enumerate->property_match);
    udev_list_table_deinit(udev_enumerate->sysattr_match);
    udev_list_table_deinit(udev_enumerate->sysname_match);
    udev_list_table_deinit(udev_enumerate->devices);

    free(udev_enumerate);
    return NULL;
}
