#include "options.h"

#include <getopt.h>
#include <stdio.h>
#include <string.h>

void print_usage(const char *program)
{
    printf("Usage: %s [OPTION]... [FILE]...\n", program);
    puts("List information about FILEs (the current directory by default).\n"
         "  -a  include hidden entries, including . and ..\n"
         "  -A  include hidden entries, excluding . and ..\n"
         "  -l  use a long listing format\n"
         "  -h  print human-readable sizes (with -l or -s)\n"
         "  -R  list subdirectories recursively\n"
         "  -r  reverse the sorting order\n"
         "  -t  sort by modification time, newest first\n"
         "  -S  sort by file size, largest first\n"
         "  -U  do not sort directory entries\n"
         "  -1  one entry per line (cancels an earlier -l)\n"
         "  -d  list directories themselves, not their contents\n"
         "  -i  display inode numbers\n"
         "  -F  append file type indicators\n"
         "  -p  append / to directory names\n"
         "  -s  show allocated size in 1K blocks\n"
         "  -n  like -l, but show numeric user and group IDs\n"
         "      --help  display this help and exit");
}

int parse_options(int argc, char **argv, LsOptions *options, int *exit_status)
{
    static const struct option long_options[] = {
        {"help", no_argument, NULL, 1000},
        {NULL, 0, NULL, 0}
    };
    memset(options, 0, sizeof(*options));
    options->sort_mode = SORT_NAME;
    *exit_status = 0;

    int ch;
    /* '+' prevents GNU getopt from permuting operands after an option. */
    while ((ch = getopt_long(argc, argv, "+aAlhRrtS1diFpsnU", long_options, NULL)) != -1) {
        switch (ch) {
        case 'a': options->show_all = true; options->almost_all = false; break;
        case 'A': options->almost_all = true; options->show_all = false; break;
        case 'l': options->long_format = true; break;
        case 'h': options->human_readable = true; break;
        case 'R': options->recursive = true; break;
        case 'r': options->reverse = true; break;
        case 't': options->sort_mode = SORT_MTIME; break;
        case 'S': options->sort_mode = SORT_SIZE; break;
        case 'U': options->sort_mode = SORT_NONE; break;
        case '1': options->long_format = false; break;
        case 'd': options->directory_only = true; break;
        case 'i': options->show_inode = true; break;
        case 'F': options->classify = true; break;
        case 'p': options->slash_directories = true; break;
        case 's': options->show_blocks = true; break;
        case 'n': options->numeric_ids = true; options->long_format = true; break;
        case 1000:
            print_usage(argv[0]);
            return -1;
        default:
            fprintf(stderr, "Try '%s --help' for usage information.\n", argv[0]);
            *exit_status = 2;
            return -1;
        }
    }
    return optind;
}
