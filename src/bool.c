#include "yog/class.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/get_args.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_TYPE(v)   do { \
    YOG_ASSERT(env, IS_BOOL(v), "value isn't bool."); \
} while (0)

static YogVal
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal t = YUNDEF;
    PUSH_LOCAL(env, t);

    CHECK_TYPE(self);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_s", params, args, kw);

    const char* s;
    if (VAL2BOOL(self)) {
        s = "true";
    }
    else {
        s = "false";
    }
    t = YogString_new_str(env, s);

    RETURN(env, t);
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "hash", params, args, kw);

    int_t n;
    if (IS_TRUE(self)) {
        n = 1;
    }
    else {
        n = 0;
    }

    RETURN(env, INT2VAL(n));
}

YogVal
YogBool_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Bool", env->vm->cObject);
    YogClass_define_method(env, klass, "hash", hash);
    YogClass_define_method(env, klass, "to_s", to_s);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
