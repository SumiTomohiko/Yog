#include <stdio.h>
#include "yog/yog.h"

void 
YogVal_print(YogEnv* env, YogVal val) 
{
    switch (VAL_TYPE(val)) {
    case VAL_UNDEF:
        printf("<undef>\n");
        break;
    case VAL_INT:
        printf("<int: %d>\n", VAL2INT(val));
        break;
    case VAL_FLOAT:
        printf("<float: %f>\n", VAL2FLOAT(val));
        break;
    case VAL_PTR:
        printf("<ptr: %p>\n", VAL2PTR(val));
        break;
    case VAL_OBJ:
        printf("<object: %p>\n", VAL2OBJ(val));
        break;
    case VAL_BOOL:
        if (VAL2BOOL(val)) {
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
        printf("<symbol: %d>\n", VAL2ID(val));
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
    switch (VAL_TYPE(val)) {
    case VAL_INT:
        return VAL2INT(val);
        break;
    case VAL_FLOAT:
        return VAL2FLOAT(val);
        break;
    case VAL_OBJ:
        return (int)VAL2OBJ(val);
        break;
    case VAL_PTR:
        return (int)VAL2PTR(val);
        break;
    case VAL_BOOL:
        if (VAL2BOOL(val)) {
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
        return VAL2ID(val);
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
    if (VAL_TYPE(a) != VAL_TYPE(b)) {
        return FALSE;
    }

#define RETURN(f, a, b) do {            \
    return f(a) == f(b) ? TRUE : FALSE; \
} while (0)
    switch (VAL_TYPE(a)) {
    case VAL_INT:
        RETURN(VAL2INT, a, b);
        break;
    case VAL_FLOAT:
        RETURN(VAL2FLOAT, a, b);
        break;
    case VAL_SYMBOL:
        RETURN(VAL2ID, a, b);
        break;
    case VAL_PTR:
        RETURN(VAL2PTR, a, b);
        break;
    case VAL_BOOL:
        if (VAL2BOOL(a)) {
            if (VAL2BOOL(b)) {
                return TRUE;
            }
            else {
                return FALSE;
            }
        }
        else {
            if (VAL2BOOL(b)) {
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
    VAL2BOOL(val) = b; \
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
    VAL_TYPE(val) = type; \
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
    VAL_TYPE(val) = type; \
    f(val) = v; \
    return val; \
} while (0)

YogVal 
YogVal_obj(YogBasicObj* obj) 
{
    RETURN_VAL(VAL_OBJ, VAL2OBJ, obj);
}

YogVal
YogVal_ptr(void * ptr)
{
    RETURN_VAL(VAL_PTR, VAL2PTR, ptr);
}

YogVal
YogVal_int(int n)
{
    RETURN_VAL(VAL_INT, VAL2INT, n);
}

YogVal 
YogVal_float(float f) 
{
    RETURN_VAL(VAL_FLOAT, VAL2FLOAT, f);
}

YogVal
YogVal_symbol(ID id) 
{
    RETURN_VAL(VAL_SYMBOL, VAL2ID, id);
}

YogKlass* 
YogVal_get_klass(YogEnv* env, YogVal val) 
{
    switch (VAL_TYPE(val)) {
    case VAL_INT:
        return ENV_VM(env)->int_klass;
        break;
    case VAL_OBJ:
        {
            YogBasicObj* obj = VAL2OBJ(val);
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
        YogBasicObj* obj = VAL2OBJ(val);
        if (obj->flags & HAS_ATTRS) {
            RET_ATTR(obj);
        }
        else {
            /* TODO: generic attribute */
            Yog_assert(env, FALSE, "Not implemented.");
        }
    }

    YogKlass* klass = YogVal_get_klass(env, val);
    do {
        RET_ATTR(klass);
        klass = klass->super;
    } while (klass != NULL);
#undef RET_ATTR

    Yog_assert(env, FALSE, "Can't get attribute.");

    /* NOTREACHED */
    return YogVal_nil();
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
