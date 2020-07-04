#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "udev_util.h"
#include "udev.h"

int xasprintf(char *str, const char *fmt, ...)
{
    va_list list;
    int len;

    va_start(list, fmt);
    len = vsnprintf(NULL, 0, fmt, list);
    va_end(list);

    if (len == -1) {
        return -1;
    }

    str = malloc(len + 1);

    if (!str) {
        return -1;
    }

    len = vsnprintf(str, len + 1, fmt, list);

    if (len == -1) {
        free(str);
        return -1;
    }

    return len;
}

// TODO readlink+malloc
