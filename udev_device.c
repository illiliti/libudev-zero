#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "udev.h"
#include "udev_list.h"
#include "udev_util.h"
#include "udev_device.h"

struct udev_device
{
    struct udev_list_entry properties;
    struct udev_list_entry sysattrs;
	struct udev_device *parent;
    struct udev *udev;
    char *subsystem;
    char *syspath;
    char *sysname;
    char *devpath;
    char *devnode;
    char *devtype;
    char *driver;
    char *sysnum;
    dev_t devnum;
    int refcount;
};

UDEV_EXPORT const char *udev_device_get_syspath(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device->syspath;
}

UDEV_EXPORT const char *udev_device_get_sysname(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device->sysname;
}

UDEV_EXPORT const char *udev_device_get_sysnum(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device->sysnum;
}

UDEV_EXPORT const char *udev_device_get_devpath(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device->devpath;
}

UDEV_EXPORT const char *udev_device_get_devnode(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device->devnode;
}

UDEV_EXPORT dev_t udev_device_get_devnum(struct udev_device *udev_device)
{
    if (!udev_device) {
        return makedev(0, 0);
    }

    return udev_device->devnum;
}

UDEV_EXPORT const char *udev_device_get_devtype(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device->devtype;
}

UDEV_EXPORT const char *udev_device_get_subsystem(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device->subsystem;
}

UDEV_EXPORT const char *udev_device_get_driver(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device->driver;
}

UDEV_EXPORT struct udev *udev_device_get_udev(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device->udev;
}

UDEV_EXPORT struct udev_device *udev_device_get_parent(struct udev_device *udev_device)
{
    char *path;
    int slash;

    if (!udev_device) {
        return NULL;
    }

    path = strdup(udev_device->syspath);

    while (1) {
        slash = strrchr(path, '/') - path;

        if (!slash) {
            free(path);
            return NULL;
        }

        path[slash] = '\0';

        udev_device->parent = udev_device_new_from_syspath(udev_device->udev, path);

        if (udev_device->parent) {
            free(path);
            return udev_device->parent;
        }
    }
}

UDEV_EXPORT struct udev_device *udev_device_get_parent_with_subsystem_devtype(struct udev_device *udev_device, const char *subsystem, const char *devtype)
{
    const char *parent_subsystem, *parent_devtype;
    struct udev_device *parent;

    if (!udev_device || !subsystem) {
        return NULL;
    }

    while ((parent = udev_device_get_parent(udev_device))) {
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
    }

    return NULL;
}

UDEV_EXPORT int udev_device_get_is_initialized(struct udev_device *udev_device)
{
    return 1;
}

UDEV_EXPORT const char *udev_device_get_action(struct udev_device *udev_device)
{
    return NULL;
}

UDEV_EXPORT int udev_device_has_tag(struct udev_device *udev_device, const char *tag)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

UDEV_EXPORT struct udev_list_entry *udev_device_get_devlinks_list_entry(struct udev_device *udev_device)
{
    // XXX NOT IMPLEMENTED
    return NULL;
}

UDEV_EXPORT struct udev_list_entry *udev_device_get_properties_list_entry(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return &udev_device->properties;
}

UDEV_EXPORT struct udev_list_entry *udev_device_get_tags_list_entry(struct udev_device *udev_device)
{
    // XXX NOT IMPLEMENTED
    return NULL;
}

UDEV_EXPORT struct udev_list_entry *udev_device_get_sysattr_list_entry(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return &udev_device->sysattrs;
}

UDEV_EXPORT const char *udev_device_get_property_value(struct udev_device *udev_device, const char *key)
{
}

UDEV_EXPORT const char *udev_device_get_sysattr_value(struct udev_device *udev_device, const char *sysattr)
{
    struct udev_list_entry *list_entry;
    char data[1024], *path = NULL;
    struct stat st;
    ssize_t len;
    int fd;

    if (!udev_device || !sysattr) {
        return NULL;
    }

    list_entry = udev_list_entry_get_by_name(&udev_device->sysattrs, sysattr);

    if (list_entry) {
        return udev_list_entry_get_value(list_entry);
    }

    if (xasprintf(path, "%s/%s", udev_device->syspath, sysattr) == -1) {
        return NULL;
    }

    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        free(path);
        return NULL;
    }

    fd = open(path, O_RDONLY);

    if (fd == -1) {
        free(path);
        return NULL;
    }

    len = read(fd, data, sizeof(data));

    if (len == -1) {
        close(fd);
        free(path);
        return NULL;
    }

    close(fd);
    free(path);
    data[len] = '\0';
    list_entry = udev_list_entry_add(&udev_device->sysattrs, sysattr, data);
    return udev_list_entry_get_value(list_entry);
}

