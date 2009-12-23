#include "yog/array.h"
#include "yog/class.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
to_s(YogEnv* env)
{
    return YogString_new_str(env, "nil");
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    return INT2VAL(2);
}

YogVal
YogNil_define_class(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Nil", env->vm->cObject);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, klass, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
