#include "yog/config.h"
#include <ctype.h>
#include <errno.h>
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oniguruma.h"
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/misc.h"
#include "yog/regexp.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_STRING)) { \
        YogError_raise_TypeError((env), "self must be String"); \
    } \
} while (0)

ID
YogString_intern(YogEnv* env, YogVal s)
{
    return YogVM_intern(env, env->vm, STRING_CSTR(s));
}

static void
YogCharArray_clear(YogEnv* env, YogVal array)
{
    if (0 < PTR_AS(YogCharArray, array)->size) {
        PTR_AS(YogCharArray, array)->items[0] = '\0';
    }
}

static YogVal
YogCharArray_new(YogEnv* env, uint_t size)
{
    YogVal array = ALLOC_OBJ_ITEM(env, NULL, NULL, YogCharArray, size, char);
    PTR_AS(YogCharArray, array)->size = size;
    YogCharArray_clear(env, array);

    return array;
}

YogVal
YogCharArray_new_str(YogEnv* env, const char* s)
{
    size_t size = strlen(s) + 1;
    YogVal array = YogCharArray_new(env, size);
    memcpy(PTR_AS(YogCharArray, array)->items, s, size);
    PTR_AS(YogCharArray, array)->size = size;

    return array;
}

static void
YogString_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogString* s = PTR_AS(YogString, ptr);
#define KEEP(member)    YogGC_keep(env, &s->member, keeper, heap)
    KEEP(body);
    KEEP(encoding);
#undef KEEP
}

uint_t
YogString_size(YogEnv* env, YogVal string)
{
    return PTR_AS(YogString, string)->size - 1;
}

static void
ensure_body(YogEnv* env, YogVal string)
{
    SAVE_ARG(env, string);

    if (!IS_PTR(PTR_AS(YogString, string)->body)) {
#define CAPACITY    (1)
        YogVal body = YogCharArray_new(env, CAPACITY);
#undef CAPACITY
        PTR_AS(YogString, string)->body = body;
    }

    RETURN_VOID(env);
}

/**
 * ensure_size
 * "size" includes null terminator '\0'.
 */
static void
ensure_size(YogEnv* env, YogVal string, uint_t size)
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, string);

    YogVal body = PTR_AS(YogString, string)->body;
    PUSH_LOCAL(env, body);
    if (!IS_PTR(body) || (PTR_AS(YogCharArray, body)->size < size)) {
        uint_t capacity = 1;
        if (IS_PTR(body)) {
            capacity = PTR_AS(YogCharArray, body)->size;
        }
        do {
#define RATIO   (2)
            capacity = RATIO * capacity;
#undef RATIO
        } while (capacity < size);

        YogVal new_body = YogCharArray_new(env, capacity);
        char* p = PTR_AS(YogCharArray, new_body)->items;
        char* q = PTR_AS(YogCharArray, body)->items;
        uint_t size = PTR_AS(YogString, string)->size;
        memcpy(p, q, size);

        PTR_AS(YogString, string)->body = new_body;
    }

    RETURN_VOID(env);
}

void
YogString_push(YogEnv* env, YogVal string, char c)
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, string);

    ensure_body(env, string);

    YogVal body = PTR_AS(YogString, string)->body;
    uint_t needed_size = PTR_AS(YogString, string)->size + 1;
    if (PTR_AS(YogCharArray, body)->size < needed_size) {
        ensure_size(env, string, needed_size);
        body = PTR_AS(YogString, string)->body;
    }

    uint_t size = PTR_AS(YogString, string)->size;
    PTR_AS(YogCharArray, body)->items[size - 1] = c;
    PTR_AS(YogCharArray, body)->items[size] = '\0';
    PTR_AS(YogString, string)->size++;

    RETURN_VOID(env);
}

