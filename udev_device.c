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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <linux/input.h>

#include "udev.h"
#include "udev_list.h"

// sizeof(unsigned long) == 4 * 8 == 32 on 32-bit systems.
// sizeof(unsigned long) == 8 * 8 == 64 on 64-bit systems.
#ifndef LONG_BIT
#define LONG_BIT (sizeof(unsigned long) * 8)
#endif

// https://kernel.org/doc/html/latest/input/ff.html#querying-device-capabilities
// https://kernel.org/doc/html/latest/input/input-programming.html#bits-to-longs-bit-word-bit-mask
#define BIT_WORD(x) ((x) / LONG_BIT)
#define BIT_MASK(x) (1UL << ((x) % LONG_BIT))
#define BITS_TO_LONGS(x) (((x) + LONG_BIT - 1) / LONG_BIT)

#ifndef INPUT_PROP_POINTING_STICK
#define INPUT_PROP_POINTING_STICK 0x05
#endif

#ifndef INPUT_PROP_ACCELEROMETER
#define INPUT_PROP_ACCELEROMETER 0x06
#endif

struct udev_device {
    struct udev_list_entry properties;
    struct udev_list_entry sysattrs;
    struct udev_device *parent;
    struct udev *udev;
    int refcount;
};

const char *udev_device_get_syspath(struct udev_device *udev_device)
{
    return udev_device_get_property_value(udev_device, "SYSPATH");
}

const char *udev_device_get_sysname(struct udev_device *udev_device)
{
    return udev_device_get_property_value(udev_device, "SYSNAME");
}

const char *udev_device_get_sysnum(struct udev_device *udev_device)
{
    return udev_device_get_property_value(udev_device, "SYSNUM");
}

const char *udev_device_get_devpath(struct udev_device *udev_device)
{
    return udev_device_get_property_value(udev_device, "DEVPATH");
}

const char *udev_device_get_devnode(struct udev_device *udev_device)
{
    return udev_device_get_property_value(udev_device, "DEVNAME");
}

unsigned long long udev_device_get_seqnum(struct udev_device *udev_device)
{
    const char *seqnum;

    seqnum = udev_device_get_property_value(udev_device, "SEQNUM");

    if (!seqnum) {
        return 0;
    }

    return strtoull(seqnum, NULL, 10);
}

unsigned long long udev_device_get_usec_since_initialized(struct udev_device *udev_device)
{
    return 0;
}

dev_t udev_device_get_devnum(struct udev_device *udev_device)
{
    const char *major, *minor;

    major = udev_device_get_property_value(udev_device, "MAJOR");
    minor = udev_device_get_property_value(udev_device, "MINOR");

    if (!major || !minor) {
        return makedev(0, 0);
    }

    return makedev(atoi(major), atoi(minor));
}

const char *udev_device_get_devtype(struct udev_device *udev_device)
{
    return udev_device_get_property_value(udev_device, "DEVTYPE");
}

const char *udev_device_get_subsystem(struct udev_device *udev_device)
{
    return udev_device_get_property_value(udev_device, "SUBSYSTEM");
}

const char *udev_device_get_driver(struct udev_device *udev_device)
{
    return udev_device_get_property_value(udev_device, "DRIVER");
}

struct udev *udev_device_get_udev(struct udev_device *udev_device)
{
    return udev_device ? udev_device->udev : NULL;
}

struct udev_device *udev_device_get_parent(struct udev_device *udev_device)
{
    char *pos, *path, *syspath;

    if (!udev_device) {
        return NULL;
    }

    if (udev_device->parent) {
        return udev_device->parent;
    }

    syspath = strdup(udev_device_get_syspath(udev_device));

    if (!syspath) {
        return NULL;
    }

    path = syspath + 5;

    do {
        if ((pos = strrchr(path, '/'))) {
            *pos = '\0';
        }
        else {
            break;
        }

        udev_device->parent = udev_device_new_from_syspath(udev_device->udev, syspath);
    }
    while (!udev_device->parent);

    free(syspath);
    return udev_device->parent;
}

struct udev_device *udev_device_get_parent_with_subsystem_devtype(struct udev_device *udev_device, const char *subsystem, const char *devtype)
{
    const char *parent_subsystem, *parent_devtype;
    struct udev_device *parent;

    if (!udev_device || !subsystem) {
        return NULL;
    }

