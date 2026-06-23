#include "clox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void run_file(const char *file_name);
static void run_prompt();

int main(int argc, char **argv)
{
    if(argc > 2)
    {
        fprintf(stderr, "usage: clox [script]");
        exit(EXIT_FAILURE);
    }
    else if(argc == 2)
    {
        run_file(argv[1]);
    }
    else // just `clox`, run REPL
    {
        run_prompt();
    }
}


static void run(CGS_StrView file_name, CGS_DStr line);

static void run_file(const char *file_name)
{
    FILE *f = fopen(file_name, "r");
    CGS_DStr file_data = cgs_dstr_init();
    
    errno = 0;
    CGS_Error err = cgs_fread_until(&file_data, f, EOF);
    if(err.ec != CGS_OK)
    {
        cgs_fprintf(stderr, "could not read file %?: %?: %?\n", file_name, err, strerror(errno));
    }
    
    run(cgs_strv(file_name), file_data);
}

static void run_prompt()
{
    CGS_DStr line = cgs_dstr_init();
    
    while(!feof(stdin))
    {
        cgs_fread_until(&line, stdin, '\n');
        run(cgs_strv("<REPL>"), line);
    }
}

static void run(CGS_StrView file_name, CGS_DStr src)
{
    TokenList tokens = tokenize(file_name, src);
    
    for(c_each(tokit, TokenList, tokens))
    {
        Token *tok = tokit.ref;
        
        cgs_printf("(%?, %?, %?)\n", tok2str[tok->kind], tok->lexeme, tok->kind == TK_float ? tok->data.dbl : 0.0);
    }
}