void
YogString_clear(YogEnv* env, YogVal string)
{
    if (IS_PTR(PTR_AS(YogString, string)->body)) {
        YogCharArray_clear(env, PTR_AS(YogString, string)->body);
        PTR_AS(YogString, string)->size = 1;
    }
    else {
        PTR_AS(YogString, string)->size = 0;
    }
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogVal obj = ALLOC_OBJ(env, YogString_keep_children, NULL, YogString);
    YogBasicObj_init(env, obj, TYPE_STRING, 0, klass);

    PTR_AS(YogString, obj)->encoding = YUNDEF;
    PTR_AS(YogString, obj)->size = 0;
    PTR_AS(YogString, obj)->body = YUNDEF;

    RETURN(env, obj);
}

YogVal
YogString_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal self = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS2(env, self, body);

    self = alloc(env, env->vm->cString);
    body = YogCharArray_new(env, 1);

    PTR_AS(YogString, self)->encoding = YUNDEF;
    PTR_AS(YogString, self)->size = 1;
    PTR_AS(YogString, self)->body = body;

    RETURN(env, self);
}

YogVal
YogString_of_encoding(YogEnv* env, YogVal encoding)
{
    SAVE_ARG(env, encoding);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogString_new(env);
    PTR_AS(YogString, s)->encoding = encoding;

    RETURN(env, s);
}

YogVal
YogString_from_range(YogEnv* env, YogVal enc, const char* start, const char* end)
{
    SAVE_ARG(env, enc);

    uint_t size = 0;
    if (start <= end) {
        size = end - start + 1;
    }

    /* FIXME: dirty hack */
    char* escaped_from_gc = (char*)YogSysdeps_alloca(sizeof(char) * size);
    strncpy(escaped_from_gc, start, size);

    YogVal body = YogCharArray_new(env, size + 1);
    memcpy(PTR_AS(YogCharArray, body)->items, escaped_from_gc, size);
    PTR_AS(YogCharArray, body)->items[size] = '\0';
    PUSH_LOCAL(env, body);

    YogVal s = YogString_new(env);
    PTR_AS(YogString, s)->encoding = enc;
    PTR_AS(YogString, s)->size = size + 1;
    PTR_AS(YogString, s)->body = body;

    RETURN(env, s);
}

YogVal
YogString_of_size(YogEnv* env, uint_t size)
{
    SAVE_LOCALS(env);
    YogVal string = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS2(env, string, body);

    string = alloc(env, env->vm->cString);
    if (size == 0) {
        RETURN(env, string);
    }
    body = YogCharArray_new(env, size);

    PTR_AS(YogString, string)->encoding = YNIL;
    PTR_AS(YogString, string)->size = 1;
    PTR_AS(YogString, string)->body = body;

    RETURN(env, string);
}

YogVal
YogString_from_str(YogEnv* env, const char* s)
{
    SAVE_LOCALS(env);
    YogVal str = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS2(env, str, body);

    size_t len = strlen(s);
    char* buffer = (char*)YogSysdeps_alloca(sizeof(char) * (len + 1));
    memcpy(buffer, s, len + 1);
    body = YogCharArray_new_str(env, buffer);

    str = alloc(env, env->vm->cString);
    PTR_AS(YogString, str)->encoding = YNIL;
    PTR_AS(YogString, str)->size = len + 1;
    PTR_AS(YogString, str)->body = body;

    RETURN(env, str);
}

YogVal
YogString_clone(YogEnv* env, YogVal string)
{
    SAVE_ARG(env, string);

    YogVal body = PTR_AS(YogString, string)->body;
    YogVal s = YogString_from_str(env, PTR_AS(YogCharArray, body)->items);
    PTR_AS(YogString, s)->encoding = PTR_AS(YogString, string)->encoding;

    RETURN(env, s);
}

char
YogString_at(YogEnv* env, YogVal s, uint_t n)
{
    YogVal body = PTR_AS(YogString, s)->body;
    return PTR_AS(YogCharArray, body)->items[n];
}

