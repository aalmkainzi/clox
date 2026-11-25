#define _GNU_SOURCE

#define NOB_REBUILD_URSELF(binary_path, source_path) "cc", "-Iexternal", "-o", binary_path, "external/cgs/cgs.c", source_path
#include "cgs/cgs.h"
#include <stdlib.h>
#define NOB_IMPLEMENTATION
#include "nob.h"

#define nob_isatty isatty
#define nob_fileno fileno

bool run_dir_tests(Nob_Walk_Entry entry);
bool run_test_on_entry(Nob_Walk_Entry entry);

char *red_begin   = "";
char *green_begin = "";
char *color_reset = "";
 
int main()
{
    if (nob_isatty(nob_fileno(stdout)))
    {
        red_begin   = "\x1b[31m";
        green_begin = "\x1b[32m";
        color_reset = "\x1b[0m";
    }

    nob_walk_dir("test/tests", run_dir_tests);

    cgs_printfln("%?all tests pass%?\n", green_begin, color_reset);
}

bool run_dir_tests(Nob_Walk_Entry entry)
{
    if (entry.type != NOB_FILE_DIRECTORY || entry.level == 0)
        return true;

    char *dir_path = nob_temp_strdup(entry.path);
    char *dir_name = nob_temp_file_name(entry.path);

    cgs_printfln("Running %? tests:", dir_name);

    bool ok = nob_walk_dir(dir_path, run_test_on_entry, .data = dir_name);

    return ok;
}

bool run_test_on_entry(Nob_Walk_Entry entry)
{
    if (entry.type != NOB_FILE_REGULAR)
        return true;

    Cmd cmd = {};
    cmd_append(&cmd, "./clox", entry.path, "-t");
    bool success = cmd_run(&cmd, .stdout_path = "/dev/null");

    cgs_putc(stderr, '\n');

    cmd_free(cmd);

    if (!success)
    {
        cgs_fprintfln(
            stderr,
            "%?failed%? on %?",
            red_begin,
            color_reset,
            nob_temp_file_name(entry.path)
        );
        return false;
    }

    return true;
}
