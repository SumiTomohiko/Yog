#include <string.h>
#include "yog/array.h"
#include "yog/class.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
inspect(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    YogVal sym = YUNDEF;
    PUSH_LOCALS2(env, s, sym);

    s = YogString_from_str(env, ":");
    sym = YogVM_id2name(env, env->vm, VAL2ID(self));
    YogString_add(env, s, sym);

    RETURN(env, s);
}

static YogVal
to_s(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogVM_id2name(env, env->vm, VAL2ID(self));

    RETURN(env, s);
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    return INT2VAL(VAL2ID(self));
}

static YogVal
equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
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

void
YogSymbol_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cSymbol = YUNDEF;
    PUSH_LOCAL(env, cSymbol);
    YogVM* vm = env->vm;

    cSymbol = YogClass_new(env, "Symbol", vm->cObject);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cSymbol, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("==", equal);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("inspect", inspect);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD
    vm->cSymbol = cSymbol;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