static uint_t
find(YogEnv* env, YogVal self, YogVal substr, uint_t from)
{
    SAVE_ARGS2(env, self, substr);

    uint_t self_size = YogString_size(env, self);
    uint_t substr_size = YogString_size(env, substr);
    uint_t end_pos = self_size - substr_size;
    if (self_size < end_pos) {
        RETURN(env, UNSIGNED_MAX);
    }

    uint_t i;
    for (i = from; i <= end_pos; i++) {
        uint_t j;
        for (j = 0; j < substr_size; j++) {
            char c1 = YogString_at(env, self, i + j);
            char c2 = YogString_at(env, substr, j);
            if (c1 != c2) {
                break;
            }
        }
        if (j == substr_size) {
            RETURN(env, i);
        }
    }

    RETURN(env, UNSIGNED_MAX);
}

void
YogString_add(YogEnv* env, YogVal self, YogVal s)
{
    SAVE_ARGS2(env, self, s);

    uint_t self_size = YogString_size(env, self);
    uint_t s_size = YogString_size(env, s);
    uint_t size = self_size + s_size + 1;
    ensure_size(env, self, size);
    memcpy(STRING_CSTR(self) + self_size, STRING_CSTR(s), s_size);
    STRING_CSTR(self)[self_size + s_size] = '\0';
    PTR_AS(YogString, self)->size = size;

    RETURN_VOID(env);
}

static YogVal
gsub(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal substr = YUNDEF;
    YogVal s = YUNDEF;
    YogVal to = YUNDEF;
    PUSH_LOCALS3(env, substr, s, to);

    YogCArg params[] = { { "substr", &substr }, { "to", &to }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "gsub", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    if (!IS_PTR(substr) || (BASIC_OBJ_TYPE(substr) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "substring must be String");
    }
    if (!IS_PTR(to) || (BASIC_OBJ_TYPE(to) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "replacing string must be String");
    }

#define ADD_STR(to)    do { \
    uint_t size = to - from; \
    char* t = (char*)YogSysdeps_alloca(sizeof(char) * (size + 1)); \
    memcpy(t, STRING_CSTR(self) + from, size); \
    t[size] = '\0'; \
    YogString_add_cstr(env, s, t); \
} while (0)
    s = YogString_of_encoding(env, STRING_ENCODING(self));
    uint_t from = 0;
    uint_t index;
    while ((index = find(env, self, substr, from)) != UNSIGNED_MAX) {
        ADD_STR(index);
        YogString_add(env, s, to);
        from = index + YogString_size(env, substr);
    }
    ADD_STR(YogString_size(env, self));
#undef ADD_STR

    RETURN(env, s);
}

static YogVal
get_size(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    CHECK_SELF_TYPE(env, self);
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal enc = YUNDEF;
    PUSH_LOCAL(env, enc);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_size", params, args, kw);

    int_t n = 0;
    char* begin = STRING_CSTR(self);
    char* end = begin + YogString_size(env, self);
    char* pc = begin;
    enc = STRING_ENCODING(self);
    while (pc < end) {
        pc += YogEncoding_mbc_size(env, enc, pc);
        n++;
    }

    RETURN(env, INT2VAL(n));
}

static YogVal
to_i(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal n = YUNDEF;
    PUSH_LOCAL(env, n);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_i", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    n = YogString_to_i(env, self);

    RETURN(env, n);
}

static YogVal
to_s(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_s", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    RETURN(env, self);
}

