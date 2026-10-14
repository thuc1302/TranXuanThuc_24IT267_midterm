#ifndef MYLS_UTIL_H
#define MYLS_UTIL_H

#include <stddef.h>

/* These helpers either return valid storage or terminate on allocation failure. */
void *xmalloc(size_t size);
void *xrealloc(void *pointer, size_t size);
char *xstrdup(const char *text);
char *join_path(const char *directory, const char *name);
char *read_symlink(const char *path);
void report_error(const char *path);

#endif
