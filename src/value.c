#include <stdio.h>
#include "yog/error.h"
#include "yog/klass.h"
#include "yog/vm.h"
#include "yog/yog.h"

void 
YogVal_print(YogEnv* env, YogVal val) 
{
    if (IS_UNDEF(val)) {
        printf("<undef>\n");
    }
    else if (IS_INT(val)) {
        printf("<int: %d>\n", VAL2INT(val));
    }
    else if (IS_PTR(val)) {
        printf("<ptr: %p>\n", VAL2PTR(val));
    }
    else if (IS_BOOL(val)) {
        if (VAL2BOOL(val)) {
            printf("<bool: true>\n");
        }
        else {
            printf("<bool: false>\n");
        }
    }
    else if (IS_NIL(val)) {
        printf("<nil>\n");
    }
    else if (IS_SYMBOL(val)) {
        printf("<symbol: %d>\n", VAL2ID(val));
    }
    else {
        YOG_BUG(env, "uknown value type (0x%08x)", val);
    }
}

int 
YogVal_hash(YogEnv* env, YogVal val) 
{
    if (IS_INT(val)) {
        return VAL2INT(val);
    }
    else if (IS_PTR(val)) {
        return (int)VAL2PTR(val);
    }
    else if (IS_BOOL(val)) {
        if (VAL2BOOL(val)) {
            return 1;
        }
        else {
            return 0;
        }
    }
    else if (IS_NIL(val)) {
        return 2;
    }
    else if (IS_SYMBOL(val)) {
        return VAL2ID(val);
    }
    else {
        YOG_ASSERT(env, FALSE, "Uknown value type.");
    }

    /* NOTREACHED */
    return 0;
}

BOOL
YogVal_equals_exact(YogEnv* env, YogVal a, YogVal b) 
{
#define RET(f, a, b) do {            \
    return f(a) == f(b) ? TRUE : FALSE; \
} while (0)
    if (IS_INT(a)) {
        if (!IS_INT(b)) {
            return FALSE;
        }
        RET(VAL2INT, a, b);
    }
    else if (IS_SYMBOL(a)) {
        if (!IS_SYMBOL(b)) {
            return FALSE;
        }
        RET(VAL2ID, a, b);
    }
    else if (IS_PTR(a)) {
        if (!IS_PTR(b)) {
            return FALSE;
        }
        RET(VAL2PTR, a, b);
    }
    else if (IS_BOOL(a)) {
        if (!IS_BOOL(b)) {
            return FALSE;
        }
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
    }
    else if (IS_NIL(a)) {
        if (!IS_NIL(b)) {
            return FALSE;
        }
        return TRUE;
    }
    else {
        YOG_ASSERT(env, FALSE, "Uknown value type.");
    }
#undef RET

    /* NOTREACHED */
    return FALSE;
}

YogVal 
YogVal_get_klass(YogEnv* env, YogVal val) 
{
    if (IS_INT(val)) {
        return env->vm->cInt;
    }
    else if (IS_PTR(val)) {
        return PTR_AS(YogBasicObj, val)->klass;
    }
    else if (IS_BOOL(val)) {
        return env->vm->cBool;
    }
    else if (IS_NIL(val)) {
        return env->vm->cNil;
    }
    else {
        YOG_BUG(env, "Uknown type of value (0x%08x)", val);
    }

    /* NOTREACHED */
    return YUNDEF;
}

YogVal 
YogVal_get_attr(YogEnv* env, YogVal val, ID name) 
{
#define RET_ATTR(obj)   do { \
    YogVal attr = YogObj_get_attr(env, obj, name); \
    if (!IS_UNDEF(attr)) { \
        return attr; \
    } \
} while (0)
    if (IS_PTR(val)) {
        if (PTR_AS(YogBasicObj, val)->flags & HAS_ATTRS) {
            RET_ATTR(val);
        }
        else {
            /* TODO: generic attribute */
        }
    }

    YogVal klass = YogVal_get_klass(env, val);
    do {
        RET_ATTR(klass);
        klass = PTR_AS(YogKlass, klass)->super;
    } while (IS_PTR(klass));
#undef RET_ATTR

    YOG_ASSERT(env, FALSE, "Can't get attribute.");

    /* NOTREACHED */
    return YNIL;
}

BOOL 
YogVal_is_subklass_of(YogEnv* env, YogVal val, YogVal klass) 
{
    YogVal valklass = YogVal_get_klass(env, val);
    while (!IS_NIL(valklass)) {
        if (PTR_AS(YogKlass, valklass) == PTR_AS(YogKlass, klass)) {
            return TRUE;
        }
        valklass = PTR_AS(YogKlass, valklass)->super;
    }

    return FALSE;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