static YogVal
add(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal arg = YUNDEF;
    PUSH_LOCAL(env, arg);

    YogCArg params[] = { { "s", &arg }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "+", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    if (!IS_PTR(arg) || (BASIC_OBJ_TYPE(arg) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "can't convert %C object to string implicitly", arg);
    }

    uint_t size1 = YogString_size(env, self);
    uint_t size2 = YogString_size(env, arg);
    uint_t size = size1 + size2 + 1;
    YogVal s = YogString_of_size(env, size);
    YogVal body = PTR_AS(YogString, s)->body;
    char* p = PTR_AS(YogCharArray, body)->items;
    YogVal self_body = PTR_AS(YogString, self)->body;
    const char* q = PTR_AS(YogCharArray, self_body)->items;
    memcpy(p, q, size1);
    char* u = &PTR_AS(YogCharArray, body)->items[size1];
    YogVal arg_body = PTR_AS(YogString, arg)->body;
    const char* v = PTR_AS(YogCharArray, arg_body)->items;
    memcpy(u, v, size2);
    u[size2] = '\0';
    PTR_AS(YogString, s)->size = size;
    PTR_AS(YogString, s)->encoding = STRING_ENCODING(self);

    RETURN(env, s);
}

YogVal
YogString_multiply(YogEnv* env, YogVal self, int_t num)
{
    SAVE_ARG(env, self);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    uint_t size = YogString_size(env, self);
    if (num < 0) {
        YogError_raise_ArgumentError(env, "negative argument");
    }
    uint_t needed_size = size * num;
    if ((num != 0) && (needed_size / num != size)) {
        YogError_raise_ArgumentError(env, "argument too big");
    }
    s = YogString_of_size(env, needed_size + 1);
    int_t i;
    for (i = 0; i < num; i++) {
        memcpy(STRING_CSTR(s) + i * size, STRING_CSTR(self), size);
    }
    STRING_CSTR(s)[needed_size] = '\0';
    PTR_AS(YogString, s)->size = needed_size + 1;
    PTR_AS(YogString, s)->encoding = STRING_ENCODING(self);

    RETURN(env, s);
}

static YogVal
multiply(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal arg = YUNDEF;
    YogVal s = YUNDEF;
    PUSH_LOCALS2(env, arg, s);

    YogCArg params[] = { { "n", &arg }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "*", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (!IS_FIXNUM(arg)) {
        YogError_raise_TypeError(env, "can't multiply string by non-Fixnum of type %C", arg);
    }

    s = YogString_multiply(env, self, VAL2INT(arg));

    RETURN(env, s);
}

static YogVal
lshift(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal arg = YUNDEF;
    PUSH_LOCAL(env, arg);

    YogCArg params[] = { { "s", &arg }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "<<", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    if (!IS_PTR(arg) || (BASIC_OBJ_TYPE(arg) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "operand must be String");
    }

    uint_t size1 = YogString_size(env, self);
    uint_t size2 = YogString_size(env, arg);
    uint_t size = size1 + size2 + 1;
    ensure_size(env, self, size);
    YogVal self_body = PTR_AS(YogString, self)->body;
    char* p = &PTR_AS(YogCharArray, self_body)->items[size1];
    YogVal arg_body = PTR_AS(YogString, arg)->body;
    const char* q = PTR_AS(YogCharArray, arg_body)->items;
    memcpy(p, q, size2);
    p[size2] = '\0';
    PTR_AS(YogString, self)->size = size;

    RETURN(env, self);
}

static BOOL
index2offset(YogEnv* env, YogVal self, int_t index, uint_t* offset)
{
    SAVE_ARG(env, self);
    if (index < 0) {
        RETURN(env, FALSE);
    }

    uint_t size = YogString_size(env, self);
    uint_t n = 0;
    int_t i;
    for (i = 0; i < index; i++) {
        char* p = &STRING_CSTR(self)[n];
        n += YogEncoding_mbc_size(env, STRING_ENCODING(self), p);
        if (size <= n) {
            RETURN(env, FALSE);
        }
    }
    if (size <= n) {
        RETURN(env, FALSE);
    }
    *offset = n;

    RETURN(env, TRUE);
}

static int_t
normalize_index(YogEnv* env, YogVal self, int_t index)
{
    SAVE_ARG(env, self);

    if (0 <= index) {
        RETURN(env, index);
    }
    uint_t n = 0;
    uint_t pos = 0;
    uint_t size = YogString_size(env, self);
    while (pos < size) {
        char* p = &STRING_CSTR(self)[pos];
        pos += YogEncoding_mbc_size(env, STRING_ENCODING(self), p);
        n++;
    }

    RETURN(env, n + index);
}

static uint_t
unnormalized_index2offset(YogEnv* env, YogVal self, int_t index)
{
    SAVE_ARG(env, self);

    int_t n = normalize_index(env, self, index);
    uint_t offset = 0;
    if (!index2offset(env, self, n, &offset)) {
        YogError_raise_IndexError(env, "string index out of range");
    }

    RETURN(env, offset);
}

static YogVal
get_at(YogEnv* env, YogVal self, int_t offset)
{
    SAVE_ARG(env, self);
    YogVal c = YUNDEF;
    PUSH_LOCAL(env, c);

    char* p = &STRING_CSTR(self)[offset];
    uint_t size = YogEncoding_mbc_size(env, STRING_ENCODING(self), p);

    c = YogString_new(env);
    uint_t i;
    for (i = 0; i < size; i++) {
        YogString_push(env, c, STRING_CSTR(self)[offset + i]);
    }

    RETURN(env, c);
}

static YogVal
get(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal index = YUNDEF;
    YogVal default_ = YUNDEF;
    YogVal c = YUNDEF;
    PUSH_LOCALS3(env, index, default_, c);
    YogCArg params[] = {
        { "index", &index },
        { "|", NULL },
        { "default", &default_ },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "get", params, args, kw);
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "string index must be Fixnum");
    }

    int_t n = normalize_index(env, self, VAL2INT(index));
    uint_t offset = 0;
    if (!index2offset(env, self, n, &offset)) {
        if (!IS_UNDEF(default_)) {
            RETURN(env, default_);
        }
        YogError_raise_IndexError(env, "string index out of range");
    }

    c = get_at(env, self, offset);
    RETURN(env, c);
}

static YogVal
subscript(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal index = YUNDEF;
    YogVal retval = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS3(env, index, retval, body);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { "index", &index }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]", params, args, kw);
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "string index must be Fixnum");
    }

    uint_t offset = unnormalized_index2offset(env, self, VAL2INT(index));
    char* p = &STRING_CSTR(self)[offset];
    uint_t mbc_size = YogEncoding_mbc_size(env, STRING_ENCODING(self), p);
    uint_t size = YogString_size(env, self);
    if (size - offset < mbc_size) {
        YogError_raise_IndexError(env, "string has not enough size");
    }

    retval = get_at(env, self, offset);
    RETURN(env, retval);
}

