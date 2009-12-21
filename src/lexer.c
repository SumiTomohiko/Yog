#include "config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#include <ctype.h>
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(HAVE_UNISTD_H)
#   include <unistd.h>
#endif
#include "oniguruma.h"
#include "yog/array.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/float.h"
#include "yog/gc.h"
#include "yog/parser.h"
#include "yog/regexp.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"
#include "parser.h"

#define COMMENT_CHAR    '$'

#if defined(_MSC_VER)
#   define snprintf _snprintf
#endif

static void
YogToken_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogToken* token = PTR_AS(YogToken, ptr);
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
ValToken_new(YogEnv* env, uint_t type, YogVal val, uint_t lineno)
{
    SAVE_ARG(env, val);

    YogVal token = YogToken_new(env);
    PTR_AS(YogToken, token)->type = type;
    PTR_AS(YogToken, token)->u.val = val;
    PTR_AS(YogToken, token)->lineno = lineno;

    RETURN(env, token);
}

static YogVal
IDToken_new(YogEnv* env, uint_t type, ID id, uint_t lineno)
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
    if (fp == NULL) {
        return FALSE;
    }

    SAVE_LOCALS(env);
    PUSH_LOCAL(env, lexer);

    YogVal line = PTR_AS(YogLexer, lexer)->line;
    PUSH_LOCAL(env, line);
    YogString_clear(env, line);

    int_t c = 0;
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
    uint_t next_index = PTR_AS(YogLexer, lexer)->next_index;
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

static int_t
get_rest_size(YogEnv* env, YogVal lexer)
{
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    uint_t next_index = PTR_AS(YogLexer, lexer)->next_index;
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
    uint_t next_index = PTR_AS(YogLexer, lexer)->next_index;
    YogVal body = PTR_AS(YogString, line)->body;
    const char* ptr = &PTR_AS(YogCharArray, body)->items[next_index];
    int_t mbc_size = YogEncoding_mbc_size(env, enc, ptr);
    int_t rest_size = get_rest_size(env, lexer);
    if (rest_size < mbc_size) {
        YogError_raise_SyntaxError(env, "invalid multibyte char");
    }
    int_t i = 0;
    for (i = 0; i < mbc_size; i++) {
        char c = nextc(lexer);
        add_token_char(env, lexer, c);
    }

    RETURN_VOID(env);
}

static BOOL
is_binary_char(char c)
{
    return (c == '0') || (c == '1');
}

static BOOL
is_octet_char(char c)
{
    return ('0' <= c) && (c <= '7');
}

static BOOL
is_digit_char(char c)
{
    return ('0' <= c) && (c <= '9');
}

static BOOL
is_hex_char(char c)
{
    if (is_digit_char(c)) {
        return TRUE;
    }
    if (('a' <= c) && (c <= 'f')) {
        return TRUE;
    }
    if (('A' <= c) && (c <= 'F')) {
        return TRUE;
    }

    return FALSE;
}

static void
print_current_position(YogEnv* env, YogVal lexer)
{
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    uint_t size = STRING_SIZE(line);
    YOG_ASSERT(env, 0 < size, "invalid size (%u)", size);
#if defined(_alloca) && !defined(alloca)
#   define alloca   _alloca
#endif
    char* s = (char*)alloca(sizeof(char) * size);
    memcpy(s, STRING_CSTR(line), size);
    char* pc = s + size - 1;
    while ((*pc == '\0') || (*pc == '\r') || (*pc == '\n')) {
        *pc = '\0';
        pc--;
    }

    FILE* out = stderr;
    fprintf(out, "%s\n", s);

    uint_t pos = PTR_AS(YogLexer, lexer)->next_index - 1;
    uint_t i;
    for (i = 0; i < pos; i++) {
        fprintf(out, " ");
    }
    fprintf(out, "^\n");
}

#define NEXTC()             nextc(lexer)
#define PUSHBACK(c)         pushback(lexer, c)
#define ADD_TOKEN_CHAR(c)   add_token_char(env, lexer, c)

