#include "yog/yog.h"

int 
YogVal_hash(YogEnv* env, YogVal val) 
{
    switch (YOGVAL_TYPE(val)) {
    case VAL_INT:
        return YOGVAL_INT(val);
        break;
    case VAL_FLOAT:
        return YOGVAL_INT(val);
        break;
    case VAL_GCOBJ:
        return YOGVAL_INT(val);
        break;
    case VAL_TRUE:
        return 1;
        break;
    case VAL_FALSE:
        return 0;
        break;
    case VAL_NIL:
        return 2;
        break;
    case VAL_SYMBOL:
        return YOGVAL_SYMBOL(val);
        break;
    default:
        Yog_assert(env, FALSE, "Uknown value type.");
        break;
    }
    /* NOTREACHED */
}

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
        RETURN(YOGVAL_INT, a, b);
        break;
    case VAL_FLOAT:
        RETURN(YOGVAL_FLOAT, a, b);
        break;
    case VAL_SYMBOL:
        RETURN(YOGVAL_SYMBOL, a, b);
        break;
    case VAL_GCOBJ:
        RETURN(YOGVAL_GCOBJ, a, b);
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

#define RETURN_VAL(type, f, v)  do { \
    YogVal val; \
    YOGVAL_TYPE(val) = type; \
    f(val) = v; \
    return val; \
} while (0)

YogVal
YogVal_gcobj(YogGCObj* obj)
{
    RETURN_VAL(VAL_GCOBJ, YOGVAL_GCOBJ, obj);
}

YogVal
YogVal_int(int n)
{
    RETURN_VAL(VAL_INT, YOGVAL_INT, n);
}

YogVal
YogVal_symbol(ID id) 
{
    RETURN_VAL(VAL_SYMBOL, YOGVAL_SYMBOL, id);
}

YogVal 
YogVal_func(YogFuncBody func) 
{
    RETURN_VAL(VAL_FUNC, YOGVAL_FUNC, func);
}

#undef RETURN_VAL

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
