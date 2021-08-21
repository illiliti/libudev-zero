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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "udev.h"
#include "udev_list.h"

struct udev_list_table {
    int (*cmp)(const char *, const char *);
    struct udev_list_entry **entries;
    struct udev_list_entry *head;
    size_t cap;
    size_t len;
};

struct udev_list_entry {
    struct udev_list_table *list_table;
    struct udev_list_entry *next;
    uint64_t sum;
    char *value;
    char *name;
};

void udev_list_table_deinit(struct udev_list_table *list_table)
{
    struct udev_list_entry *list_entry;
    size_t i;

    // TODO iterate over linked list?
    for (i = 0; i < list_table->cap; i++) {
        list_entry = list_table->entries[i];

        if (!list_entry) {
            continue;
        }

        free(list_entry->value);
        free(list_entry->name);
        free(list_entry);
    }

    free(list_table->entries);
    free(list_table);
}

struct udev_list_table *udev_list_table_init(size_t cap, int (*cmp)(const char *, const char *))
{
    struct udev_list_table *list_table;

    list_table = calloc(1, sizeof(struct udev_list_table));

    if (!list_table) {
        return NULL;
    }

    list_table->entries = calloc(cap, sizeof(struct udev_list_entry *));

    if (!list_table->entries) {
        return NULL;
    }

    list_table->cmp = cmp;
    list_table->cap = cap;
    list_table->len = 0;
    return list_table;
}

static uint64_t hash_murmur64a(const char *str)
{
    const uint64_t seed = 0x3a1ddf174ff104c3;
    const uint64_t m = 0xc6a4a7935bd1e995;
    const uint64_t *data, *end;
    const unsigned char *data2;
    const int r = 47;
    uint64_t h, k;
    size_t len;

    len = strlen(str);
    h = seed ^ (len * m);
    data = (const uint64_t *)str;
    end = data + (len / 8);

    while (data != end) {
        k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    data2 = (const unsigned char *)data;

    switch (len & 0x7) {
    case 7: h ^= (uint64_t)data2[6] << 48;
            // fallthrough
    case 6: h ^= (uint64_t)data2[5] << 40;
            // fallthrough
    case 5: h ^= (uint64_t)data2[4] << 32;
            // fallthrough
    case 4: h ^= (uint64_t)data2[3] << 24;
            // fallthrough
    case 3: h ^= (uint64_t)data2[2] << 16;
            // fallthrough
    case 2: h ^= (uint64_t)data2[1] << 8;
            // fallthrough
    case 1: h ^= (uint64_t)data2[0];
            h *= m;
    }

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

static int udev_list_table_expand(struct udev_list_table *list_table)
{
    struct udev_list_entry *list_entry;
    struct udev_list_entry **entries;
    size_t cap, i, j;

    cap = list_table->cap * 2;

    // TODO cap overflow check

    entries = calloc(cap, sizeof(struct udev_list_entry *));

    if (!entries) {
        return 0;
    }

    // TODO iterate over linked list?
    for (i = 0; i < list_table->cap; i++) {
        list_entry = list_table->entries[i];

        if (!list_entry) {
            continue;
        }

        // TODO ensure that endless loop is not possible
        for (j = list_entry->sum % cap; entries[j]; j++) {
            if (j == cap) {
                j = 0;
            }
        }

        entries[j] = list_entry;
    }

    free(list_table->entries);
    list_table->entries = entries;
    list_table->cap = cap;
    return 1;
}

struct udev_list_entry *udev_list_entry_add(struct udev_list_table *list_table, const char *name, const char *value)
{
    struct udev_list_entry *list_entry;
    uint64_t sum;
    char *val;
    size_t i;

    if (list_table->len > list_table->cap / 2) {
        if (!udev_list_table_expand(list_table)) {
            return NULL;
        }
    }

    sum = hash_murmur64a(name);

    for (i = sum % list_table->cap; list_table->entries[i]; i++) {
        if (i == list_table->cap) {
            i = 0;
            continue;
        }

        list_entry = list_table->entries[i];

        // TODO compare sum?
        if (list_table->cmp(list_entry->name, name) != 0) {
            continue;
        }

        if (!list_entry->value || list_table->cmp(list_entry->value, value) == 0) {
            return list_entry;
        }

        val = strdup(value);

        if (!val) {
            return list_entry; // TODO NULL?
        }

        free(list_entry->value);
        list_entry->value = val;
        return list_entry;
    }

    list_entry = calloc(1, sizeof(struct udev_list_entry));

    if (!list_entry) {
        return NULL;
    }

    list_entry->name = strdup(name);

    if (!list_entry->name) {
        return NULL;
    }

    if (value) {
        list_entry->value = strdup(value);

        if (!list_entry->value) {
            return NULL;
        }
    }

    list_entry->next = list_table->head;
    list_table->head = list_entry;

    list_table->entries[i] = list_entry;
    list_table->len++;

    list_entry->list_table = list_table;
    list_entry->sum = sum;
    return list_entry;
}

struct udev_list_entry *udev_list_table_get_head(struct udev_list_table *list_table)
{
    return list_table ? list_table->head : NULL;
}

struct udev_list_entry *udev_list_entry_get_next(struct udev_list_entry *list_entry)
{
    return list_entry ? list_entry->next : NULL;
}

struct udev_list_entry *udev_list_entry_get_by_name(struct udev_list_entry *list_entry, const char *name)
{
    struct udev_list_table *list_table;
    uint64_t sum;
    size_t i;

    if (!list_entry || !name) {
        return NULL;
    }

    sum = hash_murmur64a(name);
    list_table = list_entry->list_table;

    for (i = sum % list_table->cap; list_table->entries[i]; i++) {
        if (i == list_table->cap) {
            i = 0;
            continue;
        }

        list_entry = list_table->entries[i];

        // TODO compare sum?
        if (list_table->cmp(list_entry->name, name) == 0) {
            return list_entry;
        }
    }

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
