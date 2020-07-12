libudev-zero
============

Drop-in replacement for libudev intented to work without daemon

What Works
----------
* [x] xorg-server
* [x] libinput
* [ ] wlroots - partially works. someone reported that mouse doesn't work
* [ ] weston - need implement udev_device_new_from_subsystem_sysname()
* [ ] kwin - fails to compile for odd reason
* [ ] ???

TODO
----

* [ ] implement hotplugging support
* [x] remove [dirty hack](https://github.com/illiliti/libudev-zero/blob/e76f9b282442505bd6b0b08b411679aae1581fa5/udev_device.c#L383). use ioctl()
