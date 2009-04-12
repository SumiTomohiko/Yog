#include <stdio.h>
#include <string.h>
#include "yog/binary.h"
#include "yog/error.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

unsigned int 
YogBinary_size(YogEnv* env, YogVal binary) 
{
    return PTR_AS(YogBinary, binary)->size;
}

void 
YogBinary_shrink(YogEnv* env, YogVal binary) 
{
    SAVE_ARG(env, binary);

    unsigned int size = YogBinary_size(env, binary);
    YogVal new_body = YogByteArray_new(env, size);
    uint8_t* to = PTR_AS(YogByteArray, new_body)->items;
    YogVal old_body = PTR_AS(YogBinary, binary)->body;
    uint8_t* from = PTR_AS(YogByteArray, old_body)->items;
    memcpy(to, from, size);
    MODIFY(env, PTR_AS(YogBinary, binary)->body, new_body);

    RETURN_VOID(env);
}

unsigned int 
YogByteArray_size(YogEnv* env, YogVal array) 
{
    return PTR_AS(YogByteArray, array)->size;
}

#if 0
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
#endif

YogVal 
YogByteArray_new(YogEnv* env, unsigned int size) 
{
    YogByteArray* array = ALLOC_OBJ_ITEM(env, NULL, NULL, YogByteArray, size, uint8_t);
    array->size = size;

    return PTR2VAL(array);
}

static void 
ensure_body_size(YogEnv* env, YogVal binary, unsigned int needed_size) 
{
    SAVE_ARG(env, binary);

    YogVal body = YUNDEF;
    PUSH_LOCAL(env, body);

    body = PTR_AS(YogBinary, binary)->body;
    size_t cur_size = PTR_AS(YogBinary, binary)->size;
    if (cur_size < needed_size) {
        unsigned int new_size = 2 * needed_size;
        YogVal new_body = YogByteArray_new(env, new_size);
        unsigned char* to = PTR_AS(YogByteArray, new_body)->items;
        unsigned char* from = PTR_AS(YogByteArray, body)->items;
        memcpy(to, from, cur_size);
        MODIFY(env, PTR_AS(YogBinary, binary)->body, new_body);
    }

    RETURN_VOID(env);
}

#define PUSH_TYPE(type, n)  do { \
    SAVE_ARG(env, binary); \
\
    size_t needed_size = PTR_AS(YogBinary, binary)->size + sizeof(type); \
    ensure_body_size(env, binary, needed_size); \
\
    YogVal body = PTR_AS(YogBinary, binary)->body; \
    unsigned int size = PTR_AS(YogBinary, binary)->size; \
    *((type*)&PTR_AS(YogByteArray, body)->items[size]) = n; \
    PTR_AS(YogBinary, binary)->size += sizeof(type); \
\
    RETURN_VOID(env); \
} while (0)

void 
YogBinary_push_uint8(YogEnv* env, YogVal binary, uint8_t n) 
{
    PUSH_TYPE(uint8_t, n);
}

void 
YogBinary_push_id(YogEnv* env, YogVal binary, ID id) 
{
    PUSH_TYPE(ID, id);
}

void 
YogBinary_push_unsigned_int(YogEnv* env, YogVal binary, unsigned int n) 
{
    PUSH_TYPE(unsigned int, n);
}

void 
YogBinary_push_pc(YogEnv* env, YogVal binary, pc_t pc) 
{
    PUSH_TYPE(pc_t, pc);
}

#undef PUSH_TYPE

static void 
YogBinary_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogBinary* bin = ptr;
    bin->body = YogVal_keep(env, bin->body, keeper);
}

YogVal 
YogBinary_new(YogEnv* env, unsigned int size) 
{
    YogVal binary = PTR2VAL(ALLOC_OBJ(env, YogBinary_keep_children, NULL, YogBinary));
    PTR_AS(YogBinary, binary)->size = 0;
    PTR_AS(YogBinary, binary)->body = YUNDEF;
    PUSH_LOCAL(env, binary);

    YogVal body = YogByteArray_new(env, size);
    MODIFY(env, PTR_AS(YogBinary, binary)->body, body);

    POP_LOCALS(env);

    return binary;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
