/*
 * this helper pretty similar to helper.sh, but it
 * doesn't write unrelated variables to file (e.g PWD or PATH)
 *
 * build:
 * cc helper.c -o helper
 *
 * usage:
 * echo /full/path/to/helper > /proc/sys/kernel/hotplug
 * echo "/full/path/to/helper UDEV_MONITOR_DIR" > /proc/sys/kernel/hotplug
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

int main(int argc, char **argv)
{
    extern char **environ;
    char path[PATH_MAX];
    char *dir;
    int fd, i;

    switch (argc) {
    case 1:
        dir = "/tmp/.libudev-zero";
        break;
    case 2:
        dir = argv[1];
        break;
    default:
        fprintf(stderr, "usage: %s [dir]\n", argv[0]);
        return 1;
    }

    snprintf(path, sizeof(path), "%s/uevent.XXXXXX", dir);
    fd = mkstemp(path);

    if (fd == -1) {
        perror("mkstemp");
        return 1;
    }

    for (i = 0; environ[i]; i++) {
        if (strncmp(environ[i], "PATH", 4) == 0 ||
            strncmp(environ[i], "HOME", 4) == 0) {
            continue;
        }

        if (write(fd, environ[i], strlen(environ[i])) == -1 ||
            write(fd, "\n", 1) == -1) {
            perror("write");
            close(fd);
            unlink(path);
            return 1;
        }
    }

    close(fd);
    return 0;
}
