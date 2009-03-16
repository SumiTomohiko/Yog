#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "oniguruma.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/regexp.h"
#include "yog/yog.h"

ID 
YogString_intern(YogEnv* env, YogVal s) 
{
    return INTERN(OBJ_AS(YogString, s)->body->items);
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
YogString_size(YogEnv* env, YogVal string) 
{
    YogCharArray* body = OBJ_AS(YogString, string)->body;
    if (body != NULL) {
        return body->size;
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

    if (OBJ_AS(YogString, string)->body == NULL) {
#define CAPACITY    (1)
        YogCharArray* body = YogCharArray_new(env, CAPACITY);
#undef CAPACITY
        OBJ_AS(YogString, string)->body = body;
    }

    RETURN_VOID(env);
}

static void 
ensure_size(YogEnv* env, YogVal string, unsigned int size) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, string);

    YogVal body = PTR2VAL(OBJ_AS(YogString, string)->body);
    PUSH_LOCAL(env, body);
    if ((VAL2PTR(body) == NULL) || (PTR_AS(YogCharArray, body)->capacity < size)) {
        unsigned int capacity = 1;
        if (VAL2PTR(body) != NULL) {
            capacity = PTR_AS(YogCharArray, body)->capacity;
        }
        do {
#define RATIO   (2)
            capacity = RATIO * capacity;
#undef RATIO
        } while (capacity < size);

        YogCharArray* new_body = YogCharArray_new(env, capacity);
        memcpy(new_body->items, PTR_AS(YogCharArray, body)->items, PTR_AS(YogCharArray, body)->size);
        new_body->size = PTR_AS(YogCharArray, body)->size;

        OBJ_AS(YogString, string)->body = new_body;
    }

    RETURN_VOID(env);
}

void 
YogString_push(YogEnv* env, YogVal string, char c) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, string);

    ensure_body(env, string);

    YogCharArray* body = OBJ_AS(YogString, string)->body;
    unsigned int needed_size = body->size + 1;
    if (body->capacity < needed_size) {
        ensure_size(env, string, needed_size);
        body = OBJ_AS(YogString, string)->body;
    }

    body->items[body->size - 1] = c;
    body->items[body->size] = '\0';
    body->size++;

    RETURN_VOID(env);
}

void 
YogString_clear(YogEnv* env, YogVal string) 
{
    if (OBJ_AS(YogString, string)->body != NULL) {
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
    s->encoding = NULL;
    s->body = NULL;

    RETURN(env, OBJ2VAL(obj));
}

YogVal 
YogString_new(YogEnv* env) 
{
    YogVal string = allocate(env, ENV_VM(env)->cString);

    OBJ_AS(YogString, string)->encoding = NULL;
    OBJ_AS(YogString, string)->body = NULL;

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

    YogVal body = PTR2VAL(YogCharArray_new(env, size + 1));
    memcpy(PTR_AS(YogCharArray, body)->items, escaped_from_gc, size);
    PTR_AS(YogCharArray, body)->items[size] = '\0';
    PUSH_LOCAL(env, body);

    YogVal s = YogString_new(env);
    OBJ_AS(YogString, s)->encoding = PTR_AS(YogEncoding, enc);
    OBJ_AS(YogString, s)->body = PTR_AS(YogCharArray, body);

    RETURN(env, s);
}

YogVal 
YogString_new_size(YogEnv* env, unsigned int size) 
{
    SAVE_LOCALS(env);
    YogVal string = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS2(env, string, body);

    string = allocate(env, ENV_VM(env)->cString);
    body = PTR2VAL(YogCharArray_new(env, size));

    OBJ_AS(YogString, string)->encoding = NULL;
    OBJ_AS(YogString, string)->body = PTR_AS(YogCharArray, body);

    RETURN(env, string);
}

#define RETURN_STR(s)   do { \
    size_t len = strlen(s); \
    char buffer[len + 1]; \
    strcpy(buffer, s); \
    YogVal body = PTR2VAL(YogCharArray_new_str(env, buffer)); \
    PUSH_LOCAL(env, body); \
    \
    YogVal string = allocate(env, ENV_VM(env)->cString); \
    OBJ_AS(YogString, string)->encoding = NULL; \
    OBJ_AS(YogString, string)->body = PTR_AS(YogCharArray, body); \
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

    YogVal s = YogString_new_str(env, OBJ_AS(YogString, string)->body->items);
    OBJ_AS(YogString, s)->encoding = OBJ_AS(YogString, string)->encoding;

    RETURN(env, s);
}

char 
YogString_at(YogEnv* env, YogVal s, unsigned int n) 
{
    return OBJ_AS(YogString, s)->body->items[n];
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
    memcpy(OBJ_AS(YogString, s)->body->items, OBJ_AS(YogString, self)->body->items, size1);
    memcpy(&OBJ_AS(YogString, s)->body->items[size1 - 1], OBJ_AS(YogString, arg)->body->items, size2);

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
    memcpy(&OBJ_AS(YogString, self)->body->items[size1 - 1], OBJ_AS(YogString, arg)->body->items, size2);

    POP_LOCALS(env);
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
    YogVal retval = YUNDEF;
    PUSH_LOCALS3(env, self, arg, retval);
    CHECK_INT(arg, "string index must be integer");

    retval = YogString_new(env);
    YogString* s = OBJ_AS(YogString, self);
    int index = VAL2INT(arg);
    const char* p = index2ptr(env, s, index);
    unsigned int offset = p - s->body->items;

    unsigned int size = YogEncoding_mbc_size(env, s->encoding, p);
    if ((s->body->size - 1) - (p - s->body->items) < size) {
        YogError_raise_index_error(env, "string index out of range");
    }
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        p = OBJ_AS(YogString, self)->body->items + offset + i;
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
        ensure_size(env, OBJ2VAL(s), s->body->size - size_orig + size);
        /* FIXME: dirty hack */
        s = OBJ_AS(YogString, SELF(env));
        t = OBJ_AS(YogString, ARG(env, 1));
        q = t->body->items;

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
        YogCharArray* body = s->body;
        const char* start = body->items + i;
        const char* p = memchr(start, '\n', body->size - i - 1);
        if (p == NULL) {
            p = body->items + body->size - 1;
        }
        YogEncoding* enc = s->encoding;
        p = YogEncoding_left_adjust_char_head(env, enc, body->items, p);
        const char* end = p - 1;
        const char* next = p + YogEncoding_mbc_size(env, enc, p);
        i = next - OBJ_AS(YogString, SELF(env))->body->items;
        YogVal line = YogString_new_range(env, PTR2VAL(enc), start, end);
        YogVal block = ARG(env, 0);
        YogVal args[] = { line, };

        unsigned int size = OBJ_AS(YogString, SELF(env))->body->size;

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
        YogCharArray* body = s->body;
        unsigned char p = body->items[i];
        YogVal block = ARG(env, 0);
        YogVal args[] = { INT2VAL(p), };

        i++;
        unsigned int size = body->size;

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
        YogCharArray* body = s->body;
        const char* start = body->items + i;
        YogEncoding* enc = s->encoding;
        const char* next = start + YogEncoding_mbc_size(env, enc, start);
        i = next - OBJ_AS(YogString, SELF(env))->body->items;
        const char* end = next - 1;
        YogVal c = YogString_new_range(env, PTR2VAL(enc), start, end);
        YogVal block = ARG(env, 0);
        YogVal args[] = { c, };

        unsigned int size = OBJ_AS(YogString, SELF(env))->body->size;

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
    YogVal klass = YogKlass_new(env, "String", ENV_VM(env)->cObject);
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
