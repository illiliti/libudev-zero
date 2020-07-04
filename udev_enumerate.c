#include <stdlib.h>

#include "udev.h"
#include "udev_list.h"

struct udev_enumerate
{
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

UDEV_EXPORT int udev_enumerate_scan_devices(struct udev_enumerate *udev_enumerate)
{
}

UDEV_EXPORT int udev_enumerate_scan_subsystems(struct udev_enumerate *udev_enumerate)
{
}

UDEV_EXPORT struct udev_list_entry *udev_enumerate_get_list_entry(struct udev_enumerate *udev_enumerate)
{
    if (!udev_enumerate) {
        return NULL;
    }

    return &udev_enumerate->devices;
}

UDEV_EXPORT int udev_enumerate_add_syspath(struct udev_enumerate *udev_enumerate, const char *syspath)
{
    // XXX NOT IMPLEMENTED
    return 0;
}

UDEV_EXPORT struct udev *udev_enumerate_get_udev(struct udev_enumerate *udev_enumerate)
{
    if (!udev_enumerate) {
        return NULL;
    }

    return udev_enumerate->udev;
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
}
