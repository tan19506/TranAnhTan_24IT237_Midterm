# Midterm Project — Simplified `ls(1)`

A modular C implementation of a subset of the NetBSD `ls(1)` command based on the supplied `ls.pdf` manual.

## Implemented options

`-A`, `-a`, `-c`, `-d`, `-F`, `-f`, `-h`, `-i`, `-k`, `-l`, `-n`, `-q`, `-R`, `-r`, `-S`, `-s`, `-t`, `-u`, `-w`.

## Requirements

- GCC-compatible C compiler
- GNU Make
- Unix-like system (Ubuntu recommended)

## Build

```sh
make
```

## Run examples

```sh
./myls
./myls -a
./myls -l
./myls -li
./myls -F /etc
./myls -S
./myls -t
./myls -R /etc/apt
./myls -d /tmp
./myls /etc/passwd /etc
```

Clean generated files:

```sh
make clean
```

## Source layout

- `src/main.c`: entry point and exit status
- `src/ls_options.c`, `src/ls_options.h`: option parsing
- `src/ls_display.c`, `src/ls_display.h`: filesystem traversal, sorting, and output
- `Makefile`: build rules
- `.gitignore`: ignores compiled artifacts

## Notes

This is a student implementation, not a complete replacement for the system `ls`. Exact output formatting and NetBSD-specific file types may vary by operating system. The project should be tested against the supplied manual before submission.

## GitHub repository

Add your actual GitHub repository URL here before submission.
