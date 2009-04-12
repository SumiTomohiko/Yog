#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "oniguruma.h"
#include "yog/encoding.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/klass.h"
#include "yog/regexp.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

ID 
YogString_intern(YogEnv* env, YogVal s) 
{
    YogVal body = OBJ_AS(YogString, s)->body;
    const char* p = PTR_AS(YogCharArray, body)->items;
    return INTERN(p);
}

static void 
YogCharArray_clear(YogEnv* env, YogVal array)
{
    if (0 < PTR_AS(YogCharArray, array)->capacity) {
        PTR_AS(YogCharArray, array)->size = 1;
        PTR_AS(YogCharArray, array)->items[0] = '\0';
    }
    else {
        PTR_AS(YogCharArray, array)->size = 0;
    }
}

YogVal 
YogCharArray_new(YogEnv* env, unsigned int capacity) 
{
    YogCharArray* array = ALLOC_OBJ_ITEM(env, NULL, NULL, YogCharArray, capacity, char);
    array->capacity = capacity;
    YogCharArray_clear(env, PTR2VAL(array));

    return PTR2VAL(array);
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
YogString_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogBasicObj_keep_children(env, ptr, keeper);

    YogString* s = ptr;
#define KEEP(member)    s->member = YogVal_keep(env, s->member, keeper)
    KEEP(body);
    KEEP(encoding);
#undef KEEP
}

unsigned int 
YogString_size(YogEnv* env, YogVal string) 
{
    YogVal body = OBJ_AS(YogString, string)->body;
    if (IS_PTR(body)) {
        return PTR_AS(YogCharArray, body)->size;
    }
    else {
        return 1;
    }
}

static void 
ensure_body(YogEnv* env, YogVal string) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, string);

    if (!IS_PTR(OBJ_AS(YogString, string)->body)) {
#define CAPACITY    (1)
        YogVal body = YogCharArray_new(env, CAPACITY);
#undef CAPACITY
        MODIFY(env, OBJ_AS(YogString, string)->body, body);
    }

    RETURN_VOID(env);
}

static void 
ensure_size(YogEnv* env, YogVal string, unsigned int size) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, string);

    YogVal body = OBJ_AS(YogString, string)->body;
    PUSH_LOCAL(env, body);
    if (!IS_PTR(body) || (PTR_AS(YogCharArray, body)->capacity < size)) {
        unsigned int capacity = 1;
        if (!IS_PTR(body)) {
            capacity = PTR_AS(YogCharArray, body)->capacity;
        }
        do {
#define RATIO   (2)
            capacity = RATIO * capacity;
#undef RATIO
        } while (capacity < size);

        YogVal new_body = YogCharArray_new(env, capacity);
        char* p = PTR_AS(YogCharArray, new_body)->items;
        char* q = PTR_AS(YogCharArray, body)->items;
        unsigned int size = PTR_AS(YogCharArray, body)->size;
        memcpy(p, q, size);
        PTR_AS(YogCharArray, new_body)->size = size;

        MODIFY(env, OBJ_AS(YogString, string)->body, new_body);
    }

    RETURN_VOID(env);
}

void 
YogString_push(YogEnv* env, YogVal string, char c) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, string);

    ensure_body(env, string);

    YogVal body = OBJ_AS(YogString, string)->body;
    unsigned int needed_size = PTR_AS(YogCharArray, body)->size + 1;
    if (PTR_AS(YogCharArray, body)->capacity < needed_size) {
        ensure_size(env, string, needed_size);
        body = OBJ_AS(YogString, string)->body;
    }

    unsigned int size = PTR_AS(YogCharArray, body)->size;
    PTR_AS(YogCharArray, body)->items[size - 1] = c;
    PTR_AS(YogCharArray, body)->items[size] = '\0';
    PTR_AS(YogCharArray, body)->size++;

    RETURN_VOID(env);
}

void 
YogString_clear(YogEnv* env, YogVal string) 
{
    if (IS_PTR(OBJ_AS(YogString, string)->body)) {
        YogCharArray_clear(env, OBJ_AS(YogString, string)->body);
    }
}

