#include <stdio.h>
#include <string.h>
#include "yog/yog.h"

unsigned int 
YogByteArray_size(YogEnv* env, YogByteArray* array) 
{
    return array->size;
}

uint8_t 
YogByteArray_at(YogEnv* env, YogByteArray* array, unsigned int n) 
{
    Yog_assert(env, n < YogByteArray_size(env, array), "");
    return array->items[n];
}

void 
YogByteArray_print(YogEnv* env, YogByteArray* array) 
{
    printf("%s:%d array->items=%p\n", __FILE__, __LINE__, array->items);
    printf("   |");
    unsigned int i = 0;
#define MAX (16)
    for (i = 0; i < MAX; i++) {
        printf(" %02X", i);
    }

    printf("\n---+");
    for (i = 0; i < MAX; i++) {
        printf("---");
    }

    for (i = 0; i < YogByteArray_size(env, array); i++) {
        if (i % MAX == 0) {
            printf("\n%02X |", (i / MAX) * 10);
        }

        printf(" %02X", YogByteArray_at(env, array, i));
    }
#undef MAX
    printf("\n");
}

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
        printf("%s:%d body: \n", __FILE__, __LINE__);
        YogByteArray_print(env, body);
        unsigned int new_size = 2 * needed_size;
        printf("%s:%d body->items[0]=%02x\n", __FILE__, __LINE__, body->items[0]);
        YogByteArray* new_body = YogByteArray_new(env, new_size);
        printf("%s:%d body->items[0]=%02x\n", __FILE__, __LINE__, body->items[0]);
#if 0
        printf("%s:%d &body->items[0]=%p\n", __FILE__, __LINE__, &body->items[0]);
        printf("%s:%d body->items=%p\n", __FILE__, __LINE__, body->items);
#endif
        unsigned int i = 0;
        printf("%s:%d body->size=%d\n", __FILE__, __LINE__, body->size);
        printf("%s:%d body->items=%p\n", __FILE__, __LINE__, body->items);
        printf("%s:%d new_body->items=%p\n", __FILE__, __LINE__, new_body->items);
        for (i = 0; i < body->size; i++) {
            new_body->items[i] = YogByteArray_at(env, body, i);
            printf("%s:%d body->items[%d]=%02x\n", __FILE__, __LINE__, i, body->items[i]);
            printf("%s:%d new_body->items[%d]=%02x\n", __FILE__, __LINE__, i, new_body->items[i]);
        }
#if 0
        memcpy(new_body->items, body->items, body->size);
        printf("%s:%d new_body=%p\n", __FILE__, __LINE__, new_body);
        printf("%s:%d sizeof(struct YogByteArray)=%d\n", __FILE__, __LINE__, sizeof(struct YogByteArray));
        printf("%s:%d new_body->items=%p, body->items=%p, body->size=%d\n", __FILE__, __LINE__, new_body->items, body->items, body->size);
#endif
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
    printf("%s:%d n=%d\n", __FILE__, __LINE__, n);
    PUSH_TYPE(uint8_t, n);
    YogByteArray_print(env, binary->body);
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
