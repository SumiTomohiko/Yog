#include <gmp.h>
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/float.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/klass.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_TYPE(v) do { \
    YOG_ASSERT(env, IS_INT(v), "Value isn't int."); \
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
YogInt_add_bignum(YogEnv* env, YogVal self, YogVal bignum)
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

    if (IS_INT(right)) {
        result = YogVal_from_int(env, VAL2INT(self) + VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = (double)VAL2INT(self) + FLOAT_NUM(right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogInt_add_bignum(env, self, right);
        RETURN(env, result);
    }

    YOG_BUG(env, "Int#+ failed");

    /* NOTREACHED */
    RETURN(env, INT2VAL(0));
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

    if (IS_INT(right)) {
        result = YogVal_from_int(env, VAL2INT(self) - VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = (double)VAL2INT(self) - FLOAT_NUM(right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        bignum = YogBignum_from_int(env, VAL2INT(self));
        result = YogBignum_sub(env, bignum, right);
        RETURN(env, result);
    }

    YOG_BUG(env, "Int#- failed");

    /* NOTREACHED */
    RETURN(env, INT2VAL(0));
}

static YogVal
multiply_int(YogEnv* env, YogVal self, YogVal right)
{
    YOG_ASSERT(env, IS_INT(self), "self must be integer");
    YOG_ASSERT(env, IS_INT(right), "right must be integer");

    SAVE_ARGS2(env, self, right);
    YogVal bignum1 = YUNDEF;
    YogVal bignum2 = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS3(env, bignum1, bignum2, result);

    int n = VAL2INT(self);
    int m = VAL2INT(right);
    int l = n * m;
    if ((l / n == m) && FIXABLE(l)) {
        RETURN(env, INT2VAL(l));
    }

    bignum1 = YogBignum_from_int(env, n);
    bignum2 = YogBignum_from_int(env, m);
    result = YogBignum_multiply(env, bignum1, bignum2);

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

    if (IS_INT(right)) {
        result = multiply_int(env, self, right);
        RETURN(env, result);
    }
    else if (IS_BOOL(right) || IS_NIL(right) || IS_SYMBOL(right)) {
        YogError_raise_binop_type_error(env, self, right, "*");
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = (double)VAL2INT(self) * FLOAT_NUM(right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        bignum = YogBignum_from_int(env, VAL2INT(self));
        result = YogBignum_multiply(env, bignum, right);
        RETURN(env, result);
    }

    YOG_BUG(env, "Int#* failed");

    /* NOTREACHED */
    RETURN(env, INT2VAL(0));
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

    if (IS_INT(right)) {
        FLOAT_NUM(result) = (double)VAL2INT(self) / VAL2INT(right);
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        FLOAT_NUM(result) = VAL2INT(self) / FLOAT_NUM(right);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        FLOAT_NUM(result) = VAL2INT(self) / mpz_get_d(BIGNUM_NUM(right));
    }

    RETURN(env, result);
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

    if (IS_INT(right)) {
        result = INT2VAL(VAL2INT(self) / VAL2INT(right));
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = VAL2INT(self) / FLOAT_NUM(right);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogBignum_from_int(env, VAL2INT(self));
        mpz_fdiv_q(BIGNUM_NUM(result), BIGNUM_NUM(result), BIGNUM_NUM(right));
    }

    RETURN(env, result);
}

static YogVal 
less(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal n = YogArray_at(env, args, 0);

    CHECK_ARGS(self, n);

    YogVal retval;
    if (VAL2INT(self) < VAL2INT(n)) {
        retval = YTRUE;
    }
    else {
        retval = YFALSE;
    }
    RETURN(env, retval);
}

static YogVal 
times(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    int n = VAL2INT(self);

    unsigned int i = 0;
    unsigned int argc = 1;
    for (i = 0; i < n; i++) {
        YogVal args[argc];
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

YogVal 
YogInt_klass_new(YogEnv* env) 
{
    YogVal klass = YogKlass_new(env, "Int", env->vm->cObject);
    PUSH_LOCAL(env, klass);
#define DEFINE_METHOD(name, f)  YogKlass_define_method(env, klass, name, f)
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("-", subtract);
    DEFINE_METHOD("*", multiply);
    DEFINE_METHOD("/", divide);
    DEFINE_METHOD("//", floor_divide);
    DEFINE_METHOD("<", less);
#undef DEFINE_METHOD
    YogKlass_define_method(env, klass, "-self", negative);
    YogKlass_define_method(env, klass, "to_s", to_s);
    YogKlass_define_method(env, klass, "times", times);

    POP_LOCALS(env);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
