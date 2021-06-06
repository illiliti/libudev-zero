# libudev-zero

Drop-in replacement for `libudev` that intended to work with any device manager

## Why?

Because `udev` sucks, it is bloated and overengineered. `udev` is just
like `systemd`, it locks you into using non-portable crap that you can't
avoid because multiple programs depend on it. Look, even FreeBSD was forced
to rewrite[0] this crappy library because `libinput` hard-depends on `udev`.
Without `libinput` you can't use `wayland` and many other cool stuff.

Michael Forney (author of `cproc`, `samurai`, Oasis Linux, ...) decided to
fork[1] `libinput` and remove the hard dependency on `udev`. Is this a
solution? Yes. Is this a complete solution? No. This fork has a lot of
disadvantages like requiring patching applications to use `libinput_netlink`
instead of the `libinput_udev` API in order to use the automatic detection of
input devices and hotplugging. Static configuration is also required for
anything other than input devices (e.g drm devices). Moreover hotplugging is
vulnerable to race conditions when `libinput` handles the `uevent` faster than
the device manager which can lead to file permission issues. `libudev-zero`
prevents these race conditions by design.

Thankfully `udev` has stable API and hopefully no changes will be made to it
the future. On this basis I decided to create this clean-room implementation
of `libudev` which can be used with any or without a device manager.

[0] https://github.com/FreeBSDDesktop/libudev-devd  
[1] https://github.com/oasislinux/libinput

## What Works

* [x] xorg-server
* [ ] dosfstools - need to implement udev_enumerate_add_match_parent()
* [x] libinput
* [x] usbutils
* [x] wlroots
* [x] weston
* [x] libusb
* [x] kwin
* [ ] ???

## Dependencies

* C99 compiler (build time)
* POSIX make (build time)
* POSIX & XSI libc
* inotify & eventfd
* Linux >= 2.6.39

## Installation

```sh
make
make PREFIX=/usr install
```

## Hotplugging

Note that hotplugging support is fully optional. You can skip
this step if you don't have a need for the hotplugging capability.

In order to use hotplugging, you need to configure device manager to send
`uevent` messages to `UDEV_MONITOR_DIR`. `UDEV_MONITOR_DIR` is arbitrary
shared directory used by `libudev-zero` to receive `uevent` messages. By
default, `UDEV_MONITOR_DIR` points to `/tmp/.libudev-zero`. You can change
that directory at compile time by passing `-DUDEV_MONITOR_DIR=<dir>` to
`CFLAGS` or at runtime by setting `UDEV_MONITOR_DIR` environment variable.

Keep in mind that already processed `uevent` messages wouldn't be automatically
purged. You can set `UDEV_MONITOR_DIR` to directory on tmpfs to purge them on
reboot/shutdown.

Refer to [contrib](contrib) for usage examples and configs.

## Donate

You can send a donation to `BTC: 1BwrcsgtWZeLVvNeEQSg4A28a3yrGN3FpK`

Thank you very much!
