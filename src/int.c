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
sub(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
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
    DEFINE_METHOD("-", sub);
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
