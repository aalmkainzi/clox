#define _GNU_SOURCE

#define NOB_REBUILD_URSELF(binary_path, source_path) "cc", "-Iexternal", "-o", binary_path, "external/cgs/cgs.c", source_path
#include "cgs/cgs.h"
#include <stdlib.h>
#define NOB_IMPLEMENTATION
#include "nob.h"

#define nob_isatty isatty
#define nob_fileno fileno

bool run_test_on_entry(Nob_Walk_Entry entry);

char *red_begin   = "";
char *green_begin = "";
char *color_reset = "";
 
int main(int argc, char *argv[])
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (nob_isatty(nob_fileno(stdout)))
    {
        red_begin   = "\x1b[31m";
        green_begin = "\x1b[32m";
        color_reset = "\x1b[0m";
    }

    nob_walk_dir("test/tests", run_test_on_entry);

    cgs_printfln("%?all tests pass%?\n", green_begin, color_reset);
}

bool run_test_on_entry(Nob_Walk_Entry entry)
{
    if (entry.type != NOB_FILE_REGULAR)
        return true;

    FILE *f = fopen(entry.path, "r");
    if (f == NULL)
        return true;

    CGS_DStr first_line = cgs_dstr_init();
    CGS_Error err       = cgs_fread_until(&first_line, f, '\n');
    fclose(f);
    if (err.ec != CGS_OK)
        return true;

    Cmd cmd = {};
    cmd_append(&cmd, "./clox", entry.path, "-t");
    bool success = cmd_run(&cmd, .stdout_path = "/dev/null");

    cgs_putc(stderr, '\n');

    cmd_free(cmd);
    cgs_dstr_deinit(&first_line);

    if (!success)
    {
        cgs_fprintfln(
            stderr,
            "%?failed%? on %?",
            red_begin,
            color_reset,
            nob_temp_file_name(entry.path)
        );
        exit(EXIT_FAILURE);
    }

    return true;
}
