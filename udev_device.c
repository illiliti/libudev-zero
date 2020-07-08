#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "udev.h"
#include "udev_list.h"
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

    // TODO retrieve sysname from DEVPATH if DEVNANE is NULL
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
    char *tmp, *path;

    if (!udev_device) {
        return NULL;
    }

    path = strdup(udev_device_get_syspath(udev_device));

    do {
        if ((tmp = strrchr(path, '/'))) {
            *tmp = '\0';
        }
        else {
            break;
        }

        udev_device->parent = udev_device_new_from_syspath(udev_device->udev, path);
    }
    while (!udev_device->parent);

    free(path);
    return udev_device->parent;
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
    char data[1024], path[PATH_MAX];
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

    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        return NULL;
    }

    fd = open(path, O_RDONLY | O_NOFOLLOW);

    if (fd == -1) {
        return NULL;
    }

    if (read(fd, data, sizeof(data)) == -1) {
        close(fd);
        return NULL;
    }

    close(fd);
    data[strcspn(data, "\n")] = '\0';
    list_entry = udev_list_entry_add(&udev_device->sysattrs, sysattr, data);
    return udev_list_entry_get_value(list_entry);
}

UDEV_EXPORT int udev_device_set_sysattr_value(struct udev_device *udev_device, const char *sysattr, const char *value)
{
    char path[PATH_MAX];
    struct stat st;
    int fd;

    if (!udev_device || !sysattr || !value) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", udev_device_get_syspath(udev_device), sysattr);

    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        return -1;
    }

    fd = open(path, O_WRONLY | O_NOFOLLOW);

    if (fd == -1) {
        return -1;
    }

    if (write(fd, value, strlen(value)) == -1) {
        close(fd);
        return -1;
    }

    close(fd);
    udev_list_entry_add(&udev_device->sysattrs, sysattr, value);
    return 0;
}

const char *udev_device_read_symlink(struct udev_device *udev_device, const char *name)
{
    char link[PATH_MAX], path[PATH_MAX];

    snprintf(path, sizeof(path), "%s/%s", udev_device_get_syspath(udev_device), name);

    if (!realpath(path, link)) {
        return NULL;
    }

    return strrchr(link, '/') + 1;
}

void udev_device_set_properties_from_uevent(struct udev_device *udev_device)
{
    char *key, *val, line[1024], path[PATH_MAX];
    char *sysname, devnode[PATH_MAX];
    FILE *file;
    int i;

    snprintf(path, sizeof(path), "%s/uevent", udev_device_get_syspath(udev_device));

    file = fopen(path, "r");

    if (!file) {
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

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

            snprintf(devnode, sizeof(devnode), "/dev/%s", line + 8);
            udev_list_entry_add(&udev_device->properties, "DEVNODE", devnode);
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
            key = line;
            key[strcspn(key, "=")] = '\0';

            udev_list_entry_add(&udev_device->properties, key, val);
        }
    }

    fclose(file);
}

// TODO extract INPUT properties using libevdev or direct ioctl's(complex), HELP WANTED!
// Very very very dirty hack to detect INPUT properties. False positives are guaranteed
void udev_device_set_properties_from_ioctl(struct udev_device *udev_device)
{
    const char *name;

    if (strcmp(udev_device_get_subsystem(udev_device), "input") != 0) {
        return;
    }

    udev_list_entry_add(&udev_device->properties, "ID_INPUT", "1");
    name = udev_device_get_sysattr_value(udev_device, "name");

    if (!name) {
        return;
    }

    // Your mind will be dead after reading this code
    if (fnmatch("*[Mm][Ou][Uu][Ss][Ee]*", name, 0) == 0) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_MOUSE", "1");
    }
    else if (fnmatch("*[Tt][Oo][Uu][Cc][Hh][Pp][Aa][Dd]*", name, 0) == 0) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_TOUCHPAD", "1");
    }
    else if (fnmatch("*[Kk][Ee][Yy][Bb][Oo][Aa][Rr][Dd]*", name, 0) == 0) {
        udev_list_entry_add(&udev_device->properties, "ID_INPUT_KEYBOARD", "1");
    }
}

UDEV_EXPORT struct udev_device *udev_device_new_from_syspath(struct udev *udev, const char *syspath)
{
    char path[PATH_MAX], file[PATH_MAX];
    struct udev_device *udev_device;
    struct stat st;

    if (!udev || !syspath) {
        return NULL;
    }

    if (!realpath(syspath, path)) {
        return NULL;
    }

    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode) == 0) {
        return NULL;
    }

    snprintf(file, sizeof(file), "%s/uevent", path);

    if (access(file, R_OK) == -1) {
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

    udev_list_entry_add(&udev_device->properties, "SYSPATH", path);
    udev_list_entry_add(&udev_device->properties, "DEVPATH", path + 4);
    udev_list_entry_add(&udev_device->properties, "DRIVER", udev_device_read_symlink(udev_device, "driver"));
    udev_list_entry_add(&udev_device->properties, "SUBSYSTEM", udev_device_read_symlink(udev_device, "subsystem"));

    udev_device_set_properties_from_uevent(udev_device);
    udev_device_set_properties_from_ioctl(udev_device);

    return udev_device;
}

UDEV_EXPORT struct udev_device *udev_device_new_from_devnum(struct udev *udev, char type, dev_t devnum)
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
