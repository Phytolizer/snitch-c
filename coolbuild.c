#include "coolbuild.h"

#define WFLAGS "-Wall", "-Wextra", "-Wpedantic", "-Wmissing-prototypes"
#define GFLAG "-ggdb3"
#define IFLAGS "-Iinclude"
#define LDFLAGS "-lpcre2-8"
#define LDFLAGS_FMT "v"

#define CFLAGS "-std=c11", WFLAGS, GFLAG, IFLAGS

const char* const sources[] = {
        "source/list.c",
        "source/main.c",
        "source/report.c",
        "source/string_span.c",
        "source/todo.c",
};

static const char* cc(void) {
    return "gcc";
}

int main(int argc, char** argv) {
    REBUILD_SELF(argc, argv);

    MKDIRS("build", "obj", "source");

    const char** objects;
    COMPILE_OBJECTS(sources, CFLAGS, &objects);

    char** link_cmd =
            collect_args("vpvv" LDFLAGS_FMT, cc(), objects, "-o", "build/snitch", LDFLAGS);
    run_command(link_cmd);

    if (argc > 1) {
        if (strcmp(argv[1], "run") == 0) {
            char** run_args = collect_args("vp", "build/snitch", &argv[2]);
            run_command(run_args);
        } else {
            fprintf(stderr, "Unknown subcommand: %s\n", argv[1]);
            return 1;
        }
    }
}
