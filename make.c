#include "make.h"

#define CC "cc"
#define CFLAGS "-Wall", "-Wextra", "-std=c11", "-pedantic"
#define LIBS "-lncurses"

int main(int argc, char **argv)
{
    make_rebuild(argc, argv);
    make_assert(argc > 1, "no subcommand provided");

    if (strcmp(argv[1], "build") == 0) {
        make_cc("asdf", "asdf.c", CFLAGS, LIBS);
    } else if (strcmp(argv[1], "run") == 0) {
        make_assert(argc > 2, "no input file path provided");
        make_cc("asdf", "asdf.c", CFLAGS, LIBS);
        make_cmd("./asdf", argv[2]);
    } else {
        make_error("unknown subcommand `%s`", argv[1]);
    }
}