UDEV_EXPORT int udev_device_set_sysattr_value(struct udev_device *udev_device, const char *sysattr, const char *value)
{
    size_t len = strlen(value);
    char *path = NULL;
    struct stat st;
    int fd;

    if (!udev_device || !sysattr || !value) {
        return -1;
    }

    if (xasprintf(path, "%s/%s", udev_device->syspath, sysattr) == -1) {
        return -1;
    }

    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        free(path);
        return -1;
    }

    fd = open(path, O_WRONLY | O_NOFOLLOW);

    if (fd == -1) {
        free(path);
        return -1;
    }

    if (write(fd, value, len) == -1) {
        close(fd);
        free(path);
        return -1;
    }

    close(fd);
    free(path);
    udev_list_entry_add(&udev_device->sysattrs, sysattr, value);
    return 0;
}

char *udev_device_read_uevent(struct udev_device *udev_device, const char *name)
{
    char *line, *path = NULL, *data = NULL;
    size_t nlen = strlen(name), glen = 0;
    FILE *file;

    if (xasprintf(path, "%s/%s", udev_device->syspath, "uevent") == -1) {
        return NULL;
    }

    file = fopen(path, "r");

    if (!file) {
        free(path);
        return NULL;
    }

    while (getline(&line, &glen, file) != -1) {
        if (strncmp(line, name, nlen) == 0) {
            data = strdup(line + nlen);
            break;
        }
    }

    fclose(file);
    free(line);
    free(path);
    return data;
}

char *udev_device_read_symlink(struct udev_device *udev_device, const char *name)
{
    char *link, *data, *path = NULL;

    if (xasprintf(path, "%s/%s", udev_device->syspath, name) == -1) {
        return NULL;
    }

    link = realpath(path, NULL); // XXX XSI

    if (!link) {
        free(path);
        return NULL;
    }

    data = strdup(strrchr(link, '/') + 1);
    free(path);
    free(link);
    return data;
}

void udev_device_set_subsystem(struct udev_device *udev_device)
{
    char *subsystem;

    udev_device->subsystem = NULL;

    subsystem = udev_device_read_symlink(udev_device, "subsystem");

    if (!subsystem) {
        return;
    }

    udev_device->subsystem = subsystem;
}

void udev_device_set_sysname(struct udev_device *udev_device)
{
    char *devname, *sysname;

    udev_device->sysname = NULL;

    devname = udev_device_read_uevent(udev_device, "DEVNAME=");

    if (!devname) {
        return;
    }

    sysname = strrchr(devname, '/');

    if (!sysname) {
        sysname = devname;
    }
    else {
        sysname++;
    }

    udev_device->sysname = strdup(sysname);
    free(devname);
}

void udev_device_set_devnode(struct udev_device *udev_device)
{
    char *devname, *devnode = NULL;

    udev_device->devnode = NULL;

    devname = udev_device_read_uevent(udev_device, "DEVNAME=");

    if (!devname) {
        return;
    }

    if (xasprintf(devnode, "/dev/%s", devname) == -1) {
        free(devname);
        return;
    }

    udev_device->devnode = devnode;
    free(devname);
}

void udev_device_set_devtype(struct udev_device *udev_device)
{
    char *devtype;

    udev_device->devtype = NULL;

    devtype = udev_device_read_uevent(udev_device, "DEVTYPE=");

    if (!devtype) {
        return;
    }

    udev_device->devtype = devtype;
}

void udev_device_set_driver(struct udev_device *udev_device)
{
    char *driver;

    udev_device->driver = NULL;

    driver = udev_device_read_symlink(udev_device, "driver");

    if (!driver) {
        return;
    }

    udev_device->driver = driver;
}

