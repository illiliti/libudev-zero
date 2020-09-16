#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <linux/input.h>

#include "udev.h"
#include "udev_list.h"

enum { BITS_MAX = 96 };

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

dev_t udev_device_get_devnum(struct udev_device *udev_device)
{
    const char *major, *minor;

    major = udev_device_get_property_value(udev_device, "MAJOR");
    minor = udev_device_get_property_value(udev_device, "MINOR");

    if (!major && !minor) {
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

    parent = udev_device_get_parent(udev_device);

    while (parent) {
        parent_subsystem = udev_device_get_subsystem(parent);
        parent_devtype = udev_device_get_devtype(parent);

        if (parent_subsystem && strcmp(parent_subsystem, subsystem) == 0) {
            if (!devtype) {
                return parent;
            }

            if (parent_devtype && strcmp(parent_devtype, devtype) == 0) {
                return parent;
            }
        }

        parent = udev_device_get_parent(parent);
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
    char data[BUFSIZ], path[PATH_MAX];
    struct stat st;
    size_t len;
    FILE *file;
    char *pos;

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

    len = fread(data, 1, sizeof(data), file);

    if (len != sizeof(data) && ferror(file)) {
        fclose(file);
        return NULL;
    }

    if ((pos = memchr(data, '\n', len))) {
        *pos = '\0';
    }
    else {
        data[len] = '\0';
    }

    fclose(file);
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

static char *udev_device_read_symlink(struct udev_device *udev_device, const char *name)
{
    char link[PATH_MAX], path[PATH_MAX];
    ssize_t len;

    snprintf(path, sizeof(path), "%s/%s", udev_device_get_syspath(udev_device), name);
    len = readlink(path, link, sizeof(link));

    if (len == -1) {
        return NULL;
    }

    link[len] = '\0';
    return strdup(strrchr(link, '/') + 1);
}

static void udev_device_set_properties_from_uevent(struct udev_device *udev_device)
{
    char line[LINE_MAX], path[PATH_MAX];
    char *pos, devnode[PATH_MAX];
    FILE *file;

    snprintf(path, sizeof(path), "%s/uevent", udev_device_get_syspath(udev_device));
    file = fopen(path, "r");

    if (!file) {
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "DEVNAME", 7) == 0) {
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

static int populate_bit(unsigned long *arr, const char *val)
{
    char *bit, *bits, *save;
    int i;

    if (!val) {
        return 0;
    }

    bits = strdup(val);

    if (!bits) {
        return 0;
    }

    bit = strtok_r(bits, " ", &save);

    for (i = 0; bit && i < BITS_MAX; i++) {
        arr[i] = strtoul(bit, NULL, 16);
        bit = strtok_r(NULL, " ", &save);
    }

    free(bits);
    return i;
}

static int find_bit(unsigned long *arr, int cnt, int bit)
{
    int i;

    if (!cnt) {
        return 0;
    }

    for (i = 0; i < cnt; i++) {
        if (arr[i] & (1UL << (bit % LONG_BIT))) {
            return 1;
        }
    }

    return 0;
}

static void udev_device_set_properties_from_evdev(struct udev_device *udev_device)
{
    int ev_cnt, rel_cnt, key_cnt, abs_cnt, prop_cnt;
    unsigned long prop_bits[BITS_MAX] = {0};
    unsigned long abs_bits[BITS_MAX] = {0};
    unsigned long rel_bits[BITS_MAX] = {0};
    unsigned long key_bits[BITS_MAX] = {0};
    unsigned long ev_bits[BITS_MAX] = {0};
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

    ev_cnt = populate_bit(ev_bits, udev_device_get_property_value(parent, "EV"));
    abs_cnt = populate_bit(abs_bits, udev_device_get_property_value(parent, "ABS"));
    rel_cnt = populate_bit(rel_bits, udev_device_get_property_value(parent, "REL"));
    key_cnt = populate_bit(key_bits, udev_device_get_property_value(parent, "KEY"));
    prop_cnt = populate_bit(prop_bits, udev_device_get_property_value(parent, "PROP"));

    udev_list_entry_add(&udev_device->properties, "ID_INPUT", "1", 0);

    if (find_bit(prop_bits, prop_cnt, INPUT_PROP_POINTING_STICK)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_POINTINGSTICK", "1", 0);
    }

    if (find_bit(prop_bits, prop_cnt, INPUT_PROP_ACCELEROMETER)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_ACCELEROMETER", "1", 0);
    }

    if (find_bit(ev_bits, ev_cnt, EV_SW)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_SWITCH", "1", 0);
    }

    if (find_bit(ev_bits, ev_cnt, EV_REL)) {
        if (find_bit(rel_bits, rel_cnt, REL_Y) && find_bit(rel_bits, rel_cnt, REL_X) &&
            find_bit(key_bits, key_cnt, BTN_MOUSE)) {
            udev_list_entry_add(&udev_device->properties, "ID_INPUT_MOUSE", "1", 0);
        }
    }
    else if (find_bit(ev_bits, ev_cnt, EV_ABS)) {
        if (find_bit(key_bits, key_cnt, BTN_SELECT) || find_bit(key_bits, key_cnt, BTN_TR) ||
            find_bit(key_bits, key_cnt, BTN_START) || find_bit(key_bits, key_cnt, BTN_TL)) {
            udev_list_entry_add(&udev_device->properties, "ID_INPUT_JOYSTICK", "1", 0);
        }
        else if (find_bit(abs_bits, abs_cnt, ABS_Y) && find_bit(abs_bits, abs_cnt, ABS_X)) {
            if (find_bit(abs_bits, abs_cnt, ABS_Z) && !find_bit(ev_bits, ev_cnt, EV_KEY)) {
                udev_list_entry_add(&udev_device->properties, "ID_INPUT_ACCELEROMETER", "1", 0);
            }
            else if (find_bit(key_bits, key_cnt, BTN_STYLUS) || find_bit(key_bits, key_cnt, BTN_TOOL_PEN)) {
                udev_list_entry_add(&udev_device->properties, "ID_INPUT_TABLET", "1", 0);
            }
            else if (find_bit(key_bits, key_cnt, BTN_TOUCH)) {
                if (find_bit(key_bits, key_cnt, BTN_TOOL_FINGER)) {
                    udev_list_entry_add(&udev_device->properties, "ID_INPUT_TOUCHPAD", "1", 0);
                }
                else {
                    udev_list_entry_add(&udev_device->properties, "ID_INPUT_TOUCHSCREEN", "1", 0);
                }
            }
            else if (find_bit(key_bits, key_cnt, BTN_MOUSE)) {
                udev_list_entry_add(&udev_device->properties, "ID_INPUT_MOUSE", "1", 0);
            }
        }
    }
    else if (find_bit(ev_bits, ev_cnt, EV_KEY)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_KEY", "1", 0);

        if (find_bit(key_bits, key_cnt, KEY_ENTER)) {
            udev_list_entry_add(&udev_device->properties, "ID_INPUT_KEYBOARD", "1", 0);
        }
    }
}

static void udev_device_set_properties_from_props(struct udev_device *udev_device)
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
    driver = udev_device_read_symlink(udev_device, "driver");
    subsystem = udev_device_read_symlink(udev_device, "subsystem");

    udev_list_entry_add(&udev_device->properties, "SUBSYSTEM", subsystem, 0);
    udev_list_entry_add(&udev_device->properties, "SYSNAME", sysname, 0);
    udev_list_entry_add(&udev_device->properties, "DRIVER", driver, 0);

    for (i = 0; sysname[i] != '\0'; i++) {
        if (sysname[i] >= '0' && sysname[i] <= '9') {
            udev_list_entry_add(&udev_device->properties, "SYSNUM", sysname + i, 0);
            break;
        }
    }

    udev_device_set_properties_from_uevent(udev_device);
    udev_device_set_properties_from_evdev(udev_device);
    udev_device_set_properties_from_props(udev_device);

    free(driver);
    free(subsystem);
    return udev_device;
}

struct udev_device *udev_device_new_from_devnum(struct udev *udev, char type, dev_t devnum)
{
    char path[PATH_MAX];

    if (!udev || !type || !devnum) {
        return NULL;
    }

    switch (type) {
    case 'c':
        snprintf(path, sizeof(path), "/sys/dev/char/%d:%d", major(devnum), minor(devnum));
        break;
    case 'b':
        snprintf(path, sizeof(path), "/sys/dev/block/%d:%d", major(devnum), minor(devnum));
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
    char *pos, *sysname;
    struct stat st;
    FILE *file;
    int i;

    udev_device = calloc(1, sizeof(struct udev_device));

    if (!udev_device) {
        return NULL;
    }

    if (stat(path, &st) != 0 || st.st_size > 8192) {
        free(udev_device);
        return NULL;
    }

    file = fopen(path, "r");

    if (!file) {
        free(udev_device);
        return NULL;
    }

    udev_device->udev = udev;
    udev_device->refcount = 1;
    udev_device->parent = NULL;

    udev_list_entry_init(&udev_device->properties);
    udev_list_entry_init(&udev_device->sysattrs);

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "DEVPATH", 7) == 0) {
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
        }
        else if (strncmp(line, "DEVNAME", 7) == 0) {
            snprintf(devnode, sizeof(devnode), "/dev/%s", line + 8);
            udev_list_entry_add(&udev_device->properties, "DEVNAME", devnode, 0);
        }
        else if ((pos = strchr(line, '='))) {
            *pos = '\0';
            udev_list_entry_add(&udev_device->properties, line, pos + 1, 0);
        }
    }

    fclose(file);

    if (!udev_device_get_syspath(udev_device)) {
        udev_device_unref(udev_device);
        return NULL;
    }

    udev_device_set_properties_from_props(udev_device);
    udev_device_set_properties_from_evdev(udev_device);
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
