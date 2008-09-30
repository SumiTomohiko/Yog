#include <string.h>
#include "yog/yog.h"

YogCharArray* 
YogCharArray_new(YogEnv* env, unsigned int size) 
{
    YogCharArray* array = ALLOC_OBJ_ITEM(env, GCOBJ_CHAR_ARRAY, YogCharArray, size, char);

    return array;
}

YogCharArray* 
YogCharArray_new_str(YogEnv* env, const char* s) 
{
    size_t size = strlen(s) + 1;
    YogCharArray* array = YogCharArray_new(env, size);
    memcpy(array->items, s, size);

    return array;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
