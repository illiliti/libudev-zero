#!/bin/sh -f
#
# NOTE: you don't need this if you have mdev/mdevd, refer to mdev.conf
# NOTE: you need this if you want to use bare-bones CONFIG_UEVENT_HELPER
#
# usage:
# echo /full/path/to/helper.sh > /proc/sys/kernel/hotplug
# echo "/full/path/to/helper.sh UDEV_MONITOR_DIR" > /proc/sys/kernel/hotplug

exec env > "${1:-/tmp/.libudev-zero}/uevent.$$"
