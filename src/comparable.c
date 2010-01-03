#include "yog/array.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/misc.h"
#include "yog/module.h"
#include "yog/vm.h"
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
greater_equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (0 <= compare(env, self, obj)) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
greater(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (0 < compare(env, self, obj)) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
less_equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (compare(env, self, obj) <= 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
less(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (compare(env, self, obj) < 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
not_equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (compare(env, self, obj) != 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    if (compare(env, self, obj) == 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

void
YogComparable_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal mComparable = YUNDEF;
    PUSH_LOCAL(env, mComparable);
    YogVM* vm = env->vm;

    mComparable = YogModule_new(env);
#define DEFINE_FUNCTION(name, f)    do { \
    YogModule_define_function(env, mComparable, pkg, (name), (f)); \
} while (0)
    DEFINE_FUNCTION("==", equal);
    DEFINE_FUNCTION("!=", not_equal);
    DEFINE_FUNCTION("<", less);
    DEFINE_FUNCTION("<=", less_equal);
    DEFINE_FUNCTION(">", greater);
    DEFINE_FUNCTION(">=", greater_equal);
#undef DEFINE_FUNCTION
    vm->mComparable = mComparable;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
