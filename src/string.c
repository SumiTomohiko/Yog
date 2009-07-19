#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oniguruma.h"
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/encoding.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/regexp.h"
#include "yog/thread.h"
#include "yog/yog.h"

#define CHECK_INT(v, msg)   do { \
    if (!IS_INT(v)) { \
        YogError_raise_TypeError(env, msg); \
    } \
} while (0)

ID 
YogString_intern(YogEnv* env, YogVal s) 
{
    YogVal body = PTR_AS(YogString, s)->body;
    const char* p = PTR_AS(YogCharArray, body)->items;
    return INTERN(p);
}

static void 
YogCharArray_clear(YogEnv* env, YogVal array)
{
    if (0 < PTR_AS(YogCharArray, array)->size) {
        PTR_AS(YogCharArray, array)->items[0] = '\0';
    }
}

static YogVal 
YogCharArray_new(YogEnv* env, unsigned int size) 
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

    YogString* s = ptr;
#define KEEP(member)    YogGC_keep(env, &s->member, keeper, heap)
    KEEP(body);
    KEEP(encoding);
#undef KEEP
}

unsigned int 
YogString_size(YogEnv* env, YogVal string) 
{
    return PTR_AS(YogString, string)->size;
}

static void 
ensure_body(YogEnv* env, YogVal string) 
{
    SAVE_ARG(env, string);

    if (!IS_PTR(PTR_AS(YogString, string)->body)) {
#define CAPACITY    (1)
        YogVal body = YogCharArray_new(env, CAPACITY);
#undef CAPACITY
        MODIFY(env, PTR_AS(YogString, string)->body, body);
    }

    RETURN_VOID(env);
}

static void 
ensure_size(YogEnv* env, YogVal string, unsigned int size) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, string);

    YogVal body = PTR_AS(YogString, string)->body;
    PUSH_LOCAL(env, body);
    if (!IS_PTR(body) || (PTR_AS(YogCharArray, body)->size < size)) {
        unsigned int capacity = 1;
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
        unsigned int size = PTR_AS(YogString, string)->size;
        memcpy(p, q, size);

        MODIFY(env, PTR_AS(YogString, string)->body, new_body);
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
    unsigned int needed_size = PTR_AS(YogString, string)->size + 1;
    if (PTR_AS(YogCharArray, body)->size < needed_size) {
        ensure_size(env, string, needed_size);
        body = PTR_AS(YogString, string)->body;
    }

    unsigned int size = PTR_AS(YogString, string)->size;
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
allocate(YogEnv* env, YogVal klass) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogVal obj = ALLOC_OBJ(env, YogString_keep_children, NULL, YogString);
    YogBasicObj_init(env, obj, 0, klass);

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

    self = allocate(env, env->vm->cString);
    body = YogCharArray_new(env, 1);

    PTR_AS(YogString, self)->encoding = YUNDEF;
    PTR_AS(YogString, self)->size = 1;
    PTR_AS(YogString, self)->body = body;

    RETURN(env, self);
}

YogVal 
YogString_new_range(YogEnv* env, YogVal enc, const char* start, const char* end)
{
    SAVE_ARG(env, enc);

    unsigned int size = 0;
    if (start <= end) {
        size = end - start + 1;
    }

    /* FIXME: dirty hack */
    char escaped_from_gc[size];
    strncpy(escaped_from_gc, start, size);

    YogVal body = YogCharArray_new(env, size + 1);
    memcpy(PTR_AS(YogCharArray, body)->items, escaped_from_gc, size);
    PTR_AS(YogCharArray, body)->items[size] = '\0';
    PUSH_LOCAL(env, body);

    YogVal s = YogString_new(env);
    MODIFY(env, PTR_AS(YogString, s)->encoding, enc);
    PTR_AS(YogString, s)->size = size + 1;
    MODIFY(env, PTR_AS(YogString, s)->body, body);

    RETURN(env, s);
}