static YogVal 
allocate(YogEnv* env, YogVal klass) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogBasicObj* obj = ALLOC_OBJ(env, YogString_keep_children, NULL, YogString);
    YogBasicObj_init(env, obj, 0, klass);

    YogString* s = (YogString*)obj;
    s->encoding = YUNDEF;
    s->body = YUNDEF;

    RETURN(env, OBJ2VAL(obj));
}

YogVal 
YogString_new(YogEnv* env) 
{
    YogVal string = allocate(env, env->vm->cString);

    OBJ_AS(YogString, string)->encoding = YUNDEF;
    OBJ_AS(YogString, string)->body = YUNDEF;

    return string;
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
    MODIFY(env, OBJ_AS(YogString, s)->encoding, enc);
    MODIFY(env, OBJ_AS(YogString, s)->body, body);

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
    body = YogCharArray_new(env, size);

    OBJ_AS(YogString, string)->encoding = YNIL;
    MODIFY(env, OBJ_AS(YogString, string)->body, body);

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
    OBJ_AS(YogString, string)->encoding = YNIL; \
    OBJ_AS(YogString, string)->body = body; \
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

    YogVal body = OBJ_AS(YogString, string)->body;
    YogVal s = YogString_new_str(env, PTR_AS(YogCharArray, body)->items);
    MODIFY(env, OBJ_AS(YogString, s)->encoding, OBJ_AS(YogString, string)->encoding);

    RETURN(env, s);
}

char 
YogString_at(YogEnv* env, YogVal s, unsigned int n) 
{
    YogVal body = OBJ_AS(YogString, s)->body;
    return PTR_AS(YogCharArray, body)->items[n];
}

static YogVal 
to_s(YogEnv* env)
{
    return SELF(env);
}

static YogVal 
add(YogEnv* env) 
{
    YogVal self = SELF(env);
    YogVal arg = ARG(env, 0);
    PUSH_LOCALS2(env, self, arg);

    unsigned int size1 = YogString_size(env, self);
    unsigned int size2 = YogString_size(env, arg);
    unsigned int size = size1 + size2 - 1;
    YogVal s = YogString_new_size(env, size);
    YogVal body = OBJ_AS(YogString, s)->body;
    char* p = PTR_AS(YogCharArray, body)->items;
    YogVal self_body = OBJ_AS(YogString, self)->body;
    const char* q = PTR_AS(YogCharArray, self_body)->items;
    memcpy(p, q, size1);
    char* u = &PTR_AS(YogCharArray, body)->items[size1 - 1];
    YogVal arg_body = OBJ_AS(YogString, arg)->body;
    const char* v = PTR_AS(YogCharArray, arg_body)->items;
    memcpy(u, v, size2);

    POP_LOCALS(env);
    return s;
}

static YogVal 
lshift(YogEnv* env) 
{
    YogVal self = SELF(env);
    YogVal arg = ARG(env, 0);
    PUSH_LOCALS2(env, self, arg);

    unsigned int size1 = YogString_size(env, self);
    unsigned int size2 = YogString_size(env, arg);
    unsigned int size = size1 + size2 - 1;
    ensure_size(env, self, size);
    YogVal self_body = OBJ_AS(YogString, self)->body;
    char* p = &PTR_AS(YogCharArray, self_body)->items[size1 - 1];
    YogVal arg_body = OBJ_AS(YogString, arg)->body;
    const char* q = PTR_AS(YogCharArray, arg_body)->items;
    memcpy(p, q, size2);

    POP_LOCALS(env);
    return self;
}

static char* 
index2ptr(YogEnv* env, YogString* s, unsigned int index)
{
    YogVal enc = s->encoding;
    YogVal body = s->body;
    unsigned int size = PTR_AS(YogCharArray, body)->size;
    const char* end = &PTR_AS(YogCharArray, body)->items[size - 1];
    char* p = PTR_AS(YogCharArray, body)->items;
    unsigned int i = 0;
    for (i = 0; i < index; i++) {
        unsigned int size = YogEncoding_mbc_size(env, enc, p);
        p += size;
        if (end <= p) {
            YogError_raise_index_error(env, "string index out of range");
        }
    }

    return p;
}

