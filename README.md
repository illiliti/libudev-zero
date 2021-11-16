# libudev-zero

Drop-in replacement for `libudev` intended to work with any device manager

## Why?

We all know that systemd is very hostile towards portability. Udev inherited
the same problem by exposing a very badly designed library interface. This is
dramatically reduces portability, user choice and basically creates vendor
lock-in because library interface highly tied to udev daemon.

Another udev problem is non-portable home-grown language called "udev rules".
Udev authors definitely don't know(or care) why it's better to avoid reinventing
the wheel. Strictly speaking, I think they did that on purpose to overcomplicate
udev as much as possible. Why? So that only authors(RedHat/IBM) can rule and dictate
the future of udev. The recent eudev death only proves that it's really hard to
support such unmaintainable mess.

Udev hwdb is yet another illustration of systemd-like approach. What the hell
"userspace /dev" has to do with parsing hardware database(pci.ids, usb.ids)
and setting/remapping buttons? Udev smells like systemd by trying to implement
all possible functionality in the single daemon/code base. Standalone UNIX-way
programs much better suited for such purposes.

## Pros/Cons

Keep in mind that libudev-zero isn't ideal. Here is some pros/cons:

### Pros

* Very portable. Doesn't depend on GNU features.
* No lock-in. Any device manager can be used, even smdev and CONFIG_UEVENT_HELPER.
* Source code is much cleaner than udev because of less abstractions and clever code.

### Cons

* Udev rules must be converted to shell script in order to work with any device manager.
* Udev hwdb interface isn't implemented. pciutils and usbutils will not display any meaningful info.
* Many functions and interfaces still aren't implemented, which may lead to breakage in some programs.

## What doesn't work

* dosfstools - requires udev_enumerate_add_match_parent()
* PulseAudio - highly depends on udev internal properties. [workaround](https://gist.github.com/capezotte/03ee5548218e819b06459819bb120b4b#pulseaudio)
* udisks2 - highly depends on udev internal properties
* android-tools - requires udev rules for non-root usage
* NetworkManager - needs investigation
* libgudev - needs investigation
* PipeWire - depends on udev internal properties. [patch](https://github.com/illiliti/libudev-zero/issues/26#issuecomment-846858706)
* ldm - depends on udev internal properties
* lvm2 - uses deprecated `udev_queue` API
* cups - needs investigation
* ???

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
