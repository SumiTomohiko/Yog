#include "yog/klass.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    const char* t = YogVM_id2name(env, env->vm, VAL2ID(self));
    s = YogString_new_str(env, t);

    RETURN(env, s);
}

YogVal
YogSymbol_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Symbol", env->vm->cObject);
    YogKlass_define_method(env, klass, "to_s", to_s);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
