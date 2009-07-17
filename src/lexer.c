#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "oniguruma.h"
#include "yog/encoding.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/float.h"
#include "yog/gc.h"
#include "yog/parser.h"
#include "yog/regexp.h"
#include "yog/table.h"
#include "yog/thread.h"
#include "yog/yog.h"

#include "parser.h"

static void 
YogToken_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogToken* token = ptr;
    switch (token->type) {
    case TK_NUMBER: /* FALLTHRU */
    case TK_REGEXP: /* FALLTHRU */
    case TK_STRING: /* FALLTHRU */
        YogGC_keep(env, &token->u.val, keeper, heap);
        break;
    default:
        break;
    }
}

static YogVal 
YogToken_new(YogEnv* env) 
{
    YogVal token = ALLOC_OBJ(env, YogToken_keep_children, NULL, YogToken);
    PTR_AS(YogToken, token)->type = 0;
    PTR_AS(YogToken, token)->u.val = YUNDEF;
    PTR_AS(YogToken, token)->lineno = 0;

    return token;
}

static YogVal 
ValToken_new(YogEnv* env, unsigned int type, YogVal val, unsigned int lineno) 
{
    SAVE_ARG(env, val);

    YogVal token = YogToken_new(env);
    PTR_AS(YogToken, token)->type = type;
    PTR_AS(YogToken, token)->u.val = val;
    PTR_AS(YogToken, token)->lineno = lineno;

    RETURN(env, token);
}

static YogVal 
IDToken_new(YogEnv* env, unsigned int type, ID id, unsigned int lineno) 
{
    YogVal token = YogToken_new(env);
    PTR_AS(YogToken, token)->type = type;
    PTR_AS(YogToken, token)->u.id = id;
    PTR_AS(YogToken, token)->lineno = lineno;

    return token;
}

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

    PTR_AS(YogLexer, lexer)->lineno++;

    RETURN(env, TRUE);
}

static char
nextc(YogVal lexer) 
{
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    unsigned int next_index = PTR_AS(YogLexer, lexer)->next_index;
    YogVal body = PTR_AS(YogString, line)->body;
    char c = PTR_AS(YogCharArray, body)->items[next_index];
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
    YogVal enc = PTR_AS(YogString, buffer)->encoding;
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    unsigned int next_index = PTR_AS(YogLexer, lexer)->next_index;
    YogVal body = PTR_AS(YogString, line)->body;
    const char* ptr = &PTR_AS(YogCharArray, body)->items[next_index];
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

static void
print_current_position(YogEnv* env, YogVal lexer)
{
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    unsigned int size = STRING_SIZE(line);
    YOG_ASSERT(env, 0 < size, "invalid size (%u)", size);
    char s[size];
    memcpy(s, STRING_CSTR(line), size);
    char* pc = s + size - 1;
    while ((*pc == '\0') || (*pc == '\r') || (*pc == '\n')) {
        *pc = '\0';
        pc--;
    }

    FILE* out = stderr;
    fprintf(out, "%s\n", s);

    unsigned int pos = PTR_AS(YogLexer, lexer)->next_index - 1;
    unsigned int i;
    for (i = 0; i < pos; i++) {
        fprintf(out, " ");
    }
    fprintf(out, "^\n");
}

BOOL 
YogLexer_next_token(YogEnv* env, YogVal lexer, YogVal* token)
{
    SAVE_ARG(env, lexer);

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

#define ADD_TOKEN_CHAR(c)                   add_token_char(env, lexer, c)
#define RETURN_VAL_TOKEN(type, val)         do { \
    *token = ValToken_new(env, type, val, PTR_AS(YogLexer, lexer)->lineno); \
    RETURN(env, TRUE); \
} while (0)
#define RETURN_ID_TOKEN(type, s)            do { \
    ID id = INTERN(s); \
    *token = IDToken_new(env, type, id, PTR_AS(YogLexer, lexer)->lineno); \
    RETURN(env, TRUE); \
} while (0)
#define RETURN_TOKEN(type_)                 do { \
    RETURN_VAL_TOKEN((type_), YUNDEF); \
} while (0)
#define BUFSIZE                             (4)
#define RETURN_ID_TOKEN1(type, c)           do { \
    char buffer[BUFSIZE]; \
    snprintf(buffer, sizeof(buffer), "%c", (c)); \
    RETURN_ID_TOKEN((type), buffer); \
} while (0)
#define RETURN_INT  do { \
    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer; \
    YogVal num = YogString_to_i(env, buffer); \
    SET_STATE(LS_OP); \
    RETURN_VAL_TOKEN(TK_NUMBER, num); \
} while (0)
    switch (c) {
    case '0':
        {
            int c2 = NEXTC();
            if ((c2 == 'b') || (c2 == 'B')) {
                ADD_TOKEN_CHAR(c);
                ADD_TOKEN_CHAR(c2);

                c = NEXTC();
                if ((c != '0') && (c != '1')) {
                    print_current_position(env, lexer);
                    YogError_raise_SyntaxError(env, "numeric literal without digits");
                }
                ADD_TOKEN_CHAR(c);

                while (1) {
                    c = NEXTC();
                    if ((c == '0') || (c == '1')) {
                        ADD_TOKEN_CHAR(c);
                    }
                    else if (c == '_') {
                        ADD_TOKEN_CHAR(c);

                        c = NEXTC();
                        if (c == '_') {
                            print_current_position(env, lexer);
                            YogError_raise_SyntaxError(env, "trailing `_' in number");
                        }
                        else if ((c != '0') && (c != '1')) {
                            print_current_position(env, lexer);
                            YogError_raise_SyntaxError(env, "numeric literal without digits");
                        }
                        ADD_TOKEN_CHAR(c);
                    }
                    else {
                        PUSHBACK(c);
                        RETURN_INT;
                    }
                }
            }

            PUSHBACK(c2);
        }
        /* FALLTHRU */
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
        {
            do {
                ADD_TOKEN_CHAR(c);
                c = NEXTC();
            } while (isdigit(c));

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
                    YogVal body = PTR_AS(YogString, buffer)->body;
                    sscanf(PTR_AS(YogCharArray, body)->items, "%f", &f);
                    YogVal val = YogFloat_new(env);
                    PTR_AS(YogFloat, val)->val = f;
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
            YogVal val = YogString_clone(env, buffer);
            SET_STATE(LS_OP);
            RETURN_VAL_TOKEN(TK_STRING, val);
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
            YogVal val = YogRegexp_new(env, buffer, option);

            SET_STATE(LS_EXPR);
            RETURN_VAL_TOKEN(TK_REGEXP, val);
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

            if (c == '!') {
                ADD_TOKEN_CHAR(c);

                c = NEXTC();
                if (c == '?') {
                    ADD_TOKEN_CHAR(c);
                }
                else {
                    PUSHBACK(c);
                }
            }
            else if (c == '?') {
                ADD_TOKEN_CHAR(c);
            }
            else {
                PUSHBACK(c);
            }

            YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
            YogVal body = PTR_AS(YogString, buffer)->body;
            const char* name = PTR_AS(YogCharArray, body)->items;
            if (PTR_AS(YogLexer, lexer)->state == LS_NAME) {
                ID id = INTERN(name);
                unsigned int lineno = PTR_AS(YogLexer, lexer)->lineno;
                *token = IDToken_new(env, TK_NAME, id, lineno);
            }
            else {
                const KeywordTableEntry* entry = __Yog_lookup_keyword__(name, strlen(name));
                if (entry != NULL) {
                    unsigned int lineno = PTR_AS(YogLexer, lexer)->lineno;
                    *token = ValToken_new(env, entry->type, YUNDEF, lineno);
                }
                else {
                    ID id = INTERN(name);
                    unsigned int lineno = PTR_AS(YogLexer, lexer)->lineno;
                    *token = IDToken_new(env, TK_NAME, id, lineno);
                }
            }
            SET_STATE(LS_OP);
            RETURN(env, TRUE);
            break;
        }
    }
