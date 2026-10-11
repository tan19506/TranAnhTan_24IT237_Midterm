#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#include "ls_display.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

typedef struct {
    char *name;
    char *path;
    struct stat st;
} Entry;

static const LsOptions *active_options;

static char *join_path(const char *dir, const char *name) {
    size_t a = strlen(dir), b = strlen(name);
    int slash = a > 0 && dir[a - 1] != '/';
    if (a > SIZE_MAX - b - (size_t)slash - 1) return NULL;
    char *p = malloc(a + b + (size_t)slash + 1);
    if (!p) return NULL;
    memcpy(p, dir, a);
    size_t pos = a;
    if (slash) p[pos++] = '/';
    memcpy(p + pos, name, b + 1);
    return p;
}

static int is_dot_name(const char *name) {
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static time_t selected_time(const struct stat *st, const LsOptions *o) {
    if (o->use_ctime) return st->st_ctime;
    if (o->use_atime) return st->st_atime;
    return st->st_mtime;
}

static char *format_name(const char *name, const LsOptions *o) {
    if (!o->quote_nonprint) return strdup(name);
    size_t n = strlen(name);
    char *out = malloc(n + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)name[i];
        out[i] = (c < 32 || c == 127) ? '?' : (char)c;
    }
    out[n] = '\0';
    return out;
}

static char file_type(mode_t mode) {
    if (S_ISDIR(mode)) return 'd';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    return '-';
}

static void permission_string(mode_t m, char out[11]) {
    out[0] = file_type(m);
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_IXUSR) ? 'x' : '-';
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_IXGRP) ? 'x' : '-';
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_IXOTH) ? 'x' : '-';
    if (m & S_ISUID) out[3] = (m & S_IXUSR) ? 's' : 'S';
    if (m & S_ISGID) out[6] = (m & S_IXGRP) ? 's' : 'S';
    if (m & S_ISVTX) out[9] = (m & S_IXOTH) ? 't' : 'T';
    out[10] = '\0';
}

static void print_size(off_t size, const LsOptions *o) {
    if (o->human) {
        double v = (double)size;
        const char *units[] = {"B", "K", "M", "G", "T", "P"};
        int i = 0;
        while (v >= 1024.0 && i < 5) { v /= 1024.0; i++; }
        if (i == 0) printf("%lld%s", (long long)size, units[i]);
        else printf("%.1f%s", v, units[i]);
    } else if (o->kilobytes) {
        printf("%lld", (long long)((size + 1023) / 1024));
    } else {
        printf("%lld", (long long)size);
    }
}

static void print_blocks(const struct stat *st, const LsOptions *o) {
    long long blocks = st->st_blocks > 0 ? (long long)st->st_blocks : 0;
    if (o->human) print_size((off_t)(blocks * 512), o);
    else if (o->kilobytes) printf("%lld", (blocks + 1) / 2);
    else printf("%lld", blocks);
}

static void print_suffix(const struct stat *st, const LsOptions *o) {
    if (!o->classify) return;
    if (S_ISDIR(st->st_mode)) putchar('/');
    else if (S_ISLNK(st->st_mode)) putchar('@');
    else if (S_ISFIFO(st->st_mode)) putchar('|');
    else if (S_ISSOCK(st->st_mode)) putchar('=');
    else if (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) putchar('*');
}

