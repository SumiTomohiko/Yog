#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "yog/parser.h"
#include "yog/yog.h"

#include "parser.h"

static BOOL
readline(YogEnv* env, YogLexer* lexer, FILE* fp) 
{
    YogString* line = lexer->line;
    YogString_clear(env, line);

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

    if (YogString_size(env, line) - 1 == 0) {
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
is_name_char(char c) 
{
    if (isascii(c)) {
        return isalnum(c) || (c == '_');
    }
    else {
        return TRUE;
    }
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

static YogEncoding* 
get_encoding(YogEnv* env, YogLexer* lexer) 
{
    if (lexer->encoding == NULL) {
        lexer->encoding = YogEncoding_get_default(env);
    }
    return lexer->encoding;
}

static int 
next_token(YogEnv* env, YogLexer* lexer) 
{
    clear_buffer(env, lexer);

#define NEXTC()                 nextc(lexer)
#define PUSHBACK(c)             pushback(lexer, c)
    char c = 0;
    do {
        if (lexer->next_index < YogString_size(env, lexer->line) - 1) {
            c = NEXTC();
            if (is_whitespace(c)) {
                skip_whitespace(lexer);
                continue;
            }
            else if (c == '#') {
                do {
                    c = NEXTC();
                } while ((c != '\r') && (c != '\n'));
            }
            break;
        }
        else {
            if (!readline(env, lexer, lexer->fp)) {
                return -1;
            }
            lexer->next_index = 0;
        }
    } while (1);

#define ADD_TOKEN_CHAR(c)       add_token_char(env, lexer, c)
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
    switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            {
                do {
                    ADD_TOKEN_CHAR(c);
                    c = NEXTC();
                } while (isdigit(c));

#define RETURN_INT  do { \
    int n = atoi(lexer->buffer->body->items); \
    YogVal val = YogVal_int(n); \
    RETURN_VAL(val, NUMBER); \
} while (0)
                if (c == '.') {
                    int c2 = NEXTC();
                    if (isdigit(c2)) {
                        ADD_TOKEN_CHAR(c);
                        do {
                            ADD_TOKEN_CHAR(c2);
                            c2 = NEXTC();
                        } while (isdigit(c2));
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
        default:
            {
                do {
                    if (isascii(c)) {
                        ADD_TOKEN_CHAR(c);
                        c = NEXTC();
                    }
                    else {
                        YogEncoding* enc = get_encoding(env, lexer);
                        const char* ptr = &lexer->line->body->items[lexer->next_index - 1];
                        int mbc_size = YogEncoding_mbc_size(env, enc, ptr);
                        int rest_size = (YogString_size(env, lexer->line) - 1) - lexer->next_index;
                        if (rest_size < mbc_size) {
                            Yog_assert(env, FALSE, "Invalid multibyte character.");
                        }
                        int i = 0;
                        for (i = 0; i < mbc_size; i++) {
                            ADD_TOKEN_CHAR(c);
                            c = NEXTC();
                        }
                    }
                } while (is_name_char(c));
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
#undef ADD_TOKEN_CHAR
#undef PUSHBACK
#undef NEXTC

    return 0;
}

static BOOL 
is_coding_char(char c) 
{
    return isalnum(c) || (c == '_') || (c == '-');
}

static YogEncoding* 
read_encoding(YogEnv* env, YogLexer* lexer) 
{
    YogEncoding* encoding = NULL;

    while (readline(env, lexer, lexer->fp)) {
        lexer->next_index = 0;

        skip_whitespace(lexer);
        int c = nextc(lexer);
        if (c != '#') {
            continue;
        }

        const char* s = &lexer->line->body->items[lexer->next_index];
#define KEY     "coding"
        const char* ptr = strstr(s, KEY);
        if (ptr == NULL) {
            continue;
        }
        ptr += strlen(KEY);
#undef KEY
        if ((*ptr != '=') && (*ptr != ':')) {
            continue;
        }
        lexer->next_index += ptr - s + 1;
        skip_whitespace(lexer);

        clear_buffer(env, lexer);
        c = nextc(lexer);
        while (is_coding_char(c)) {
            add_token_char(env, lexer, c);
            c = nextc(lexer);
        }
        YogString* coding = YogEncoding_normalize_name(env, lexer->buffer);
        if (YogString_size(env, coding) - 1 < 1) {
            continue;
        }

        ID id = YogString_intern(env, coding);
        YogVal key = YogVal_symbol(id);
        YogVal val = YogVal_undef();
        if (!YogTable_lookup(env, ENV_VM(env)->encodings, key, &val)) {
            continue;
        }
        encoding = YOGVAL_PTR(val);
        break;
    }

    return encoding;
}

static void 
reset_lexer(YogEnv* env, YogLexer* lexer) 
{
    fseek(lexer->fp, 0, SEEK_SET);
    YogString_clear(env, lexer->line);
    lexer->next_index = 0;
}

void 
YogLexer_read_encoding(YogEnv* env, YogLexer* lexer) 
{
    lexer->encoding = read_encoding(env, lexer);
    reset_lexer(env, lexer);
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
