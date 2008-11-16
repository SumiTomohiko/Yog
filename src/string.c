#include <stdarg.h>
#include <stdio.h>
#include <string.h>
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
    YogCharArray* array = ALLOC_OBJ_ITEM(env, NULL, YogCharArray, capacity, char);
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
gc_string_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogString* s = ptr;
    s->body = do_gc(env, s->body);
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
grow_body(YogEnv* env, YogString* string) 
{
    YogCharArray* old_body = string->body;
#define RATIO   (2)
    YogCharArray* new_body = YogCharArray_new(env, RATIO * old_body->capacity);
#undef RATIO
    memcpy(new_body->items, old_body->items, old_body->size);
    new_body->size = old_body->size;

    string->body = new_body;
}

void 
YogString_push(YogEnv* env, YogString* string, char c) 
{
    ensure_body(env, string);

    YogCharArray* body = string->body;
    if (body->capacity < body->size + 1) {
        grow_body(env, string);
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
    YogBasicObj* obj = ALLOC_OBJ(env, gc_string_children, YogString);
    YogBasicObj_init(env, obj, 0, klass);

    return obj;
}

YogString* 
YogString_new(YogEnv* env) 
{
    YogString* string = (YogString*)allocate(env, ENV_VM(env)->string_klass);
    string->body = NULL;
    return string;
}

#define RETURN_STR(s)   do { \
    YogCharArray* body = YogCharArray_new_str(env, s); \
    YogString* string = (YogString*)allocate(env, ENV_VM(env)->string_klass); \
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
YogString_clone(YogEnv* env, YogString* s) 
{
    return YogString_new_str(env, s->body->items);
}

char 
YogString_at(YogEnv* env, YogString* s, unsigned int n) 
{
    return s->body->items[n];
}

YogKlass* 
YogString_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, allocate, "String", ENV_VM(env)->obj_klass);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
