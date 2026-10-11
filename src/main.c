#include "ls_options.h"
#include "ls_display.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    LsOptions options;
    if (parse_options(argc, argv, &options) != 0) {
        fprintf(stderr, "Usage: %s [-AacdFfhiklnqRrSstuw] [file ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    return run_ls(argc, argv, &options) ? EXIT_FAILURE : EXIT_SUCCESS;
}
