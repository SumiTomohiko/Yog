#include <string.h>
#include "yog/yog.h"

YogValArray* 
YogValArray_new(YogEnv* env, unsigned int size) 
{
    YogValArray* array = ALLOC_OBJ_ITEM(env, GCOBJ_VAL_ARRAY, YogValArray, size, YogVal);
    array->size = size;

    return array;
}

void 
YogArray_push(YogEnv* env, YogArray* array, YogVal val) 
{
    if (array->body->size < array->size + 1) {
        YogValArray* old_body = array->body;

#define INCREASE_RATIO  (2)
        unsigned int new_size = INCREASE_RATIO * old_body->size;
#undef INCREASE_RATIO
        YogValArray* new_body = YogValArray_new(env, new_size);
        memcpy(new_body->items, old_body->items, sizeof(YogVal) * old_body->size);

        array->body = new_body;
    }

    array->body->items[array->size] = val;
    array->size++;
}

YogArray* 
YogArray_new(YogEnv* env)
{
#define INIT_SIZE   (1)
    YogValArray* body = YogValArray_new(env, INIT_SIZE);
#undef INIT_SIZE

    YogArray* array = ALLOC_OBJ(env, GCOBJ_ARRAY, YogArray);
    array->size = 0;
    array->body = body;

    return array;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