    for (parent = udev_device_get_parent(udev_device); parent; parent = udev_device_get_parent(parent)) {
        parent_subsystem = udev_device_get_subsystem(parent);
        parent_devtype = udev_device_get_devtype(parent);

        if (!parent_subsystem || strcmp(parent_subsystem, subsystem) != 0) {
            continue;
        }

        if (!devtype || (parent_devtype && strcmp(parent_devtype, devtype) == 0)) {
            return parent;
        }
    }

    return NULL;
}

/* XXX NOT IMPLEMENTED */ int udev_device_get_is_initialized(struct udev_device *udev_device)
{
    return 1;
}

const char *udev_device_get_action(struct udev_device *udev_device)
{
    return udev_device_get_property_value(udev_device, "ACTION");
}

/* XXX NOT IMPLEMENTED */ int udev_device_has_tag(struct udev_device *udev_device, const char *tag)
{
    return 0;
}

/* XXX NOT IMPLEMENTED */ struct udev_list_entry *udev_device_get_devlinks_list_entry(struct udev_device *udev_device)
{
    return NULL;
}

struct udev_list_entry *udev_device_get_properties_list_entry(struct udev_device *udev_device)
{
    return udev_device ? udev_list_entry_get_next(&udev_device->properties) : NULL;
}

/* XXX NOT IMPLEMENTED */ struct udev_list_entry *udev_device_get_tags_list_entry(struct udev_device *udev_device)
{
    return NULL;
}

struct udev_list_entry *udev_device_get_sysattr_list_entry(struct udev_device *udev_device)
{
    return udev_device ? udev_list_entry_get_next(&udev_device->sysattrs) : NULL;
}

const char *udev_device_get_property_value(struct udev_device *udev_device, const char *key)
{
    if (!udev_device || !key) {
        return NULL;
    }

    return udev_list_entry_get_value(udev_list_entry_get_by_name(&udev_device->properties, key));
}

const char *udev_device_get_sysattr_value(struct udev_device *udev_device, const char *sysattr)
{
    struct udev_list_entry *list_entry;
    char data[4096], path[PATH_MAX];
    struct stat st;
    size_t len;
    FILE *file;

    if (!udev_device || !sysattr) {
        return NULL;
    }

    list_entry = udev_list_entry_get_by_name(&udev_device->sysattrs, sysattr);

    if (list_entry) {
        return udev_list_entry_get_value(list_entry);
    }

    snprintf(path, sizeof(path), "%s/%s", udev_device_get_syspath(udev_device), sysattr);

    if (lstat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return NULL;
    }

    file = fopen(path, "r");

    if (!file) {
        return NULL;
    }

    // TODO dynamic allocation of data
    len = fread(data, 1, sizeof(data) - 1, file);

    if (len != sizeof(data) - 1 && ferror(file)) {
        fclose(file);
        return NULL;
    }

    fclose(file);
    data[len] = '\0';

    // TODO strrchr?
    while (len-- > 0 && data[len] == '\n') {
        data[len] = '\0';
    }

    list_entry = udev_list_entry_add(&udev_device->sysattrs, sysattr, data, 0);
    return udev_list_entry_get_value(list_entry);
}

int udev_device_set_sysattr_value(struct udev_device *udev_device, const char *sysattr, const char *value)
{
    char path[PATH_MAX];
    struct stat st;
    size_t len;
    FILE *file;

    if (!udev_device || !sysattr || !value) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", udev_device_get_syspath(udev_device), sysattr);

    if (lstat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return -1;
    }

    file = fopen(path, "w");

    if (!file) {
        return -1;
    }

    len = strlen(value);

    if (fwrite(value, 1, len, file) != len) {
        fclose(file);
        return -1;
    }

    fclose(file);
    udev_list_entry_add(&udev_device->sysattrs, sysattr, value, 1);
    return 0;
}

static char *read_symlink(const char *syspath, const char *name)
{
    char link[PATH_MAX], path[PATH_MAX];
    ssize_t len;

    snprintf(path, sizeof(path), "%s/%s", syspath, name);
    len = readlink(path, link, sizeof(link));

    if (len == -1) {
        return NULL;
    }

    link[len] = '\0';
    return strdup(strrchr(link, '/') + 1);
}