static YogVal 
subscript(YogEnv* env) 
{
    YogVal self = SELF(env);
    YogVal arg = ARG(env, 0);
    YogVal retval = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS4(env, self, arg, retval, body);
    CHECK_INT(arg, "string index must be integer");

    retval = YogString_new(env);
    YogString* s = OBJ_AS(YogString, self);
    int index = VAL2INT(arg);
    const char* p = index2ptr(env, s, index);
    body = s->body;
    const char* q = PTR_AS(YogCharArray, body)->items;
    unsigned int offset = p - q;

    unsigned int mbc_size = YogEncoding_mbc_size(env, s->encoding, p);
    unsigned int size = PTR_AS(YogCharArray, body)->size;
    if ((size - 1) - offset < mbc_size) {
        YogError_raise_index_error(env, "string index out of range");
    }
    unsigned int i;
    for (i = 0; i < mbc_size; i++) {
        p = PTR_AS(YogCharArray, body)->items + offset + i;
        YogString_push(env, retval, *p);
        p++;
    }

    POP_LOCALS(env);
    return retval;
}

static YogVal 
assign_subscript(YogEnv* env) 
{
    YogVal self = SELF(env);
    YogVal arg0 = ARG(env, 0);
    YogVal arg1 = ARG(env, 1);
    CHECK_INT(arg0, "string index must be integer");

    YogString* s = OBJ_AS(YogString, self);
    int index = VAL2INT(arg0);
    char* p = index2ptr(env, s, index);
    unsigned int size_orig = YogEncoding_mbc_size(env, s->encoding, p);
    YogVal body0 = s->body;

    YogString* t = OBJ_AS(YogString, arg1);
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
        unsigned int size = PTR_AS(YogCharArray, body0)->size;
        const char* r = PTR_AS(YogCharArray, body0)->items;
        memcpy(p, p + (size_orig - mbc_size), size - (r - p));
    }
    else if (size_orig < mbc_size) {
        const char* r = PTR_AS(YogCharArray, body0)->items;
        unsigned int n = PTR_AS(YogCharArray, body0)->size;
        ensure_size(env, OBJ2VAL(s), n - size_orig + mbc_size);
        /* FIXME: dirty hack */
        s = OBJ_AS(YogString, SELF(env));
        t = OBJ_AS(YogString, ARG(env, 1));
        q = PTR_AS(YogCharArray, t->body)->items;

        p = PTR_AS(YogCharArray, s->body)->items + (p - r);
        memmove(p + (mbc_size - size_orig), p, PTR_AS(YogCharArray, s->body)->size - (p - PTR_AS(YogCharArray, s->body)->items));
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

    return arg1;
}

static YogVal 
match(YogEnv* env) 
{
    YogVal self = SELF(env);
    YogVal arg = ARG(env, 0);

    YogString* s = OBJ_AS(YogString, self);
    YogRegexp* regexp = OBJ_AS(YogRegexp, arg);
    OnigUChar* begin = (OnigUChar*)PTR_AS(YogCharArray, s->body)->items;
    OnigUChar* end = begin + PTR_AS(YogCharArray, s->body)->size;
    OnigRegion* region = onig_region_new();
    int r = onig_search(regexp->onig_regexp, begin, end, begin, end, region, ONIG_OPTION_NONE);
    if (r == ONIG_MISMATCH) {
        return YNIL;
    }

    YogMatch* match = YogMatch_new(env, self, arg, region);
    YogVal retval = OBJ2VAL(match);

    return retval;
}

