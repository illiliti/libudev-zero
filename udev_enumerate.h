int udev_enumerate_filter_subsystem(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device);
int udev_enumerate_filter_property(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device);
int udev_enumerate_filter_sysname(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device);
int udev_enumerate_filter_sysattr(struct udev_enumerate *udev_enumerate, struct udev_device *udev_device);
void udev_enumerate_add_device(struct udev_enumerate *udev_enumerate, const char type, dev_t devnum);
void udev_enumerate_scan_dir(struct udev_enumerate *udev_enumerate, const char *path);
