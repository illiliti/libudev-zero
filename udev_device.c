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
    int refcount;
};

UDEV_EXPORT const char *udev_device_get_syspath(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "SYSPATH");
}

UDEV_EXPORT const char *udev_device_get_sysname(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "DEVNAME");
}

UDEV_EXPORT const char *udev_device_get_sysnum(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "SYSNUM");
}

UDEV_EXPORT const char *udev_device_get_devpath(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "DEVPATH");
}

UDEV_EXPORT const char *udev_device_get_devnode(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "DEVNODE");
}

UDEV_EXPORT dev_t udev_device_get_devnum(struct udev_device *udev_device)
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

UDEV_EXPORT const char *udev_device_get_devtype(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "DEVTYPE");
}

UDEV_EXPORT const char *udev_device_get_subsystem(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "SUBSYSTEM");
}

UDEV_EXPORT const char *udev_device_get_driver(struct udev_device *udev_device)
{
    if (!udev_device) {
        return NULL;
    }

    return udev_device_get_property_value(udev_device, "DRIVER");
}

UDEV_EXPORT struct udev *udev_device_get_udev(struct udev_device *udev_device)
{
    return udev_device ? udev_device->udev : NULL;
}

UDEV_EXPORT struct udev_device *udev_device_get_parent(struct udev_device *udev_device)
{
    char *path;
    int slash;

    if (!udev_device) {
        return NULL;
    }

    path = strdup(udev_device_get_property_value(udev_device, "SYSPATH"));

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
    return udev_device ? udev_list_entry_get_next(&udev_device->properties) : NULL;
}

UDEV_EXPORT struct udev_list_entry *udev_device_get_tags_list_entry(struct udev_device *udev_device)
{
    // XXX NOT IMPLEMENTED
    return NULL;
}

UDEV_EXPORT struct udev_list_entry *udev_device_get_sysattr_list_entry(struct udev_device *udev_device)
{
    return udev_device ? udev_list_entry_get_next(&udev_device->sysattrs) : NULL;
}

UDEV_EXPORT const char *udev_device_get_property_value(struct udev_device *udev_device, const char *key)
{
    struct udev_list_entry *list_entry;

    list_entry = udev_list_entry_get_by_name(&udev_device->properties, key);

    return udev_list_entry_get_value(list_entry); 
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

    if (xasprintf(path, "%s/%s", udev_device_get_property_value(udev_device, "SYSPATH"), sysattr) == -1) {
        return NULL;
    }

    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        free(path);
        return NULL;
    }

    fd = open(path, O_RDONLY | O_NOFOLLOW);

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
    data[len] = '\0'; // XXX strip trailing '\n'
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

    if (xasprintf(path, "%s/%s", udev_device_get_property_value(udev_device, "SYSPATH"), sysattr) == -1) {
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

const char *udev_device_read_symlink(struct udev_device *udev_device, const char *name)
{
    char *link, *data, *path = NULL;

    if (xasprintf(path, "%s/%s", udev_device_get_property_value(udev_device, "SYSPATH"), name) == -1) {
        return NULL;
    }

    link = realpath(path, NULL); // XXX XSI

    if (!link) {
        free(path);
        return NULL;
    }

    data = strrchr(link, '/') + 1;
    free(path);
    free(link);
    return data;
}

void udev_device_set_properties_from_uevent(struct udev_device *udev_device)
{
    char *key, *val, *line, *path = NULL;
    char *sysname, *devnode = NULL; 
    size_t len = 0;
    FILE *file;
    int i;

    if (xasprintf(path, "%s/uevent", udev_device_get_property_value(udev_device, "SYSPATH")) == -1) {
        return;
    }

    file = fopen(path, "r");

    if (!file) {
        free(path);
        return;
    }

    while (getline(&line, &len, file) != -1) {
        if (strncmp(line, "DEVNAME", 7) == 0) {
            sysname = strrchr(line + 8, '/');

            if (!sysname) {
                sysname = line + 8;
            }
            else {
                sysname++;
            }

            udev_list_entry_add(&udev_device->properties, "DEVNAME", sysname);

            for (i = 0; sysname[i] != '\0'; i++) {
                if (sysname[i] >= '0' && sysname[i] <= '9') {
                    udev_list_entry_add(&udev_device->properties, "SYSNUM", sysname + i);
                    break;
                }
            }

            if (xasprintf(devnode, "/dev/%s", line + 8) == -1) {
                continue;
            }

            udev_list_entry_add(&udev_device->properties, "DEVNODE", devnode);
            free(devnode);
        }
        else if (strncmp(line, "DEVTYPE", 7) == 0) {
            udev_list_entry_add(&udev_device->properties, "DEVTYPE", line + 8);
        }
        else if (strncmp(line, "DRIVER", 6) == 0) {
            continue;
        }
        else if (strncmp(line, "MAJOR", 5) == 0) {
            udev_list_entry_add(&udev_device->properties, "MAJOR", line + 6);
        }
        else if (strncmp(line, "MINOR", 5) == 0) {
            udev_list_entry_add(&udev_device->properties, "MINOR", line + 6);
        }
        else if (strchr(line, '=')) {
            val = strchr(line, '=') + 1;
            key = strdup(line);

            for (i = 0; key[i] != '\0'; i++) {
                if (key[i] == '=') {
                    key[i] = '\0';
                    break;
                }
            }

            udev_list_entry_add(&udev_device->properties, key, val);
            free(key);
        }
    }

    fclose(file);
    free(line);
    free(path);
}

void udev_device_set_properties_from_ioctl(struct udev_device *udev_device)
{
    // TODO extract INPUT properties using libevdev or direct ioctl's(complex), HELP WANTED!
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

    if (xasprintf(file, "%s/uevent", path) == -1) {
        free(path);
        return NULL;
    }

    if (access(file, R_OK) == -1) {
        free(file);
        free(path);
        return NULL;
    }

    udev_device = calloc(1, sizeof(struct udev_device));

    if (!udev_device) {
        free(file);
        free(path);
        return NULL;
    }

    udev_device->udev = udev;
    udev_device->refcount = 1;

    udev_list_entry_init(&udev_device->properties);
    udev_list_entry_init(&udev_device->sysattrs);

    udev_list_entry_add(&udev_device->properties, "SYSPATH", path);
    udev_list_entry_add(&udev_device->properties, "DEVPATH", path + 4);
    udev_list_entry_add(&udev_device->properties, "DRIVER", udev_device_read_symlink(udev_device, "driver"));
    udev_list_entry_add(&udev_device->properties, "SUBSYSTEM", udev_device_read_symlink(udev_device, "subsystem"));

    udev_device_set_properties_from_uevent(udev_device);
    udev_device_set_properties_from_ioctl(udev_device);

    free(file);
    free(path);
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

    free(udev_device);
    return NULL;
}
