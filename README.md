# libudev-zero
Drop-in replacement for `libudev` that enables you to use any device manager
you like without worrying about the `udev` dependency at all!

## Why?
Because `udev` sucks, it is bloated and overengineered. `udev` is just like `systemd`, it locks you into using non-portable crap that you can't avoid because multiple programs depend on it. Look, even FreeBSD was forced to rewrite[1] this crappy library because `libinput` hard-depends on `udev`. Without `libinput` you can't use `wayland` and many other cool stuff.

Michael Forney (author of `cproc`, `samurai`, Oasis Linux, ...) decided to fork[2] `libinput` and remove the hard dependency on `udev`. Is this a solution? Yes.
Is this a complete solution? No. This fork has a lot of disadvantages like requiring patching applications to use `libinput_netlink` instead of the `libinput_udev` API in order to use the automatic detection of input devices and hotplugging. Static configuration is also required for anything other than input devices (e.g drm devices). Moreover hotplugging is vulnerable to race conditions when `libinput` handles the `uevent` faster than the device manager which can lead to file permission issues. `libudev-zero` prevents these race conditions by design.

Thankfully `udev` has stable API and hopefully no changes will be made to it the future. On this basis I decided to create this clean-room implementation of `libudev` which can be used with any or without a device manager.

[1] https://github.com/FreeBSDDesktop/libudev-devd  
[2] https://github.com/oasislinux/libinput

## What Works
* [x] xorg-server
* [ ] dosfstools - need to implement udev_enumerate_add_match_parent()
* [x] libinput
* [x] usbutils
* [x] wlroots
* [x] weston
* [x] libusb
* [x] kwin - [fix](https://github.com/dilyn-corner/KISS-kde/commit/0cc72748e46f859a0fced55b0c3fcc1dd9586a38)
* [ ] ???

## Dependencies
* C99 compiler (build time)
* POSIX make (build time)
* POSIX & XSI libc
* inotify
* Linux >= 2.6.39

## Installation

```sh
make
make PREFIX=/usr install # this will overwrite udev libraries if they exist
# rebuild all the packages that depend on libudev, and you're ready to go.
```

## Hotplugging
Note that hotplugging support is fully optional! You can skip this step if you don't have a need for the hotplugging capability.

Hotplugging is fairly uncomplicated and not overengineered at all. Everything is portable as much as possible. To use hotplugging the only thing you need is a `uevent` receiver (like a device manager, busybox's `uevent`, `CONFIG_UEVENT_HELPER`, ...). I will explain only the `mdev` and `CONFIG_UEVENT_HELPER` methods because their usage is fairly basic. For busybox's `uevent` you need to write your own parser which is kinda, well, complex.

`UDEV_MONITOR_DIR` is an arbitrary directory where the `uevent` files are stored. The default is `/tmp/.libudev-zero`. You can change it at build time by appending `-DUDEV_MONITOR_DIR=<dir>` to `CFLAGS`. I don't recommend setting `UDEV_MONITOR_DIR` to regular filesystems (i.e non-tmpfs) because temporary files aren't automatically discarded after reboot or termination (yet).

### a) the `mdev` method
1. merge [mdev.conf](contrib/mdev.conf) with your `mdev.conf`
2. restart the `mdev` daemon

### b) the `CONFIG_UEVENT_HELPER` method
1. ensure that `CONFIG_UEVENT_HELPER` is enabled in your kernel configuration
2. add the directory containing [helper.sh](contrib/helper.sh) after marking it as executable or [helper.c](contrib/helper.c) after compiling it, to `/proc/sys/kernel/hotplug`

#### example:
```sh
echo "/full/path/to/helper"       > /proc/sys/kernel/hotplug # will use the default UDEV_MONITOR_DIR
OR
echo "/full/path/to/helper <dir>" > /proc/sys/kernel/hotplug # change <dir> to your UDEV_MONITOR_DIR
```

---
Then you can run an application that uses hotplugging like `xorg-server` to see if it's working by unplugging and plugging something back. If you face any problems while trying out any of these methods, please create an issue.

## TODO
* [x] speed up performance
* [x] extend device support
* [x] implement hotplugging support

## Donate
You can send a donation to `BTC: 1BwrcsgtWZeLVvNeEQSg4A28a3yrGN3FpK` if you like this project.

Thank you very much!
