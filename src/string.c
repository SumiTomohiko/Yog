#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "oniguruma.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/regexp.h"
#include "yog/yog.h"

ID 
YogString_intern(YogEnv* env, YogString* s) 
{
    return INTERN(s->body->items);
}

static void 
YogCharArray_clear(YogEnv* env, YogCharArray* array) 
{
    if (0 < array->capacity) {
        array->size = 1;
        array->items[0] = '\0';
    }
    else {
        array->size = 0;
    }
}

YogCharArray* 
YogCharArray_new(YogEnv* env, unsigned int capacity) 
{
    YogCharArray* array = ALLOC_OBJ_ITEM(env, NULL, NULL, YogCharArray, capacity, char);
    array->capacity = capacity;
    YogCharArray_clear(env, array);

    return array;
}

YogCharArray* 
YogCharArray_new_str(YogEnv* env, const char* s) 
{
    size_t size = strlen(s) + 1;
    YogCharArray* array = YogCharArray_new(env, size);
    memcpy(array->items, s, size);
    array->size = size;

    return array;
}

static void 
YogString_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogBasicObj_keep_children(env, ptr, keeper);

    YogString* s = ptr;
#define KEEP(member)    s->member = (*keeper)(env, s->member)
    KEEP(encoding);
    KEEP(body);
#undef KEEP
}

unsigned int 
YogString_size(YogEnv* env, YogString* string) 
{
    YogCharArray* body = string->body;
    if (body != NULL) {
        return body->size;
    }
    else {
        return 1;
    }
}

static void 
ensure_body(YogEnv* env, YogString* string) 
{
    if (string->body == NULL) {
#define CAPACITY    (1)
        string->body = YogCharArray_new(env, CAPACITY);
#undef CAPACITY
    }
}

static void 
ensure_size(YogEnv* env, YogString* string, unsigned int size) 
{
    YogCharArray* body = string->body;
    if ((body == NULL) || (body->capacity < size)) {
        unsigned int capacity = 1;
        if (body != NULL) {
            capacity = body->capacity;
        }
        do {
#define RATIO   (2)
            capacity = RATIO * capacity;
#undef RATIO
        } while (capacity < size);

        YogCharArray* new_body = YogCharArray_new(env, capacity);
        memcpy(new_body->items, body->items, body->size);
        new_body->size = body->size;

        string->body = new_body;
    }
}

void 
YogString_push(YogEnv* env, YogString* string, char c) 
{
    ensure_body(env, string);

    YogCharArray* body = string->body;
    unsigned int needed_size = body->size + 1;
    if (body->capacity < needed_size) {
        ensure_size(env, string, needed_size);
        body = string->body;
    }

    body->items[body->size - 1] = c;
    body->items[body->size] = '\0';
    body->size++;
}

void 
YogString_clear(YogEnv* env, YogString* string) 
{
    if (string->body != NULL) {
        YogCharArray_clear(env, string->body);
    }
}

static YogBasicObj* 
allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, YogString_keep_children, NULL, YogString);
    YogBasicObj_init(env, obj, 0, klass);

    YogString* s = (YogString*)obj;
    s->encoding = NULL;
    s->body = NULL;

    return obj;
}

YogString* 
YogString_new(YogEnv* env) 
{
    YogString* string = (YogString*)allocate(env, ENV_VM(env)->cString);

    string->encoding = NULL;
    string->body = NULL;

    return string;
}

YogString* 
YogString_new_size(YogEnv* env, unsigned int size) 
{
    YogString* string = (YogString*)allocate(env, ENV_VM(env)->cString);

    string->encoding = NULL;
    string->body = YogCharArray_new(env, size);

    return string;
}

#define RETURN_STR(s)   do { \
    YogCharArray* body = YogCharArray_new_str(env, s); \
    YogString* string = (YogString*)allocate(env, ENV_VM(env)->cString); \
    string->encoding = NULL; \
    string->body = body; \
    return string; \
} while (0)

YogString* 
YogString_new_str(YogEnv* env, const char* s) 
{
    RETURN_STR(s);
}

