#include "yog/array.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/get_args.h"
#include "yog/misc.h"
#include "yog/module.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
cmp(YogEnv* env, YogVal self, YogVal x)
{
    SAVE_ARGS2(env, self, x);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogEval_call_method1(env, self, "<=>", x);

    RETURN(env, retval);
}

static int_t
conv_to_cmp(YogEnv* env, YogVal c, YogVal x, YogVal y)
{
    SAVE_ARGS3(env, c, x, y);
    if (!IS_FIXNUM(c)) {
        YogError_raise_TypeError(env, "comparison of %C with %C failed", x, y);
    }
    RETURN(env, VAL2INT(c));
}

static YogVal
greater_equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal x = YUNDEF;
    YogVal c = YUNDEF;
    PUSH_LOCALS2(env, x, c);
    YogCArg params[] = { { "x", &x }, { NULL, NULL } };
    YogGetArgs_parse_args(env, ">=", params, args, kw);

    c = cmp(env, self, x);
    if (0 <= conv_to_cmp(env, c, self, x)) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
greater(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal x = YUNDEF;
    YogVal c = YUNDEF;
    PUSH_LOCALS2(env, x, c);
    YogCArg params[] = { { "x", &x }, { NULL, NULL } };
    YogGetArgs_parse_args(env, ">", params, args, kw);

    c = cmp(env, self, x);
    if (0 < conv_to_cmp(env, c, self, x)) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
less_equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal x = YUNDEF;
    YogVal c = YUNDEF;
    PUSH_LOCALS2(env, x, c);
    YogCArg params[] = { { "x", &x }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "<=", params, args, kw);

    c = cmp(env, self, x);
    if (conv_to_cmp(env, c, self, x) <= 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
less(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal x = YUNDEF;
    YogVal c = YUNDEF;
    PUSH_LOCALS2(env, x, c);
    YogCArg params[] = { { "x", &x }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "<", params, args, kw);

    c = cmp(env, self, x);
    if (conv_to_cmp(env, c, self, x) < 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
not_equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal x = YUNDEF;
    YogVal c = YUNDEF;
    PUSH_LOCALS2(env, x, c);
    YogCArg params[] = { { "x", &x }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "!=", params, args, kw);

    c = cmp(env, self, x);
    if (IS_NIL(c)) {
        RETURN(env, YTRUE);
    }
    if (conv_to_cmp(env, c, self, x) != 0) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal x = YUNDEF;
    YogVal c = YUNDEF;
    PUSH_LOCALS2(env, x, c);
    YogCArg params[] = { { "x", &x }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "==", params, args, kw);

    c = cmp(env, self, x);
    if (IS_NIL(c)) {
        RETURN(env, YFALSE);
    }
    if (conv_to_cmp(env, c, self, x) == 0) {
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
