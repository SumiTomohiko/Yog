#include <math.h>
#include <gmp.h>
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/float.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/yog.h"

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal f = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, YogFloat);
    YogBasicObj_init(env, f, 0, klass);
    PTR_AS(YogFloat, f)->val = 0;

    RETURN(env, f);
}

YogVal
YogFloat_new(YogEnv* env)
{
    YogVal f = allocate(env, env->vm->cFloat);
    PTR_AS(YogFloat, f)->val = 0;
    return f;
}

static YogVal
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    if (isnan(FLOAT_NUM(self))) {
        s = YogString_new_str(env, "NaN");
    }
    else {
        s = YogString_new_format(env, "%g", PTR_AS(YogFloat, self)->val);
    }

    RETURN(env, s);
}

static YogVal
negative(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal f = YUNDEF;
    PUSH_LOCAL(env, f);

    f = YogFloat_new(env);
    FLOAT_NUM(f) = - FLOAT_NUM(self);

    RETURN(env, f);
}

static YogVal
positive(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    return self;
}

static YogVal
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) + VAL2INT(right);
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) + mpz_get_d(BIGNUM_NUM(right));
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) + FLOAT_NUM(right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "+");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
subtract(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) - VAL2INT(right);
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) - mpz_get_d(BIGNUM_NUM(right));
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) - FLOAT_NUM(right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "-");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
multiply(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) * VAL2INT(right);
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) * mpz_get_d(BIGNUM_NUM(right));
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) * FLOAT_NUM(right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "*");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
div(YogEnv* env, YogVal self, YogVal right, const char* opname)
{
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");

    SAVE_ARGS2(env, self, right);
    YogVal result = YUNDEF;
    PUSH_LOCAL(env, result);

    if (IS_FIXNUM(right)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) / VAL2INT(right);
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) / mpz_get_d(BIGNUM_NUM(right));
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) / FLOAT_NUM(right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, opname);

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
divide(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogVal right = YogArray_at(env, args, 0);
    return div(env, self, right, "/");
}

static YogVal
floor_divide(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogVal right = YogArray_at(env, args, 0);
    return div(env, self, right, "//");
}

static YogVal
power_float(YogEnv* env, YogVal self, double exp)
{
    SAVE_ARG(env, self);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    if (FLOAT_NUM(self) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "0.0 cannot be raised to a negative power");
    }

    retval = YogFloat_new(env);
    FLOAT_NUM(retval) = pow(FLOAT_NUM(self), exp);

    RETURN(env, retval);
}

static YogVal
power_int(YogEnv* env, YogVal self, int_t exp)
{
    SAVE_ARG(env, self);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    if (FLOAT_NUM(self) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "0.0 cannot be raised to a negative power");
    }

    retval = YogFloat_new(env);
    FLOAT_NUM(retval) = pow(FLOAT_NUM(self), (double)exp);

    RETURN(env, retval);
}

YogVal
YogFloat_power(YogEnv* env, YogVal self, int_t exp)
{
    return power_int(env, self, exp);
}

static YogVal
power(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS2(env, retval, right);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");

    if (IS_FIXNUM(right)) {
        retval = power_int(env, self, VAL2INT(right));
        RETURN(env, retval);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        retval = power_float(env, self, FLOAT_NUM(right));
        RETURN(env, retval);
    }

    YogError_raise_binop_type_error(env, self, right, "**");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

YogVal
YogFloat_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Float", env->vm->cObject);
    YogKlass_define_allocator(env, klass, allocate);
    YogKlass_define_method(env, klass, "*", multiply);
    YogKlass_define_method(env, klass, "**", power);
    YogKlass_define_method(env, klass, "+", add);
    YogKlass_define_method(env, klass, "+self", positive);
    YogKlass_define_method(env, klass, "-", subtract);
    YogKlass_define_method(env, klass, "-self", negative);
    YogKlass_define_method(env, klass, "/", divide);
    YogKlass_define_method(env, klass, "//", floor_divide);
    YogKlass_define_method(env, klass, "to_s", to_s);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
