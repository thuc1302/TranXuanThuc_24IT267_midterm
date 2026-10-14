#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

void *xmalloc(size_t size)
{
    void *memory = malloc(size == 0 ? 1 : size);
    if (memory == NULL) {
        fprintf(stderr, "myls: out of memory\n");
        exit(EXIT_FAILURE);
    }
    return memory;
}

void *xrealloc(void *pointer, size_t size)
{
    void *memory = realloc(pointer, size == 0 ? 1 : size);
    if (memory == NULL) {
        fprintf(stderr, "myls: out of memory\n");
        exit(EXIT_FAILURE);
    }
    return memory;
}

char *xstrdup(const char *text)
{
    size_t length = strlen(text);
    if (length == SIZE_MAX) {
        fprintf(stderr, "myls: path is too long\n");
        exit(EXIT_FAILURE);
    }
    char *copy = xmalloc(length + 1);
    memcpy(copy, text, length + 1);
    return copy;
}

char *join_path(const char *directory, const char *name)
{
    size_t parent_length = strlen(directory);
    size_t name_length = strlen(name);
    bool separator = parent_length > 0 && directory[parent_length - 1] != '/';

    if (parent_length > SIZE_MAX - name_length - 2) {
        fprintf(stderr, "myls: path is too long\n");
        exit(EXIT_FAILURE);
    }

    char *result = xmalloc(parent_length + name_length + (separator ? 2 : 1));
    memcpy(result, directory, parent_length);
    size_t pos = parent_length;
    if (separator)
        result[pos++] = '/';
    memcpy(result + pos, name, name_length + 1);
    return result;
}

char *read_symlink(const char *path)
{
    size_t capacity = 128;
    for (;;) {
        char *buffer = xmalloc(capacity);
        ssize_t bytes = readlink(path, buffer, capacity - 1);
        if (bytes < 0) {
            free(buffer);
            return NULL;
        }
        if ((size_t)bytes < capacity - 1) {
            buffer[bytes] = '\0';
            return buffer;
        }
        free(buffer);
        if (capacity > SIZE_MAX / 2) {
            errno = ENAMETOOLONG;
            return NULL;
        }
        capacity *= 2;
    }
}

void report_error(const char *path)
{
    /* perror includes the system-provided description of errno. */
    fprintf(stderr, "myls: %s: %s\n", path, strerror(errno));
}
