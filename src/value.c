#include <stdio.h>
#include "yog/yog.h"

void 
YogVal_print(YogEnv* env, YogVal val) 
{
    switch (YOGVAL_TYPE(val)) {
    case VAL_INT:
        printf("<int: %d>\n", YOGVAL_INT(val));
        break;
    case VAL_FLOAT:
        printf("<float: %f>\n", YOGVAL_FLOAT(val));
        break;
    case VAL_GCOBJ:
        printf("<object: %p>\n", YOGVAL_GCOBJ(val));
        break;
    case VAL_TRUE:
        printf("<bool: true>\n");
        break;
    case VAL_FALSE:
        printf("<bool: false>\n");
        break;
    case VAL_NIL:
        printf("<nil>\n");
        break;
    case VAL_SYMBOL:
        printf("<symbol: %d>\n", YOGVAL_SYMBOL(val));
        break;
    case VAL_FUNC:
        printf("<function: %p>\n", YOGVAL_FUNC(val));
        break;
    default:
        Yog_assert(env, FALSE, "Uknown value type.");
        break;
    }
    /* NOTREACHED */
}
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

YogKlass* 
YogVal_get_klass(YogEnv* env, YogVal val) 
{
    switch (YOGVAL_TYPE(val)) {
    case VAL_INT:
        return ENV_VM(env)->int_klass;
        break;
    case VAL_FLOAT:
    case VAL_GCOBJ:
    case VAL_TRUE:
    case VAL_FALSE:
    case VAL_NIL:
    case VAL_SYMBOL:
    default:
        Yog_assert(env, FALSE, "Uknown value type.");
        break;
    }
    /* NOTREACHED */
}

#undef RETURN_VAL

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