static void set_properties_from_uevent(struct udev_device *udev_device)
{
    char line[LINE_MAX], path[PATH_MAX], devnode[PATH_MAX];
    FILE *file;
    char *pos;

    snprintf(path, sizeof(path), "%s/uevent", udev_device_get_syspath(udev_device));
    file = fopen(path, "r");

    if (!file) {
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        line[strlen(line) - 1] = '\0';

        if (strncmp(line, "DEVNAME=", 8) == 0) {
            snprintf(devnode, sizeof(devnode), "/dev/%s", line + 8);
            udev_list_entry_add(&udev_device->properties, "DEVNAME", devnode, 0);
        }
        else if ((pos = strchr(line, '='))) {
            *pos = '\0';
            udev_list_entry_add(&udev_device->properties, line, pos + 1, 1);
        }
    }

    fclose(file);
}

static void make_bit(unsigned long *arr, int cnt, const char *str)
{
    size_t len;
    int i;

    if (!str) {
        return;
    }

    for (i = 0, len = strlen(str); len > 0; len--) {
        if (str[len] == ' ') {
            arr[i++] = strtoul(str + len + 1, NULL, 16);

            if (i == cnt) {
                return;
            }
        }
    }

    arr[i] = strtoul(str, NULL, 16);
}

static int test_bit(unsigned long *arr, unsigned long bit)
{
    return arr[BIT_WORD(bit)] & BIT_MASK(bit);
}

static void set_properties_from_evdev(struct udev_device *udev_device)
{
    // https://kernel.org/doc/html/latest/driver-api/input.html#c.input_dev
    unsigned long prop_bits[BITS_TO_LONGS(INPUT_PROP_CNT)] = {0};
    unsigned long abs_bits[BITS_TO_LONGS(ABS_CNT)] = {0};
    unsigned long rel_bits[BITS_TO_LONGS(REL_CNT)] = {0};
    unsigned long key_bits[BITS_TO_LONGS(KEY_CNT)] = {0};
    unsigned long ev_bits[BITS_TO_LONGS(EV_CNT)] = {0};
    struct udev_device *parent;
    const char *subsystem;

    subsystem = udev_device_get_subsystem(udev_device);

    if (!subsystem || strcmp(subsystem, "input") != 0) {
        return;
    }

    parent = udev_device;

    while (1) {
        if (!parent) {
            return;
        }

        if (udev_device_get_property_value(parent, "EV")) {
            break;
        }

        parent = udev_device_get_parent_with_subsystem_devtype(parent, "input", NULL);
    }

    make_bit(ev_bits, sizeof(ev_bits) / sizeof(ev_bits[0]), udev_device_get_property_value(parent, "EV"));
    make_bit(abs_bits, sizeof(abs_bits) / sizeof(abs_bits[0]), udev_device_get_property_value(parent, "ABS"));
    make_bit(rel_bits, sizeof(rel_bits) / sizeof(rel_bits[0]), udev_device_get_property_value(parent, "REL"));
    make_bit(key_bits, sizeof(key_bits) / sizeof(key_bits[0]), udev_device_get_property_value(parent, "KEY"));
    make_bit(prop_bits, sizeof(prop_bits) / sizeof(prop_bits[0]), udev_device_get_property_value(parent, "PROP"));

    udev_list_entry_add(&udev_device->properties, "ID_INPUT", "1", 0);

    if (test_bit(prop_bits, INPUT_PROP_POINTING_STICK)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_POINTINGSTICK", "1", 0);
    }

    if (test_bit(prop_bits, INPUT_PROP_ACCELEROMETER)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_ACCELEROMETER", "1", 0);
    }

    if (test_bit(ev_bits, EV_SW)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_SWITCH", "1", 0);
    }

    if (test_bit(ev_bits, EV_REL)) {
        if (test_bit(rel_bits, REL_Y) && test_bit(rel_bits, REL_X) &&
            test_bit(key_bits, BTN_MOUSE)) {
            udev_list_entry_add(&udev_device->properties, "ID_INPUT_MOUSE", "1", 0);
        }
    }
    else if (test_bit(ev_bits, EV_ABS)) {
        if (test_bit(key_bits, BTN_SELECT) || test_bit(key_bits, BTN_TR) ||
            test_bit(key_bits, BTN_START) || test_bit(key_bits, BTN_TL)) {
            if (test_bit(key_bits, BTN_TOUCH)) {
                udev_list_entry_add(&udev_device->properties, "ID_INPUT_TOUCHSCREEN", "1", 0);
            }
            else {
                udev_list_entry_add(&udev_device->properties, "ID_INPUT_JOYSTICK", "1", 0);
            }
        }
        else if (test_bit(abs_bits, ABS_Y) && test_bit(abs_bits, ABS_X)) {
            if (test_bit(abs_bits, ABS_Z) && !test_bit(ev_bits, EV_KEY)) {
                udev_list_entry_add(&udev_device->properties, "ID_INPUT_ACCELEROMETER", "1", 0);
            }
            else if (test_bit(key_bits, BTN_STYLUS) || test_bit(key_bits, BTN_TOOL_PEN)) {
                udev_list_entry_add(&udev_device->properties, "ID_INPUT_TABLET", "1", 0);
            }
            else if (test_bit(key_bits, BTN_TOUCH)) {
                if (test_bit(key_bits, BTN_TOOL_FINGER)) {
                    udev_list_entry_add(&udev_device->properties, "ID_INPUT_TOUCHPAD", "1", 0);
                }
                else {
                    udev_list_entry_add(&udev_device->properties, "ID_INPUT_TOUCHSCREEN", "1", 0);
                }
            }
            else if (test_bit(key_bits, BTN_MOUSE)) {
                udev_list_entry_add(&udev_device->properties, "ID_INPUT_MOUSE", "1", 0);
            }
        }
    }

    // TODO do not assume keyboard if EV_KEY
    if (test_bit(ev_bits, EV_KEY)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_KEY", "1", 0);

        if (test_bit(key_bits, KEY_ENTER)) {
            udev_list_entry_add(&udev_device->properties, "ID_INPUT_KEYBOARD", "1", 0);
        }
    }
}

