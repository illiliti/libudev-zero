#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <sys/sysmacros.h>

#include "udev.h"
#include "udev_list.h"

struct udev_device {
    struct udev_list_entry properties;
    struct udev_list_entry sysattrs;
    struct udev_device *parent;
    struct udev *udev;
    int refcount;
};

const char *udev_device_get_syspath(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "SYSPATH");
}

const char *udev_device_get_sysname(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "SYSNAME");
}

const char *udev_device_get_sysnum(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "SYSNUM");
}

const char *udev_device_get_devpath(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "DEVPATH");
}

const char *udev_device_get_devnode(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "DEVNAME");
}

dev_t udev_device_get_devnum(struct udev_device *udev_device)
{
    const char *major, *minor;

    if (!udev_device) {
        return makedev(0, 0);
    }

    major = udev_device_get_property_value(udev_device, "MAJOR");
    minor = udev_device_get_property_value(udev_device, "MINOR");

    if (!major && !minor) {
        return makedev(0, 0);
    }

    return makedev(atoi(major), atoi(minor));
}

const char *udev_device_get_devtype(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "DEVTYPE");
}

const char *udev_device_get_subsystem(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "SUBSYSTEM");
}

const char *udev_device_get_driver(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "DRIVER");
}

struct udev *udev_device_get_udev(struct udev_device *udev_device)
{
    return udev_device ? udev_device->udev : NULL;
}

struct udev_device *udev_device_get_parent(struct udev_device *udev_device)
{
    char *path, *syspath;

    if (!udev_device) {
        return NULL;
    }

    syspath = strdup(udev_device_get_syspath(udev_device));

    if (!syspath) {
        return NULL;
    }

    path = syspath + 5;