static void
read_number(YogEnv* env, YogVal lexer, BOOL (*is_valid_char)(char))
{
    SAVE_ARG(env, lexer);

    char c = NEXTC();
    if (!is_valid_char(c)) {
        print_current_position(env, lexer);
        YogError_raise_SyntaxError(env, "numeric literal without digits");
    }
    ADD_TOKEN_CHAR(c);

    while (1) {
        c = NEXTC();
        if (is_valid_char(c)) {
            ADD_TOKEN_CHAR(c);
        }
        else if (c == '_') {
            ADD_TOKEN_CHAR(c);

            c = NEXTC();
            if (c == '_') {
                print_current_position(env, lexer);
                YogError_raise_SyntaxError(env, "trailing `_' in number");
            }
            else if (!is_valid_char(c)) {
                print_current_position(env, lexer);
                YogError_raise_SyntaxError(env, "numeric literal without digits");
            }
            ADD_TOKEN_CHAR(c);
        }
        else {
            PUSHBACK(c);
            RETURN_VOID(env);
        }
    }
}

static void
enqueue_heredoc(YogEnv* env, YogVal lexer, YogVal heredoc)
{
    SAVE_ARGS2(env, lexer, heredoc);
    YogVal heredoc_queue = YUNDEF;
    PUSH_LOCAL(env, heredoc_queue);

    heredoc_queue = PTR_AS(YogLexer, lexer)->heredoc_queue;
    if (!IS_PTR(heredoc_queue)) {
        heredoc_queue = YogArray_new(env);
        PTR_AS(YogLexer, lexer)->heredoc_queue = heredoc_queue;
    }
    YogArray_push(env, heredoc_queue, heredoc);

    RETURN_VOID(env);
}

struct HereDoc {
    YogVal str;
    YogVal end;
    uint_t lineno;
};

typedef struct HereDoc HereDoc;

static void
HereDoc_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    HereDoc* heredoc = PTR_AS(HereDoc, ptr);
#define KEEP(member)    YogGC_keep(env, &heredoc->member, keeper, heap)
    KEEP(str);
    KEEP(end);
#undef KEEP
}

static YogVal
HereDoc_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal heredoc = YUNDEF;
    PUSH_LOCAL(env, heredoc);

    heredoc = ALLOC_OBJ(env, HereDoc_keep_children, NULL, HereDoc);
    PTR_AS(HereDoc, heredoc)->str = YUNDEF;
    PTR_AS(HereDoc, heredoc)->end = YUNDEF;
    PTR_AS(HereDoc, heredoc)->lineno = 0;

    RETURN(env, heredoc);
}

static void
skip_comment(YogEnv* env, YogVal lexer)
{
    SAVE_ARG(env, lexer);

    do {
        uint_t next_index = PTR_AS(YogLexer, lexer)->next_index;
        YogVal line = PTR_AS(YogLexer, lexer)->line;
        uint_t size = YogString_size(env, line) - 1;
        if ((2 <= size) && (next_index < size - 1)) {
            char c = NEXTC();
            if (c != COMMENT_CHAR) {
                continue;
            }
            char c2 = NEXTC();
            if (c2 != ')') {
                PUSHBACK(c2);
                continue;
            }
            RETURN_VOID(env);
        }

        if (!readline(env, lexer, PTR_AS(YogLexer, lexer)->fp)) {
            YogError_raise_SyntaxError(env, "EOF while scanning comment");
        }

        PTR_AS(YogLexer, lexer)->next_index = 0;
    } while (1);

    /* NOTREACHED */
    RETURN_VOID(env);
}

BOOL
YogLexer_next_token(YogEnv* env, YogVal lexer, const char* filename, YogVal* token)
{
    SAVE_ARG(env, lexer);
    YogVal heredoc_end = YUNDEF;
    YogVal str = YUNDEF;
    YogVal heredoc = YUNDEF;
    YogVal heredoc_queue = YUNDEF;
    YogVal end = YUNDEF;
    YogVal line = YUNDEF;
    PUSH_LOCALS6(env, heredoc_end, str, heredoc, heredoc_queue, end, line);

    clear_buffer(env, lexer);

#define SET_STATE(stat)     PTR_AS(YogLexer, lexer)->state = stat
    char c = 0;
    do {
        uint_t next_index = PTR_AS(YogLexer, lexer)->next_index;
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
            while (1) {
                heredoc_queue = PTR_AS(YogLexer, lexer)->heredoc_queue;
                if (!IS_PTR(heredoc_queue) || (YogArray_size(env, heredoc_queue) < 1)) {
                    break;
                }
                heredoc = YogArray_shift(env, heredoc_queue);
                while (1) {
                    if (!readline(env, lexer, PTR_AS(YogLexer, lexer)->fp)) {
                        YogError_raise_SyntaxError(env, "file \"%s\", line %u: EOF while scanning heredoc", filename, PTR_AS(HereDoc, heredoc)->lineno);
                    }
                    end = PTR_AS(HereDoc, heredoc)->end;
                    uint_t size = YogString_size(env, end) - 1;
                    line = PTR_AS(YogLexer, lexer)->line;
                    if (strncmp(STRING_CSTR(end), STRING_CSTR(line), size) == 0) {
                        if ((STRING_CSTR(line)[size] == '\r') || (STRING_CSTR(line)[size] == '\n')) {
                            break;
                        }
                    }

                    str = PTR_AS(HereDoc, heredoc)->str;
                    YogString_add(env, str, line);
                }
            }

            if (!readline(env, lexer, PTR_AS(YogLexer, lexer)->fp)) {
                RETURN(env, FALSE);
            }
            PTR_AS(YogLexer, lexer)->next_index = 0;
        }
    } while (1);

