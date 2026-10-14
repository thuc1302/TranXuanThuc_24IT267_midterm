#include "listing.h"
#include "display.h"
#include "entries.h"
#include "util.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

/* An explicit stack prevents C call-stack overflow for deep directory trees. */
typedef struct {
    char **paths;
    size_t length;
    size_t capacity;
} DirectoryStack;

static void stack_push(DirectoryStack *stack, const char *path)
{
    if (stack->length == stack->capacity) {
        size_t capacity = stack->capacity ? stack->capacity * 2 : 16;
        if (capacity < stack->capacity || capacity > SIZE_MAX / sizeof(char *)) {
            fprintf(stderr, "myls: too many directories\n");
            exit(EXIT_FAILURE);
        }
        stack->paths = xrealloc(stack->paths, capacity * sizeof(char *));
        stack->capacity = capacity;
    }
    stack->paths[stack->length++] = xstrdup(path);
}

static char *stack_pop(DirectoryStack *stack)
{
    return stack->paths[--stack->length];
}

static void display_all(const EntryList *entries, const LsOptions *options)
{
    for (size_t i = 0; i < entries->length; ++i)
        display_entry(&entries->items[i], options);
}

/* Prints a directory and pushes its children in reverse display order. */
static void walk_directory(const char *root, bool headings,
                           const LsOptions *options, bool *any_output,
                           int *error_seen)
{
    DirectoryStack stack = {0};
    stack_push(&stack, root);

    while (stack.length > 0) {
        char *path = stack_pop(&stack);
        EntryList children;
        entries_init(&children);
        if (!entries_read_directory(&children, path, options, error_seen)) {
            entries_free(&children);
            free(path);
            continue;
        }
        entries_sort(&children, options);

        if (*any_output)
            putchar('\n');
        if (headings)
            printf("%s:\n", path);
        display_directory_total(&children, options);
        display_all(&children, options);
        *any_output = true;

        if (options->recursive) {
            /* . and .. must NEVER be revisited, including when -a is used. */
            for (size_t i = children.length; i > 0; --i) {
                const LsEntry *child = &children.items[i - 1];
                if (S_ISDIR(child->info.st_mode) &&
                    strcmp(child->name, ".") != 0 &&
                    strcmp(child->name, "..") != 0)
                    stack_push(&stack, child->path);
            }
        }
        entries_free(&children);
        free(path);
    }
    free(stack.paths);
}

int run_listing(int operand_count, char **operands, const LsOptions *options)
{
    EntryList files;
    EntryList directories;
    entries_init(&files);
    entries_init(&directories);
    int error_seen = 0;

    char *current_directory[] = {"."};
    if (operand_count == 0) {
        operands = current_directory;
        operand_count = 1;
    }

    for (int i = 0; i < operand_count; ++i) {
        const char *path = operands[i];
        EntryList item;
        entries_init(&item);

        /* Normally a command-line symlink to a directory is followed,
           except when -l, -d, or -F requests information about the link. */
        bool follow = !options->long_format && !options->directory_only &&
                      !options->classify;
        if (entries_add(&item, path, path, follow, &error_seen)) {
            if (S_ISDIR(item.items[0].info.st_mode) && !options->directory_only)
                entries_add(&directories, path, path, follow, &error_seen);
            else
                entries_add(&files, path, path, follow, &error_seen);
        }
        entries_free(&item);
    }

    entries_sort(&files, options);
    entries_sort(&directories, options);
    display_all(&files, options);

    bool any_output = files.length > 0;
    bool headings = options->recursive || operand_count > 1;
    for (size_t i = 0; i < directories.length; ++i)
        walk_directory(directories.items[i].path, headings, options,
                       &any_output, &error_seen);

    entries_free(&files);
    entries_free(&directories);
    return error_seen ? EXIT_FAILURE : EXIT_SUCCESS;
}
