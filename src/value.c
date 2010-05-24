#include <stdio.h>
#include "yog/bignum.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/string.h"
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
YogVal_get_class(YogEnv* env, YogVal val)
{
    if (IS_FIXNUM(val)) {
        return env->vm->cFixnum;
    }
    else if (IS_PTR(val)) {
        DEBUG(TRACE("val=0x%08x, klass=0x%08x", val, PTR_AS(YogBasicObj, val)->klass));
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

    c = YogVal_get_class(env, attr);
    if (PTR_AS(YogClass, c)->call_get_descr == NULL) {
        RETURN(env, attr);
    }
    v = PTR_AS(YogClass, c)->call_get_descr(env, attr, obj, klass);

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

    klass = YogVal_get_class(env, self);

    if (IS_PTR(self) && (PTR_AS(YogBasicObj, self)->flags & HAS_ATTRS)) {
        attr = YogObj_get_attr(env, self, name);
        if (!IS_UNDEF(attr)) {
            val = YogVal_get_descr(env, attr, self, klass);
            RETURN(env, val);
        }
    }

    attr = YogClass_get_attr(env, klass, name);
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

    klass = YogVal_get_class(env, val);
    GetAttrCaller getter = PTR_AS(YogClass, klass)->call_get_attr;
    if (getter == NULL) {
        getter = get_attr_default;
    }
    attr = (*getter)(env, val, name);

    RETURN(env, attr);
}

BOOL
YogVal_is_subclass_of(YogEnv* env, YogVal val, YogVal klass)
{
    YogVal valclass = YogVal_get_class(env, val);
    while (!IS_NIL(valclass)) {
        if (PTR_AS(YogClass, valclass) == PTR_AS(YogClass, klass)) {
            return TRUE;
        }
        valclass = PTR_AS(YogClass, valclass)->super;
    }

    return FALSE;
}

YogVal
YogVal_from_long_long(YogEnv* env, long long n)
{
    if (FIXABLE(n)) {
        return INT2VAL(n);
    }
    return YogBignum_from_long_long(env, n);
}

YogVal
YogVal_from_unsigned_long_long(YogEnv* env, unsigned long long n)
{
    if (n < YINT_MAX) {
        return INT2VAL(n);
    }
    return YogBignum_from_unsigned_long_long(env, n);
}

YogVal
YogVal_from_unsigned_int(YogEnv* env, uint_t n)
{
    if (n < YINT_MAX) {
        return INT2VAL(n);
    }
    return YogBignum_from_unsigned_int(env, n);
}

YogVal
YogVal_from_int(YogEnv* env, int_t n)
{
    if (FIXABLE(n)) {
        return INT2VAL(n);
    }
    return YogBignum_from_int(env, n);
}

void
YogVal_set_attr(YogEnv* env, YogVal obj, ID name, YogVal val)
{
    SAVE_ARGS2(env, obj, val);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    if ((PTR_AS(YogBasicObj, obj)->flags & HAS_ATTRS) == 0) {
        YogError_raise_AttributeError(env, "%C object has no attribute \"%I\"", obj, name);
    }

    YogObj_set_attr_id(env, obj, name, val);

    RETURN_VOID(env);
}

SIGNED_TYPE
YogVal_to_signed_type(YogEnv* env, YogVal self, const char* name)
{
    SAVE_ARG(env, self);

    if (IS_FIXNUM(self)) {
        RETURN(env, VAL2INT(self));
    }
    if (IS_PTR(self) && (BASIC_OBJ_TYPE(self) == TYPE_BIGNUM)) {
        RETURN(env, YogBignum_to_signed_type(env, self, name));
    }

    YogError_raise_TypeError(env, "%s must be a Fixnum or Bignum", name);

    /* NOTREACHED */
    RETURN(env, 0);
}

YogVal
YogVal_get_class_name(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal name = YUNDEF;
    YogVal klass = YUNDEF;
    PUSH_LOCALS2(env, name, klass);

    klass = YogVal_get_class(env, self);
    name = YogVM_id2name(env, env->vm, PTR_AS(YogClass, klass)->name);

    RETURN(env, name);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
