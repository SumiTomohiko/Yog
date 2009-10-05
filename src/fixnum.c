#include "config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#if defined(HAVE_MALLOC_H)
#   include <malloc.h>
#endif
#include <stdlib.h>
#include <math.h>
#include <gmp.h>
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/float.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/class.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_TYPE(v) do { \
    YOG_ASSERT(env, IS_FIXNUM((v)), "Value isn't Fixnum (0x%08x)", (v)); \
} while (0)

#define CHECK_ARGS(self, v) do { \
    CHECK_TYPE(self); \
    CHECK_TYPE(v); \
} while (0)

static YogVal
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    CHECK_TYPE(self);
    YogVal retval = YogString_new_format(env, "%d", VAL2INT(self));
    RETURN(env, retval);
}

YogVal
YogFixnum_add_bignum(YogEnv* env, YogVal self, YogVal bignum)
{
    SAVE_ARGS2(env, self, bignum);
    YogVal left_and_result = YUNDEF;
    PUSH_LOCAL(env, left_and_result);

    left_and_result = YogBignum_from_int(env, VAL2INT(self));
    mpz_add(BIGNUM_NUM(left_and_result), BIGNUM_NUM(left_and_result), BIGNUM_NUM(bignum));

    RETURN(env, left_and_result);
}

