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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "udev.h"
#include "udev_list.h"

struct udev {
    int refcount;
};

struct udev *udev_new(void)
{
    struct udev *udev;

    udev = calloc(1, sizeof(*udev));

    if (!udev) {
        return NULL;
    }

    udev->refcount = 1;
    return udev;
}

struct udev *udev_ref(struct udev *udev)
{
    if (!udev) {
        return NULL;
    }

    udev->refcount++;
    return udev;
}

struct udev *udev_unref(struct udev *udev)
{
    if (!udev) {
        return NULL;
    }

    if (--udev->refcount > 0) {
        return udev;
    }

    free(udev);
    return NULL;
}

void udev_set_log_fn(struct udev *udev, void (*log_fn)(struct udev *udev,
            int priority, const char *file, int line, const char *fn,
            const char *format, va_list args))
{
}

void udev_set_log_priority(struct udev *udev, int priority)
{
}

int udev_get_log_priority(struct udev *udev)
{
    return 0;
}

struct udev_hwdb {
    struct udev_list_entry head;
    int refcount;
};

static int usb_ids_lookup_vendor(uint16_t vendorid, char *buf, size_t buflen)
{
    char *line = NULL, *name;
    size_t cap = 0, len;
    unsigned int id;
    FILE *f;

    f = fopen(USB_IDS_PATH, "r");
    if (!f) {
        return 0;
    }
    while (getline(&line, &cap, f) != -1) {
        if (line[0] == '#' || line[0] == '\t' || line[0] == '\n') {
            continue;
        }
        if (sscanf(line, "%04x", &id) != 1 || id != vendorid) {
            continue;
        }
        name = strchr(line, ' ');
        if (name) {
            while (*name == ' ') {
                name++;
            }
            len = strlen(name);
            while (len > 0 && (name[len - 1] == '\n' || name[len - 1] == '\r')) {
                name[--len] = '\0';
            }
            snprintf(buf, buflen, "%s", name);
        }
        break;
    }
    free(line);
    fclose(f);
    return buf[0] != '\0';
}

static int usb_ids_lookup_product(uint16_t vendorid, uint16_t productid, char *buf, size_t buflen)
{
    char *line = NULL, *name;
    size_t cap = 0, len;
    int in_vendor = 0;
    unsigned int id;
    FILE *f;

    f = fopen(USB_IDS_PATH, "r");
    if (!f) {
        return 0;
    }
    while (getline(&line, &cap, f) != -1) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        if (in_vendor) {
            if (line[0] != '\t') {
                break;
            }
            if (line[1] == '\t') {
                continue;
            }
            if (sscanf(line + 1, "%04x", &id) != 1 || id != productid) {
                continue;
            }
            name = strchr(line + 1, ' ');
            if (name) {
                while (*name == ' ') {
                    name++;
                }
                len = strlen(name);
                while (len > 0 && (name[len - 1] == '\n' || name[len - 1] == '\r')) {
                    name[--len] = '\0';
                }
                snprintf(buf, buflen, "%s", name);
            }
            break;
        }
        if (line[0] == '\t') {
            continue;
        }
        if (sscanf(line, "%04x", &id) != 1 || id != vendorid) {
            continue;
        }
        in_vendor = 1;
    }
    free(line);
    fclose(f);
    return buf[0] != '\0';
}

struct udev_hwdb *udev_hwdb_new(struct udev *udev)
{
    struct udev_hwdb *hwdb;

    hwdb = calloc(1, sizeof(*hwdb));
    if (!hwdb) {
        return NULL;
    }
    hwdb->refcount = 1;
    return hwdb;
}

struct udev_hwdb *udev_hwdb_ref(struct udev_hwdb *hwdb)
{
    if (!hwdb) {
        return NULL;
    }
    hwdb->refcount++;
    return hwdb;
}

struct udev_hwdb *udev_hwdb_unref(struct udev_hwdb *hwdb)
{
    if (!hwdb) {
        return NULL;
    }
    if (--hwdb->refcount > 0) {
        return NULL;
    }
    udev_list_entry_free_all(&hwdb->head);
    free(hwdb);
    return NULL;
}

struct udev_list_entry *udev_hwdb_get_properties_list_entry(struct udev_hwdb *hwdb, const char *modalias, unsigned int flags)
{
    unsigned int vendorid, productid;
    char name[64];

    if (!hwdb || !modalias) {
        return NULL;
    }
    udev_list_entry_free_all(&hwdb->head);
    hwdb->head.next = NULL;
    name[0] = '\0';
    if (sscanf(modalias, "usb:v%4xp%4x", &vendorid, &productid) == 2) {
        if (usb_ids_lookup_product((uint16_t)vendorid, (uint16_t)productid, name, sizeof(name))) {
            udev_list_entry_add(&hwdb->head, "ID_MODEL_FROM_DATABASE", name, 0);
        }
    } else if (sscanf(modalias, "usb:v%4x", &vendorid) == 1) {
        if (usb_ids_lookup_vendor((uint16_t)vendorid, name, sizeof(name))) {
            udev_list_entry_add(&hwdb->head, "ID_VENDOR_FROM_DATABASE", name, 0);
        }
    }
    return hwdb->head.next;
}
