libudev-zero
============

Drop-in replacement for libudev enables you to use any device manager you like
without worrying about udev dependency at all!

Why ?

Because udev sucks, bloated and overengineered. udev is just like
systemd, lock you into using non-portable crap that you can't avoid
because many software depends on it. Look, even FreeBSD was forced to
rewrite[1] this crappy library because libinput has mandatory udev dependency.
Without libinput you can't use wayland and some other cool stuff.

Michael Forney (cproc, samurai, Oasis Linux, ... author) decided to fork[2]
libinput and remove udev mandatority. This is solution ? Yes. This is complete
solution ? No. This fork has a lot of disadvantages like requiring patching
application from libinput_udev to libinput_netlink API in order to use
automatic detection of input devices and hotplugging. Static confuguration also
required for anything other than input devices (e.g drm devices). Moreover
hotplugging vulnerable to race conditions when libinput handles uevent faster
than device manager which can lead to file permissions issues. libudev-zero
prevents these race conditions by design.

Thanks god udev has stable API and hopefully no changes will be made in
future. On this basis i decided to create this clean-room implementation of
libudev which can be used with any or without device manager.

[1] https://github.com/FreeBSDDesktop/libudev-devd  
[2] https://github.com/oasislinux/libinput

What Works
----------

* [x] xorg-server
* [ ] dosfstools - need implement udev_enumerate_add_match_parent()
* [x] libinput
* [x] usbutils
* [x] wlroots
* [x] weston
* [x] libusb
* [x] kwin - [fix](https://github.com/dilyn-corner/KISS-kde/commit/0cc72748e46f859a0fced55b0c3fcc1dd9586a38)
* [ ] ???

Dependencies
------------

* C99 compiler (build time)
* POSIX make (build time)
* POSIX & XSI libc
* epoll & inotify
* Linux >= 2.6.39

Installation
------------

```sh
make
make PREFIX=/usr install # will overwrite existing udev libraries if any
# rebuild all packages which depends on udev
# here we go !
```

Hotplugging
-----------

Note that hotplugging support is fully optional! You can skip this step if you
don't have anything to hotplug.

There is no complicated or overengineered way to use hotplugging. Everything is
portable as much as possible. To use hotplugging the only thing you need is
uevent's receiver (device manager, busybox `uevent`, CONFIG_UEVENT_HELPER, ...).
I will describe only mdev and CONFIG_UEVENT_HELPER because their usage is very basic.
For busybox `uevent` you need to write your own parser which is kinda ... complex.

UDEV_MONITOR_DIR is arbitrary directory where uevent files stored.
Default is `/tmp/.libudev-zero`. You can change it at build time by appending
`-DUDEV_MONITOR_DIR=<dir>` to CFLAGS. I don't recommend setting UDEV_MONITOR_DIR
to regular fs (i.e non-tmpfs) because unneeded files aren't automatically discarded
after reboot or termination (yet).

1. mdev

  - merge [mdev.conf](contrib/mdev.conf) with your mdev.conf
  - restart mdev daemon

1. CONFIG_UEVENT_HELPER

  - ensure that CONFIG_UEVENT_HELPER enabled in kernel
  - add full path of [helper.sh](contrib/helper.sh) (must be executable) or
    [helper.c](contrib/helper.c) (compile it first) to /proc/sys/kernel/hotplug

    example:
    ```sh
    echo /full/path/to/helper > /proc/sys/kernel/hotplug # will use default UDEV_MONITOR_DIR
    OR
    echo "/full/path/to/helper <dir>" > /proc/sys/kernel/hotplug # change <dir> to your UDEV_MONITOR_DIR
    ```

2. run application which uses hotplugging (e.g xorg-server)
3. unplug and plug something to test working capacity

That's all! If you realized that this doesn't work for you,
you can always open an issue and describe your bug.

TODO
----

* [x] speed up performance
* [x] extend devices support
* [x] implement hotplugging support

Donate
------

You can donate if you like this project

BTC: 1BwrcsgtWZeLVvNeEQSg4A28a3yrGN3FpK

Thank you very much !