static YogVal
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);

    if (IS_FIXNUM(right)) {
        result = YogVal_from_int(env, VAL2INT(self) + VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = (double)VAL2INT(self) + FLOAT_NUM(right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogFixnum_add_bignum(env, self, right);
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
    YogVal result = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS3(env, result, bignum, right);

    right = YogArray_at(env, args, 0);

    if (IS_FIXNUM(right)) {
        result = YogVal_from_int(env, VAL2INT(self) - VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = (double)VAL2INT(self) - FLOAT_NUM(right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        bignum = YogBignum_from_int(env, VAL2INT(self));
        result = YogBignum_subtract(env, bignum, right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "-");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
multiply_int(YogEnv* env, YogVal self, YogVal right)
{
    YOG_ASSERT(env, IS_FIXNUM(self), "self must be Fixnum");
    YOG_ASSERT(env, IS_FIXNUM(right), "right must be Fixnum");

    SAVE_ARGS2(env, self, right);
    YogVal bignum1 = YUNDEF;
    YogVal bignum2 = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS3(env, bignum1, bignum2, result);

    int_t n = VAL2INT(self);
    int_t m = VAL2INT(right);
    int_t l = n * m;
    if ((l / n == m) && FIXABLE(l)) {
        RETURN(env, INT2VAL(l));
    }

    bignum1 = YogBignum_from_int(env, n);
    bignum2 = YogBignum_from_int(env, m);
    result = YogBignum_multiply(env, bignum1, bignum2);

    RETURN(env, result);
}

YogVal
YogFixnum_multiply(YogEnv* env, YogVal self, YogVal right)
{
    SAVE_ARGS2(env, self, right);
    YogVal result = YUNDEF;
    YogVal bignum = YUNDEF;
    PUSH_LOCALS2(env, result, bignum);

    bignum = YogBignum_from_int(env, VAL2INT(self));
    result = YogBignum_multiply(env, bignum, right);

    RETURN(env, result);
}

static YogVal
multiply(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal result = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS3(env, result, bignum, right);

    right = YogArray_at(env, args, 0);

    if (IS_FIXNUM(right)) {
        result = multiply_int(env, self, right);
        RETURN(env, result);
    }
    else if (IS_BOOL(right) || IS_NIL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = (double)VAL2INT(self) * FLOAT_NUM(right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogFixnum_multiply(env, self, right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cString)) {
        result = YogString_multiply(env, right, VAL2INT(self));
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "*");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static double
divide_int(YogEnv* env, YogVal left, YogVal right)
{
    if (VAL2INT(right) == 0) {
        YogError_raise_ZeroDivisionError(env, "Fixnum division by zero");
    }
    return (double)VAL2INT(left) / VAL2INT(right);
}

static double
divide_float(YogEnv* env, YogVal left, YogVal right)
{
    if (FLOAT_NUM(right) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "float division");
    }
    return VAL2INT(left) / FLOAT_NUM(right);
}

static YogVal
divide(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal result = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS3(env, result, bignum, right);

    right = YogArray_at(env, args, 0);

    if (IS_BOOL(right) || IS_NIL(right) || IS_SYMBOL(right)) {
        YogError_raise_binop_type_error(env, self, right, "/");
    }

    result = YogFloat_new(env);

    if (IS_FIXNUM(right)) {
        FLOAT_NUM(result) = divide_int(env, self, right);
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        FLOAT_NUM(result) = divide_float(env, self, right);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        FLOAT_NUM(result) = VAL2INT(self) / mpz_get_d(BIGNUM_NUM(right));
    }

    RETURN(env, result);
}

static int_t
floor_divide_int(YogEnv* env, YogVal left, YogVal right)
{
    if (VAL2INT(right) == 0) {
        YogError_raise_ZeroDivisionError(env, "Fixnum division by zero");
    }
    return VAL2INT(left) / VAL2INT(right);
}

static double
floor_divide_float(YogEnv* env, YogVal left, YogVal right)
{
    if (FLOAT_NUM(right) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "float division");
    }
    return VAL2INT(left) / FLOAT_NUM(right);
}

static YogVal
modulo(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YOG_ASSERT(env, IS_FIXNUM(self), "self is not Fixnum");

    SAVE_ARGS4(env, self, args, kw, block);
    YogVal result = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS3(env, result, bignum, right);

    right = YogArray_at(env, args, 0);

    if (IS_FIXNUM(right)) {
        result = INT2VAL(VAL2INT(self) % VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        bignum = YogBignum_from_int(env, VAL2INT(self));
        result = YogBignum_modulo(env, bignum, right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "%");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
floor_divide(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal result = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS3(env, result, bignum, right);

    right = YogArray_at(env, args, 0);

    if (IS_BOOL(right) || IS_NIL(right) || IS_SYMBOL(right)) {
        YogError_raise_binop_type_error(env, self, right, "//");
    }

    if (IS_FIXNUM(right)) {
        result = INT2VAL(floor_divide_int(env, self, right));
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = floor_divide_float(env, self, right);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogBignum_from_int(env, VAL2INT(self));
        mpz_fdiv_q(BIGNUM_NUM(result), BIGNUM_NUM(result), BIGNUM_NUM(right));
    }

    RETURN(env, result);
}

static YogVal
do_rshift(YogEnv* env, int_t val, int_t width)
{
    YOG_ASSERT(env, 0 <= width, "negative width (%d)", width);

    if (width < sizeof(int_t) * CHAR_BIT) {
        return INT2VAL(val >> width);
    }

    return INT2VAL(0);
}

static YogVal
do_lshift(YogEnv* env, int_t val, int_t width)
{
    YOG_ASSERT(env, 0 <= width, "negative width (%d)", width);

    SAVE_LOCALS(env);
    YogVal retval = YUNDEF;
    YogVal bignum = YUNDEF;
    PUSH_LOCALS2(env, retval, bignum);

    if (width < sizeof(int_t) * CHAR_BIT) {
        int_t result = val << width;
        if ((result >> width == val) && FIXABLE(result)) {
            RETURN(env, INT2VAL(result));
        }
    }

    bignum = YogBignum_from_int(env, val);
    retval = YogBignum_lshift(env, bignum, width);

    RETURN(env, retval);
}

static YogVal
xor(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YOG_ASSERT(env, IS_FIXNUM(self), "self is not Fixnum");

    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, right, retval);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        retval = INT2VAL(VAL2INT(self) ^ VAL2INT(right));
        RETURN(env, retval);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        retval = YogBignum_xor(env, right, self);
        RETURN(env, retval);
    }

    YogError_raise_binop_type_error(env, self, right, "^");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
and(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, right, retval);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        retval = INT2VAL(VAL2INT(self) & VAL2INT(right));
        RETURN(env, retval);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        retval = YogBignum_and(env, right, self);
        RETURN(env, retval);
    }

    YogError_raise_binop_type_error(env, self, right, "&");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
or(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, right, retval);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        retval = INT2VAL(VAL2INT(self) | VAL2INT(right));
        RETURN(env, retval);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        retval = YogBignum_or(env, right, self);
        RETURN(env, retval);
    }

    YogError_raise_binop_type_error(env, self, right, "|");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
rshift(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, right, retval);

    right = YogArray_at(env, args, 0);
    if (!IS_FIXNUM(right)) {
        YogError_raise_binop_type_error(env, self, right, ">>");
    }
    int_t n = VAL2INT(right);
    if (0 < n) {
        retval = do_rshift(env, VAL2INT(self), n);
    }
    else {
        retval = do_lshift(env, VAL2INT(self), - n);
    }

    RETURN(env, retval);
}

static YogVal
lshift(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, right, retval);

    right = YogArray_at(env, args, 0);
    if (!IS_FIXNUM(right)) {
        YogError_raise_binop_type_error(env, self, right, "<<");
    }
    int_t n = VAL2INT(right);
    if (0 < n) {
        retval = do_lshift(env, VAL2INT(self), n);
    }
    else {
        retval = do_rshift(env, VAL2INT(self), - n);
    }

    RETURN(env, retval);
}

static YogVal
times(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    int_t n = VAL2INT(self);

    uint_t i = 0;
    uint_t argc = 1;
    for (i = 0; i < n; i++) {
        YogVal* args = (YogVal*)alloca(sizeof(YogVal) * argc);
        args[0] = INT2VAL(i);
        YogCallable_call(env, block, argc, args);
    }

    RETURN(env, YNIL);
}

static YogVal
negative(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal n = INT2VAL(- VAL2INT(self));
    RETURN(env, n);
}

static YogVal
positive(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    return self;
}

static YogVal
not(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal n = INT2VAL(~ VAL2INT(self));
    RETURN(env, n);
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    return self;
}

static YogVal
power_int(YogEnv* env, int_t base, int_t exp)
{
    SAVE_LOCALS(env);
    YogVal retval = YUNDEF;
    YogVal f = YUNDEF;
    YogVal bignum = YUNDEF;
    PUSH_LOCALS3(env, retval, f, bignum);

    if (exp < 0) {
        if (base == 0) {
            YogError_raise_ZeroDivisionError(env, "0.0 cannot be raised to a negative power");
        }

        f = YogFloat_new(env);
        FLOAT_NUM(f) = 1 / (double)base;
        retval = YogFloat_power(env, f, - exp);
        RETURN(env, retval);
    }
    else if (exp == 0) {
        RETURN(env, INT2VAL(1));
    }

    int_t x = base;
    int_t y = exp;
    while (1 < y) {
        int_t x2 = x * base;
        if (!FIXABLE(x2) || ((x != 0) && (x2 / x != base))) {
            bignum = YogBignum_from_int(env, base);
            retval = YogBignum_power(env, bignum, INT2VAL(exp));
            RETURN(env, retval);
        }

        x = x2;
        y--;
    }

    RETURN(env, INT2VAL(x));
}

static YogVal
power(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal f = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS4(env, retval, f, bignum, right);

    YOG_ASSERT(env, IS_FIXNUM(self), "self is not Fixnum");

    right = YogArray_at(env, args, 0);

    if (IS_FIXNUM(right)) {
        retval = power_int(env, VAL2INT(self), VAL2INT(right));
        RETURN(env, retval);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        double base = (double)VAL2INT(self);
        double exp = FLOAT_NUM(right);
        f = YogFloat_new(env);
        FLOAT_NUM(f) = pow(base, exp);
        RETURN(env, f);
    }

    YogError_raise_binop_type_error(env, self, right, "**");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
compare(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, obj, retval);

    obj = YogArray_at(env, args, 0);
    if (!IS_FIXNUM(obj)) {
        YogError_raise_comparison_type_error(env, self, obj);
    }

    int_t n = VAL2INT(self) - VAL2INT(obj);
    if (n < 0) {
        RETURN(env, INT2VAL(-1));
    }
    else if (n == 0) {
        RETURN(env, INT2VAL(n));
    }

    RETURN(env, INT2VAL(1));
}

YogVal
YogFixnum_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Fixnum", env->vm->cObject);
    YogClass_include_module(env, klass, env->vm->mComparable);
#define DEFINE_METHOD(name, f)  YogClass_define_method(env, klass, name, f)
    DEFINE_METHOD("%", modulo);
    DEFINE_METHOD("&", and);
    DEFINE_METHOD("*", multiply);
    DEFINE_METHOD("**", power);
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("+self", positive);
    DEFINE_METHOD("-", subtract);
    DEFINE_METHOD("-self", negative);
    DEFINE_METHOD("/", divide);
    DEFINE_METHOD("//", floor_divide);
    DEFINE_METHOD("<<", lshift);
    DEFINE_METHOD("<=>", compare);
    DEFINE_METHOD(">>", rshift);
    DEFINE_METHOD("^", xor);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("times", times);
    DEFINE_METHOD("to_s", to_s);
    DEFINE_METHOD("|", or);
    DEFINE_METHOD("~self", not);
#undef DEFINE_METHOD

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