    do {
        if ((path = strrchr(path, '/'))) {
            *path = '\0';
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

int udev_device_get_is_initialized(struct udev_device *udev_device)
{
    return 1;
}

const char *udev_device_get_action(struct udev_device *udev_device)
{
    return NULL;
}

int udev_device_has_tag(struct udev_device *udev_device, const char *tag)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

struct udev_list_entry *udev_device_get_devlinks_list_entry(struct udev_device *udev_device)
{
    // XXX NOT IMPLEMENTED
    return NULL;
}

struct udev_list_entry *udev_device_get_properties_list_entry(struct udev_device *udev_device)
{
    return udev_device ? udev_list_entry_get_next(&udev_device->properties) : NULL;
}

struct udev_list_entry *udev_device_get_tags_list_entry(struct udev_device *udev_device)
{
    // XXX NOT IMPLEMENTED
    return NULL;
}

struct udev_list_entry *udev_device_get_sysattr_list_entry(struct udev_device *udev_device)
{
    return udev_device ? udev_list_entry_get_next(&udev_device->sysattrs) : NULL;
}

const char *udev_device_get_property_value(struct udev_device *udev_device, const char *key)
{
    struct udev_list_entry *list_entry;

    list_entry = udev_list_entry_get_by_name(&udev_device->properties, key);
    return udev_list_entry_get_value(list_entry); 
}

const char *udev_device_get_sysattr_value(struct udev_device *udev_device, const char *sysattr)
{
    struct udev_list_entry *list_entry;
    char data[BUFSIZ], path[PATH_MAX];
    struct stat st;
    int fd;

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

    fd = open(path, O_RDONLY);

    if (fd == -1) {
        return NULL;
    }

    if (read(fd, data, sizeof(data)) == -1) {
        close(fd);
        return NULL;
    }

    close(fd);
    data[strcspn(data, "\n")] = '\0';
    list_entry = udev_list_entry_add(&udev_device->sysattrs, sysattr, data, 0);
    return udev_list_entry_get_value(list_entry);
}

int udev_device_set_sysattr_value(struct udev_device *udev_device, const char *sysattr, const char *value)
{
    char path[PATH_MAX];
    struct stat st;
    int fd;

    if (!udev_device || !sysattr || !value) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", udev_device_get_syspath(udev_device), sysattr);

    if (lstat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return -1;
    }

    fd = open(path, O_WRONLY);

    if (fd == -1) {
        return -1;
    }

    if (write(fd, value, strlen(value)) == -1) {
        close(fd);
        return -1;
    }

    close(fd);
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
    char *key, *val, devnode[PATH_MAX];
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
        else if (strchr(line, '=')) {
            val = strchr(line, '=') + 1;
            key = line;
            key[strcspn(key, "=")] = '\0';

            udev_list_entry_add(&udev_device->properties, key, val, 1);
        }
    }

    fclose(file);
}

static int test_bit(unsigned long int arr[], int bit)
{
    return arr[bit / LONG_BIT] & (1 << (bit % LONG_BIT));
}

static void udev_device_set_properties_from_ioctl(struct udev_device *udev_device)
{
    unsigned long int bits[96], key_bits[96], rel_bits[96], abs_bits[96];
    const char *devnode, *subsystem;
    int fd;

    subsystem = udev_device_get_subsystem(udev_device);

    if (!subsystem || strcmp(subsystem, "input") != 0) {
        return;
    }

    devnode = udev_device_get_devnode(udev_device);

    if (!devnode) {
        return;
    }

    fd = open(devnode, O_RDONLY);

    if (fd == -1) {
        return;
    }

    if (ioctl(fd, (int)EVIOCGBIT(0, sizeof(bits)), &bits) == -1 ||
        ioctl(fd, (int)EVIOCGBIT(EV_KEY, sizeof(key_bits)), &key_bits) == -1 ||
        ioctl(fd, (int)EVIOCGBIT(EV_REL, sizeof(rel_bits)), &rel_bits) == -1 ||
        ioctl(fd, (int)EVIOCGBIT(EV_ABS, sizeof(abs_bits)), &abs_bits) == -1) {
        close(fd);
        return;
    }

    if (test_bit(bits, EV_SW)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_SWITCH", "1", 0);
    }

    if (test_bit(bits, EV_KEY) && test_bit(key_bits, KEY_ENTER)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_KEY", "1", 0);
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_KEYBOARD", "1", 0);
    }

    if (test_bit(bits, EV_REL) && test_bit(rel_bits, REL_Y) && test_bit(rel_bits, REL_X) &&
        test_bit(key_bits, BTN_MOUSE)) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_MOUSE", "1", 0);
    }

    if (test_bit(bits, EV_ABS) && test_bit(abs_bits, ABS_Y) && test_bit(abs_bits, ABS_X)) {
        if (test_bit(key_bits, BTN_TOUCH) && !test_bit(key_bits, BTN_TOOL_PEN)) {
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

    udev_list_entry_add(&udev_device->properties, "ID_INPUT", "1", 0);
    close(fd);
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
    udev_list_entry_add(&udev_device->properties, "SYSNAME", sysname, 0);

    for (i = 0; sysname[i] != '\0'; i++) {
        if (sysname[i] >= '0' && sysname[i] <= '9') {
            udev_list_entry_add(&udev_device->properties, "SYSNUM", sysname + i, 0);
            break;
        }
    }

    driver = udev_device_read_symlink(udev_device, "driver");
    subsystem = udev_device_read_symlink(udev_device, "subsystem");

    if (driver || subsystem) {
        udev_list_entry_add(&udev_device->properties, "SUBSYSTEM", subsystem, 0);
        udev_list_entry_add(&udev_device->properties, "DRIVER", driver, 0);
        free(subsystem);
        free(driver);
    }

    udev_device_set_properties_from_uevent(udev_device);
    udev_device_set_properties_from_ioctl(udev_device);

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

struct udev_device *udev_device_new_from_device_id(struct udev *udev, const char *id)
{
    // XXX NOT IMPLEMENTED
    return NULL;
}

struct udev_device *udev_device_new_from_environment(struct udev *udev)
{
    // XXX NOT IMPLEMENTED
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
