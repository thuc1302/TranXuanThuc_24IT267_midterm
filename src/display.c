#include "display.h"
#include "util.h"

#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>

static char file_type(mode_t mode)
{
    if (S_ISDIR(mode))  return 'd';
    if (S_ISLNK(mode))  return 'l';
    if (S_ISCHR(mode))  return 'c';
    if (S_ISBLK(mode))  return 'b';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    if (S_ISREG(mode))  return '-';
    return '?';
}

static void format_permissions(mode_t mode, char text[11])
{
    text[0] = file_type(mode);
    const mode_t flags[9] = {
        S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP,
        S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH
    };
    const char letters[9] = "rwxrwxrwx";
    for (size_t i = 0; i < 9; ++i)
        text[i + 1] = (mode & flags[i]) ? letters[i] : '-';

    if (mode & S_ISUID)
        text[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        text[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX)
        text[9] = (mode & S_IXOTH) ? 't' : 'T';
    text[10] = '\0';
}

static void format_size(uintmax_t bytes, bool human_readable,
                        char *out, size_t length)
{
    if (!human_readable || bytes < 1024) {
        snprintf(out, length, "%" PRIuMAX, bytes);
        return;
    }

    static const char units[] = "KMGTPE";
    long double value = (long double)bytes;
    size_t unit_index = 0;
    do {
        value /= 1024.0L;
        ++unit_index;
    } while (value >= 1024.0L && unit_index < sizeof(units) - 1);

    if (value < 10.0L)
        snprintf(out, length, "%.1Lf%c", value, units[unit_index - 1]);
    else
        snprintf(out, length, "%.0Lf%c", value, units[unit_index - 1]);
}

static void format_time(time_t modified, char *out, size_t length)
{
    struct tm local;
    if (localtime_r(&modified, &local) == NULL) {
        snprintf(out, length, "??? ?? ????");
        return;
    }

    time_t now = time(NULL);
    double age = difftime(now, modified);
    /* Approximate the six-month rule used by common ls implementations. */
    const char *format = age < -3600 || age > 15552000
                         ? "%b %e  %Y" : "%b %e %H:%M";
    if (strftime(out, length, format, &local) == 0)
        snprintf(out, length, "??? ?? ????");
}

static void format_user(uid_t uid, bool numeric, char *out, size_t length)
{
    struct passwd *user = numeric ? NULL : getpwuid(uid);
    if (user != NULL)
        snprintf(out, length, "%s", user->pw_name);
    else
        snprintf(out, length, "%ju", (uintmax_t)uid);
}

static void format_group(gid_t gid, bool numeric, char *out, size_t length)
{
    struct group *group = numeric ? NULL : getgrgid(gid);
    if (group != NULL)
        snprintf(out, length, "%s", group->gr_name);
    else
        snprintf(out, length, "%ju", (uintmax_t)gid);
}

static char indicator(const LsEntry *entry, const LsOptions *options)
{
    mode_t mode = entry->info.st_mode;
    if (S_ISDIR(mode) && (options->classify || options->slash_directories))
        return '/';
    if (!options->classify)
        return '\0';
    if (S_ISLNK(mode))  return '@';
    if (S_ISFIFO(mode)) return '|';
    if (S_ISSOCK(mode)) return '=';
    if (S_ISREG(mode) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        return '*';
    return '\0';
}

static uintmax_t disk_blocks(const struct stat *info)
{
    /* POSIX st_blocks are expressed in 512-byte units. */
    return info->st_blocks > 0 ? ((uintmax_t)info->st_blocks + 1) / 2 : 0;
}

void display_entry(const LsEntry *entry, const LsOptions *options)
{
    if (options->show_inode)
        printf("%ju ", (uintmax_t)entry->info.st_ino);

    if (options->show_blocks) {
        char blocks[64];
        format_size(disk_blocks(&entry->info) * 1024, options->human_readable,
                    blocks, sizeof(blocks));
        if (options->human_readable)
            printf("%s ", blocks);
        else
            printf("%ju ", disk_blocks(&entry->info));
    }

    if (options->long_format) {
        char permissions[11];
        char user[256];
        char group[256];
        char size[64];
        char date[64];
        format_permissions(entry->info.st_mode, permissions);
        format_user(entry->info.st_uid, options->numeric_ids, user, sizeof(user));
        format_group(entry->info.st_gid, options->numeric_ids, group, sizeof(group));
        format_time(entry->info.st_mtime, date, sizeof(date));

        if (S_ISCHR(entry->info.st_mode) || S_ISBLK(entry->info.st_mode)) {
            snprintf(size, sizeof(size), "%u, %u",
                     major(entry->info.st_rdev), minor(entry->info.st_rdev));
        } else {
            uintmax_t bytes = entry->info.st_size < 0 ? 0 :
                              (uintmax_t)entry->info.st_size;
            format_size(bytes, options->human_readable, size, sizeof(size));
        }
        printf("%s %ju %-8s %-8s %8s %s ", permissions,
               (uintmax_t)entry->info.st_nlink, user, group, size, date);
    }

    fputs(entry->name, stdout);
    char symbol = indicator(entry, options);
    if (symbol != '\0')
        putchar(symbol);

    if (options->long_format && S_ISLNK(entry->info.st_mode)) {
        char *destination = read_symlink(entry->path);
        if (destination != NULL) {
            printf(" -> %s", destination);
            free(destination);
        }
    }
    putchar('\n');
}

void display_directory_total(const EntryList *entries, const LsOptions *options)
{
    if (!options->long_format && !options->show_blocks)
        return;

    uintmax_t blocks = 0;
    for (size_t i = 0; i < entries->length; ++i) {
        blkcnt_t count = entries->items[i].info.st_blocks;
        if (count > 0) {
            uintmax_t value = (uintmax_t)count;
            if (UINTMAX_MAX - blocks < value)
                blocks = UINTMAX_MAX;
            else
                blocks += value;
        }
    }

    if (options->human_readable) {
        char readable[64];
        uintmax_t kilobytes = (blocks / 2) + (blocks % 2);
        format_size(kilobytes > UINTMAX_MAX / 1024 ? UINTMAX_MAX : kilobytes * 1024,
                    true, readable, sizeof(readable));
        printf("total %s\n", readable);
    } else {
        printf("total %ju\n", (blocks / 2) + (blocks % 2));
    }
}
