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

struct udev_list_entry {
    struct udev_list_entry *next;
    char *value;
    char *name;
};

void udev_list_entry_init(struct udev_list_entry *list_entry);
void udev_list_entry_free(struct udev_list_entry *list_entry);
void udev_list_entry_free_all(struct udev_list_entry *list_entry);
struct udev_list_entry *udev_list_entry_add(struct udev_list_entry *list_entry, const char *name, const char *value, int uniq);
