#include "snitch/list.h"
#include "snitch/report.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <subcommand>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {
        list_subcommand();
    } else if (strcmp(argv[1], "report") == 0) {
        report_subcommand();
    } else {
        fprintf(stderr, "Unknown subcommand: %s\n", argv[1]);
        return 1;
    }
}
