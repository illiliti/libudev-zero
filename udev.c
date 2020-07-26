#include <stdlib.h>

#include "udev.h"

struct udev {
    int refcount;
};

struct udev *udev_new(void)
{
    struct udev *udev;

    udev = calloc(1, sizeof(struct udev));

    if (!udev) {
        return NULL;
    }

    udev->refcount = 1;
    return udev;
}

struct udev *udev_ref(struct udev *udev)
{
    if (!udev) {
        return NULL;
    }

    udev->refcount++;
    return udev;
}

struct udev *udev_unref(struct udev *udev)
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
