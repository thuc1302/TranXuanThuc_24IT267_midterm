#ifndef MYLS_ENTRIES_H
#define MYLS_ENTRIES_H

#include "ls.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

typedef struct {
    char *name;              /* What is printed in the listing */
    char *path;              /* Filesystem path used for system calls */
    struct stat info;        /* Result of lstat (or stat for CLI directory links) */
} LsEntry;

typedef struct {
    LsEntry *items;
    size_t length;
    size_t capacity;
} EntryList;

void entries_init(EntryList *list);
void entries_free(EntryList *list);
/* On a filesystem error, reports it, sets *error_seen and returns false. */
bool entries_add(EntryList *list, const char *name, const char *path,
                 bool follow_symlink, int *error_seen);
/* Loads a directory into list. Entries that cannot be stat'ed are skipped. */
bool entries_read_directory(EntryList *list, const char *path,
                            const LsOptions *options, int *error_seen);
void entries_sort(EntryList *list, const LsOptions *options);

#endif
