libudev-zero
============

Drop-in replacement for libudev intended to work without daemon

What Works
----------
* [x] xorg-server
* [x] libinput
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

TODO
----

* [x] speed up performance
* [x] extend devices support
* [x] implement hotplugging support

Donate
------

You can donate if you like this project

BTC: 1BwrcsgtWZeLVvNeEQSg4A28a3yrGN3FpK
