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

    s = YogString_new_format(env, "%g", PTR_AS(YogFloat, self)->val);

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
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_INT(right)) {
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

YogVal 
YogFloat_klass_new(YogEnv* env) 
{
    SAVE_LOCALS(env);

    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Float", env->vm->cObject);
    YogKlass_define_allocator(env, klass, allocate);
    YogKlass_define_method(env, klass, "-self", negative);
    YogKlass_define_method(env, klass, "to_s", to_s);
    YogKlass_define_method(env, klass, "+", add);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
