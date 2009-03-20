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
#include "yog/token.h"
#include "yog/yog.h"

#include "parser.h"

static BOOL
readline(YogEnv* env, YogVal lexer, FILE* fp) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, lexer);

    YogVal line = PTR_AS(YogLexer, lexer)->line;
    PUSH_LOCAL(env, line);
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
        RETURN(env, FALSE);
    }

    if (c == '\r') {
        c = fgetc(fp);
        if (c != '\n') {
            ungetc(c, fp);
        }
    }

    RETURN(env, TRUE);
}

static char
nextc(YogVal lexer) 
{
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    unsigned int next_index = PTR_AS(YogLexer, lexer)->next_index;
    char c = OBJ_AS(YogString, line)->body->items[next_index];
    PTR_AS(YogLexer, lexer)->next_index++;

    return c;
}

static void 
pushback(YogVal lexer, char c) 
{
    PTR_AS(YogLexer, lexer)->next_index--;
}

static BOOL 
is_whitespace(char c) 
{
    return (c == ' ') || (c == '\t');
}

static void 
skip_whitespace(YogVal lexer) 
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
clear_buffer(YogEnv* env, YogVal lexer)
{
    YogString_clear(env, PTR_AS(YogLexer, lexer)->buffer);
}

static void 
add_token_char(YogEnv* env, YogVal lexer, char c) 
{
    YogString_push(env, PTR_AS(YogLexer, lexer)->buffer, c);
}

#include "keywords.inc"

static int 
get_rest_size(YogEnv* env, YogVal lexer) 
{
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    unsigned int next_index = PTR_AS(YogLexer, lexer)->next_index;
    return (YogString_size(env, line) - 1) - next_index;
}

static void 
push_multibyte_char(YogEnv* env, YogVal lexer) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, lexer);

    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
    YogEncoding* enc = OBJ_AS(YogString, buffer)->encoding;
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    unsigned int next_index = PTR_AS(YogLexer, lexer)->next_index;
    const char* ptr = &OBJ_AS(YogString, line)->body->items[next_index];
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

    RETURN_VOID(env);
}

BOOL 
YogLexer_next_token(YogEnv* env, YogVal lexer, YogVal* token)
{
    SAVE_ARG(env, lexer);

    YogVal token_val = YUNDEF;
    PUSH_LOCAL(env, token_val);

    clear_buffer(env, lexer);

#define SET_STATE(stat)     PTR_AS(YogLexer, lexer)->state = stat
#define NEXTC()             nextc(lexer)
#define PUSHBACK(c)         pushback(lexer, c)
    char c = 0;
    do {
        unsigned int next_index = PTR_AS(YogLexer, lexer)->next_index;
        YogVal line = PTR_AS(YogLexer, lexer)->line;
        if (next_index < YogString_size(env, line) - 1) {
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
            if (!readline(env, lexer, PTR_AS(YogLexer, lexer)->fp)) {
                RETURN(env, FALSE);
            }
            PTR_AS(YogLexer, lexer)->next_index = 0;
        }
    } while (1);

#define ADD_TOKEN_CHAR(c)               add_token_char(env, lexer, c)
#define RETURN_VAL_TOKEN(type_, val_)   do { \
    *token = YogToken_new(env); \
    PTR_AS(YogToken, *token)->type = (type_); \
    PTR_AS(YogToken, *token)->u.val = (val_); \
    RETURN(env, TRUE); \
} while (0)
#define RETURN_ID_TOKEN(type_, s)       do { \
    ID id = INTERN(s); \
    *token = YogToken_new(env); \
    PTR_AS(YogToken, *token)->type = (type_); \
    PTR_AS(YogToken, *token)->u.id = id; \
    RETURN(env, TRUE); \
} while (0)
#define RETURN_TOKEN(type_)             do { \
    RETURN_VAL_TOKEN((type_), YUNDEF); \
} while (0)
#define BUFSIZE                         (4)
#define RETURN_ID_TOKEN1(type, c)       do { \
    char buffer[BUFSIZE]; \
    snprintf(buffer, sizeof(buffer), "%c", (c)); \
    RETURN_ID_TOKEN((type), buffer); \
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
    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer; \
    int n = atoi(OBJ_AS(YogString, buffer)->body->items); \
    YogVal val = INT2VAL(n); \
    SET_STATE(LS_OP); \
    RETURN_VAL_TOKEN(TK_NUMBER, val); \
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
                    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
                    sscanf(OBJ_AS(YogString, buffer)->body->items, "%f", &f);
                    YogVal val = FLOAT2VAL(f);
                    RETURN_VAL_TOKEN(TK_NUMBER, val);
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

            YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
            token_val = YogString_clone(env, buffer);
            SET_STATE(LS_OP);
            RETURN_VAL_TOKEN(TK_STRING, token_val);
            break;
        }
    case '{':
        RETURN_TOKEN(TK_LBRACE);
        break;
    case '}':
        RETURN_TOKEN(TK_RBRACE);
        break;
    case '(':
        SET_STATE(LS_EXPR);
        RETURN_TOKEN(TK_LPAR);
        break;
    case ')':
        SET_STATE(LS_OP);
        RETURN_TOKEN(TK_RPAR);
        break;
    case '[':
        SET_STATE(LS_EXPR);
        RETURN_TOKEN(TK_LBRACKET);
        break;
    case ']':
        SET_STATE(LS_OP);
        RETURN_TOKEN(TK_RBRACKET);
        break;
    case '.':
        SET_STATE(LS_NAME);
        RETURN_TOKEN(TK_DOT);
        break;
    case ',':
        SET_STATE(LS_EXPR);
        RETURN_TOKEN(TK_COMMA);
        break;
    case '+':
        {
            SET_STATE(LS_EXPR);
            RETURN_ID_TOKEN1(TK_PLUS, c);
            break;
        }
    case '/':
        if (PTR_AS(YogLexer, lexer)->state == LS_OP) {
            YOG_ASSERT(env, FALSE, "not supported");
#if 0
            RETURN_TOKEN(TK_DIV);
#endif
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
            YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
            token_val = YogRegexp_new(env, buffer, option);

            SET_STATE(LS_EXPR);
            RETURN_VAL_TOKEN(TK_REGEXP, token_val);
            break;
        }
        break;
    case '=':
        SET_STATE(LS_EXPR);

        c = NEXTC();
        if (c == '~') {
            RETURN_ID_TOKEN(TK_EQUAL_TILDA, "=~");
        }
        else {
            RETURN_TOKEN(TK_EQUAL);
        }
        break;
    case '<':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '<') {
                RETURN_ID_TOKEN(TK_LSHIFT, "<<");
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN1(TK_LESS, c);
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
            RETURN_TOKEN(TK_NEWLINE);
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

            YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
            const char* name = OBJ_AS(YogString, buffer)->body->items;
            if (PTR_AS(YogLexer, lexer)->state == LS_NAME) {
                ID id = INTERN(name);
                *token = YogToken_new(env);
                PTR_AS(YogToken, *token)->type = TK_NAME;
                PTR_AS(YogToken, *token)->u.id = id;
            }
            else {
                const KeywordTableEntry* entry = __Yog_lookup_keyword__(name, strlen(name));
                if (entry != NULL) {
                    *token = YogToken_new(env);
                    PTR_AS(YogToken, *token)->type = entry->type;
                    PTR_AS(YogToken, *token)->u.val = YUNDEF;
                }
                else {
                    ID id = INTERN(name);
                    *token = YogToken_new(env);
                    PTR_AS(YogToken, *token)->type = TK_NAME;
                    PTR_AS(YogToken, *token)->u.id = id;
                }
            }
            SET_STATE(LS_OP);
            RETURN(env, TRUE);
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

    RETURN(env, 0);
}