static YogVal
assign_subscript(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal index = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, index, val);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = {
        { "index", &index },
        { "value", &val },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]=", params, args, kw);
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "string index must be Fixnum");
    }
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "value must be String");
    }

    uint_t offset = unnormalized_index2offset(env, self, VAL2INT(index));
    char* p = &STRING_CSTR(self)[offset];
    uint_t prev_size = YogEncoding_mbc_size(env, STRING_ENCODING(self), p);
    const char* q = STRING_CSTR(val);
    uint_t new_size = YogEncoding_mbc_size(env, STRING_ENCODING(val), q);
    if (new_size < prev_size) {
        memcpy(p, q, new_size);
        uint_t n = STRING_SIZE(self) - offset - prev_size;
        memmove(p + new_size, p + prev_size, n);
    }
    else if (prev_size < new_size) {
        ensure_size(env, self, STRING_SIZE(self) - prev_size + new_size);
        char* p = &STRING_CSTR(self)[offset];
        uint_t n = STRING_SIZE(self) - offset - prev_size;
        memmove(p + new_size, p + prev_size, n);
        memcpy(p, STRING_CSTR(val), new_size);
    }
    else {
        memcpy(p, q, new_size);
    }

    RETURN(env, val);
}

YogVal
YogString_match(YogEnv* env, YogVal self, YogVal regexp, int_t pos)
{
    SAVE_ARGS2(env, self, regexp);
    YogVal m = YUNDEF;
    PUSH_LOCAL(env, m);

    int_t n = normalize_index(env, self, pos);
    uint_t offset = 0;
    uint_t size = YogString_size(env, self);
    if ((0 < size) && !index2offset(env, self, n, &offset)) {
        RETURN(env, YNIL);
    }

    OnigUChar* str = (OnigUChar*)STRING_CSTR(self);
    OnigUChar* end = (OnigUChar*)&STRING_CSTR(self)[STRING_SIZE(self) - 1];
    OnigUChar* start = (OnigUChar*)&STRING_CSTR(self)[offset];
    OnigRegion* region = onig_region_new();
    int_t r = onig_search(PTR_AS(YogRegexp, regexp)->onig_regexp, str, end, start, end, region, ONIG_OPTION_NONE);
    if (r == ONIG_MISMATCH) {
        RETURN(env, YNIL);
    }

    m = YogMatch_new(env, self, regexp, region);

    RETURN(env, m);
}

