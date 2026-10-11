#ifndef LS_OPTIONS_H
#define LS_OPTIONS_H

typedef struct {
    int all, almost_all, use_ctime, directory, classify, unsorted;
    int human, inode, kilobytes, long_format, numeric_ids;
    int quote_nonprint, recursive, reverse, sort_size, sort_time;
    int use_atime, raw_nonprint, size_blocks;
} LsOptions;

int parse_options(int argc, char **argv, LsOptions *options);
#endif
