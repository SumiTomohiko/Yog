#include "yog/error.h"
#include "yog/frame.h"
#include "yog/class.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_TYPE(v)   do { \
    YOG_ASSERT(env, IS_BOOL(v), "value isn't bool."); \
} while (0)

static YogVal
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    CHECK_TYPE(self);

    const char* s = NULL;
    if (VAL2BOOL(self)) {
        s = "true";
    }
    else {
        s = "false";
    }

    return YogString_new_str(env, s);
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    int_t n;
    if (IS_TRUE(self)) {
        n = 1;
    }
    else {
        n = 0;
    }

    return INT2VAL(n);
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
