#include <stdlib.h>

#include "udev.h"

struct udev
{
    int refcount;
};

UDEV_EXPORT struct udev *udev_new(void)
{
    struct udev *udev;

    udev = calloc(1, sizeof(struct udev));
    return udev;
}

UDEV_EXPORT struct udev *udev_ref(struct udev *udev)
{
    if (!udev) {
        return NULL;
    }

    udev->refcount++;
    return udev;
}

UDEV_EXPORT struct udev *udev_unref(struct udev *udev)
{
    if (!udev) {
        return NULL;
    }

    if (--udev->refcount > 0) {
        return udev;
    }

    free(udev);
    return NULL;
}