void udev_device_set_devnum(struct udev_device *udev_device)
{
    char *major, *minor;

    udev_device->devnum = makedev(0, 0);

    major = udev_device_read_uevent(udev_device, "MAJOR=");
    minor = udev_device_read_uevent(udev_device, "MINOR=");

    if (!major && !minor) {
        return;
    }

    udev_device->devnum = makedev(atoi(major), atoi(minor));
    free(major);
    free(minor);
}

void udev_device_set_sysnum(struct udev_device *udev_device)
{
    int i;

    udev_device->sysnum = NULL;

    if (!udev_device->sysname) {
        return;
    }

    for (i = 0; udev_device->sysname[i] != '\0'; i++) {
        if (udev_device->sysname[i] >= '0' &&
            udev_device->sysname[i] <= '9') {
            udev_device->sysnum = strdup(udev_device->sysname + i);
            return;
        }
    }
}

void udev_device_set_properties(struct udev_device *udev_device)
{
}

UDEV_EXPORT struct udev_device *udev_device_new_from_syspath(struct udev *udev, const char *syspath)
{
    struct udev_device *udev_device;
    char *path, *file = NULL;
    struct stat st;

    if (!udev || !syspath) {
        return NULL;
    }

    path = realpath(syspath, NULL); // XXX XSI

    if (!path) {
        return NULL;
    }

    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode) == 0) {
        free(path);
        return NULL;
    }

    if (xasprintf(file, "%s/%s", path, "uevent") == -1) {
        free(path);
        return NULL;
    }

    if (access(file, R_OK) == -1) {
        free(file);
        free(path);
        return NULL;
    }

    free(file);

    udev_device = calloc(1, sizeof(struct udev_device));

    if (!udev_device) {
        free(path);
        return NULL;
    }

    udev_device->udev = udev;
    udev_device->refcount = 1;
    udev_device->syspath = path;
    udev_device->devpath = strdup(path + 4);

    udev_list_entry_init(&udev_device->properties);
    udev_list_entry_init(&udev_device->sysattrs);

    udev_device_set_properties(udev_device);
    udev_device_set_subsystem(udev_device);
    udev_device_set_sysname(udev_device);
    udev_device_set_devnode(udev_device);
    udev_device_set_devtype(udev_device);
    udev_device_set_driver(udev_device);
    udev_device_set_sysnum(udev_device);
    udev_device_set_devnum(udev_device);

    return udev_device;
}

UDEV_EXPORT struct udev_device *udev_device_new_from_devnum(struct udev *udev, char type, dev_t devnum)
{
    struct udev_device *udev_device;
    char *path = NULL;

    if (!udev || !type || !devnum) {
        return NULL;
    }

    switch (type) {
        case 'c':
            xasprintf(path, "/sys/dev/char/%d:%d", major(devnum), minor(devnum));
            break;
        case 'b':
            xasprintf(path, "/sys/dev/block/%d:%d", major(devnum), minor(devnum));
            break;
        default:
            return NULL;
    }

    udev_device = udev_device_new_from_syspath(udev, path);

    free(path);
    return udev_device;
}

UDEV_EXPORT struct udev_device *udev_device_new_from_subsystem_sysname(struct udev *udev, const char *subsystem, const char *sysname)
{
    // XXX NOT IMPLEMENTED
    return NULL;
}

UDEV_EXPORT struct udev_device *udev_device_new_from_device_id(struct udev *udev, const char *id)
{
    // XXX NOT IMPLEMENTED
    return NULL;
}

UDEV_EXPORT struct udev_device *udev_device_new_from_environment(struct udev *udev)
{
    // XXX NOT IMPLEMENTED
    return NULL;
}

UDEV_EXPORT struct udev_device *udev_device_ref(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    udev_device->refcount++;
    return udev_device;
}

UDEV_EXPORT struct udev_device *udev_device_unref(struct udev_device *udev_device)
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

    free(udev_device->subsystem);
    free(udev_device->syspath);
    free(udev_device->devpath);
    free(udev_device->sysname);
    free(udev_device->devnode);
    free(udev_device->devtype);
    free(udev_device->driver);
    free(udev_device->sysnum);
    free(udev_device);

    return NULL;
}