static void set_properties_from_props(struct udev_device *udev_device)
{
    const char *sysname, *subsystem;
    struct udev_device *parent;
    char id[256];

    subsystem = udev_device_get_subsystem(udev_device);

    if (!subsystem || strcmp(subsystem, "drm") != 0) {
        return;
    }

    parent = udev_device_get_parent_with_subsystem_devtype(udev_device, "pci", NULL);
    sysname = udev_device_get_sysname(parent);

    if (!sysname) {
        return;
    }

    snprintf(id, sizeof(id), "pci-%s", sysname);
    udev_list_entry_add(&udev_device->properties, "ID_PATH", id, 0);
}

struct udev_device *udev_device_new_from_syspath(struct udev *udev, const char *syspath)
{
    char path[PATH_MAX], file[PATH_MAX];
    char *subsystem, *driver, *sysname;
    struct udev_device *udev_device;
    int i;

    if (!udev || !syspath) {
        return NULL;
    }

    snprintf(file, sizeof(file), "%s/uevent", syspath);

    if (access(file, R_OK) == -1) {
        return NULL;
    }

    if (!realpath(syspath, path)) {
        return NULL;
    }

    udev_device = calloc(1, sizeof(struct udev_device));

    if (!udev_device) {
        return NULL;
    }

    udev_device->udev = udev;
    udev_device->refcount = 1;
    udev_device->parent = NULL;

    udev_list_entry_init(&udev_device->properties);
    udev_list_entry_init(&udev_device->sysattrs);

    udev_list_entry_add(&udev_device->properties, "SYSPATH", path, 0);
    udev_list_entry_add(&udev_device->properties, "DEVPATH", path + 4, 0);

    sysname = strrchr(path, '/') + 1;
    driver = read_symlink(path, "driver");
    subsystem = read_symlink(path, "subsystem");

    udev_list_entry_add(&udev_device->properties, "SUBSYSTEM", subsystem, 0);
    udev_list_entry_add(&udev_device->properties, "SYSNAME", sysname, 0);
    udev_list_entry_add(&udev_device->properties, "DRIVER", driver, 0);

    for (i = 0; sysname[i] != '\0'; i++) {
        if (sysname[i] >= '0' && sysname[i] <= '9') {
            udev_list_entry_add(&udev_device->properties, "SYSNUM", sysname + i, 0);
            break;
        }
    }

    set_properties_from_uevent(udev_device);
    set_properties_from_evdev(udev_device);
    set_properties_from_props(udev_device);

    free(driver);
    free(subsystem);
    return udev_device;
}

struct udev_device *udev_device_new_from_devnum(struct udev *udev, char type, dev_t devnum)
{
    char path[PATH_MAX];

    if (!udev) {
        return NULL;
    }

    switch (type) {
    case 'c':
        snprintf(path, sizeof(path), "/sys/dev/char/%u:%u", major(devnum), minor(devnum));
        break;
    case 'b':
        snprintf(path, sizeof(path), "/sys/dev/block/%u:%u", major(devnum), minor(devnum));
        break;
    default:
        return NULL;
    }

