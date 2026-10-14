#ifndef MYLS_LS_H
#define MYLS_LS_H

#include <stdbool.h>

typedef enum {
    SORT_NAME,
    SORT_MTIME,
    SORT_SIZE,
    SORT_NONE
} SortMode;

/* Configuration shared by the independent project modules. */
typedef struct {
    bool show_all;          /* -a: include . and .. */
    bool almost_all;        /* -A: include hidden entries except . and .. */
    bool long_format;       /* -l */
    bool human_readable;    /* -h */
    bool recursive;         /* -R */
    bool reverse;           /* -r */
    bool directory_only;    /* -d */
    bool show_inode;        /* -i */
    bool classify;          /* -F */
    bool slash_directories; /* -p */
    bool show_blocks;       /* -s */
    bool numeric_ids;       /* -n */
    SortMode sort_mode;     /* -t, -S, -U, or default */
} LsOptions;

#endif
