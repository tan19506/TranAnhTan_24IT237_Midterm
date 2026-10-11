#include "ls_options.h"
#include <getopt.h>
#include <string.h>

int parse_options(int argc, char **argv, LsOptions *o) {
    memset(o, 0, sizeof(*o));
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
        switch (c) {
            case 'A': o->almost_all = 1; break;
            case 'a': o->all = 1; break;
            case 'c': o->use_ctime = 1; o->use_atime = 0; break;
            case 'd': o->directory = 1; o->recursive = 0; break;
            case 'F': o->classify = 1; break;
            case 'f': o->unsorted = 1; o->all = 1; o->almost_all = 0; break;
            case 'h': o->human = 1; o->kilobytes = 0; break;
            case 'i': o->inode = 1; break;
            case 'k': o->kilobytes = 1; o->human = 0; break;
            case 'l': o->long_format = 1; o->numeric_ids = 0; break;
            case 'n': o->numeric_ids = 1; o->long_format = 0; break;
            case 'q': o->quote_nonprint = 1; o->raw_nonprint = 0; break;
            case 'R': o->recursive = 1; o->directory = 0; break;
            case 'r': o->reverse = 1; break;
            case 'S': o->sort_size = 1; o->sort_time = 0; break;
            case 's': o->size_blocks = 1; break;
            case 't': o->sort_time = 1; o->sort_size = 0; break;
            case 'u': o->use_atime = 1; o->use_ctime = 0; break;
            case 'w': o->raw_nonprint = 1; o->quote_nonprint = 0; break;
            default: return -1;
        }
    }
    return 0;
}
