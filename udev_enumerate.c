#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <fnmatch.h>
#include <sys/stat.h>

#include "udev.h"
#include "udev_list.h"

struct udev_enumerate {
    struct udev_list_entry subsystem_nomatch;
    struct udev_list_entry subsystem_match;
    struct udev_list_entry sysattr_nomatch;
    struct udev_list_entry property_match;
    struct udev_list_entry sysattr_match;
    struct udev_list_entry sysname_match;
    struct udev_list_entry devices;
    struct udev *udev;
    int refcount;
};

UDEV_EXPORT int udev_enumerate_add_match_subsystem(struct udev_enumerate *udev_enumerate, const char *subsystem)
{
    if (!udev_enumerate || !subsystem) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->subsystem_match, subsystem, NULL) ? 0 : -1;
}

UDEV_EXPORT int udev_enumerate_add_nomatch_subsystem(struct udev_enumerate *udev_enumerate, const char *subsystem)
{
    if (!udev_enumerate || !subsystem) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->subsystem_nomatch, subsystem, NULL) ? 0 : -1;
}

UDEV_EXPORT int udev_enumerate_add_match_sysattr(struct udev_enumerate *udev_enumerate, const char *sysattr, const char *value)
{
    if (!udev_enumerate || !sysattr) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->sysattr_match, sysattr, value) ? 0 : -1;
}

UDEV_EXPORT int udev_enumerate_add_nomatch_sysattr(struct udev_enumerate *udev_enumerate, const char *sysattr, const char *value)
{
    if (!udev_enumerate || !sysattr) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->sysattr_nomatch, sysattr, value) ? 0 : -1;
}

UDEV_EXPORT int udev_enumerate_add_match_property(struct udev_enumerate *udev_enumerate, const char *property, const char *value)
{
    if (!udev_enumerate || !property) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->property_match, property, value) ? 0 : -1;
}

UDEV_EXPORT int udev_enumerate_add_match_sysname(struct udev_enumerate *udev_enumerate, const char *sysname)
{
    if (!udev_enumerate || !sysname) {
        return -1;
    }

    return udev_list_entry_add(&udev_enumerate->sysname_match, sysname, NULL) ? 0 : -1;
}

UDEV_EXPORT int udev_enumerate_add_match_tag(struct udev_enumerate *udev_enumerate, const char *tag)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

UDEV_EXPORT int udev_enumerate_add_match_parent(struct udev_enumerate *udev_enumerate, struct udev_device *parent)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

UDEV_EXPORT int udev_enumerate_add_match_is_initialized(struct udev_enumerate *udev_enumerate)
{
    return 0;
}

