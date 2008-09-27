#include "yog/yog.h"

YogVal
YogVal_nil() 
{
    YogVal nil;
    VAL_TYPE(nil) = VAL_NIL;

    return nil;
}

YogVal
YogVal_obj(YogObj* obj)
{
    YogVal val;
    VAL_TYPE(val) = VAL_OBJ;
    VAL_OBJ(val) = obj;

    return val;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
