#include <string.h>
#include "yog/yog.h"

YogByteArray* 
YogByteArray_new(YogEnv* env, unsigned int size) 
{
    YogByteArray* array = ALLOC_OBJ_ITEM(env, GCOBJ_BYTE_ARRAY, YogByteArray, size, uint8_t);
    array->size = 0;
    array->capacity = size;

    return array;
}

static void 
ensure_capacity(YogEnv* env, YogBinary* binary, unsigned int needed_size) 
{
    YogByteArray* body = binary->body;
    if (body->capacity < needed_size) {
        unsigned int new_size = 2 * needed_size;
        YogByteArray* new_body = YogByteArray_new(env, new_size);
        memcpy(new_body->items, body->items, body->size);
        new_body->size = body->size;
        binary->body = new_body;
    }
}

#define PUSH_TYPE(type, n)  do { \
    ensure_capacity(env, binary, binary->body->size + sizeof(type)); \
\
    YogByteArray* body = binary->body; \
    *((type*)&body->items[body->size]) = n; \
    body->size += sizeof(type); \
} while (0)

void 
YogBinary_push_uint8(YogEnv* env, YogBinary* binary, uint8_t n) 
{
    PUSH_TYPE(uint8_t, n);
}

void 
YogBinary_push_uint32(YogEnv* env, YogBinary* binary, uint32_t n) 
{
    PUSH_TYPE(uint32_t, n);
}

#undef PUSH_TYPE

YogBinary* 
YogBinary_new(YogEnv* env, unsigned int size) 
{
    YogByteArray* body = YogByteArray_new(env, size);
    YogBinary* binary = ALLOC_OBJ(env, GCOBJ_BINARY, YogBinary);
    binary->body = body;

    return binary;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
