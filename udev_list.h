struct udev_list_entry {
    struct udev_list_entry *next;
    char *value;
    char *name;
};

void udev_list_entry_init(struct udev_list_entry *list_entry);
void udev_list_entry_free(struct udev_list_entry *list_entry);
void udev_list_entry_free_all(struct udev_list_entry *list_entry);
struct udev_list_entry *udev_list_entry_add(struct udev_list_entry *list_entry, const char *name, const char *value, int uniq);
