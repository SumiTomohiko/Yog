#include "yog/config.h"
#include <ctype.h>
#if defined(YOG_HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(YOG_HAVE_UNISTD_H)
#   include <unistd.h>
#endif
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/dict.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/float.h"
#include "yog/gc.h"
#include "yog/handle.h"
#include "yog/parser.h"
#include "yog/regexp.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/table.h"
#include "yog/token.h"
#include "yog/vm.h"
#include "yog/yog.h"

static void
YogToken_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogToken* token = PTR_AS(YogToken, ptr);
    switch (token->type) {
    case TK_NUMBER: /* FALLTHRU */
    case TK_REGEXP: /* FALLTHRU */
    case TK_STRING: /* FALLTHRU */
        YogGC_KEEP(env, token, u.val, keeper, heap);
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
    YogGC_UPDATE_PTR(env, PTR_AS(YogToken, token), u.val, val);
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
readline(YogEnv* env, YogVal lexer)
{
    FILE* fp = PTR_AS(YogLexer, lexer)->fp;
    if (fp == NULL) {
        return FALSE;
    }
    SAVE_ARG(env, lexer);
    YogVal line = YogBinary_new(env);
    PUSH_LOCAL(env, line);

    int_t c;
    while ((c = fgetc(fp)) != EOF) {
        YogBinary_push_char(env, line, c);
        if ((c == '\n') || (c == '\r')) {
            break;
        }
    }
    if (YogBinary_size(env, line) == 0) {
        RETURN(env, FALSE);
    }
    if (c == '\r') {
        c = fgetc(fp);
        if (c != '\n') {
            ungetc(c, fp);
        }
    }

    YogVal s = YogBinary_to_s(env, line, PTR_AS(YogLexer, lexer)->encoding);
    YogGC_UPDATE_PTR(env, PTR_AS(YogLexer, lexer), line, s);
    PTR_AS(YogLexer, lexer)->lineno++;

    RETURN(env, TRUE);
}

static YogChar
nextc(YogVal lexer)
{
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    uint_t next_index = PTR_AS(YogLexer, lexer)->next_index;
    if (STRING_SIZE(line) <= next_index) {
        return '\0';
    }
    YogChar c = STRING_CHARS(line)[next_index];
    PTR_AS(YogLexer, lexer)->next_index++;
    return c;
}

static void
pushback(YogVal lexer, YogChar c)
{
    if (c == '\0') {
        return;
    }
    PTR_AS(YogLexer, lexer)->next_index--;
}

static void
skip_whitespace(YogVal lexer)
{
    YogChar c;
    do {
        c = nextc(lexer);
    } while (isspace(c));
    pushback(lexer, c);
}

static BOOL
is_name_char(YogChar c)
{
    if (isascii(c)) {
        return isalnum(c) || (c == '_');
    }
    return TRUE;
}

static void
clear_buffer(YogEnv* env, YogVal lexer)
{
    YogString_clear(env, PTR_AS(YogLexer, lexer)->buffer);
}

static void
add_token_char(YogEnv* env, YogVal lexer, YogChar c)
{
    YogString_push(env, PTR_AS(YogLexer, lexer)->buffer, c);
}

#include "keywords.inc"

static int_t
get_rest_size(YogEnv* env, YogVal lexer)
{
    YogVal line = PTR_AS(YogLexer, lexer)->line;
    uint_t next_index = PTR_AS(YogLexer, lexer)->next_index;
    return YogString_size(env, line) - next_index;
}

static BOOL
is_binary_char(YogChar c)
{
    return (c == '0') || (c == '1');
}

static BOOL
is_octet_char(YogChar c)
{
    return ('0' <= c) && (c <= '7');
}

static BOOL
is_digit_char(YogChar c)
{
    return ('0' <= c) && (c <= '9');
}

static BOOL
is_hex_char(YogChar c)
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
print_current_position(YogEnv* env, YogHandle* lexer)
{
    YogHandle* line = VAL2HDL(env, HDL_AS(YogLexer, lexer)->line);
    YogHandle* enc = VAL2HDL(env, HDL_AS(YogLexer, lexer)->encoding);
    YogVal bin = YogEncoding_conv_from_yog(env, enc, line);
    char* pc = BINARY_CSTR(bin) + BINARY_SIZE(bin) - 2;
    while ((*pc == '\r') || (*pc == '\n')) {
        *pc = '\0';
        pc--;
    }
    FILE* out = stderr;
    fprintf(out, "%s\n", BINARY_CSTR(bin));

    uint_t pos = HDL_AS(YogLexer, lexer)->next_index - 1;
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
read_number(YogEnv* env, YogVal lexer, BOOL (*is_valid_char)(YogChar))
{
    YogHandle* h_lexer = YogHandle_REGISTER(env, lexer);

    YogChar c = NEXTC();
    if (!is_valid_char(c)) {
        print_current_position(env, h_lexer);
        YogError_raise_SyntaxError(env, "Numeric literal without digits");
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
                print_current_position(env, h_lexer);
                YogError_raise_SyntaxError(env, "Trailing \"_\" in number");
            }
            else if (!is_valid_char(c)) {
                print_current_position(env, h_lexer);
                const char* msg = "Numeric literal without digits";
                YogError_raise_SyntaxError(env, msg);
            }
            ADD_TOKEN_CHAR(c);
        }
        else {
            PUSHBACK(c);
            return;
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
        YogGC_UPDATE_PTR(env, PTR_AS(YogLexer, lexer), heredoc_queue, heredoc_queue);
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
#define KEEP(member)    YogGC_KEEP(env, heredoc, member, keeper, heap)
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

    uint_t depth = 1;
    do {
        YogChar c = NEXTC();
        if (c == ':') {
            YogChar c2 = NEXTC();
            if (c2 == ')') {
                depth--;
                if (depth == 0) {
                    RETURN_VOID(env);
                }
            }
            else {
                PUSHBACK(c2);
            }
        }
        else if (c == '(') {
            YogChar c2 = NEXTC();
            if (c2 == ':') {
                depth++;
            }
            else {
                PUSHBACK(c2);
            }
        }
        else if (c == '\0') {
            if (!readline(env, lexer)) {
                YogError_raise_SyntaxError(env, "EOF while scanning comment");
            }
            PTR_AS(YogLexer, lexer)->next_index = 0;
        }
    } while (1);

    /* NOTREACHED */
    RETURN_VOID(env);
}

static BOOL
read_elf(YogEnv* env, YogVal lexer)
{
    SAVE_ARG(env, lexer);
    YogChar c = NEXTC();
    if (c != 'e') {
        PUSHBACK(c);
        RETURN(env, FALSE);
    }
    YogChar c2 = NEXTC();
    if (c2 != 'l') {
        PUSHBACK(c2);
        PUSHBACK(c);
        RETURN(env, FALSE);
    }
    YogChar c3 = NEXTC();
    if (c3 != 'f') {
        PUSHBACK(c3);
        PUSHBACK(c2);
        PUSHBACK(c);
        RETURN(env, FALSE);
    }
    RETURN(env, TRUE);
}

static void
raise_heredoc_error(YogEnv* env, YogHandle* filename, YogVal heredoc)
{
    const char* fmt = "File \"%S\", line %u: EOF while scanning heredoc";
    uint_t lineno = PTR_AS(HereDoc, heredoc)->lineno;
    YogError_raise_SyntaxError(env, fmt, HDL2VAL(filename), lineno);
}

static void
restore_paren_depth(YogEnv* env, YogVal lexer)
{
    YogVal stack = PTR_AS(YogLexer, lexer)->paren_depth_stack;
    uint_t depth = VAL2INT(YogArray_pop(env, stack));
    PTR_AS(YogLexer, lexer)->paren_depth = depth;
}

static void
push_ended_type(YogEnv* env, YogVal lexer, uint_t type)
{
    YogVal stack = PTR_AS(YogLexer, lexer)->ended_type_stack;
    YogArray_push(env, stack, INT2VAL(type));
}

static void
push_paren_depth(YogEnv* env, YogVal lexer)
{
    YogVal stack = PTR_AS(YogLexer, lexer)->paren_depth_stack;
    YogArray_push(env, stack, INT2VAL(PTR_AS(YogLexer, lexer)->paren_depth));
}

YogVal
YogLexer_next_token(YogEnv* env, YogVal lexer, YogHandle* filename)
{
    SAVE_ARG(env, lexer);
    YogVal heredoc_end = YUNDEF;
    YogVal str = YUNDEF;
    YogVal heredoc = YUNDEF;
    YogVal heredoc_queue = YUNDEF;
    YogVal end_mark = YUNDEF;
    YogVal tok = YUNDEF;
    PUSH_LOCALS6(env, heredoc_end, str, heredoc, heredoc_queue, end_mark, tok);

    clear_buffer(env, lexer);

#define SET_STATE(stat)     PTR_AS(YogLexer, lexer)->state = stat
#define IS_STATE(stat)      (PTR_AS(YogLexer, lexer)->state == stat)

#define RETURN_VAL_TOKEN(type, val)         do { \
    uint_t lineno = PTR_AS(YogLexer, lexer)->lineno; \
    RETURN(env, ValToken_new(env, type, val, lineno)); \
} while (0)
#define RETURN_ID_TOKEN(type, s)            do { \
    ID id = YogVM_intern(env, env->vm, s); \
    uint_t lineno = PTR_AS(YogLexer, lexer)->lineno; \
    RETURN(env, IDToken_new(env, type, id, lineno)); \
} while (0)
#define RETURN_TOKEN(type_)                 do { \
    RETURN_VAL_TOKEN((type_), YUNDEF); \
} while (0)
#define BUFSIZE                             (4)
#define RETURN_ID_TOKEN1(type, c)           do { \
    char buffer[BUFSIZE]; \
    YogSysdeps_snprintf(buffer, sizeof(buffer), "%c", (c)); \
    RETURN_ID_TOKEN((type), buffer); \
} while (0)
#define RETURN_INT  do { \
    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer; \
    YogVal num = YogString_to_i(env, buffer); \
    SET_STATE(LS_OP); \
    RETURN_VAL_TOKEN(TK_NUMBER, num); \
} while (0)

    YogChar c;
    do {
        uint_t next_index = PTR_AS(YogLexer, lexer)->next_index;
        YogVal line = PTR_AS(YogLexer, lexer)->line;
        if (IS_PTR(line) && (next_index < STRING_SIZE(line))) {
            c = NEXTC();
            if (isblank(c)) {
                skip_whitespace(lexer);
                continue;
            }
            else if (c == '#') {
                do {
                    c = NEXTC();
                } while ((c != '\r') && (c != '\n') && (c != '\0'));
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
                    if (!readline(env, lexer)) {
                        raise_heredoc_error(env, filename, heredoc);
                        /* NOTREACHED */
                    }
                    end_mark = PTR_AS(HereDoc, heredoc)->end;
                    uint_t size = STRING_SIZE(end_mark);
                    line = PTR_AS(YogLexer, lexer)->line;
                    if (YogString_strncmp(env, end_mark, line, size) == 0) {
                        YogChar c = STRING_CHARS(line)[size];
                        if ((c == '\r') || (c == '\n')) {
                            break;
                        }
                    }

                    str = PTR_AS(HereDoc, heredoc)->str;
                    YogString_append(env, str, line);
                }
            }

            if (!readline(env, lexer)) {
                RETURN_TOKEN(TK_EOF);
            }
            PTR_AS(YogLexer, lexer)->next_index = 0;
        }
    } while (1);

    switch (c) {
    case '0':
        {
            YogChar c2 = NEXTC();
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
                YogChar c2 = NEXTC();
                if (isdigit(c2)) {
                    ADD_TOKEN_CHAR(c);
                    do {
                        ADD_TOKEN_CHAR(c2);
                        c2 = NEXTC();
                    } while (isdigit(c2));
                    PUSHBACK(c2);

                    YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
                    YogVal val = YogFloat_from_str(env, buffer);
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
        {
            YogChar quote = c;

            c = NEXTC();
            while (c != quote) {
                if (c == '\\') {
                    int_t rest_size = get_rest_size(env, lexer);
                    if (rest_size < 1) {
                        const char* msg = "EOL while scanning string literal";
                        YogError_raise_SyntaxError(env, msg);
                    }
                    c = NEXTC();
                    switch (c) {
                    case '\\':
                        ADD_TOKEN_CHAR('\\');
                        break;
                    case 'n':
                        ADD_TOKEN_CHAR('\n');
                        break;
                    case 'r':
                        ADD_TOKEN_CHAR('\r');
                        break;
                    case 't':
                        ADD_TOKEN_CHAR('\t');
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

            YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
            YogVal val = YogString_clone(env, buffer);
            SET_STATE(LS_OP);
            RETURN_VAL_TOKEN(TK_STRING, val);
            break;
        }
    case '{':
        PTR_AS(YogLexer, lexer)->paren_depth++;
        RETURN_TOKEN(TK_LBRACE);
        break;
    case '}':
        PTR_AS(YogLexer, lexer)->paren_depth--;
        RETURN_TOKEN(TK_RBRACE);
        break;
    case '(':
        {
            YogChar c2 = NEXTC();
            if (c2 == ':') {
                skip_comment(env, lexer);
                RETURN(env, YogLexer_next_token(env, lexer, filename));
            }
            PUSHBACK(c2);
            SET_STATE(LS_EXPR);
            PTR_AS(YogLexer, lexer)->paren_depth++;
            RETURN_TOKEN(TK_LPAR);
        }
        break;
    case ')':
        SET_STATE(LS_OP);
        PTR_AS(YogLexer, lexer)->paren_depth--;
        RETURN_TOKEN(TK_RPAR);
        break;
    case '[':
        if (IS_STATE(LS_NAME)) {
            YogChar c2 = NEXTC();
            if (c2 == ']') {
                YogChar c3 = NEXTC();
                if (c3 == '=') {
                    RETURN_ID_TOKEN(TK_NAME, "[]=");
                }
                PUSHBACK(c2);
                RETURN_ID_TOKEN(TK_NAME, "[]");
            }
        }

        SET_STATE(LS_EXPR);
        PTR_AS(YogLexer, lexer)->paren_depth++;
        RETURN_TOKEN(TK_LBRACKET);
        break;
    case ']':
        SET_STATE(LS_OP);
        PTR_AS(YogLexer, lexer)->paren_depth--;
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
            YogChar c2 = NEXTC();
            if (c2 == '=') {
                SET_STATE(LS_EXPR);
                RETURN_TOKEN(TK_PLUS_EQUAL);
            }
            else if (IS_STATE(LS_NAME) && (c2 == 's') && read_elf(env, lexer)) {
                SET_STATE(LS_OP);
                RETURN_ID_TOKEN(TK_NAME, "+self");
            }
            else {
                PUSHBACK(c2);
                SET_STATE(LS_EXPR);
                RETURN_ID_TOKEN1(TK_PLUS, c);
            }
        }
        break;
    case '-':
        {
            YogChar c2 = NEXTC();
            if (c2 == '=') {
                SET_STATE(LS_EXPR);
                RETURN_TOKEN(TK_MINUS_EQUAL);
            }
            else if (IS_STATE(LS_NAME) && (c2 == 's') && read_elf(env, lexer)) {
                SET_STATE(LS_OP);
                RETURN_ID_TOKEN(TK_NAME, "-self");
            }
            else {
                PUSHBACK(c2);
                SET_STATE(LS_EXPR);
                RETURN_ID_TOKEN1(TK_MINUS, c);
            }
        }
        break;
    case '*':
        {
            SET_STATE(LS_EXPR);

            YogChar c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_STAR_EQUAL);
            }
            else if (c2 == '*') {
                YogChar c3 = NEXTC();
                if (c3 == '=') {
                    RETURN_TOKEN(TK_STAR_STAR_EQUAL);
                }
                else {
                    PUSHBACK(c3);
                    RETURN_ID_TOKEN(TK_STAR_STAR, "**");
                }
            }
            else {
                PUSHBACK(c2);
                RETURN_ID_TOKEN1(TK_STAR, c);
            }
        }
        break;
    case '/':
        if (!IS_STATE(LS_EXPR)) {
            SET_STATE(LS_EXPR);

            YogChar c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_DIV_EQUAL);
            }
            else if (c2 == '/') {
                YogChar c3 = NEXTC();
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
            YogChar delimitor = c;

            c = NEXTC();
            while (c != delimitor) {
                if (c == '\\') {
                    int_t rest_size = get_rest_size(env, lexer);
                    if (rest_size < 1) {
                        const char* msg = "EOL while scanning regexp literal";
                        YogError_raise_SyntaxError(env, msg);
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

            BOOL ignore_case = TRUE;
            c = NEXTC();
            if (c != 'i') {
                ignore_case = FALSE;
                PUSHBACK(c);
            }

            YogVal buffer = PTR_AS(YogLexer, lexer)->buffer;
            YogVal val = YogRegexp_new(env, buffer, ignore_case);

            SET_STATE(LS_EXPR);
            RETURN_VAL_TOKEN(TK_REGEXP, val);
            break;
        }
        break;
    case '%':
        {
            SET_STATE(LS_EXPR);

            YogChar c2 = NEXTC();
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

            YogChar c2 = NEXTC();
            switch (c2) {
            case '~':
                RETURN_ID_TOKEN(TK_EQUAL_TILDA, "=~");
                break;
            case '=':
                RETURN_ID_TOKEN(TK_EQUAL_EQUAL, "==");
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
            YogChar c2 = NEXTC();
            switch (c2) {
            case '<':
                if (IS_STATE(LS_EXPR)) {
                    YogChar c3 = NEXTC();
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
                    YogGC_UPDATE_PTR(env, PTR_AS(HereDoc, heredoc), str, str);
                    YogGC_UPDATE_PTR(env, PTR_AS(HereDoc, heredoc), end, heredoc_end);
                    PTR_AS(HereDoc, heredoc)->lineno = lineno;
                    enqueue_heredoc(env, lexer, heredoc);

                    SET_STATE(LS_OP);
                    RETURN_VAL_TOKEN(TK_STRING, str);
                }
                else {
                    SET_STATE(LS_EXPR);

                    YogChar c3 = NEXTC();
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
                {
                    SET_STATE(LS_EXPR);

                    YogChar c3 = NEXTC();
                    if (c3 == '>') {
                        RETURN_ID_TOKEN(TK_UFO, "<=>");
                    }
                    else {
                        PUSHBACK(c3);
                        RETURN_ID_TOKEN(TK_LESS_EQUAL, "<=");
                    }
                }
                break;
            default:
                PUSHBACK(c2);
                RETURN_ID_TOKEN(TK_LESS, "<");
                break;
            }
        }
        break;
    case '>':
        {
            SET_STATE(LS_EXPR);

            YogChar c2 = NEXTC();
            switch (c2) {
            case '>':
                {
                    YogChar c3 = NEXTC();
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
                RETURN_ID_TOKEN(TK_GREATER_EQUAL, ">=");
                break;
            default:
                PUSHBACK(c2);
                RETURN_ID_TOKEN(TK_GREATER, ">");
                break;
            }
        }
        break;
    case '|':
        {
            SET_STATE(LS_EXPR);

            YogChar c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_BAR_EQUAL);
            }
            else if (c2 == '|') {
                YogChar c3 = NEXTC();
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

            YogChar c2 = NEXTC();
            if (c2 == '=') {
                RETURN_TOKEN(TK_AND_EQUAL);
            }
            else if (c2 == '&') {
                YogChar c3 = NEXTC();
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

            YogChar c2 = NEXTC();
            if (c2 == '=') {
                RETURN_ID_TOKEN(TK_NOT_EQUAL, "!=");
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

            YogChar c2 = NEXTC();
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
        {
            YogChar c2 = NEXTC();
            if (IS_STATE(LS_NAME) && (c2 == 's') && read_elf(env, lexer)) {
                SET_STATE(LS_OP);
                RETURN_ID_TOKEN(TK_NAME, "~self");
            }

            PUSHBACK(c2);
            SET_STATE(LS_EXPR);
            RETURN_ID_TOKEN(TK_TILDA, "~");
        }
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
            if (0 < PTR_AS(YogLexer, lexer)->paren_depth) {
                RETURN(env, YogLexer_next_token(env, lexer, filename));
            }
            SET_STATE(LS_EXPR);
            RETURN_TOKEN(TK_NEWLINE);
            break;
        }
    case ':':
        SET_STATE(LS_EXPR);
        RETURN_TOKEN(TK_COLON);
        break;
    case '\'':
        {
            c = NEXTC();
            while (isalnum(c) || (c == '_')) {
                ADD_TOKEN_CHAR(c);
                c = NEXTC();
            }
            PUSHBACK(c);

            ID id = YogString_intern(env, PTR_AS(YogLexer, lexer)->buffer);
            RETURN_VAL_TOKEN(TK_SYMBOL, ID2VAL(id));
        }
        break;
    case '@':
        RETURN_TOKEN(TK_AT);
        break;
    case '?':
        RETURN_TOKEN(TK_QUESTION);
        break;
    default:
        {
            do {
                ADD_TOKEN_CHAR(c);
                c = NEXTC();
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

            YogHandle* buffer = VAL2HDL(env, PTR_AS(YogLexer, lexer)->buffer);
            if (IS_STATE(LS_NAME)) {
                ID id = YogString_intern(env, HDL2VAL(buffer));
                uint_t lineno = PTR_AS(YogLexer, lexer)->lineno;
                tok = IDToken_new(env, TK_NAME, id, lineno);
                SET_STATE(LS_OP);
            }
            else {
                YogVal bin = YogString_to_bin_in_default_encoding(env, buffer);
                const char* name = BINARY_CSTR(bin);
                const KeywordTableEntry* entry = __Yog_lookup_keyword__(name, strlen(name));
                uint_t lineno = PTR_AS(YogLexer, lexer)->lineno;
                if (entry != NULL) {
                    uint_t type = entry->type;
                    YogVal stack;
                    switch (type) {
                    case TK_DO:
                        push_paren_depth(env, lexer);
                        PTR_AS(YogLexer, lexer)->paren_depth = 0;
                        /* FALLTHRU */
                    case TK_CLASS:
                    case TK_DEF:
                    case TK_IF:
                    case TK_MODULE:
                    case TK_TRY:
                    case TK_WHILE:
                        push_ended_type(env, lexer, type);
                        break;
                    case TK_END:
                        stack = PTR_AS(YogLexer, lexer)->ended_type_stack;
                        if (VAL2INT(YogArray_pop(env, stack)) == TK_DO) {
                            restore_paren_depth(env, lexer);
                        }
                        break;
                    default:
                        break;
                    }
                    tok = ValToken_new(env, type, YUNDEF, lineno);
                    SET_STATE(type == TK_DEF ? LS_NAME : LS_EXPR);
                }
                else {
                    ID id = YogVM_intern2(env, env->vm, HDL2VAL(buffer));
                    tok = IDToken_new(env, TK_NAME, id, lineno);
                    SET_STATE(LS_OP);
                }
            }
            RETURN(env, tok);
            break;
        }
    }

#undef RETURN_INT
#undef BUFSIZE
#undef RETURN_NAME
#undef RETURN_VAL
#undef IS_STATE
#undef SET_STATE

    RETURN(env, 0);
}

#undef ADD_TOKEN_CHAR
#undef PUSHBACK
#undef NEXTC

static BOOL
is_coding_char(YogChar c)
{
    return isalnum(c) || (c == '_') || (c == '-');
}

static YogVal
get_encoding(YogEnv* env, YogVal coding)
{
    YogHandle* h = VAL2HDL(env, coding);
    YogVal val = YogDict_get(env, env->vm->encodings, coding);
    if (IS_UNDEF(val)) {
        YogError_raise_SyntaxError(env, "Unsupported encoding: %S", HDL2VAL(h));
        /* NOTREACHED */
    }
    return val;
}

static YogVal
read_encoding(YogEnv* env, YogVal lexer)
{
    SAVE_ARG(env, lexer);
    YogVal line = YUNDEF;
    YogVal buffer = YUNDEF;
    YogVal coding = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS4(env, line, buffer, coding, val);

    while (readline(env, lexer)) {
        PTR_AS(YogLexer, lexer)->next_index = 0;

        skip_whitespace(lexer);
        YogChar c = nextc(lexer);
        if (c != '#') {
            continue;
        }

#define KEY "coding"
        line = PTR_AS(YogLexer, lexer)->line;
        int_t pos = YogString_strstr(env, line, KEY);
        if (pos < 0) {
            continue;
        }
        pos += strlen(KEY);
#undef KEY
        c = STRING_CHARS(line)[pos];
        if ((c != '=') && (c != ':')) {
            continue;
        }
        PTR_AS(YogLexer, lexer)->next_index = pos + 1;
        skip_whitespace(lexer);

        clear_buffer(env, lexer);
        c = nextc(lexer);
        while (is_coding_char(c)) {
            add_token_char(env, lexer, c);
            c = nextc(lexer);
        }
        buffer = PTR_AS(YogLexer, lexer)->buffer;
        coding = YogEncoding_normalize_name(env, buffer);
        if (STRING_SIZE(coding) < 1) {
            continue;
        }
        RETURN(env, get_encoding(env, coding));
    }

    RETURN(env, YUNDEF);
}

static void
reset_lexer(YogEnv* env, YogVal lexer)
{
    fseek(PTR_AS(YogLexer, lexer)->fp, 0, SEEK_SET);
    PTR_AS(YogLexer, lexer)->line = YUNDEF;
    PTR_AS(YogLexer, lexer)->next_index = 0;
    PTR_AS(YogLexer, lexer)->lineno = 0;
}

void
YogLexer_set_encoding(YogEnv* env, YogVal lexer, YogVal encoding)
{
    YogGC_UPDATE_PTR(env, PTR_AS(YogLexer, lexer), encoding, encoding);
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
#define KEEP(member)    YogGC_KEEP(env, lexer, member, keeper, heap)
    KEEP(line);
    KEEP(buffer);
    KEEP(heredoc_queue);
    KEEP(encoding);
    KEEP(ended_type_stack);
    KEEP(paren_depth_stack);
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
    PTR_AS(YogLexer, lexer)->paren_depth = 0;
    PTR_AS(YogLexer, lexer)->ended_type_stack = YUNDEF;
    PTR_AS(YogLexer, lexer)->paren_depth_stack = YUNDEF;

    PTR_AS(YogLexer, lexer)->line = YUNDEF;
    buffer = YogString_new(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogLexer, lexer), buffer, buffer);
    YogGC_UPDATE_PTR(env, PTR_AS(YogLexer, lexer), encoding, env->vm->encUtf8);
    YogVal ended_type_stack = YogArray_new(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogLexer, lexer), ended_type_stack, ended_type_stack);
    YogVal paren_depth_stack = YogArray_new(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogLexer, lexer), paren_depth_stack, paren_depth_stack);

    RETURN(env, lexer);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
