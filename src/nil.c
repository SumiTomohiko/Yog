#include "yog/array.h"
#include "yog/env.h"
#include "yog/klass.h"
#include "yog/string.h"
#include "yog/thread.h"
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

static YogVal
equal(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal obj = YUNDEF;
    PUSH_LOCALS2(env, retval, obj);

    obj = YogArray_at(env, args, 0);
    if (IS_NIL(obj)) {
        retval = YTRUE;
    }
    else {
        retval = YFALSE;
    }

    RETURN(env, retval);
}

YogVal
YogNil_klass_new(YogEnv* env)
{
    YogVal klass = YogKlass_new(env, "Nil", env->vm->cObject);
    PUSH_LOCAL(env, klass);

    YogKlass_define_method(env, klass, "equal?", equal);
    YogKlass_define_method(env, klass, "hash", hash);
    YogKlass_define_method(env, klass, "to_s", to_s);

    POP_LOCALS(env);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