    return udev_device_new_from_syspath(udev, path);
}

struct udev_device *udev_device_new_from_subsystem_sysname(struct udev *udev, const char *subsystem, const char *sysname)
{
    const char *fmt[] = { "/sys/bus/%s/devices/%s", "/sys/class/%s/%s", NULL };
    char path[PATH_MAX];
    int i;

    if (!udev || !subsystem || !sysname) {
        return NULL;
    }

    for (i = 0; fmt[i]; i++) {
        snprintf(path, sizeof(path), fmt[i], subsystem, sysname);

        if (access(path, R_OK) == 0) {
            return udev_device_new_from_syspath(udev, path);
        }
    }

    return NULL;
}

struct udev_device *udev_device_new_from_file(struct udev *udev, const char *path)
{
    char line[LINE_MAX], syspath[PATH_MAX], devnode[PATH_MAX];
    struct udev_device *udev_device;
    struct stat st;
    char *sysname;
    FILE *file;
    int i, cnt;
    char *pos;

    if (stat(path, &st) != 0 || st.st_size > 8192) {
        return NULL;
    }

    file = fopen(path, "r");

    if (!file) {
        return NULL;
    }

    udev_device = calloc(1, sizeof(struct udev_device));

    if (!udev_device) {
        fclose(file);
        return NULL;
    }

    cnt = 0;

    udev_device->udev = udev;
    udev_device->refcount = 1;
    udev_device->parent = NULL;

    udev_list_entry_init(&udev_device->properties);
    udev_list_entry_init(&udev_device->sysattrs);

    while (fgets(line, sizeof(line), file)) {
        line[strlen(line) - 1] = '\0';

        if (strncmp(line, "DEVPATH=", 8) == 0) {
            snprintf(syspath, sizeof(syspath), "/sys%s", line + 8);
            udev_list_entry_add(&udev_device->properties, "SYSPATH", syspath, 0);
            udev_list_entry_add(&udev_device->properties, "DEVPATH", line + 8, 0);

            sysname = strrchr(syspath, '/') + 1;
            udev_list_entry_add(&udev_device->properties, "SYSNAME", sysname, 0);

            for (i = 0; sysname[i] != '\0'; i++) {
                if (sysname[i] >= '0' && sysname[i] <= '9') {
                    udev_list_entry_add(&udev_device->properties, "SYSNUM", sysname + i, 0);
                    break;
                }
            }

            cnt++;
        }
        else if (strncmp(line, "DEVNAME=", 8) == 0) {
            snprintf(devnode, sizeof(devnode), "/dev/%s", line + 8);
            udev_list_entry_add(&udev_device->properties, "DEVNAME", devnode, 0);
        }
        else {
            pos = strchr(line, '=');

            // file is malformed, abort here.
            if (!pos) {
                cnt = 0;
                break;
            }

            *pos = '\0';

            if (strncmp(line, "SUBSYSTEM", 9) == 0 ||
                strncmp(line, "ACTION", 6) == 0 ||
                strncmp(line, "SEQNUM", 6) == 0) {
                cnt++;
            }

            udev_list_entry_add(&udev_device->properties, line, pos + 1, 0);
        }
    }

    fclose(file);

    // https://freedesktop.org/software/systemd/man/udev_device_new_from_environment.html
    // > The keys DEVPATH, SUBSYSTEM, ACTION, and SEQNUM are mandatory.
    if (cnt != 4) {
        udev_device_unref(udev_device);
        return NULL;
    }

    set_properties_from_props(udev_device);
    set_properties_from_evdev(udev_device);
    return udev_device;
}

/* XXX NOT IMPLEMENTED */ struct udev_device *udev_device_new_from_device_id(struct udev *udev, const char *id)
{
    return NULL;
}

/* XXX NOT IMPLEMENTED */ struct udev_device *udev_device_new_from_environment(struct udev *udev)
{
    return NULL;
}

struct udev_device *udev_device_ref(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    udev_device->refcount++;
    return udev_device;
}

struct udev_device *udev_device_unref(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    if (--udev_device->refcount > 0) {
        return NULL;
    }

    if (udev_device->parent) {
        udev_device_unref(udev_device->parent);
    }

    udev_list_entry_free_all(&udev_device->properties);
    udev_list_entry_free_all(&udev_device->sysattrs);

    free(udev_device);
    return NULL;
}
