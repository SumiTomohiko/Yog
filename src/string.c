#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "yog/yog.h"

YogCharArray* 
YogCharArray_new(YogEnv* env, unsigned int capacity) 
{
    YogCharArray* array = ALLOC_OBJ_ITEM(env, NULL, YogCharArray, capacity, char);
    array->capacity = capacity;
    array->size = 0;

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

#define RETURN_STR(s)   do { \
    YogCharArray* body = YogCharArray_new_str(env, s); \
    YogString* string = ALLOC_OBJ(env, gc_string_children, YogString); \
    YogBasicObj_init(env, YOGBASICOBJ(string), ENV_VM(env)->string_klass); \
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

YogKlass* 
YogString_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "String", ENV_VM(env)->obj_klass);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