#define RETURN_VAL_TOKEN(type, val)         do { \
    *token = ValToken_new(env, type, val, PTR_AS(YogLexer, lexer)->lineno); \
    RETURN(env, TRUE); \
} while (0)
#define RETURN_ID_TOKEN(type, s)            do { \
    ID id = YogVM_intern(env, env->vm, s); \
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
            int_t c2 = NEXTC();
            if ((c2 == 'b') || (c2 == 'B')) {
                ADD_TOKEN_CHAR(c);
                ADD_TOKEN_CHAR(c2);
                read_number(env, lexer, is_binary_char);
                RETURN_INT;
            }
            else if ((c2 == 'd') || (c2 == 'D')) {
                ADD_TOKEN_CHAR(c);
                ADD_TOKEN_CHAR(c2);
                read_number(env, lexer, is_digit_char);
                RETURN_INT;
            }
            else if ((c2 == 'o') || (c2 == 'O')) {
                ADD_TOKEN_CHAR(c);
                ADD_TOKEN_CHAR(c2);
                read_number(env, lexer, is_octet_char);
                RETURN_INT;
            }
            else if ((c2 == 'x') || (c2 == 'X')) {
                ADD_TOKEN_CHAR(c);
                ADD_TOKEN_CHAR(c2);
                read_number(env, lexer, is_hex_char);
                RETURN_INT;
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
                int_t c2 = NEXTC();
                if (isdigit(c2)) {
                    ADD_TOKEN_CHAR(c);
                    do {
                        ADD_TOKEN_CHAR(c2);
                        c2 = NEXTC();
                    } while (isdigit(c2));
                    PUSHBACK(c2);

                    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
                    double f = strtod(STRING_CSTR(buffer), NULL);
                    YogVal val = YogFloat_new(env);
                    PTR_AS(YogFloat, val)->val = f;
                    SET_STATE(LS_OP);
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
                        int_t rest_size = get_rest_size(env, lexer);
                        if (rest_size < 1) {
                            YogError_raise_SyntaxError(env, "EOL while scanning string literal");
                        }
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
        {
            char c2 = NEXTC();
            if (c2 == COMMENT_CHAR) {
                skip_comment(env, lexer);
                RETURN(env, YogLexer_next_token(env, lexer, filename, token));
            }
            else {
                PUSHBACK(c2);
                SET_STATE(LS_EXPR);
                RETURN_TOKEN(TK_LPAR);
            }
        }
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

            char c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_PLUS_EQUAL);
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN1(TK_PLUS, c);
            }
        }
        break;
    case '-':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_MINUS_EQUAL);
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN1(TK_MINUS, c);
            }
        }
        break;
    case '*':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_STAR_EQUAL);
            }
            else if (c2 == '*') {
                char c3 = NEXTC();
                if (c3 == '=') {
                    RETURN_TOKEN(TK_STAR_STAR_EQUAL);
                }
                else {
                    PUSHBACK(c3);
                    RETURN_TOKEN(TK_STAR_STAR);
                }
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN1(TK_STAR, c);
            }
        }
        break;
    case '/':
        if (PTR_AS(YogLexer, lexer)->state == LS_OP) {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_DIV_EQUAL);
            }
            else if (c2 == '/') {
                char c3 = NEXTC();
                if (c3 == '=') {
                    RETURN_TOKEN(TK_DIV_DIV_EQUAL);
                }
                else {
                    PUSHBACK(c3);
                    RETURN_ID_TOKEN(TK_DIV_DIV, "//");
                }
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN1(TK_DIV, c);
            }
        }
        else {
            char delimitor = c;

            c = NEXTC();
            while (c != delimitor) {
                if (isascii(c)) {
                    if (c == '\\') {
                        int_t rest_size = get_rest_size(env, lexer);
                        if (rest_size < 1) {
                            YogError_raise_SyntaxError(env, "EOL while scanning regexp literal");
                        }
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
    case '%':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_PERCENT_EQUAL);
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN1(TK_PERCENT, c);
            }
        }
        break;
    case '=':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            switch (c2) {
            case '~':
                RETURN_ID_TOKEN(TK_EQUAL_TILDA, "=~");
                break;
            case '=':
                RETURN_TOKEN(TK_EQUAL_EQUAL);
                break;
            case '>':
                RETURN_TOKEN(TK_EQUAL_GREATER);
                break;
            default:
                PUSHBACK(c2);
                RETURN_TOKEN(TK_EQUAL);
                break;
            }
        }
        break;
    case '<':
        {
            char c2 = NEXTC();
            switch (c2) {
            case '<':
                if (PTR_AS(YogLexer, lexer)->state == LS_EXPR) {
                    char c3 = NEXTC();
                    if (!isalpha(c3) && (c3 != '_')) {
                        PUSHBACK(c3);
                        RETURN_ID_TOKEN(TK_LSHIFT, "<<");
                    }

                    heredoc_end = YogString_new(env);
                    do {
                        YogString_push(env, heredoc_end, c3);
                        c3 = NEXTC();
                    } while (isalnum(c3) || (c3 == '_'));
                    PUSHBACK(c3);

                    str = YogString_new(env);
                    uint_t lineno = PTR_AS(YogLexer, lexer)->lineno;
                    heredoc = HereDoc_new(env);
                    PTR_AS(HereDoc, heredoc)->str = str;
                    PTR_AS(HereDoc, heredoc)->end = heredoc_end;
                    PTR_AS(HereDoc, heredoc)->lineno = lineno;
                    enqueue_heredoc(env, lexer, heredoc);

                    SET_STATE(LS_OP);
                    RETURN_VAL_TOKEN(TK_STRING, str);
                }
                else {
                    SET_STATE(LS_EXPR);

                    char c3 = NEXTC();
                    if (c3 == '=') {
                        RETURN_TOKEN(TK_LSHIFT_EQUAL);
                    }
                    else {
                        PUSHBACK(c3);
                        RETURN_ID_TOKEN(TK_LSHIFT, "<<");
                    }
                }
                break;
            case '=':
                SET_STATE(LS_EXPR);
                RETURN_TOKEN(TK_LESS_EQUAL);
                break;
            default:
                PUSHBACK(c2);
                RETURN_TOKEN(TK_LESS);
                break;
            }
        }
        break;
    case '>':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            switch (c2) {
            case '>':
                {
                    char c3 = NEXTC();
                    switch (c3) {
                    case '=':
                        RETURN_TOKEN(TK_RSHIFT_EQUAL);
                        break;
                    default:
                        PUSHBACK(c3);
                        RETURN_ID_TOKEN(TK_RSHIFT, ">>");
                        break;
                    }
                }
                break;
            case '=':
                RETURN_TOKEN(TK_GREATER_EQUAL);
                break;
            default:
                PUSHBACK(c2);
                RETURN_TOKEN(TK_GREATER);
                break;
            }
        }
        break;
    case '|':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_BAR_EQUAL);
            }
            else if (c2 == '|') {
                char c3 = NEXTC();
                if (c3 == '=') {
                    RETURN_TOKEN(TK_BAR_BAR_EQUAL);
                }
                else {
                    PUSHBACK(c3);
                    RETURN_TOKEN(TK_BAR_BAR);
                }
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN(TK_BAR, "|");
            }
        }
        break;
    case '&':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_AND_EQUAL);
            }
            else if (c2 == '&') {
                char c3 = NEXTC();
                if (c3 == '=') {
                    RETURN_TOKEN(TK_AND_AND_EQUAL);
                }
                else {
                    PUSHBACK(c3);
                    RETURN_TOKEN(TK_AND_AND);
                }
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN(TK_AND, "&");
            }
        }
        break;
    case '!':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_NOT_EQUAL);
            }
            else {
                PUSHBACK(c2);
                RETURN_TOKEN(TK_NOT);
            }
        }
        break;
    case '^':
        {
            SET_STATE(LS_EXPR);

            char c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_XOR_EQUAL);
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN(TK_XOR, "^");
            }
        }
        break;
    case '~':
        SET_STATE(LS_EXPR);
        RETURN_ID_TOKEN(TK_TILDA, "~");
        break;
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
    case ':':
        {
            c = NEXTC();
            if (!isalpha(c)) {
                PUSHBACK(c);
                RETURN_TOKEN(TK_COLON);
            }

            do {
                ADD_TOKEN_CHAR(c);
                c = NEXTC();
            } while (isalpha(c) || (c == '_'));
            PUSHBACK(c);

            ID id = YogString_intern(env, PTR_AS(YogLexer, lexer)->buffer);
            RETURN_VAL_TOKEN(TK_SYMBOL, ID2VAL(id));
        }
        break;
    case '@':
        RETURN_TOKEN(TK_AT);
        break;
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
                ID id = YogVM_intern(env, env->vm, name);
                uint_t lineno = PTR_AS(YogLexer, lexer)->lineno;
                *token = IDToken_new(env, TK_NAME, id, lineno);
                SET_STATE(LS_OP);
            }
            else {
                const KeywordTableEntry* entry = __Yog_lookup_keyword__(name, strlen(name));
                if (entry != NULL) {
                    uint_t lineno = PTR_AS(YogLexer, lexer)->lineno;
                    *token = ValToken_new(env, entry->type, YUNDEF, lineno);
                    SET_STATE(LS_EXPR);
                }
                else {
                    ID id = YogVM_intern(env, env->vm, name);
                    uint_t lineno = PTR_AS(YogLexer, lexer)->lineno;
                    *token = IDToken_new(env, TK_NAME, id, lineno);
                    SET_STATE(LS_OP);
                }
            }
            RETURN(env, TRUE);
            break;
        }
    }