YogString* 
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

YogString* 
YogString_clone(YogEnv* env, YogString* string) 
{
    YogString* s = YogString_new_str(env, string->body->items);
    s->encoding = string->encoding;

    return s;
}

char 
YogString_at(YogEnv* env, YogString* s, unsigned int n) 
{
    return s->body->items[n];
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

    YogString* s1 = OBJ_AS(YogString, self);
    YogString* s2 = OBJ_AS(YogString, arg);
    unsigned int size1 = YogString_size(env, s1);
    unsigned int size2 = YogString_size(env, s2);
    unsigned int size = size1 + size2 - 1;
    YogString* s = YogString_new_size(env, size);
    memcpy(s->body->items, s1->body->items, size1);
    memcpy(&s->body->items[size1 - 1], s2->body->items, size2);

    return OBJ2VAL(s);
}

static YogVal 
lshift(YogEnv* env) 
{
    YogVal self = SELF(env);
    YogVal arg = ARG(env, 0);

    YogString* s1 = OBJ_AS(YogString, self);
    YogString* s2 = OBJ_AS(YogString, arg);
    unsigned int size1 = YogString_size(env, s1);
    unsigned int size2 = YogString_size(env, s2);
    unsigned int size = size1 + size2 - 1;
    ensure_size(env, s1, size);
    memcpy(&s1->body->items[size1 - 1], s2->body->items, size2);

    return self;
}

static char* 
index2ptr(YogEnv* env, YogString* s, unsigned int index)
{
    YogEncoding* enc = s->encoding;
    const char* end = &s->body->items[s->body->size - 1];
    char* p = s->body->items;
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
    CHECK_INT(arg, "string index must be integer");

    YogString* retval = YogString_new(env);
    YogString* s = OBJ_AS(YogString, self);
    int index = VAL2INT(arg);
    const char* p = index2ptr(env, s, index);

    unsigned int size = YogEncoding_mbc_size(env, s->encoding, p);
    if ((s->body->size - 1) - (p - s->body->items) < size) {
        YogError_raise_index_error(env, "string index out of range");
    }
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogString_push(env, retval, *p);
        p++;
    }

    return OBJ2VAL(retval);
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

    YogString* t = OBJ_AS(YogString, arg1);
    const char* q = t->body->items;
    unsigned int size = YogEncoding_mbc_size(env, t->encoding, q);
    if (size < size_orig) {
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            *p = *q;
            p++;
            q++;
        }
        memcpy(p, p + (size_orig - size), s->body->size - (s->body->items - p));
    }
    else if (size_orig < size) {
        const char* r = s->body->items;
        ensure_size(env, s, s->body->size - size_orig + size);
        p = s->body->items + (p - r);
        memmove(p + (size - size_orig), p, s->body->size - (p - s->body->items));
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            *p = *q;
            p++;
            q++;
        }
    }
    else {
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
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
    OnigUChar* begin = (OnigUChar*)s->body->items;
    OnigUChar* end = begin + s->body->size;
    OnigRegion* region = onig_region_new();
    int r = onig_search(regexp->onig_regexp, begin, end, begin, end, region, ONIG_OPTION_NONE);
    if (r == ONIG_MISMATCH) {
        return YNIL;
    }

    YogMatch* match = YogMatch_new(env, s, regexp, region);
    YogVal retval = OBJ2VAL(match);

    return retval;
}

YogKlass* 
YogString_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "String", ENV_VM(env)->cObject);
    YogKlass_define_allocator(env, klass, allocate);
    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);
    YogKlass_define_method(env, klass, "+", add, 0, 0, 0, 0, "s", NULL);
    YogKlass_define_method(env, klass, "<<", lshift, 0, 0, 0, 0, "s", NULL);
    YogKlass_define_method(env, klass, "[]", subscript, 0, 0, 0, 0, "n", NULL);
    YogKlass_define_method(env, klass, "[]=", assign_subscript, 0, 0, 0, 0, "n", "s", NULL);
    YogKlass_define_method(env, klass, "=~", match, 0, 0, 0, 1, "regexp", NULL);

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