YogVal 
YogString_new_size(YogEnv* env, unsigned int size) 
{
    SAVE_LOCALS(env);
    YogVal string = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS2(env, string, body);

    string = allocate(env, env->vm->cString);
    if (size == 0) {
        RETURN(env, string);
    }
    body = YogCharArray_new(env, size);

    PTR_AS(YogString, string)->encoding = YNIL;
    PTR_AS(YogString, string)->size = 1;
    MODIFY(env, PTR_AS(YogString, string)->body, body);

    RETURN(env, string);
}

#define RETURN_STR(s)   do { \
    size_t len = strlen(s); \
    char buffer[len + 1]; \
    strcpy(buffer, s); \
    YogVal body = YogCharArray_new_str(env, buffer); \
    PUSH_LOCAL(env, body); \
    \
    YogVal string = allocate(env, env->vm->cString); \
    PTR_AS(YogString, string)->encoding = YNIL; \
    PTR_AS(YogString, string)->size = len + 1; \
    PTR_AS(YogString, string)->body = body; \
    \
    POP_LOCALS(env); \
    return string; \
} while (0)

YogVal 
YogString_new_str(YogEnv* env, const char* s) 
{
    RETURN_STR(s);
}

YogVal 
YogString_new_format(YogEnv* env, const char* fmt, ...) 
{
    va_list ap;
    va_start(ap, fmt);
#define BUFSIZE (1024)
    char buf[BUFSIZE];
    vsnprintf(buf, BUFSIZE, fmt, ap);
#undef BUFSIZE
    va_end(ap);
    RETURN_STR(buf);
}

#undef RETURN_STR

YogVal 
YogString_clone(YogEnv* env, YogVal string) 
{
    SAVE_ARG(env, string);

    YogVal body = PTR_AS(YogString, string)->body;
    YogVal s = YogString_new_str(env, PTR_AS(YogCharArray, body)->items);
    MODIFY(env, PTR_AS(YogString, s)->encoding, PTR_AS(YogString, string)->encoding);

    RETURN(env, s);
}

char 
YogString_at(YogEnv* env, YogVal s, unsigned int n) 
{
    YogVal body = PTR_AS(YogString, s)->body;
    return PTR_AS(YogCharArray, body)->items[n];
}

static YogVal 
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    return self;
}

static YogVal 
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal arg = YogArray_at(env, args, 0);
    PUSH_LOCAL(env, arg);

    unsigned int size1 = YogString_size(env, self);
    unsigned int size2 = YogString_size(env, arg);
    unsigned int size = size1 + size2 - 1;
    YogVal s = YogString_new_size(env, size);
    YogVal body = PTR_AS(YogString, s)->body;
    char* p = PTR_AS(YogCharArray, body)->items;
    YogVal self_body = PTR_AS(YogString, self)->body;
    const char* q = PTR_AS(YogCharArray, self_body)->items;
    memcpy(p, q, size1);
    char* u = &PTR_AS(YogCharArray, body)->items[size1 - 1];
    YogVal arg_body = PTR_AS(YogString, arg)->body;
    const char* v = PTR_AS(YogCharArray, arg_body)->items;
    memcpy(u, v, size2);

    RETURN(env, s);
}

static YogVal 
lshift(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal arg = YUNDEF;
    PUSH_LOCAL(env, arg);

    arg = YogArray_at(env, args, 0);

    unsigned int size1 = YogString_size(env, self);
    unsigned int size2 = YogString_size(env, arg);
    unsigned int size = size1 + size2 - 1;
    ensure_size(env, self, size);
    YogVal self_body = PTR_AS(YogString, self)->body;
    char* p = &PTR_AS(YogCharArray, self_body)->items[size1 - 1];
    YogVal arg_body = PTR_AS(YogString, arg)->body;
    const char* q = PTR_AS(YogCharArray, arg_body)->items;
    memcpy(p, q, size2);

    RETURN(env, self);
}

static char* 
index2ptr(YogEnv* env, YogString* s, unsigned int index)
{
    YogVal enc = s->encoding;
    YogVal body = s->body;
    unsigned int size = s->size;
    const char* end = &PTR_AS(YogCharArray, body)->items[size - 1];
    char* p = PTR_AS(YogCharArray, body)->items;
    unsigned int i = 0;
    for (i = 0; i < index; i++) {
        unsigned int size = YogEncoding_mbc_size(env, enc, p);
        p += size;
        if (end <= p) {
            YogError_raise_IndexError(env, "string index out of range");
        }
    }

    return p;
}

