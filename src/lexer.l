%{
#include <stdlib.h>
#include "yog/yog.h"
#include "parser.h"

#if 0
static YogBinary* yg_lex_buffer = NULL;
static Bool output_last_terminator = FALSE;

static void 
push_char(char c) 
{
    YogBinary_push(yog_parsing.vm, yg_lex_buffer, c);
}
#endif

#if 0
#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) (result = my_yyinput(buf, max_size))

static int
my_yyinput(char *buf, int max_size)
{
#if 0
    if (eof) {
        if (!output_last_terminator) {
            output_last_terminator = TRUE;
            buf[0] = '\n';
            return 1;
        } 
        else {
            return 0;
        }
    }
#endif

    int ch;
    int len;

    if (feof(yyin))
        return 0;

    for (len = 0; len < max_size; len++) {
        ch = getc(yyin);
        if (ch == EOF)
            break;
        buf[len] = ch;
    }
    return len;
}
#endif

static int
yywrap()
{
    return 1;
}

static ID 
intern(const char* s) 
{
    YogEnv* env = Yog_get_parsing_env();
    YogVm* vm = ENV_VM(env);
    ID id = YogVm_intern(env, vm, s);

    return id;
}

#define RETURN_NAME(token) do { \
    yylval.name = intern(yytext); \
    return token; \
} while (0)

#define YY_SKIP_YYWRAP
%}
%%
<INITIAL>"("        return LPAR;
<INITIAL>")"        return RPAR;
<INITIAL>","        return COMMA;
<INITIAL>"="        return EQUAL;
<INITIAL>"\n"       return NEWLINE;
<INITIAL>"as"       return AS;
<INITIAL>"break"    return BREAK;
<INITIAL>"def"      return DEF;
<INITIAL>"else"     return ELSE;
<INITIAL>"end"      return END;
<INITIAL>"except"   return EXCEPT;
<INITIAL>"finally"  return FINALLY;
<INITIAL>"next"     return NEXT;
<INITIAL>"try"      return TRY;
<INITIAL>"while"    return WHILE;
<INITIAL>"+"        {
    RETURN_NAME(PLUS);
}
<INITIAL>"<"        {
    RETURN_NAME(COMP_OP);
}
<INITIAL>[a-z]+     {
    RETURN_NAME(NAME);
}
<INITIAL>[0-9]+     {
    YogVal val = YogVal_int(atoi(yytext));
    yylval.val = val;
    return NUMBER;
}
<INITIAL>[ \t]
%%
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