static YogVal 
each_line(YogEnv* env) 
{
    unsigned int i = 0;
    do {
        YogVal self = SELF(env);
        YogString* s = OBJ_AS(YogString, self);
        YogVal body = s->body;
        const char* base = PTR_AS(YogCharArray, body)->items;
        const char* start = base + i;
        unsigned int self_size = PTR_AS(YogCharArray, body)->size;
        const char* p = memchr(start, '\n', self_size - i - 1);
        if (p == NULL) {
            p = base + self_size - 1;
        }
        YogVal enc = s->encoding;
        p = YogEncoding_left_adjust_char_head(env, enc, base, p);
        const char* end = p - 1;
        const char* next = p + YogEncoding_mbc_size(env, enc, p);
        i = next - PTR_AS(YogCharArray, OBJ_AS(YogString, SELF(env))->body)->items;
        YogVal line = YogString_new_range(env, enc, start, end);
        YogVal block = ARG(env, 0);
        YogVal args[] = { line, };

        unsigned int size = PTR_AS(YogCharArray, OBJ_AS(YogString, SELF(env))->body)->size;

        YogThread_call_block(env, env->th, block, sizeof(args) / sizeof(args[0]), args);

        if (size - 1 < i) {
            break;
        }
    } while (1);

    return YNIL;
}

static YogVal 
each_byte(YogEnv* env) 
{
    unsigned int i = 0;
    do {
        YogVal self = SELF(env);
        YogString* s = OBJ_AS(YogString, self);
        YogVal body = s->body;
        unsigned char p = PTR_AS(YogCharArray, body)->items[i];
        YogVal block = ARG(env, 0);
        YogVal args[] = { INT2VAL(p), };

        i++;
        unsigned int size = PTR_AS(YogCharArray, body)->size;

        YogThread_call_block(env, env->th, block, sizeof(args) / sizeof(args[0]), args);

        if (size - 1 < i + 1) {
            break;
        }
    } while (1);

    return YNIL;
}

static YogVal 
each_char(YogEnv* env) 
{
    unsigned int i = 0;
    do {
        YogVal self = SELF(env);
        YogString* s = OBJ_AS(YogString, self);
        YogVal body = s->body;
        const char* start = PTR_AS(YogCharArray, body)->items + i;
        YogVal enc = s->encoding;
        const char* next = start + YogEncoding_mbc_size(env, enc, start);
        i = next - PTR_AS(YogCharArray, OBJ_AS(YogString, SELF(env))->body)->items;
        const char* end = next - 1;
        YogVal c = YogString_new_range(env, enc, start, end);
        YogVal block = ARG(env, 0);
        YogVal args[] = { c, };

        unsigned int size = PTR_AS(YogCharArray, OBJ_AS(YogString, SELF(env))->body)->size;

        YogThread_call_block(env, env->th, block, sizeof(args) / sizeof(args[0]), args);

        if (size - 1 < i + 1) {
            break;
        }
    } while (1);

    return YNIL;
}

YogVal 
YogString_klass_new(YogEnv* env) 
{
    YogVal klass = YogKlass_new(env, "String", env->vm->cObject);
    PUSH_LOCAL(env, klass);

    YogKlass_define_allocator(env, klass, allocate);
    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);
    YogKlass_define_method(env, klass, "+", add, 0, 0, 0, 0, "s", NULL);
    YogKlass_define_method(env, klass, "<<", lshift, 0, 0, 0, 0, "s", NULL);
    YogKlass_define_method(env, klass, "[]", subscript, 0, 0, 0, 0, "n", NULL);
    YogKlass_define_method(env, klass, "[]=", assign_subscript, 0, 0, 0, 0, "n", "s", NULL);
    YogKlass_define_method(env, klass, "=~", match, 0, 0, 0, 1, "regexp", NULL);
    YogKlass_define_method(env, klass, "each_line", each_line, 1, 0, 0, 1, "block", NULL);
    YogKlass_define_method(env, klass, "each_byte", each_byte, 1, 0, 0, 1, "block", NULL);
    YogKlass_define_method(env, klass, "each_char", each_char, 1, 0, 0, 1, "block", NULL);

    POP_LOCALS(env);
    return klass;
}

char* 
YogString_dup(YogEnv* env, const char* s) 
{
    size_t size = strlen(s) + 1;
    char* p = ALLOC_OBJ_SIZE(env, NULL, NULL, size);
    memcpy(p, s, size);

    return p;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
