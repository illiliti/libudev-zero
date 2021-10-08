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

#include <string.h>
#include <stdlib.h>

#include "udev.h"
#include "udev_list.h"

void udev_list_entry_init(struct udev_list_entry *list_entry)
{
    list_entry->value = NULL;
    list_entry->name = NULL;
    list_entry->next = NULL;
}

void udev_list_entry_free(struct udev_list_entry *list_entry)
{
    free(list_entry->value);
    free(list_entry->name);
    free(list_entry);
}

void udev_list_entry_free_all(struct udev_list_entry *list_entry)
{
    struct udev_list_entry *list_entry2 = list_entry->next;

    while (list_entry2) {
        list_entry = list_entry2;
        list_entry2 = list_entry->next;
        udev_list_entry_free(list_entry);
    }
}

struct udev_list_entry *udev_list_entry_add(struct udev_list_entry *list_entry, const char *name, const char *value, int uniq)
{
    struct udev_list_entry *list_entry2;

    if (uniq) {
        list_entry2 = udev_list_entry_get_by_name(list_entry, name);

        if (list_entry2 && value) {
            if (list_entry2->value && strcmp(list_entry2->value, value) == 0) {
                return list_entry2;
            }

            free(list_entry2->value);
            list_entry2->value = strdup(value);

            if (!list_entry2->value) {
                return NULL;
            }

            return list_entry2;
        }
    }

    list_entry2 = calloc(1, sizeof(*list_entry2));

    if (!list_entry2) {
        return NULL;
    }

    list_entry2->name = strdup(name);

    if (!list_entry2->name) {
        free(list_entry2);
        return NULL;
    }

    if (value) {
        list_entry2->value = strdup(value);

        if (!list_entry2->value) {
            free(list_entry2->name);
            free(list_entry2);
            return NULL;
        }
    }

    list_entry2->next = list_entry->next;
    list_entry->next = list_entry2;
    return list_entry2;
}

struct udev_list_entry *udev_list_entry_get_next(struct udev_list_entry *list_entry)
{
    return list_entry ? list_entry->next : NULL;
}

struct udev_list_entry *udev_list_entry_get_by_name(struct udev_list_entry *list_entry, const char *name)
{
    if (!list_entry || !name) {
        return NULL;
    }

    do {
        if (list_entry->name && strcmp(list_entry->name, name) == 0) {
            return list_entry;
        }
    }
    while ((list_entry = list_entry->next));

    return NULL;
}

const char *udev_list_entry_get_name(struct udev_list_entry *list_entry)
{
    return list_entry ? list_entry->name : NULL;
}

const char *udev_list_entry_get_value(struct udev_list_entry *list_entry)
{
    return list_entry ? list_entry->value : NULL;
}