static void print_entry(const Entry *e, const LsOptions *o) {
    char *name = format_name(e->name, o);
    if (!name) name = strdup(e->name);
    if (o->inode) printf("%llu ", (unsigned long long)e->st.st_ino);
    if (o->size_blocks) { print_blocks(&e->st, o); putchar(' '); }

    if (o->long_format || o->numeric_ids) {
        char perms[11], timebuf[32];
        struct tm tmv;
        permission_string(e->st.st_mode, perms);
        time_t t = selected_time(&e->st, o);
        if (localtime_r(&t, &tmv)) strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", &tmv);
        else strcpy(timebuf, "??? ?? ??:??");
        printf("%s %2lu ", perms, (unsigned long)e->st.st_nlink);
        if (o->numeric_ids) printf("%u %u ", (unsigned)e->st.st_uid, (unsigned)e->st.st_gid);
        else {
            struct passwd *pw = getpwuid(e->st.st_uid);
            struct group *gr = getgrgid(e->st.st_gid);
            printf("%-8s %-8s ", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");
        }
        print_size(e->st.st_size, o);
        printf(" %s %s", timebuf, name ? name : e->name);
        if (S_ISLNK(e->st.st_mode)) {
            char target[PATH_MAX + 1];
            ssize_t n = readlink(e->path, target, PATH_MAX);
            if (n >= 0) { target[n] = '\0'; printf(" -> %s", target); }
        }
    } else {
        printf("%s", name ? name : e->name);
        print_suffix(&e->st, o);
    }
    putchar('\n');
    free(name);
}

static int compare_entries(const void *x, const void *y) {
    const Entry *a = x, *b = y;
    const LsOptions *o = active_options;
    int r = 0;
    if (o->sort_size) {
        if (a->st.st_size < b->st.st_size) r = 1;
        else if (a->st.st_size > b->st.st_size) r = -1;
    } else if (o->sort_time) {
        time_t ta = selected_time(&a->st, o), tb = selected_time(&b->st, o);
        if (ta < tb) r = 1;
        else if (ta > tb) r = -1;
    }
    if (!r) r = strcmp(a->name, b->name);
    return o->reverse ? -r : r;
}

static int add_entry(Entry **arr, size_t *n, size_t *cap, const char *name,
                     const char *path, const LsOptions *o) {
    if (name[0] == '.' && !o->all && !o->almost_all) return 0;
    if (o->almost_all && is_dot_name(name)) return 0;
    if (*n == *cap) {
        size_t nc = *cap ? *cap * 2 : 16;
        if (nc < *cap || nc > SIZE_MAX / sizeof(Entry)) return -1;
        Entry *tmp = realloc(*arr, nc * sizeof(Entry));
        if (!tmp) return -1;
        *arr = tmp;
        *cap = nc;
    }
    Entry e = {0};
    e.name = strdup(name);
    e.path = strdup(path);
    if (!e.name || !e.path) { free(e.name); free(e.path); return -1; }
    if (lstat(path, &e.st) != 0) {
        fprintf(stderr, "myls: %s: %s\n", path, strerror(errno));
        free(e.name); free(e.path);
        return 0;
    }
    (*arr)[(*n)++] = e;
    return 0;
}

static int list_path(const char *path, const LsOptions *o, int heading);

static int list_directory(const char *path, const LsOptions *o, int heading) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "myls: %s: %s\n", path, strerror(errno));
        return 1;
    }
    Entry *arr = NULL;
    size_t n = 0, cap = 0;
    struct dirent *de;
    int status = 0;
    while ((de = readdir(d)) != NULL) {
        char *full = join_path(path, de->d_name);
        if (!full || add_entry(&arr, &n, &cap, de->d_name, full, o) < 0) {
            fprintf(stderr, "myls: memory allocation failed\n");
            free(full); status = 1; break;
        }
        free(full);
    }
    closedir(d);
    if (!o->unsorted) { active_options = o; qsort(arr, n, sizeof(Entry), compare_entries); }
    if (heading) printf("%s:\n", path);
    if (o->long_format || o->numeric_ids) {
        long long total = 0;
        for (size_t i = 0; i < n; i++) total += arr[i].st.st_blocks > 0 ? arr[i].st.st_blocks : 0;
        printf("total %lld\n", total);
    }
    for (size_t i = 0; i < n; i++) print_entry(&arr[i], o);
    if (o->recursive) {
        for (size_t i = 0; i < n; i++) {
            if (!S_ISDIR(arr[i].st.st_mode) || is_dot_name(arr[i].name)) continue;
            putchar('\n');
            if (list_path(arr[i].path, o, 1)) status = 1;
        }
    }
    for (size_t i = 0; i < n; i++) { free(arr[i].name); free(arr[i].path); }
    free(arr);
    return status;
}

static int list_path(const char *path, const LsOptions *o, int heading) {
    struct stat st;
    if ((o->directory ? lstat(path, &st) : stat(path, &st)) != 0) {
        fprintf(stderr, "myls: %s: %s\n", path, strerror(errno));
        return 1;
    }
    if (S_ISDIR(st.st_mode) && !o->directory) return list_directory(path, o, heading);
    Entry e = {(char *)path, (char *)path, st};
    print_entry(&e, o);
    return 0;
}

int run_ls(int argc, char **argv, const LsOptions *o) {
    int first = optind, count = argc - first;
    if (count == 0) {
        if (o->directory) return list_path(".", o, 0);
        return list_directory(".", o, 0);
    }
    int status = 0;
    for (int pass = 0; pass < 2; pass++) {
        int printed = 0;
        for (int i = first; i < argc; i++) {
            struct stat st;
            int rc = o->directory ? lstat(argv[i], &st) : stat(argv[i], &st);
            int isdir = rc == 0 && S_ISDIR(st.st_mode) && !o->directory;
            if ((pass == 0 && isdir) || (pass == 1 && !isdir)) continue;
            if (printed && pass == 1) putchar('\n');
            if (list_path(argv[i], o, count > 1 && pass == 1)) status = 1;
            printed++;
        }
    }
    return status;
}
