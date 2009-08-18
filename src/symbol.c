#include "yog/array.h"
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

static YogVal
hash(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    return INT2VAL(VAL2ID(self));
}

static YogVal
equal(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal obj = YUNDEF;
    PUSH_LOCALS2(env, retval, obj);

    obj = YogArray_at(env, args, 0);
    if (IS_SYMBOL(obj) && (self == obj)) {
        retval = YTRUE;
    }
    else {
        retval = YFALSE;
    }

    RETURN(env, retval);
}

YogVal
YogSymbol_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Symbol", env->vm->cObject);
#define DEFINE_METHOD(name, f)  YogKlass_define_method(env, klass, name, f)
    DEFINE_METHOD("==", equal);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
