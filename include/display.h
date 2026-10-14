#ifndef MYLS_DISPLAY_H
#define MYLS_DISPLAY_H

#include "entries.h"

void display_entry(const LsEntry *entry, const LsOptions *options);
void display_directory_total(const EntryList *entries, const LsOptions *options);

#endif
