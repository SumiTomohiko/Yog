#include <stdio.h>
#include "yog/bignum.h"
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
    else if (IS_FIXNUM(val)) {
        printf("<int_t: %d>\n", VAL2INT(val));
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

YogVal
YogVal_get_klass(YogEnv* env, YogVal val)
{
    if (IS_FIXNUM(val)) {
        return env->vm->cFixnum;
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
    else if (IS_SYMBOL(val)) {
        return env->vm->cSymbol;
    }
    else {
        YOG_BUG(env, "Uknown type of value (0x%08x)", val);
    }

    /* NOTREACHED */
    return YUNDEF;
}

YogVal
YogVal_get_descr(YogEnv* env, YogVal attr, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, attr, obj, klass);
    YogVal c = YUNDEF;
    YogVal v = YUNDEF;
    PUSH_LOCALS2(env, c, v);

    c = YogVal_get_klass(env, attr);
    if (PTR_AS(YogKlass, c)->call_get_descr == NULL) {
        RETURN(env, attr);
    }
    v = PTR_AS(YogKlass, c)->call_get_descr(env, attr, obj, klass);

    RETURN(env, v);
}

static YogVal
get_attr_default(YogEnv* env, YogVal self, ID name)
{
    SAVE_ARG(env, self);
    YogVal klass = YUNDEF;
    YogVal attr = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS3(env, klass, attr, val);

    klass = YogVal_get_klass(env, self);

    if (IS_PTR(self) && (PTR_AS(YogBasicObj, self)->flags & HAS_ATTRS)) {
        attr = YogObj_get_attr(env, self, name);
        if (!IS_UNDEF(attr)) {
            val = YogVal_get_descr(env, attr, self, klass);
            RETURN(env, val);
        }
    }

    attr = YogKlass_get_attr(env, klass, name);
    if (!IS_UNDEF(attr)) {
        attr = YogVal_get_descr(env, attr, self, klass);
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
    GetAttrCaller getter = PTR_AS(YogKlass, klass)->call_get_attr;
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

YogVal
YogVal_from_int(YogEnv* env, int_t n)
{
    if (!FIXABLE(n)) {
        return YogBignum_from_int(env, n);
    }
    return INT2VAL(n);
}

void
YogVal_set_attr(YogEnv* env, YogVal obj, ID name, YogVal val)
{
    if ((PTR_AS(YogBasicObj, obj)->flags & HAS_ATTRS) == 0) {
        YogVal klass = PTR_AS(YogBasicObj, obj)->klass;
        ID id = PTR_AS(YogKlass, klass)->name;
        YogError_raise_AttributeError(env, "%s object has no attribute '%s'", YogVM_id2name(env, env->vm, id), YogVM_id2name(env, env->vm, name));
    }

    YogObj_set_attr_id(env, obj, name, val);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