static YogVal 
subscript(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal arg = YogArray_at(env, args, 0);
    YogVal retval = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS3(env, arg, retval, body);
    CHECK_INT(arg, "string index must be integer");

    retval = YogString_new(env);
    YogString* s = PTR_AS(YogString, self);
    int index = VAL2INT(arg);
    const char* p = index2ptr(env, s, index);
    body = s->body;
    const char* q = PTR_AS(YogCharArray, body)->items;
    unsigned int offset = p - q;

    unsigned int mbc_size = YogEncoding_mbc_size(env, s->encoding, p);
    unsigned int size = s->size;
    if ((size - 1) - offset < mbc_size) {
        YogError_raise_IndexError(env, "string index out of range");
    }
    unsigned int i;
    for (i = 0; i < mbc_size; i++) {
        p = PTR_AS(YogCharArray, body)->items + offset + i;
        YogString_push(env, retval, *p);
        p++;
    }

    RETURN(env, retval);
}

static YogVal 
assign_subscript(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal arg0 = YogArray_at(env, args, 0);
    YogVal arg1 = YogArray_at(env, args, 1);
    PUSH_LOCALS2(env, arg0, arg1);
    CHECK_INT(arg0, "string index must be integer");

    YogString* s = PTR_AS(YogString, self);
    int index = VAL2INT(arg0);
    char* p = index2ptr(env, s, index);
    unsigned int size_orig = YogEncoding_mbc_size(env, s->encoding, p);
    YogVal body0 = s->body;

    YogString* t = PTR_AS(YogString, arg1);
    YogVal body1 = t->body;
    const char* q = PTR_AS(YogCharArray, body1)->items;
    unsigned int mbc_size = YogEncoding_mbc_size(env, t->encoding, q);
    if (mbc_size < size_orig) {
        unsigned int i = 0;
        for (i = 0; i < mbc_size; i++) {
            *p = *q;
            p++;
            q++;
        }
        unsigned int size = s->size;
        const char* r = PTR_AS(YogCharArray, body0)->items;
        memcpy(p, p + (size_orig - mbc_size), size - (r - p));
    }
    else if (size_orig < mbc_size) {
        const char* r = PTR_AS(YogCharArray, body0)->items;
        unsigned int n = s->size;
        ensure_size(env, self, n - size_orig + mbc_size);
        /* FIXME: dirty hack */
        s = PTR_AS(YogString, self);
        t = PTR_AS(YogString, arg1);
        q = PTR_AS(YogCharArray, t->body)->items;

        p = PTR_AS(YogCharArray, s->body)->items + (p - r);
        memmove(p + (mbc_size - size_orig), p, s->size - (p - PTR_AS(YogCharArray, s->body)->items));
        unsigned int i = 0;
        for (i = 0; i < mbc_size; i++) {
            *p = *q;
            p++;
            q++;
        }
    }
    else {
        unsigned int i = 0;
        for (i = 0; i < mbc_size; i++) {
            *p = *q;
            p++;
            q++;
        }
    }

    RETURN(env, arg1);
}

static YogVal 
match(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal arg = YogArray_at(env, args, 0);

    YogString* s = PTR_AS(YogString, self);
    YogRegexp* regexp = PTR_AS(YogRegexp, arg);
    OnigUChar* begin = (OnigUChar*)PTR_AS(YogCharArray, s->body)->items;
    OnigUChar* end = begin + s->size - 1;
    OnigRegion* region = onig_region_new();
    int r = onig_search(regexp->onig_regexp, begin, end, begin, end, region, ONIG_OPTION_NONE);
    if (r == ONIG_MISMATCH) {
        RETURN(env, YNIL);
    }

    YogVal retval = YogMatch_new(env, self, arg, region);
    RETURN(env, retval);
}

