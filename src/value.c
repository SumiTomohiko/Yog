#include <stdio.h>
#include "yog/error.h"
#include "yog/klass.h"
#include "yog/thread.h"
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

static YogVal
get_descr(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal c = YUNDEF;
    YogVal v = YUNDEF;
    PUSH_LOCALS2(env, c, v);

    c = YogVal_get_klass(env, attr);
    if (PTR_AS(YogKlass, c)->get_descr == NULL) {
        RETURN(env, attr);
    }
    v = (*PTR_AS(YogKlass, c)->get_descr)(env, attr, obj, klass);

    RETURN(env, v);
}

static YogVal
get_attr_default(YogEnv* env, YogVal self, ID name)
{
    SAVE_ARG(env, self);
    YogVal klass = YUNDEF;
    YogVal attr = YUNDEF;
    PUSH_LOCALS2(env, klass, attr);

#if 0
    /* TODO: test here */
    if (IS_PTR(self) && (PTR_AS(YogBasicObj, self)->flags & HAS_ATTRS)) {
        attr = YogObj_get_attr(env, self, name);
        if (!IS_UNDEF(attr)) {
            RETURN(env, attr);
        }
    }
#endif

    klass = YogVal_get_klass(env, self);
    attr = YogKlass_get_attr(env, klass, name);
    if (!IS_UNDEF(attr)) {
        attr = get_descr(env, attr, self, klass);
        RETURN(env, attr);
    }

    RETURN(env, YUNDEF);
}

YogVal 
YogVal_get_attr(YogEnv* env, YogVal val, ID name) 
{
    SAVE_ARG(env, val);
    YogVal klass = YUNDEF;
    YogVal attr = YUNDEF;
    PUSH_LOCALS2(env, klass, attr);

    klass = YogVal_get_klass(env, val);
    AttrGetter getter = PTR_AS(YogKlass, klass)->get_attr;
    if (getter == NULL) {
        getter = get_attr_default;
    }
    attr = (*getter)(env, val, name);

    RETURN(env, attr);
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
