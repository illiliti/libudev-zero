libudev-zero
============

Drop-in replacement for libudev intended to work without daemon

What Works
----------
* [x] xorg-server
* [x] libinput
* [x] wlroots
* [ ] weston - should work. need test
* [ ] libusb - broken completely
* [ ] kwin - fails to compile for odd reason
* [ ] ???

TODO
----

* [ ] implement hotplugging support
* [x] remove [dirty hack](https://github.com/illiliti/libudev-zero/blob/e76f9b282442505bd6b0b08b411679aae1581fa5/udev_device.c#L383). use ioctl()
