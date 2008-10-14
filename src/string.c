#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "yog/yog.h"

YogCharArray* 
YogCharArray_new(YogEnv* env, unsigned int capacity) 
{
    YogCharArray* array = ALLOC_OBJ_ITEM(env, GCOBJ_CHAR_ARRAY, YogCharArray, capacity, char);
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

#define RETURN_STR(s)   do { \
    YogCharArray* body = YogCharArray_new_str(env, s); \
    YogString* string = ALLOC_OBJ(env, GCOBJ_STRING, YogString); \
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

#if 0
YogString* 
YogString_new(YogEnv* env) 
{
#define INIT_CAPA   (0)
    YogCharArray* body = YogCharArray_new(env, INIT_CAPA);
#undef INIT_CAPA
    YogString* string = ALLOC_OBJ(env, GCOBJ_STRING, YogString);
    string->body = body;

    return string;
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
