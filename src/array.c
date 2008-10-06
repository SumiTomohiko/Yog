#include <string.h>
#include "yog/yog.h"

void 
YogValArray_push(YogEnv* env, YogValArray* array, YogVal val) 
{
    Yog_assert(env, array->size + 1 <= array->capacity, "Can't push.");
    array->items[array->size] = val;
    array->size++;
}

YogVal 
YogValArray_pop(YogEnv* env, YogValArray* array) 
{
    Yog_assert(env, 0 < array->size, "Can't pop.");
    YogVal val = array->items[array->size - 1];
    array->size--;
    return val;
}

YogVal 
YogArray_at(YogEnv* env, YogArray* array, unsigned int n) 
{
    Yog_assert(env, n < array->body->size + 1, "Index exceed array size.");
    return array->body->items[n];
}

unsigned int 
YogValArray_size(YogEnv* env, YogValArray* array) 
{
    return array->size;
}

unsigned int 
YogArray_size(YogEnv* env, YogArray* array) 
{
    return YogValArray_size(env, array->body);
}

YogValArray* 
YogValArray_new(YogEnv* env, unsigned int capacity) 
{
    YogValArray* array = ALLOC_OBJ_ITEM(env, GCOBJ_VAL_ARRAY, YogValArray, capacity, YogVal);
    array->size = 0;
    array->capacity = capacity;

    return array;
}

void 
YogArray_push(YogEnv* env, YogArray* array, YogVal val) 
{
    if (array->body->capacity < array->body->size + 1) {
        YogValArray* old_body = array->body;
#define INCREASE_RATIO  (2)
        unsigned int new_size = INCREASE_RATIO * old_body->size;
#undef INCREASE_RATIO
        YogValArray* new_body = YogValArray_new(env, new_size);
        memcpy(new_body->items, old_body->items, sizeof(YogVal) * old_body->size);

        array->body = new_body;
    }

    YogValArray_push(env, array->body, val);
}

YogArray* 
YogArray_new(YogEnv* env)
{
#define INIT_SIZE   (1)
    YogValArray* body = YogValArray_new(env, INIT_SIZE);
#undef INIT_SIZE

    YogArray* array = ALLOC_OBJ(env, GCOBJ_ARRAY, YogArray);
    array->body = body;

    return array;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
