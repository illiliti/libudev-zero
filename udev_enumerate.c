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

int udev_enumerate_add_match_subsystem(struct udev_enumerate *udev_enumerate, const char *subsystem)
{
    return udev_enumerate ? !!udev_list_entry_add(&udev_enumerate->subsystem_match, subsystem, NULL, 0) - 1 : -1;
}

int udev_enumerate_add_nomatch_subsystem(struct udev_enumerate *udev_enumerate, const char *subsystem)
{
    return udev_enumerate ? !!udev_list_entry_add(&udev_enumerate->subsystem_nomatch, subsystem, NULL, 0) - 1 : -1;
}

int udev_enumerate_add_match_sysattr(struct udev_enumerate *udev_enumerate, const char *sysattr, const char *value)
{
    return udev_enumerate ? !!udev_list_entry_add(&udev_enumerate->sysattr_match, sysattr, value, 0) - 1 : -1;
}

int udev_enumerate_add_nomatch_sysattr(struct udev_enumerate *udev_enumerate, const char *sysattr, const char *value)
{
    return udev_enumerate ? !!udev_list_entry_add(&udev_enumerate->sysattr_nomatch, sysattr, value, 0) - 1 : -1;
}

int udev_enumerate_add_match_property(struct udev_enumerate *udev_enumerate, const char *property, const char *value)
{
    return udev_enumerate ? !!udev_list_entry_add(&udev_enumerate->property_match, property, value, 0) - 1 : -1;
}

int udev_enumerate_add_match_sysname(struct udev_enumerate *udev_enumerate, const char *sysname)
{
    return udev_enumerate ? !!udev_list_entry_add(&udev_enumerate->sysname_match, sysname, NULL, 0) - 1 : -1;
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

static int filter_sysname(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
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

static int filter_property(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
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

            if (value && value2) {
                if (fnmatch(property, property2, 0) == 0 &&
                    fnmatch(value, value2, 0) == 0) {
                    return 1;
                }
            }

            list_entry2 = udev_list_entry_get_next(list_entry2);
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    return 0;
}

static int filter_sysattr(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    const char *sysattr, *value, *value2;
    struct udev_list_entry *list_entry;

    list_entry = udev_list_entry_get_next(&udev_enumerate->sysattr_nomatch);

    while (list_entry) {
        sysattr = udev_list_entry_get_name(list_entry);
        value2 = udev_list_entry_get_value(list_entry);
        value = udev_device_get_sysattr_value(udev_device, sysattr);

        if (value && value2 && fnmatch(value2, value, 0) == 0) {
            return 0;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    list_entry = udev_list_entry_get_next(&udev_enumerate->sysattr_match);

    if (list_entry) {
        while (list_entry) {
            sysattr = udev_list_entry_get_name(list_entry);
            value2 = udev_list_entry_get_value(list_entry);
            value = udev_device_get_sysattr_value(udev_device, sysattr);

            if (value && value2 && fnmatch(value2, value, 0) == 0) {
                return 1;
            }

            list_entry = udev_list_entry_get_next(list_entry);
        }

        return 0;
    }

    return 1;
}

static void add_device(struct udev_enumerate *udev_enumerate, const char *path)
{
    struct udev_device *udev_device;

    udev_device = udev_device_new_from_syspath(udev_enumerate->udev, path);

    if (!udev_device) {
        return;
    }

    if (!filter_subsystem(udev_enumerate, udev_device) ||
        !filter_sysname(udev_enumerate, udev_device) ||
        !filter_property(udev_enumerate, udev_device) ||
        !filter_sysattr(udev_enumerate, udev_device)) {
        udev_device_unref(udev_device);
        return;
    }

    udev_list_entry_add(&udev_enumerate->devices, udev_device_get_syspath(udev_device), NULL, 0);

    udev_device_unref(udev_device);
}

static int filter_dot(const struct dirent *de)
{
    return de->d_name[0] != '.';
}

static int scan_devices(struct udev_enumerate *udev_enumerate, const char *path)
{
    int i, cnt;
    struct dirent **de;

    cnt = scandir(path, &de, filter_dot, NULL);

    if (cnt == -1) {
        return 0;
    }

    for (i = 0; i < cnt; i++) {
        char device_path[PATH_MAX];
        snprintf(device_path, sizeof(device_path), "%s/%s", path, de[i]->d_name);
        add_device(udev_enumerate, device_path);
    }

    for (i = 0; i < cnt; i++) {
        free(de[i]);
    }

    free(de);
    return 1;
}

int udev_enumerate_scan_devices(struct udev_enumerate *udev_enumerate)
{
    int i, cnt, ret;
    struct dirent **de;

    ret = 0;

    if (!udev_enumerate) {
        return -1;
    }

    cnt = scandir("/sys/class", &de, filter_dot, NULL);

    if (cnt == -1)
        return -1;

    for (i = 0; i < cnt; i++) {
        char device_path[PATH_MAX];
        snprintf(device_path, sizeof(device_path), "/sys/class/%s", de[i]->d_name);
        if (!scan_devices(udev_enumerate, device_path)) {
            ret = -1;
        }
    }

    for (i = 0; i < cnt; i++) {
        free(de[i]);
    }

    free(de);
    return ret;
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

    udev_enumerate = calloc(1, sizeof(*udev_enumerate));

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