static int udev_enumerate_filter_subsystem(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *subsystem;

    subsystem = udev_device_get_subsystem(udev_device);
    list_entry = udev_list_entry_get_next(&udev_enumerate->subsystem_nomatch);

    if (!subsystem) {
        return 0;
    }

    while (list_entry) {
        if (fnmatch(udev_list_entry_get_name(list_entry), subsystem, 0) == 0) {
            return 0;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    list_entry = udev_list_entry_get_next(&udev_enumerate->subsystem_match);

    if (list_entry) {
        while (list_entry) {
            if (fnmatch(udev_list_entry_get_name(list_entry), subsystem, 0) == 0) {
                return 1;
            }

            list_entry = udev_list_entry_get_next(list_entry);
        }

        return 0;
    }

    return 1;
}

static int udev_enumerate_filter_sysname(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *sysname;

    sysname = udev_device_get_sysname(udev_device);
    list_entry = udev_list_entry_get_next(&udev_enumerate->sysname_match);

    if (!list_entry) {
        return 1;
    }

    while (list_entry) {
        if (fnmatch(udev_list_entry_get_name(list_entry), sysname, 0) == 0) {
            return 1;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    return 0;
}

static int udev_enumerate_filter_property(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    const char *property, *property2, *value, *value2;
    struct udev_list_entry *list_entry, *list_entry2;

    list_entry = udev_list_entry_get_next(&udev_enumerate->property_match);

    if (!list_entry) {
        return 1;
    }

    while (list_entry) {
        property = udev_list_entry_get_name(list_entry);
        value = udev_list_entry_get_value(list_entry);

        list_entry2 = udev_device_get_properties_list_entry(udev_device);

        if (!list_entry2) {
            return 0;
        }

        while (list_entry2) {
            property2 = udev_list_entry_get_name(list_entry2);
            value2 = udev_list_entry_get_value(list_entry2);

            if (!value || !value2) {
                continue;
            }

            if (fnmatch(property, property2, 0) == 0 &&
                fnmatch(value, value2, 0) == 0) {
                return 1;
            }

            list_entry2 = udev_list_entry_get_next(list_entry2);
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    return 0;
}

static int udev_enumerate_filter_sysattr(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device)
{
    struct udev_list_entry *list_entry;
    const char *sysattr, *value;

    list_entry = udev_list_entry_get_next(&udev_enumerate->sysattr_nomatch);

    while (list_entry) {
        sysattr = udev_list_entry_get_name(list_entry);
        value = udev_device_get_sysattr_value(udev_device, sysattr);

        if (!value) {
            return 1;
        }

        if (fnmatch(udev_list_entry_get_value(list_entry), value, 0) == 0) {
            return 0;
        }

        list_entry = udev_list_entry_get_next(list_entry);
    }

    list_entry = udev_list_entry_get_next(&udev_enumerate->sysattr_match);

    if (list_entry) {
        while (list_entry) {
            sysattr = udev_list_entry_get_name(list_entry);
            value = udev_device_get_sysattr_value(udev_device, sysattr);

            if (!value) {
                return 0;
            }

            if (fnmatch(udev_list_entry_get_value(list_entry), value, 0) == 0) {
                return 1;
            }

            list_entry = udev_list_entry_get_next(list_entry);
        }

        return 0;
    }

    return 1;
}

static void udev_enumerate_add_device(struct udev_enumerate *udev_enumerate, const char type, dev_t devnum)
{
    struct udev_device *udev_device;

    udev_device = udev_device_new_from_devnum(udev_enumerate->udev, type, devnum);

    if (!udev_device) {
        return;
    }

    if (!udev_enumerate_filter_subsystem(udev_enumerate, udev_device) ||
        !udev_enumerate_filter_sysname(udev_enumerate, udev_device) ||
        !udev_enumerate_filter_property(udev_enumerate, udev_device) ||
        !udev_enumerate_filter_sysattr(udev_enumerate, udev_device)) {
        udev_device_unref(udev_device);
        return;
    }

    udev_list_entry_add(&udev_enumerate->devices, udev_device_get_syspath(udev_device), NULL);
    udev_device_unref(udev_device);
}

static void udev_enumerate_scan_dir(struct udev_enumerate *udev_enumerate, const char *path)
{
    char file[PATH_MAX];
    struct dirent *de;
    struct stat st;
    DIR *dp;

    dp = opendir(path);

    if (!dp) {
        return;
    }

    while ((de = readdir(dp))) {
        if (strcmp(de->d_name, ".") == 0 ||
            strcmp(de->d_name, "..") == 0) {
            continue;
        }

        snprintf(file, sizeof(file), "%s/%s", path, de->d_name);

        if (lstat(file, &st) != 0 || S_ISLNK(st.st_mode)) {
            continue;
        }
        else if (S_ISDIR(st.st_mode)) {
            udev_enumerate_scan_dir(udev_enumerate, file);
        }
        else if (S_ISBLK(st.st_mode)) {
            udev_enumerate_add_device(udev_enumerate, 'b', st.st_rdev);
        }
        else if (S_ISCHR(st.st_mode)) {
            udev_enumerate_add_device(udev_enumerate, 'c', st.st_rdev);
        }
    }

    closedir(dp);
}

UDEV_EXPORT int udev_enumerate_scan_devices(struct udev_enumerate *udev_enumerate)
{
    udev_enumerate_scan_dir(udev_enumerate, "/dev");
    return 0;
}

UDEV_EXPORT int udev_enumerate_scan_subsystems(struct udev_enumerate *udev_enumerate)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

UDEV_EXPORT struct udev_list_entry *udev_enumerate_get_list_entry(struct udev_enumerate *udev_enumerate)
{
    return udev_enumerate ? udev_list_entry_get_next(&udev_enumerate->devices) : NULL;
}

UDEV_EXPORT int udev_enumerate_add_syspath(struct udev_enumerate *udev_enumerate, const char *syspath)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

UDEV_EXPORT struct udev *udev_enumerate_get_udev(struct udev_enumerate *udev_enumerate)
{
    return udev_enumerate ? udev_enumerate->udev : NULL;
}

UDEV_EXPORT struct udev_enumerate *udev_enumerate_new(struct udev *udev)
{
    struct udev_enumerate *udev_enumerate;

    if (!udev) {
        return NULL;
    }

    udev_enumerate = calloc(1, sizeof(struct udev_enumerate));

    if (!udev_enumerate) {
        return NULL;
    }

    udev_enumerate->refcount = 1;
    udev_enumerate->udev = udev;

    udev_list_entry_init(&udev_enumerate->subsystem_nomatch);
    udev_list_entry_init(&udev_enumerate->subsystem_match);
    udev_list_entry_init(&udev_enumerate->sysattr_nomatch);
    udev_list_entry_init(&udev_enumerate->property_match);
    udev_list_entry_init(&udev_enumerate->sysattr_match);
    udev_list_entry_init(&udev_enumerate->sysname_match);
    udev_list_entry_init(&udev_enumerate->devices);

    return udev_enumerate;
}

UDEV_EXPORT struct udev_enumerate *udev_enumerate_ref(struct udev_enumerate *udev_enumerate)
{
    if (!udev_enumerate) {
        return NULL;
    }

    udev_enumerate->refcount++;
    return udev_enumerate;
}

UDEV_EXPORT struct udev_enumerate *udev_enumerate_unref(struct udev_enumerate *udev_enumerate)
{
    if (!udev_enumerate) {
        return NULL;
    }

    if (--udev_enumerate->refcount > 0) {
        return NULL;
    }

    udev_list_entry_free_all(&udev_enumerate->subsystem_nomatch);
    udev_list_entry_free_all(&udev_enumerate->subsystem_match);
    udev_list_entry_free_all(&udev_enumerate->sysattr_nomatch);
    udev_list_entry_free_all(&udev_enumerate->property_match);
    udev_list_entry_free_all(&udev_enumerate->sysattr_match);
    udev_list_entry_free_all(&udev_enumerate->sysname_match);
    udev_list_entry_free_all(&udev_enumerate->devices);

    free(udev_enumerate);
    return NULL;
}
