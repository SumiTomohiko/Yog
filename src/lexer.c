#include <stdio.h>
#include <string.h>
#include "yog/yog.h"

#include "parser.h"

static BOOL
readline(YogEnv* env, YogLexer* lexer) 
{
    YogString* line = lexer->line;
    YogString_clear(env, line);

    FILE* fp = lexer->fp;
    int c = 0;
    do {
        c = fgetc(fp);
        if (c == EOF) {
            break;
        }
        if ((c == '\n') || (c == '\r')) {
            YogString_push(env, line, c);
            break;
        }
        YogString_push(env, line, c);
    } while (1);

    if (YogString_size(env, line) == 0) {
        return FALSE;
    }

    if (c == '\r') {
        c = fgetc(fp);
        if (c != '\n') {
            ungetc(c, fp);
        }
    }

    return TRUE;
}

static char
nextc(YogLexer* lexer) 
{
    YogString* line = lexer->line;
    char c = line->body->items[lexer->next_index];
    lexer->next_index++;

    return c;
}

static void 
pushback(YogLexer* lexer, char c) 
{
    lexer->next_index--;
}

static BOOL 
is_whitespace(char c) 
{
    return (c == ' ') || (c == '\t');
}

static void 
skip_whitespace(YogLexer* lexer) 
{
    char c = 0;
    do {
        c = nextc(lexer);
    } while (is_whitespace(c));

    pushback(lexer, c);
}

static BOOL
is_alphabet(char c) 
{
    return (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z'));
}

static BOOL 
is_number(char c) 
{
    return ('0' <= c) && (c <= '9');
}

static BOOL 
is_alphnum(char c) 
{
    return is_alphabet(c) || is_number(c);
}

static BOOL 
is_ident_char(char c) 
{
    return is_alphnum(c) || (c == '_');
}

static void 
clear_buffer(YogEnv* env, YogLexer* lexer) 
{
    YogString_clear(env, lexer->buffer);
}

static void 
add_token_char(YogEnv* env, YogLexer* lexer, char c) 
{
    YogString_push(env, lexer->buffer, c);
}

#include "src/keywords.inc"

static int 
next_token(YogEnv* env, YogLexer* lexer) 
{
    clear_buffer(env, lexer);

    if (YogString_size(env, lexer->line) - 1 == lexer->next_index) {
        if (!readline(env, lexer)) {
            return -1;
        }
        lexer->next_index = 0;
    }

    skip_whitespace(lexer);

#define NEXTC()                 nextc(lexer)
#define ADD_TOKEN_CHAR(c)       add_token_char(env, lexer, c)
#define PUSHBACK(c)             pushback(lexer, c)
#define RETURN_VAL(val, type)   do { \
    yylval.val = val; \
    return type; \
} while (0)
#define BUFSIZE     (4)
#define RETURN_NAME1(c, type)   do { \
    char buffer[BUFSIZE]; \
    snprintf(buffer, sizeof(buffer), "%c", c); \
    ID name = INTERN(buffer); \
    yylval.name = name; \
    return type; \
} while (0)
    char c = NEXTC();
    switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            {
                do {
                    ADD_TOKEN_CHAR(c);
                    c = NEXTC();
                } while (is_number(c));

#define RETURN_INT  do { \
    int n = atoi(lexer->buffer->body->items); \
    YogVal val = YogVal_int(n); \
    RETURN_VAL(val, NUMBER); \
} while (0)
                if (c == '.') {
                    int c2 = NEXTC();
                    if (is_number(c2)) {
                        ADD_TOKEN_CHAR(c);
                        do {
                            ADD_TOKEN_CHAR(c2);
                            c2 = NEXTC();
                        } while (is_number(c2));
                        PUSHBACK(c2);

                        float f = 0;
                        sscanf(lexer->buffer->body->items, "%f", &f);
                        YogVal val = YogVal_float(f);
                        RETURN_VAL(val, NUMBER);
                    }
                    else {
                        PUSHBACK(c2);
                        PUSHBACK(c);
                        RETURN_INT;
                    }
                }
                else {
                    PUSHBACK(c);
                    RETURN_INT;
                }
#undef RETURN_INT
                break;
            }
        case '(':
            return LPAR;
            break;
        case ')':
            return RPAR;
            break;
        case '[':
            return LBRACKET;
            break;
        case ']':
            return RBRACKET;
            break;
        case '.':
            return DOT;
            break;
        case '+':
            {
                RETURN_NAME1(c, PLUS);
                break;
            }
        case '=':
            {
                return EQUAL;
                break;
            }
        case '<':
            {
                RETURN_NAME1(c, LESS);
                break;
            }
        case '\r':
            {
                c = NEXTC();
                if (c != '\n') {
                    PUSHBACK(c);
                }
            }
            /* FALLTHRU */
        case '\n':
            {
                return NEWLINE;
                break;
            }
        case '\0':
            return 0;
            break;
        default:
            {
                do {
                    ADD_TOKEN_CHAR(c);
                    c = NEXTC();
                } while (is_ident_char(c));
                PUSHBACK(c);

                const char* name = lexer->buffer->body->items;
                const KeywordTableEntry* entry = __Yog_lookup_keyword__(name, strlen(name));
                if (entry != NULL) {
                    return entry->type;
                }
                else {
                    yylval.name = INTERN(name);
                    return NAME;
                }
                break;
            }
    }
#undef RETURN_NAME1
#undef BUFSIZE
#undef RETURN_VAL
#undef PUSHBACK
#undef ADD_TOKEN_CHAR
#undef NEXTC

    return 0;
}

int 
yylex(YogLexer* lexer) 
{
    return next_token(lexer->env, lexer);
}

YogLexer* 
YogLexer_new(YogEnv* env) 
{
    YogLexer* lexer = ALLOC_OBJ(env, NULL, YogLexer);
    lexer->env = env;
    lexer->encoding = NULL;
    lexer->fp = NULL;
    lexer->line = YogString_new(env);
    lexer->next_index = 0;
    lexer->buffer = YogString_new(env);

    return lexer;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