static YogVal
match(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal m = YUNDEF;
    YogVal regexp = YUNDEF;
    PUSH_LOCALS2(env, m, regexp);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { "regexp", &regexp }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "=~", params, args, kw);
    if (!IS_PTR(regexp) || (BASIC_OBJ_TYPE(regexp) != TYPE_REGEXP)) {
        YogError_raise_TypeError(env, "operand must be Regexp");
    }

    m = YogString_match(env, self, regexp, 0);

    RETURN(env, m);
}

static YogVal
each_line(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal arg[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(arg), arg);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "each_line", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    uint_t i = 0;
    uint_t size = YogString_size(env, self);
    while (i < size) {
        YogString* s = PTR_AS(YogString, self);
        YogVal body = s->body;
        const char* base = PTR_AS(YogCharArray, body)->items;
        const char* start = base + i;
        uint_t self_size = s->size;
        const char* p = (const char*)memchr(start, '\n', self_size - i - 1);
        if (p == NULL) {
            p = base + self_size - 1;
        }
        YogVal enc = s->encoding;
        p = YogEncoding_left_adjust_char_head(env, enc, base, p);
        const char* end = p - 1;
        const char* next = p + YogEncoding_mbc_size(env, enc, p);
        i = next - PTR_AS(YogCharArray, PTR_AS(YogString, self)->body)->items;
        arg[0] = YogString_from_range(env, enc, start, end);

        YogCallable_call(env, block, array_sizeof(arg), arg);
    }

    RETURN(env, YNIL);
}

static YogVal
dump(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "dump", params, args, kw);

    s = YogString_of_encoding(env, STRING_ENCODING(self));
#define FORMAT              "0x%02x"
#define ADD_CHAR(fmt, i)    do { \
    char buf[6]; \
    snprintf(buf, array_sizeof(buf), fmt, STRING_CSTR(self)[i]); \
    YogString_add_cstr(env, s, buf); \
} while (0)
    ADD_CHAR(FORMAT, 0);
    uint_t size = YogString_size(env, self) + 1;
    uint_t i;
    for (i = 1; i < size; i++) {
        ADD_CHAR(" " FORMAT, i);
    }
#undef ADD_CHAR
#undef FORMAT

    RETURN(env, s);
}

static YogVal
each_byte(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal a[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(a), a);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "each_byte", params, args, kw);

    uint_t i = 0;
    uint_t size = YogString_size(env, self);
    while (i < size) {
        a[0] = INT2VAL((unsigned char)STRING_CSTR(self)[i]);
        i++;

        YogCallable_call(env, block, array_sizeof(a), a);
    }

    RETURN(env, self);
}

