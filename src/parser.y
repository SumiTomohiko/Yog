%{
#include <stdio.h>
#include "yog/yog.h"

static void 
yyerror(char* s)
{
    fprintf(stderr, "%s\n", s);
}

#if 0
/* XXX: To avoid warning. Better way? */
int yylex(void);
#endif
%}

%union {
    YogNode* node;
}

%token ADD
%token EQUAL
%token IDENTIFIER
%token PLUS

%type<node> module

%%
module: ADD { $$ = NULL; }
      ;
%%
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
