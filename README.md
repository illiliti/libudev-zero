# libudev-zero

Drop-in replacement for `libudev` intended to work with any device manager

## Why?

Because `udev` sucks, it is bloated and overengineered. `udev` is just
like `systemd`, it locks you into using non-portable crap that you can't
avoid because multiple programs depend on it. Look, even FreeBSD was forced
to rewrite[0] this crappy library because `libinput` hard-depends on `udev`.
Without `libinput` you can't use `wayland` and many other cool stuff.

Michael Forney (author of `cproc`, `samurai`, Oasis Linux, ...) decided to
fork[1] `libinput` and remove the hard dependency on `udev`. However, this
fork has a drawback that requires patching applications to use `libinput_netlink`
instead of the `libinput_udev` API in order to use the automatic detection of
input devices and hotplugging. Static configuration is also required for anything
other than input devices (e.g drm devices).

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
* Linux >= 2.6.39

## Installation

```sh
make
make PREFIX=/usr install
```

## Hotplugging

Note that hotplugging support is fully optional. You can skip
this step if you don't have a need for the hotplugging capability.

If you're using mdev-like device manager, refer to [mdev.conf](contrib/mdev.conf)
for config example.

If you're using other device manager, you need to configure it to rebroadcast
kernel uevents. You can do this by either patching(see below) device manager
or simply executing [helper.c](contrib/helper.c) for each uevent.

If you're developing your own device manager, you need to rebroadcast kernel
uevents to `0x4` netlink group of `NETLINK_KOBJECT_UEVENT`. This is required
because libudev-zero can't simply listen to kernel uevents due to potential
race conditions. Refer(but don't copy blindly) to [helper.c](contrib/helper.c)
for example how it could be implemented in C.

Don't hesitate to ask me everything you don't understand. I'm usually hanging
around in #kisslinux at libera.chat, but you can also email me or open an issue here.

## Future directions

1. Write a better cross-platform(*nix, maybe macos and windows) device enumeration library.
2. Convince mainstream apps(libinput, wlroots, ...) to use new library instead of libudev.
3. Declare libudev as obsolete library and archive this project.

## Donate

You can send a donation to `BTC: 1BwrcsgtWZeLVvNeEQSg4A28a3yrGN3FpK`

Thank you very much!