#undef RETURN_INT
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

static YogVal 
read_encoding(YogEnv* env, YogVal lexer) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, lexer);

    YogVal encoding = YUNDEF;

    while (readline(env, lexer, PTR_AS(YogLexer, lexer)->fp)) {
        PTR_AS(YogLexer, lexer)->next_index = 0;

        skip_whitespace(lexer);
        int c = nextc(lexer);
        if (c != '#') {
            continue;
        }

        YogVal line = PTR_AS(YogLexer, lexer)->line;
        unsigned int next_index = PTR_AS(YogLexer, lexer)->next_index;
        YogVal body = PTR_AS(YogString, line)->body;
        const char* s = &PTR_AS(YogCharArray, body)->items[next_index];
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
        if (!YogTable_lookup(env, env->vm->encodings, key, &val)) {
            continue;
        }
        encoding = val;
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
    PTR_AS(YogLexer, lexer)->lineno = 0;
}

void 
YogLexer_read_encoding(YogEnv* env, YogVal lexer) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, lexer);

    YogVal enc = read_encoding(env, lexer);
    if (!IS_PTR(enc)) {
        enc = YogEncoding_get_default(env);
    }
    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
    MODIFY(env, PTR_AS(YogString, buffer)->encoding, enc);
    reset_lexer(env, lexer);

    RETURN_VOID(env);
}

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogLexer* lexer = ptr;
#define KEEP(member)    YogGC_keep(env, &lexer->member, keeper, heap)
    KEEP(line);
    KEEP(buffer);
#undef KEEP
}

YogVal 
YogLexer_new(YogEnv* env) 
{
    SAVE_LOCALS(env);

    YogVal lexer = ALLOC_OBJ(env, keep_children, NULL, YogLexer);
    PTR_AS(YogLexer, lexer)->state = LS_EXPR;
    PTR_AS(YogLexer, lexer)->fp = NULL;
    PTR_AS(YogLexer, lexer)->line = YUNDEF;
    PTR_AS(YogLexer, lexer)->next_index = 0;
    PTR_AS(YogLexer, lexer)->buffer = YUNDEF;
    PTR_AS(YogLexer, lexer)->lineno = 0;
    PUSH_LOCAL(env, lexer);

    YogVal line = YogString_new(env);
    MODIFY(env, PTR_AS(YogLexer, lexer)->line, line);

    YogVal buffer = YogString_new(env);
    MODIFY(env, PTR_AS(YogLexer, lexer)->buffer, buffer);

    RETURN(env, lexer);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
