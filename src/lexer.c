#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "oniguruma.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/parser.h"
#include "yog/regexp.h"
#include "yog/st.h"
#include "yog/yog.h"

#include "parser.h"

static BOOL
readline(YogEnv* env, YogLexer* lexer, FILE* fp) 
{
    YogVal line = lexer->line;
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
    YogVal line = lexer->line;
    char c = OBJ_AS(YogString, line)->body->items[lexer->next_index];
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

static int 
get_rest_size(YogEnv* env, YogLexer* lexer) 
{
    return (YogString_size(env, lexer->line) - 1) - lexer->next_index;
}

static void 
push_multibyte_char(YogEnv* env, YogLexer* lexer) 
{
    YogEncoding* enc = OBJ_AS(YogString, lexer->buffer)->encoding;
    const char* ptr = &OBJ_AS(YogString, lexer->line)->body->items[lexer->next_index];
    int mbc_size = YogEncoding_mbc_size(env, enc, ptr);
    int rest_size = get_rest_size(env, lexer);
    if (rest_size < mbc_size) {
        YOG_ASSERT(env, FALSE, "Invalid multibyte character.");
    }
    int i = 0;
    for (i = 0; i < mbc_size; i++) {
        char c = nextc(lexer);
        add_token_char(env, lexer, c);
    }
}

static int 
next_token(YogEnv* env, YogLexer* lexer) 
{
    clear_buffer(env, lexer);

#define SET_STATE(stat)     lexer->state = stat
#define NEXTC()             nextc(lexer)
#define PUSHBACK(c)         pushback(lexer, c)
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
#define RETURN_NAME(s, type)    do { \
    yylval.name = INTERN(s); \
    return type; \
} while (0)
#define BUFSIZE     (4)
#define RETURN_NAME1(c, type)   do { \
    char buffer[BUFSIZE]; \
    snprintf(buffer, sizeof(buffer), "%c", c); \
    RETURN_NAME(buffer, type); \
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
    int n = atoi(OBJ_AS(YogString, lexer->buffer)->body->items); \
    YogVal val = INT2VAL(n); \
    SET_STATE(LS_OP); \
    RETURN_VAL(val, tNUMBER); \
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
                    sscanf(OBJ_AS(YogString, lexer->buffer)->body->items, "%f", &f);
                    YogVal val = FLOAT2VAL(f);
                    RETURN_VAL(val, tNUMBER);
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
    case '\"':
    case '\'':
        {
            char quote = c;

            c = NEXTC();
            while (c != quote) {
                if (isascii(c)) {
                    if (c == '\\') {
                        int rest_size = get_rest_size(env, lexer);
                        YOG_ASSERT(env, 0 < rest_size, "invalid escape");
                        c = NEXTC();
                        switch (c) {
                        case '\\':
                            ADD_TOKEN_CHAR('\\');
                            break;
                        case 'n':
                            ADD_TOKEN_CHAR('\n');
                            break;
                        default:
                            ADD_TOKEN_CHAR(c);
                            break;
                        }
                    }
                    else {
                        ADD_TOKEN_CHAR(c);
                    }
                    c = NEXTC();
                }
                else {
                    PUSHBACK(c);
                    push_multibyte_char(env, lexer);
                    c = NEXTC();
                }
            }

            yylval.val = YogString_clone(env, lexer->buffer);
            SET_STATE(LS_OP);
            return tSTRING;
            break;
        }
    case '{':
        return tLBRACE;
        break;
    case '}':
        return tRBRACE;
        break;
    case '(':
        SET_STATE(LS_EXPR);
        return tLPAR;
        break;
    case ')':
        SET_STATE(LS_OP);
        return tRPAR;
        break;
    case '[':
        SET_STATE(LS_EXPR);
        return tLBRACKET;
        break;
    case ']':
        SET_STATE(LS_OP);
        return tRBRACKET;
        break;
    case '.':
        SET_STATE(LS_NAME);
        return tDOT;
        break;
    case ',':
        SET_STATE(LS_EXPR);
        return tCOMMA;
        break;
    case '+':
        {
            SET_STATE(LS_EXPR);
            RETURN_NAME1(c, tPLUS);
            break;
        }
    case '/':
        if (lexer->state == LS_OP) {
            return tDIV;
        }
        else {
            char delimitor = c;

            c = NEXTC();
            while (c != delimitor) {
                if (isascii(c)) {
                    if (c == '\\') {
                        int rest_size = get_rest_size(env, lexer);
                        YOG_ASSERT(env, 0 < rest_size, "invalid escape");
                        c = NEXTC();
                        switch (c) {
                        case 'n':
                            ADD_TOKEN_CHAR('\n');
                            break;
                        default:
                            if (c != delimitor) {
                                ADD_TOKEN_CHAR('\\');
                            }
                            ADD_TOKEN_CHAR(c);
                            break;
                        }
                    }
                    else {
                        ADD_TOKEN_CHAR(c);
                    }
                    c = NEXTC();
                }
                else {
                    PUSHBACK(c);
                    push_multibyte_char(env, lexer);
                    c = NEXTC();
                }
            }

            BOOL ignore_case = TRUE;
            c = NEXTC();
            if (c != 'i') {
                ignore_case = FALSE;
                PUSHBACK(c);
            }

            OnigOptionType option = ONIG_OPTION_NONE;
            if (ignore_case) {
                option = ONIG_OPTION_IGNORECASE;
            }
            yylval.val = YogRegexp_new(env, lexer->buffer, option);

            SET_STATE(LS_EXPR);
            return tREGEXP;
            break;
        }
        break;
    case '=':
        SET_STATE(LS_EXPR);

        c = NEXTC();
        if (c == '~') {
            RETURN_NAME("=~", tEQUAL_TILDA);
        }
        else {
            return tEQUAL;
        }
        break;
    case '<':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '<') {
                RETURN_NAME("<<", tLSHIFT);
            }
            else {
                PUSHBACK(c2);
                RETURN_NAME1(c, tLESS);
            }
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
            SET_STATE(LS_EXPR);
            return tNEWLINE;
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
                    PUSHBACK(c);
                    push_multibyte_char(env, lexer);
                    c = NEXTC();
                }
            } while (is_name_char(c));
            PUSHBACK(c);

            int type = 0;
            const char* name = OBJ_AS(YogString, lexer->buffer)->body->items;
            if (lexer->state == LS_NAME) {
                yylval.name = INTERN(name);
                type = tNAME;
            }
            else {
                const KeywordTableEntry* entry = __Yog_lookup_keyword__(name, strlen(name));
                if (entry != NULL) {
                    type = entry->type;
                }
                else {
                    yylval.name = INTERN(name);
                    type = tNAME;
                }
            }
            SET_STATE(LS_OP);
            return type;
            break;
        }
    }
