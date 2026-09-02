
#include "cgs/cgs.h"
#include <stdlib.h>
#define NOB_IMPLEMENTATION
#include "nob.h"

#define nob_isatty isatty
#define nob_fileno fileno

bool run_repl_tests(Nob_Walk_Entry entry);
bool run_file_tests(Nob_Walk_Entry entry);

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

    nob_walk_dir("test/tests", run_repl_tests);
    nob_walk_dir("test/tests", run_file_tests);

    cgs_printfln("%?all tests pass%?\n", green_begin, color_reset);
}

bool run_test_on_entry(Nob_Walk_Entry entry, bool repl)
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

    CGS_StrView first_linev = cgs_skip(first_line, "// expected: ");
    bool expected_success   = cgs_equal(first_linev, "ok");
    bool result;

    Cmd cmd = {};
    if (repl)
    {
        cmd_append(&cmd, "./clox");
        result = cmd_run(&cmd, .stdin_path = entry.path, .stdout_path = "/dev/null");
    }
    else
    {
        cmd_append(&cmd, "./clox", entry.path);
        result = cmd_run(&cmd, .stdout_path = "/dev/null");
    }

    cgs_putc(stderr, '\n');

    cmd_free(cmd);
    cgs_dstr_deinit(&first_line);

    CGS_StrView success_map[] = {
        [false] = cgs_strv("fail"),
        [true]  = cgs_strv("ok"),
    };
    if (result != expected_success)
    {
        cgs_fprintfln(
            stderr,
            "%?%? failed%?: expected result is '%?', but got '%?'",
            red_begin,
            nob_temp_file_name(entry.path),
            color_reset,
            success_map[expected_success],
            success_map[result]
        );
        exit(EXIT_FAILURE);
    }

    return true;
}

bool run_file_tests(Nob_Walk_Entry entry)
{
    return run_test_on_entry(entry, false);
}

bool run_repl_tests(Nob_Walk_Entry entry)
{
    return run_test_on_entry(entry, true);
}
