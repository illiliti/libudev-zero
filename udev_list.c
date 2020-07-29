#include <string.h>
#include <stdlib.h>

#include "udev.h"
#include "udev_list.h"

void udev_list_entry_init(struct udev_list_entry *list_entry)
{
    list_entry->value = NULL;
    list_entry->name = NULL;
    list_entry->next = NULL;
}

void udev_list_entry_free(struct udev_list_entry *list_entry)
{
    free(list_entry->value);
    free(list_entry->name);
    free(list_entry);
}

void udev_list_entry_free_all(struct udev_list_entry *list_entry)
{
    struct udev_list_entry *tmp, *tmp2;

    tmp = list_entry->next;

    while (tmp) {
        tmp2 = tmp;
        tmp = tmp->next;
        udev_list_entry_free(tmp2);
    }
}

struct udev_list_entry *udev_list_entry_add(struct udev_list_entry *list_entry, const char *name, const char *value, int uniq)
{
    struct udev_list_entry *new, *old;

    if (uniq) {
        old = udev_list_entry_get_by_name(list_entry, name);

        if (old) {
            if (old->value && strcmp(old->value, value) == 0) {
                return old;
            }

            free(old->value);
            old->value = value ? strdup(value) : NULL;
            return old;
        }
    }

    new = calloc(1, sizeof(struct udev_list_entry));

    if (!new) {
        return NULL;
    }

    new->value = value ? strdup(value) : NULL;
    new->name = strdup(name);

    new->next = list_entry->next;
    list_entry->next = new;

    return new;
}

struct udev_list_entry *udev_list_entry_get_next(struct udev_list_entry *list_entry)
{
    return list_entry ? list_entry->next : NULL;
}

struct udev_list_entry *udev_list_entry_get_by_name(struct udev_list_entry *list_entry, const char *name)
{
    struct udev_list_entry *tmp;

    if (!list_entry || !name) {
        return NULL;
    }

    tmp = list_entry;

    while (tmp) {
        if (tmp->name && strcmp(tmp->name, name) == 0) {
            return tmp;
        }

        tmp = tmp->next;
    }

    return NULL;
}

const char *udev_list_entry_get_name(struct udev_list_entry *list_entry)
{
    return list_entry ? list_entry->name : NULL;
}

const char *udev_list_entry_get_value(struct udev_list_entry *list_entry)
{
    return list_entry ? list_entry->value : NULL;
}
