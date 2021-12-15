// Copyright 2021 Shoumodip Kar <shoumodipkar@gmail.com>

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef MAKE_H
#define MAKE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

long make_modified(const char *file_path)
{
    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
        return -1;
    } else {
        return file_stat.st_mtime;
    }
}

#define make_echo(...) \
    do { \
        printf("[info] "); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } while (0)

#define make_error(...) \
    do { \
        fprintf(stderr, "[error] "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        exit(1); \
    } while (0)

#define make_assert(condition, ...) if (!(condition)) make_error(__VA_ARGS__)

void make_vcmd(char **args)
{
    printf("[cmd] ");
    for (size_t i = 0; args[i]; ++i) printf("%s ", args[i]);
    printf("\n");

    pid_t child = fork();

    if (child == 0) {
        make_assert(execvp(args[0], args) != -1, "could not execute process: %s\n", strerror(errno));
    } else if (child > 0) {
        int status;
        wait(&status);

        int code = WEXITSTATUS(status);
        make_assert(code == 0, "process exited abormally with code %d", code);
    } else {
        make_error("could not execute process: %s\n", strerror(errno));
    }
}

#define make_cmd(...) make_vcmd((char *[]) {__VA_ARGS__, NULL})

#define make_cc(binary, source, ...) \
    if (make_modified(source) > make_modified(binary)) { \
        make_cmd("cc", "-o", binary, source, __VA_ARGS__); \
    }

#define make_rebuild(argc, argv) \
    if (argc && make_modified(__FILE__) > make_modified(*argv)) { \
        make_cmd("cc", "-o", *argv, __FILE__); \
        make_vcmd(argv); \
        exit(0); \
    }

#endif // MAKE_H