#undef RETURN_INT
#undef BUFSIZE
#undef RETURN_NAME
#undef RETURN_VAL
#undef SET_STATE

    RETURN(env, 0);
}

#undef ADD_TOKEN_CHAR
#undef PUSHBACK
#undef NEXTC

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
        int_t c = nextc(lexer);
        if (c != '#') {
            continue;
        }

        YogVal line = PTR_AS(YogLexer, lexer)->line;
        uint_t next_index = PTR_AS(YogLexer, lexer)->next_index;
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
YogLexer_set_encoding(YogEnv* env, YogVal lexer, YogVal encoding)
{
    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
    PTR_AS(YogString, buffer)->encoding = encoding;
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    PTR_AS(YogString, line)->encoding = encoding;
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
    YogLexer_set_encoding(env, lexer, enc);
    reset_lexer(env, lexer);

    RETURN_VOID(env);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogLexer* lexer = PTR_AS(YogLexer, ptr);
#define KEEP(member)    YogGC_keep(env, &lexer->member, keeper, heap)
    KEEP(line);
    KEEP(buffer);
    KEEP(heredoc_queue);
#undef KEEP
}

YogVal
YogLexer_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal lexer = YUNDEF;
    YogVal line = YUNDEF;
    YogVal buffer = YUNDEF;
    PUSH_LOCALS3(env, lexer, line, buffer);

    lexer = ALLOC_OBJ(env, keep_children, NULL, YogLexer);
    PTR_AS(YogLexer, lexer)->state = LS_EXPR;
    PTR_AS(YogLexer, lexer)->fp = NULL;
    PTR_AS(YogLexer, lexer)->line = YUNDEF;
    PTR_AS(YogLexer, lexer)->next_index = 0;
    PTR_AS(YogLexer, lexer)->buffer = YUNDEF;
    PTR_AS(YogLexer, lexer)->lineno = 0;
    PTR_AS(YogLexer, lexer)->heredoc_queue = YUNDEF;

    line = YogString_new(env);
    PTR_AS(YogLexer, lexer)->line = line;

    buffer = YogString_new(env);
    PTR_AS(YogLexer, lexer)->buffer = buffer;


    RETURN(env, lexer);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
