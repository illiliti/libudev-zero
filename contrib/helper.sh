#!/bin/sh -f
#
# this helper required for CONFIG_UEVENT_HELPER aka /proc/sys/kernel/hotplug
# for mdev you can use simple one-liner, refer to mdev.conf for more info
#
# usage:
# echo /full/path/to/helper.sh > /proc/sys/kernel/hotplug
# echo "/full/path/to/helper.sh UDEV_MONITOR_DIR" > /proc/sys/kernel/hotplug
#

# NOTE: writing variables to file (e.g PWD or PATH)
# that are not related to uevent properties is harmless
env > "${1:-/tmp/.libudev-zero}/uevent.$$"
