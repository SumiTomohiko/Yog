#include "yog/array.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/misc.h"
#include "yog/module.h"
#include "yog/yog.h"

static int_t
compare(YogEnv* env, YogVal self, YogVal obj)
{
    SAVE_ARGS2(env, self, obj);
    YogVal retval = YogEval_call_method1(env, self, "<=>", obj);
    if (!IS_FIXNUM(retval)) {
        YogError_raise_TypeError(env, "result of <=> must be Fixnum");
    }
    RETURN(env, VAL2INT(retval));
}

static YogVal
greater_equal(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (0 <= compare(env, self, obj)) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
greater(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (0 < compare(env, self, obj)) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
less_equal(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (compare(env, self, obj) <= 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
less(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (compare(env, self, obj) < 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
not_equal(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (compare(env, self, obj) != 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
equal(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (compare(env, self, obj) == 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

YogVal
YogComparable_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal mod = YUNDEF;
    PUSH_LOCAL(env, mod);

    mod = YogModule_new(env);
#define DEFINE_FUNCTION(name, f)    YogModule_define_function(env, mod, name, f)
    DEFINE_FUNCTION("==", equal);
    DEFINE_FUNCTION("!=", not_equal);
    DEFINE_FUNCTION("<", less);
    DEFINE_FUNCTION("<=", less_equal);
    DEFINE_FUNCTION(">", greater);
    DEFINE_FUNCTION(">=", greater_equal);
#undef DEFINE_FUNCTION

    RETURN(env, mod);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
