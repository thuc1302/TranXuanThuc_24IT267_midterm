#include "listing.h"
#include "options.h"

#include <locale.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    /* Respect the user's locale for name sorting and date formatting. */
    setlocale(LC_ALL, "");

    LsOptions options;
    int status;
    int first_operand = parse_options(argc, argv, &options, &status);
    if (first_operand < 0)
        return status;

    return run_listing(argc - first_operand, argv + first_operand, &options);
}
