#include "coolbuild.h"

#define WFLAGS "-Wall", "-Wextra", "-Wpedantic", "-Wmissing-prototypes"
#define GFLAG "-ggdb3"
#define IFLAGS "-Iinclude"
#define LDFLAGS "-lpcre2-8"
#define LDFLAGS_FMT "v"

#define CFLAGS "-std=gnu11", WFLAGS, GFLAG, IFLAGS

const char* const sources[] = {
        "source/main.c",
};

const char* cc(void) {
    return "gcc";
}

int main(int argc, char** argv) {
    REBUILD_SELF(argc, argv);

    MKDIRS("build", "obj", "source");

    const char** objects;
    COMPILE_OBJECTS(sources, CFLAGS, &objects);

    char** link_cmd =
            collect_args("vpvv" LDFLAGS_FMT, cc(), objects, "-o", "build/snitch", LDFLAGS);
    run_cmd(link_cmd);
}
