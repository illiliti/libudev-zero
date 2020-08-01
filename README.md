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

* [x] speed up udev_enumerate_scan_devices()
* [ ] implement hotplugging support
