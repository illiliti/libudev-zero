[![Donate with Bitcoin](https://en.cryptobadges.io/badge/micro/1BwrcsgtWZeLVvNeEQSg4A28a3yrGN3FpK)](https://en.cryptobadges.io/donate/1BwrcsgtWZeLVvNeEQSg4A28a3yrGN3FpK)

libudev-zero
============

Drop-in replacement for libudev intended to work without daemon

What Works
----------
* [x] xorg-server
* [x] libinput
* [x] wlroots
* [x] weston
* [ ] libusb - broken completely
* [x] kwin - [fix](https://github.com/dilyn-corner/KISS-kde/commit/0cc72748e46f859a0fced55b0c3fcc1dd9586a38)
* [ ] ???

Dependencies
------------

* C99 compiler (build time)
* POSIX make (build time)
* POSIX & XSI libc
* Linux >= 2.6.39

Installation
------------

```sh
make
make PREFIX=/usr install # overwrites existing udev libraries if any
```

TODO
----

* [ ] speed up performance
  - [x] threads
  - [ ] deferred plugging via udev_monitor_receive_device()
* [ ] implement hotplugging support
* [x] remove [dirty hack](https://github.com/illiliti/libudev-zero/blob/e76f9b282442505bd6b0b08b411679aae1581fa5/udev_device.c#L383). use ioctl()