#undef RETURN_NAME1
#undef BUFSIZE
#undef RETURN_NAME
#undef RETURN_VAL
#undef ADD_TOKEN_CHAR
#undef PUSHBACK
#undef NEXTC
#undef SET_STATE

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

        const char* s = &OBJ_AS(YogString, lexer->line)->body->items[lexer->next_index];
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
        YogVal coding = YogEncoding_normalize_name(env, lexer->buffer);
        if (YogString_size(env, coding) - 1 < 1) {
            continue;
        }

        ID id = YogString_intern(env, coding);
        YogVal key = ID2VAL(id);
        YogVal val = YUNDEF;
        if (!YogTable_lookup(env, ENV_VM(env)->encodings, key, &val)) {
            continue;
        }
        encoding = VAL2PTR(val);
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
    YogEncoding* enc = read_encoding(env, lexer);
    if (enc == NULL) {
        enc = YogEncoding_get_default(env);
    }
    OBJ_AS(YogString, lexer->buffer)->encoding = enc;
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
    YogLexer* lexer = ALLOC_OBJ(env, NULL, NULL, YogLexer);
    lexer->state = LS_EXPR;
    lexer->env = env;
    lexer->fp = NULL;
    lexer->line = YogString_new(env);
    lexer->next_index = 0;
    lexer->buffer = YogString_new(env);

    return lexer;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
