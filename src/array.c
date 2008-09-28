#include "yog/yog.h"

YogArray* 
YogArray_new(YogEnv* env)
{
#define INIT_SIZE   (1)
    YogValArray* body = ALLOC_OBJ_SIZE(env, OBJ_VAL_ARRAY, YogValArray, sizeof(YogValArray) + sizeof(YogVal) * INIT_SIZE);
    body->size = INIT_SIZE;
#undef INIT_SIZE

    YogArray* array = ALLOC_OBJ(env, OBJ_ARRAY, YogArray);
    array->size = 0;
    array->body = body;

    return array;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
