#include "entries.h"
#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void entries_init(EntryList *list)
{
    list->items = NULL;
    list->length = 0;
    list->capacity = 0;
}

void entries_free(EntryList *list)
{
    for (size_t i = 0; i < list->length; ++i) {
        free(list->items[i].name);
        free(list->items[i].path);
    }
    free(list->items);
    entries_init(list);
}

bool entries_add(EntryList *list, const char *name, const char *path,
                 bool follow_symlink, int *error_seen)
{
    struct stat info;
    int result = follow_symlink ? stat(path, &info) : lstat(path, &info);
    if (result < 0 && follow_symlink) {
        /* A dangling symlink is still a valid directory entry. */
        result = lstat(path, &info);
    }
    if (result < 0) {
        report_error(path);
        *error_seen = 1;
        return false;
    }

    if (list->length == list->capacity) {
        size_t capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        if (capacity < list->capacity || capacity > SIZE_MAX / sizeof(LsEntry)) {
            errno = ENOMEM;
            report_error("entry list");
            exit(EXIT_FAILURE);
        }
        list->items = xrealloc(list->items, capacity * sizeof(LsEntry));
        list->capacity = capacity;
    }

    LsEntry *entry = &list->items[list->length++];
    entry->name = xstrdup(name);
    entry->path = xstrdup(path);
    entry->info = info;
    return true;
}

static bool visible(const char *name, const LsOptions *options)
{
    if (options->show_all)
        return true;
    if (options->almost_all)
        return strcmp(name, ".") != 0 && strcmp(name, "..") != 0;
    return name[0] != '.';
}

bool entries_read_directory(EntryList *list, const char *path,
                            const LsOptions *options, int *error_seen)
{
    DIR *directory = opendir(path);
    if (directory == NULL) {
        report_error(path);
        *error_seen = 1;
        return false;
    }

    errno = 0;
    struct dirent *dirent_item;
    while ((dirent_item = readdir(directory)) != NULL) {
        if (visible(dirent_item->d_name, options)) {
            char *full_path = join_path(path, dirent_item->d_name);
            entries_add(list, dirent_item->d_name, full_path, false, error_seen);
            free(full_path);
        }
        errno = 0;
    }
    if (errno != 0) {
        report_error(path);
        *error_seen = 1;
    }
    if (closedir(directory) != 0) {
        report_error(path);
        *error_seen = 1;
    }
    return true;
}

static const LsOptions *sorting_options;

static int compare_entries(const void *left, const void *right)
{
    const LsEntry *a = left;
    const LsEntry *b = right;
    int cmp = 0;

    if (sorting_options->sort_mode == SORT_MTIME) {
        if (a->info.st_mtim.tv_sec != b->info.st_mtim.tv_sec)
            cmp = a->info.st_mtim.tv_sec > b->info.st_mtim.tv_sec ? -1 : 1;
        else if (a->info.st_mtim.tv_nsec != b->info.st_mtim.tv_nsec)
            cmp = a->info.st_mtim.tv_nsec > b->info.st_mtim.tv_nsec ? -1 : 1;
    } else if (sorting_options->sort_mode == SORT_SIZE) {
        if (a->info.st_size != b->info.st_size)
            cmp = a->info.st_size > b->info.st_size ? -1 : 1;
    }

    if (cmp == 0)
        cmp = strcoll(a->name, b->name);
    if (cmp == 0)
        cmp = strcmp(a->name, b->name);
    /* Normalize to -1/0/1 before reversing (avoids negating INT_MIN). */
    int result = (cmp > 0) - (cmp < 0);
    return sorting_options->reverse ? -result : result;
}

void entries_sort(EntryList *list, const LsOptions *options)
{
    if (list->length < 2)
        return;
    if (options->sort_mode == SORT_NONE) {
        if (options->reverse) {
            for (size_t i = 0, j = list->length - 1; i < j; ++i, --j) {
                LsEntry temporary = list->items[i];
                list->items[i] = list->items[j];
                list->items[j] = temporary;
            }
        }
        return;
    }
    sorting_options = options;
    qsort(list->items, list->length, sizeof(LsEntry), compare_entries);
}