static YogVal
each_char(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal enc = YUNDEF;
    PUSH_LOCAL(env, enc);
    YogVal a[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(a), a);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "each_char", params, args, kw);

    uint_t i = 0;
    uint_t size = YogString_size(env, self);
    while (i < size) {
        const char* start = &STRING_CSTR(self)[i];
        enc = STRING_ENCODING(self);
        const char* next = start + YogEncoding_mbc_size(env, enc, start);
        i = next - STRING_CSTR(self);
        a[0] = YogString_from_range(env, enc, start, next - 1);

        YogCallable_call(env, block, array_sizeof(a), a);
    }

    RETURN(env, self);
}

int_t
YogString_hash(YogEnv* env, YogVal self)
{
    const char* s = STRING_CSTR(self);

    /* This code came from strhash in src/table.c */
    int_t val = 0;
    char c;
    while ((c = *s++) != '\0') {
        val = val * 997 + c;
    }

    return val + (val >> 5);
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "hash", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    int_t hash = YogString_hash(env, self);

    RETURN(env, INT2VAL(hash));
}

char*
YogString_dup(YogEnv* env, const char* s)
{
    size_t size = strlen(s) + 1;
    char* p = PTR_AS(char, ALLOC_OBJ_SIZE(env, NULL, NULL, size));
    memcpy(p, s, size);

    return p;
}

static BOOL
normalize_as_number(YogEnv* env, YogVal self, YogVal* normalized, int_t* base)
{
    YOG_ASSERT(env, normalized != NULL, "normalized is NULL");
    YOG_ASSERT(env, base != NULL, "base is NULL");

    SAVE_ARG(env, self);
    YogVal body = YUNDEF;
    PUSH_LOCAL(env, body);

    uint_t size = PTR_AS(YogString, self)->size;
    *normalized = YogString_of_size(env, size + 2);
    if (size == 0) {
        RETURN(env, FALSE);
    }

    body = PTR_AS(YogString, self)->body;
    uint_t next_index = 0;
#define NEXTC   PTR_AS(YogCharArray, body)->items[next_index]
    char c = NEXTC;
    if (c == '\0') {
        RETURN(env, FALSE);
    }

    if (c == '+') {
        next_index++;
    }
    else if (c == '-') {
        YogString_push(env, *normalized, c);
        next_index++;
    }

    c = NEXTC;
    if (c == '\0') {
        RETURN(env, FALSE);
    }

    char c1 = c;
    char c2 = PTR_AS(YogCharArray, body)->items[next_index + 1];
    if (c1 == '0') {
        if ((c2 == 'b') || (c2 == 'B')) {
            *base = 2;
            next_index += 2;

            c = NEXTC;
            if ((c == '_') || (c == '\0')) {
                RETURN(env, FALSE);
            }
        }
        else if ((c2 == 'o') || (c2 == 'O')) {
            *base = 8;
            next_index += 2;

            c = NEXTC;
            if ((c == '_') || (c == '\0')) {
                RETURN(env, FALSE);
            }
        }
        else if ((c2 == 'd') || (c2 == 'D')) {
            *base = 10;
            next_index += 2;

            c = NEXTC;
            if ((c == '_') || (c == '\0')) {
                RETURN(env, FALSE);
            }
        }
        else if ((c2 == 'x') || (c2 == 'X')) {
            *base = 16;
            next_index += 2;

            c = NEXTC;
            if ((c == '_') || (c == '\0')) {
                RETURN(env, FALSE);
            }
        }
        else {
            YogString_push(env, *normalized, c1);
            *base = 10;
            next_index += 1;
        }
    }
    else {
        YogString_push(env, *normalized, c1);
        *base = 10;
        next_index += 1;
    }

    c = NEXTC;
    if (c == '\0') {
        RETURN(env, TRUE);
    }

    do {
        if (isalpha(c)) {
            YogString_push(env, *normalized, tolower(c));
            next_index++;
            c = NEXTC;
        }
        else if (c == '_') {
            next_index++;
            c = NEXTC;
            if (!isalpha(c) && !isdigit(c)) {
                RETURN(env, FALSE);
            }
        }
        else {
            YogString_push(env, *normalized, c);
            next_index++;
            c = NEXTC;
        }
    } while (c != '\0');

#undef NEXTC

    RETURN(env, TRUE);
}

