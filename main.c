#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "clox.h"
#include "expr.h"
#include "tokenize.h"

// #define local_ref(x) \
// &(typeof((void)0,x)[]){x}

static void run_file(const char *file_name);
static void run_prompt();

int main(int argc, char **argv)
{
    Expr expr = {
        .kind = EXPR_BINARY,
        .binary = {
            .op = &(Token){.kind = TK_star, .lexeme = cgs_strv("*")},
            .left = &(Expr){
                .kind = EXPR_GROUPING,
                .grouping.expr = &(Expr){
                    .kind = EXPR_BINARY,
                    .binary = {
                        .op = &(Token){.kind = TK_plus, .lexeme = cgs_strv("+")},
                        .left = &(Expr){.kind = EXPR_LITERAL, .literal = &(double){1}},
                        .right = &(Expr){.kind = EXPR_LITERAL, .literal = &(double){2}}
                    }
                }
            },
            .right = &(Expr){
                .kind = EXPR_GROUPING,
                .grouping.expr = &(Expr){
                    .kind = EXPR_BINARY,
                    .binary = {
                        .op = &(Token){.kind = TK_minus, .lexeme = cgs_strv("-")},
                        .left = &(Expr){.kind = EXPR_LITERAL, .literal = &(double){4}},
                        .right = &(Expr){.kind = EXPR_LITERAL, .literal = &(double){3}}
                    }
                }
            }
        }
    };
    
    rpn_print(cgs_writer_ptr(stdout), &expr);
    
    return 0;
    
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