static YogVal 
each_line(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal arg[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(arg), arg);

    unsigned int i = 0;
    do {
        YogString* s = PTR_AS(YogString, self);
        YogVal body = s->body;
        const char* base = PTR_AS(YogCharArray, body)->items;
        const char* start = base + i;
        unsigned int self_size = s->size;
        const char* p = memchr(start, '\n', self_size - i - 1);
        if (p == NULL) {
            p = base + self_size - 1;
        }
        YogVal enc = s->encoding;
        p = YogEncoding_left_adjust_char_head(env, enc, base, p);
        const char* end = p - 1;
        const char* next = p + YogEncoding_mbc_size(env, enc, p);
        i = next - PTR_AS(YogCharArray, PTR_AS(YogString, self)->body)->items;
        arg[0] = YogString_new_range(env, enc, start, end);

        unsigned int size = PTR_AS(YogCharArray, PTR_AS(YogString, self)->body)->size;

        YogCallable_call(env, block, array_sizeof(arg), arg);

        if (size - 1 < i) {
            break;
        }
    } while (1);

    RETURN(env, YNIL);
}

static YogVal 
each_byte(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal arg[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(arg), arg);

    unsigned int i = 0;
    do {
        YogString* s = PTR_AS(YogString, self);
        YogVal body = s->body;
        unsigned char p = PTR_AS(YogCharArray, body)->items[i];
        arg[0] = INT2VAL(p);

        i++;
        unsigned int size = s->size;

        YogCallable_call(env, block, array_sizeof(arg), arg);

        if (size - 1 < i + 1) {
            break;
        }
    } while (1);

    RETURN(env, YNIL);
}

static YogVal 
each_char(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal arg[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(arg), arg);

    unsigned int i = 0;
    do {
        YogString* s = PTR_AS(YogString, self);
        YogVal body = s->body;
        const char* start = PTR_AS(YogCharArray, body)->items + i;
        YogVal enc = s->encoding;
        const char* next = start + YogEncoding_mbc_size(env, enc, start);
        i = next - PTR_AS(YogCharArray, PTR_AS(YogString, self)->body)->items;
        const char* end = next - 1;
        arg[0] = YogString_new_range(env, enc, start, end);

        unsigned int size = PTR_AS(YogCharArray, PTR_AS(YogString, self)->body)->size;

        YogCallable_call(env, block, array_sizeof(arg), arg);

        if (size - 1 < i + 1) {
            break;
        }
    } while (1);

    RETURN(env, YNIL);
}

YogVal 
YogString_klass_new(YogEnv* env) 
{
    YogVal klass = YogKlass_new(env, "String", env->vm->cObject);
    PUSH_LOCAL(env, klass);

    YogKlass_define_allocator(env, klass, allocate);
    YogKlass_define_method(env, klass, "to_s", to_s);
    YogKlass_define_method(env, klass, "+", add);
    YogKlass_define_method(env, klass, "<<", lshift);
    YogKlass_define_method(env, klass, "[]", subscript);
    YogKlass_define_method(env, klass, "[]=", assign_subscript);
    YogKlass_define_method(env, klass, "=~", match);
    YogKlass_define_method(env, klass, "each_line", each_line);
    YogKlass_define_method(env, klass, "each_byte", each_byte);
    YogKlass_define_method(env, klass, "each_char", each_char);

    POP_LOCALS(env);
    return klass;
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
normalize_as_number(YogEnv* env, YogVal self, YogVal* normalized, int* base)
{
    YOG_ASSERT(env, normalized != NULL, "normalized is NULL");
    YOG_ASSERT(env, base != NULL, "base is NULL");

    SAVE_ARG(env, self);
    YogVal body = YUNDEF;
    PUSH_LOCAL(env, body);

    unsigned int size = PTR_AS(YogString, self)->size;
    *normalized = YogString_new_size(env, size + 2);
    if (size == 0) {
        RETURN(env, FALSE);
    }

    body = PTR_AS(YogString, self)->body;
    unsigned int next_index = 0;
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
    const char* s = PTR_AS(YogCharArray, body)->items; \
    YogError_raise_ValueError(env, "invalid literal: %s", s); \
    RETURN(env, INT2VAL(0)); \
} while (0)
    int base;
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

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
