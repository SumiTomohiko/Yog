#include "yog/array.h"
#include "yog/class.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
to_s(YogEnv* env)
{
    return YogString_from_str(env, "nil");
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    return INT2VAL(2);
}

void
YogNil_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cNil = YUNDEF;
    PUSH_LOCAL(env, cNil);
    YogVM* vm = env->vm;

    cNil = YogClass_new(env, "Nil", vm->cObject);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cNil, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD
    vm->cNil = cNil;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
