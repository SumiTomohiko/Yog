#include "yog/yog.h"

YogByteArray* 
YogByteArray_new(YogEnv* env, unsigned int size) 
{
    YogByteArray* array = ALLOC_OBJ_ITEM(env, GCOBJ_BYTE_ARRAY, YogByteArray, size, unsigned char);
    array->size = 0;
    array->capacity = size;

    return array;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
