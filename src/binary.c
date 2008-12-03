#include <stdio.h>
#include <string.h>
#include "yog/binary.h"
#include "yog/yog.h"

unsigned int 
YogBinary_size(YogEnv* env, YogBinary* binary) 
{
    return binary->size;
}

void 
YogBinary_shrink(YogEnv* env, YogBinary* binary) 
{
    unsigned int size = YogBinary_size(env, binary);
    YogByteArray* body = YogByteArray_new(env, size);
    memcpy(body->items, binary->body->items, size);
    binary->body = body;
}

unsigned int 
YogByteArray_size(YogEnv* env, YogByteArray* array) 
{
    return array->size;
}

uint8_t 
YogByteArray_at(YogEnv* env, YogByteArray* array, unsigned int n) 
{
    YOG_ASSERT(env, n < YogByteArray_size(env, array), "");
    return array->items[n];
}

void 
YogByteArray_print(YogEnv* env, YogByteArray* array) 
{
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
    YogByteArray* array = ALLOC_OBJ_ITEM(env, NULL, YogByteArray, size, uint8_t);
    array->size = size;

    return array;
}

static void 
ensure_body_size(YogEnv* env, YogBinary* binary, unsigned int needed_size) 
{
    YogByteArray* body = binary->body;
    if (body->size < needed_size) {
        unsigned int new_size = 2 * needed_size;
        YogByteArray* new_body = YogByteArray_new(env, new_size);
        memcpy(new_body->items, body->items, binary->size);
        binary->body = new_body;
    }
}

#define PUSH_TYPE(type, n)  do { \
    ensure_body_size(env, binary, binary->size + sizeof(type)); \
\
    YogByteArray* body = binary->body; \
    *((type*)&body->items[binary->size]) = n; \
    binary->size += sizeof(type); \
} while (0)

void 
YogBinary_push_uint8(YogEnv* env, YogBinary* binary, uint8_t n) 
{
    PUSH_TYPE(uint8_t, n);
}

void 
YogBinary_push_id(YogEnv* env, YogBinary* binary, ID id) 
{
    PUSH_TYPE(ID, id);
}

void 
YogBinary_push_unsigned_int(YogEnv* env, YogBinary* binary, unsigned int n) 
{
    PUSH_TYPE(unsigned int, n);
}

void 
YogBinary_push_pc(YogEnv* env, YogBinary* binary, pc_t pc) 
{
    PUSH_TYPE(pc_t, pc);
}

#undef PUSH_TYPE

static void 
YogBinary_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogBinary* bin = ptr;
    bin->body = (*keeper)(env, bin->body);
}

YogBinary* 
YogBinary_new(YogEnv* env, unsigned int size) 
{
    YogBinary* binary = ALLOC_OBJ(env, YogBinary_keep_children, YogBinary);
    binary->size = 0;
    binary->body = YogByteArray_new(env, size);

    return binary;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
