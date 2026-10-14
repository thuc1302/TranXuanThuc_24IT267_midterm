#ifndef MYLS_LISTING_H
#define MYLS_LISTING_H

#include "ls.h"

/* Returns EXIT_SUCCESS or EXIT_FAILURE based on filesystem errors. */
int run_listing(int operand_count, char **operands, const LsOptions *options);

#endif
