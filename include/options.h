#ifndef MYLS_OPTIONS_H
#define MYLS_OPTIONS_H

#include "ls.h"

/* Return index of first operand, or -1 for --help / invalid options. */
int parse_options(int argc, char **argv, LsOptions *options, int *exit_status);
void print_usage(const char *program);

#endif
