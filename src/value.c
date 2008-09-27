#include "yog/yog.h"

BOOL
YogVal_equals_exact(YogEnv* env, YogVal a, YogVal b) 
{
    if (YOGVAL_TYPE(a) != YOGVAL_TYPE(b)) {
        return FALSE;
    }

#define RETURN(f, a, b) do {            \
    return f(a) == f(b) ? TRUE : FALSE; \
} while (0)
    switch (YOGVAL_TYPE(a)) {
        case VAL_INT:
            RETURN(YOGVAL_NUM, a, b);
            break;
        case VAL_FLOAT:
            RETURN(YOGVAL_FLOAT, a, b);
            break;
        case VAL_SYMBOL:
            RETURN(YOGVAL_SYMBOL, a, b);
            break;
        case VAL_OBJ:
            RETURN(YOGVAL_OBJ, a, b);
            break;
        case VAL_TRUE:
        case VAL_FALSE:
        case VAL_NIL:
            return TRUE;
            break;
        default:
            Yog_assert(env, FALSE, "Uknown value type.");
            break;
    }
#undef RETURN
    /* NOTREACHED */
}

YogVal
YogVal_nil() 
{
    YogVal nil;
    YOGVAL_TYPE(nil) = VAL_NIL;

    return nil;
}

YogVal
YogVal_obj(YogObj* obj)
{
    YogVal val;
    YOGVAL_TYPE(val) = VAL_OBJ;
    YOGVAL_OBJ(val) = obj;

    return val;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