static BOOL 
is_coding_char(char c) 
{
    return isalnum(c) || (c == '_') || (c == '-');
}

static YogEncoding* 
read_encoding(YogEnv* env, YogVal lexer) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, lexer);

    YogEncoding* encoding = NULL;

    while (readline(env, lexer, PTR_AS(YogLexer, lexer)->fp)) {
        PTR_AS(YogLexer, lexer)->next_index = 0;

        skip_whitespace(lexer);
        int c = nextc(lexer);
        if (c != '#') {
            continue;
        }

        YogVal line = PTR_AS(YogLexer, lexer)->line;
        unsigned int next_index = PTR_AS(YogLexer, lexer)->next_index;
        const char* s = &OBJ_AS(YogString, line)->body->items[next_index];
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
        PTR_AS(YogLexer, lexer)->next_index += ptr - s + 1;
        skip_whitespace(lexer);

        clear_buffer(env, lexer);
        c = nextc(lexer);
        while (is_coding_char(c)) {
            add_token_char(env, lexer, c);
            c = nextc(lexer);
        }
        YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
        YogVal coding = YogEncoding_normalize_name(env, buffer);
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

    RETURN(env, encoding);
}

static void 
reset_lexer(YogEnv* env, YogVal lexer) 
{
    fseek(PTR_AS(YogLexer, lexer)->fp, 0, SEEK_SET);
    YogString_clear(env, PTR_AS(YogLexer, lexer)->line);
    PTR_AS(YogLexer, lexer)->next_index = 0;
}

void 
YogLexer_read_encoding(YogEnv* env, YogVal lexer) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, lexer);

    YogEncoding* enc = read_encoding(env, lexer);
    if (enc == NULL) {
        enc = YogEncoding_get_default(env);
    }
    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
    OBJ_AS(YogString, buffer)->encoding = enc;
    reset_lexer(env, lexer);

    RETURN_VOID(env);
}

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogLexer* lexer = ptr;
#define KEEP(member)    lexer->member = YogVal_keep(env, lexer->member, keeper)
    KEEP(line);
    KEEP(buffer);
#undef KEEP
}

YogVal 
YogLexer_new(YogEnv* env) 
{
    SAVE_LOCALS(env);

    YogVal lexer = PTR2VAL(ALLOC_OBJ(env, keep_children, NULL, YogLexer));
    PTR_AS(YogLexer, lexer)->state = LS_EXPR;
    PTR_AS(YogLexer, lexer)->env = env;
    PTR_AS(YogLexer, lexer)->fp = NULL;
    PTR_AS(YogLexer, lexer)->line = YUNDEF;
    PTR_AS(YogLexer, lexer)->next_index = 0;
    PTR_AS(YogLexer, lexer)->buffer = YUNDEF;
    PUSH_LOCAL(env, lexer);

    YogVal line = YogString_new(env);
    PTR_AS(YogLexer, lexer)->line = line;

    YogVal buffer = YogString_new(env);
    PTR_AS(YogLexer, lexer)->buffer = buffer;

    RETURN(env, lexer);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
