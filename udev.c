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

#include <stdlib.h>

#include "udev.h"

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

/* XXX NOT IMPLEMENTED */ struct udev_hwdb *udev_hwdb_new(struct udev *udev)
{
    return NULL;
}

/* XXX NOT IMPLEMENTED */ struct udev_hwdb *udev_hwdb_ref(struct udev_hwdb *hwdb)
{
    return NULL;
}

/* XXX NOT IMPLEMENTED */ struct udev_hwdb *udev_hwdb_unref(struct udev_hwdb *hwdb)
{
    return NULL;
}

/* XXX NOT IMPLEMENTED */ struct udev_list_entry *udev_hwdb_get_properties_list_entry(struct udev_hwdb *hwdb, const char *modalias, unsigned int flags)
{
    return NULL;
}
