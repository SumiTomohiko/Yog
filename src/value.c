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
    case VAL_PTR:
        printf("<object: %p>\n", YOGVAL_PTR(val));
        break;
    case VAL_BOOL:
        if (YOGVAL_BOOL(val)) {
            printf("<bool: true>\n");
        }
        else {
            printf("<bool: false>\n");
        }
        break;
    case VAL_NIL:
        printf("<nil>\n");
        break;
    case VAL_SYMBOL:
        printf("<symbol: %d>\n", YOGVAL_SYMBOL(val));
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
    case VAL_OBJ:
        return (int)YOGVAL_OBJ(val);
        break;
    case VAL_PTR:
        return (int)YOGVAL_PTR(val);
        break;
    case VAL_BOOL:
        if (YOGVAL_BOOL(val)) {
            return 1;
        }
        else {
            return 0;
        }
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
    return 0;
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
    case VAL_PTR:
        RETURN(YOGVAL_PTR, a, b);
        break;
    case VAL_BOOL:
        if (YOGVAL_BOOL(a)) {
            if (YOGVAL_BOOL(b)) {
                return TRUE;
            }
            else {
                return FALSE;
            }
        }
        else {
            if (YOGVAL_BOOL(b)) {
                return FALSE;
            }
            else {
                return TRUE;
            }
        }
    case VAL_NIL:
        return TRUE;
        break;
    default:
        Yog_assert(env, FALSE, "Uknown value type.");
        break;
    }
#undef RETURN

    /* NOTREACHED */
    return FALSE;
}

#define RETURN_BOOL(b)  do { \
    YogVal val; \
    val.type = VAL_BOOL; \
    YOGVAL_BOOL(val) = b; \
    return val; \
} while (0)

YogVal 
YogVal_true()
{
    RETURN_BOOL(TRUE);
}

YogVal 
YogVal_false()
{
    RETURN_BOOL(FALSE);
}

#undef RETURN_BOOL

#define RETURN_VAL(type)    do { \
    YogVal val; \
    YOGVAL_TYPE(val) = type; \
    return val; \
} while (0)

YogVal
YogVal_nil() 
{
    RETURN_VAL(VAL_NIL);
}

YogVal 
YogVal_undef() 
{
    RETURN_VAL(VAL_UNDEF);
}

#undef RETURN_VAL

#define RETURN_VAL(type, f, v)  do { \
    YogVal val; \
    YOGVAL_TYPE(val) = type; \
    f(val) = v; \
    return val; \
} while (0)

YogVal 
YogVal_obj(YogBasicObj* obj) 
{
    RETURN_VAL(VAL_OBJ, YOGVAL_OBJ, obj);
}

YogVal
YogVal_ptr(void * ptr)
{
    RETURN_VAL(VAL_PTR, YOGVAL_PTR, ptr);
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

YogKlass* 
YogVal_get_klass(YogEnv* env, YogVal val) 
{
    switch (YOGVAL_TYPE(val)) {
    case VAL_INT:
        return ENV_VM(env)->int_klass;
        break;
    case VAL_OBJ:
        {
            YogBasicObj* obj = YOGVAL_OBJ(val);
            return obj->klass;
            break;
        }
    case VAL_BOOL:
        return ENV_VM(env)->bool_klass;
        break;
    case VAL_FLOAT:
    case VAL_NIL:
    case VAL_SYMBOL:
    case VAL_PTR:
    default:
        Yog_assert(env, FALSE, "Uknown value type.");
        break;
    }

    /* NOTREACHED */
    return NULL;
}

#undef RETURN_VAL

YogVal 
YogVal_get_attr(YogEnv* env, YogVal val, ID name) 
{
#define RET_ATTR(obj)   do { \
    YogVal attr = YogObj_get_attr(env, YOGOBJ(obj), name); \
    if (!IS_UNDEF(attr)) { \
        return attr; \
    } \
} while (0)
    if (IS_OBJ(val)) {
        YogBasicObj* obj = YOGVAL_OBJ(val);
        YogVm* vm = ENV_VM(env);
        if ((obj->klass == vm->obj_klass) || (obj->klass == vm->klass_klass)) {
            RET_ATTR(obj);
        }
        else {
            /* TODO: generic attribute */
            Yog_assert(env, FALSE, "Not implemented.");
        }
    }

    YogKlass* klass = YogVal_get_klass(env, val);
    RET_ATTR(klass);
#undef RET_ATTR

    Yog_assert(env, FALSE, "Can't get attribute.");

    /* NOTREACHED */
    return YogVal_nil();
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
