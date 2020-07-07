const char *udev_device_read_symlink(struct udev_device *udev_device, const char *name);
void udev_device_set_properties_from_uevent(struct udev_device *udev_device);
void udev_device_set_properties_from_ioctl(struct udev_device *udev_device);