YogVal
YogString_to_i(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal normalized = YUNDEF;
    YogVal body = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal v = YUNDEF;
    PUSH_LOCALS4(env, normalized, body, bignum, v);

#define RAISE_VALUE_ERROR   do { \
    YogError_raise_ValueError(env, "invalid literal: %S", self); \
    RETURN(env, INT2VAL(0)); \
} while (0)
    int_t base;
    if (!normalize_as_number(env, self, &normalized, &base)) {
        RAISE_VALUE_ERROR;
    }

    body = PTR_AS(YogString, normalized)->body;
    char* endptr = NULL;
    errno = 0;
    long n = strtol(PTR_AS(YogCharArray, body)->items, &endptr, base);
    if (*endptr != '\0') {
        RAISE_VALUE_ERROR;
    }
    if (errno == 0) {
        YogVal v = YogVal_from_int(env, n);
        RETURN(env, v);
    }
    else if (errno == ERANGE) {
        bignum = YogBignum_from_str(env, normalized, base);
        RETURN(env, bignum);
    }

    RAISE_VALUE_ERROR;
#undef RAISE_VALUE_ERROR

    /* NOTREACHED */
    RETURN(env, self);
}

void
YogString_add_cstr(YogEnv* env, YogVal self, const char* s)
{
    SAVE_ARG(env, self);
    YogVal body = YUNDEF;
    PUSH_LOCAL(env, body);

    uint_t size1 = YogString_size(env, self);
    uint_t size2 = strlen(s);
    uint_t size = size1 + size2 + 1;
    YOG_ASSERT(env, size1 <= size, "maybe overflow (%u + %u = %u)", size1, size2, size);
    body = YogCharArray_new(env, size);
    char* top = PTR_AS(YogCharArray, body)->items;
    memcpy(top, STRING_CSTR(self), size1);
    memcpy(top + size1, s, size2);
    top[size1 + size2] = '\0';
    PTR_AS(YogString, self)->body = body;
    PTR_AS(YogString, self)->size = size;

    RETURN_VOID(env);
}

static YogVal
compare(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, obj, retval);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { "s", &obj}, { NULL, NULL } };
    YogGetArgs_parse_args(env, "<=>", params, args, kw);
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_STRING)) {
        RETURN(env, YNIL);
    }

    int_t n = strcmp(STRING_CSTR(self), STRING_CSTR(obj));
    if (n < 0) {
        RETURN(env, INT2VAL(-1));
    }
    else if (n == 0) {
        RETURN(env, INT2VAL(n));
    }

    RETURN(env, INT2VAL(1));
}

void
YogString_eval_builtin_script(YogEnv* env, YogVal klass)
{
#if !defined(MINIYOG)
    const char* src =
#   include "string.inc"
    ;
    YogMisc_eval_source(env, klass, src);
#endif
}

void
YogString_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cString = YUNDEF;
    PUSH_LOCAL(env, cString);
    YogVM* vm = env->vm;

    cString = YogClass_new(env, "String", vm->cObject);
    YogClass_define_allocator(env, cString, alloc);
    YogClass_include_module(env, cString, vm->mComparable);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cString, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("*", multiply);
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("<<", lshift);
    DEFINE_METHOD("<=>", compare);
    DEFINE_METHOD("=~", match);
    DEFINE_METHOD("[]", subscript);
    DEFINE_METHOD("[]=", assign_subscript);
    DEFINE_METHOD("dump", dump);
    DEFINE_METHOD("each_byte", each_byte);
    DEFINE_METHOD("each_char", each_char);
    DEFINE_METHOD("each_line", each_line);
    DEFINE_METHOD("get", get);
    DEFINE_METHOD("gsub", gsub);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("to_i", to_i);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD
    YogClass_define_property(env, cString, pkg, "size", get_size, NULL);
    vm->cString = cString;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
