#include "yog/array.h"
#include "yog/dict.h"
#include "yog/class.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
get_size(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    int_t size = YogDict_size(env, self);
    retval = YogVal_from_int(env, size);

    RETURN(env, retval);
}

static YogVal
include(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal elem = YUNDEF;
    PUSH_LOCAL(env, elem);

    elem = YogArray_at(env, args, 0);
    if (YogDict_include(env, self, elem)) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

void
YogSet_add(YogEnv* env, YogVal self, YogVal elem)
{
    SAVE_ARGS2(env, self, elem);
    YogDict_set(env, self, elem, INT2VAL(1));
    RETURN_VOID(env);
}

static YogVal
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal elem = YUNDEF;
    PUSH_LOCAL(env, elem);

    uint_t size = YogArray_size(env, args);
    uint_t i;
    for (i = 0; i < size; i++) {
        elem = YogArray_at(env, args, i);
        YogSet_add(env, self, elem);
    }

    RETURN(env, self);
}

YogVal
YogSet_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal set = YUNDEF;
    PUSH_LOCAL(env, set);

    set = YogDict_allocate(env, env->vm->cSet);

    RETURN(env, set);
}

YogVal
YogSet_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Set", env->vm->cObject);
    YogClass_define_allocator(env, klass, YogDict_allocate);
    YogClass_define_method(env, klass, "add", add);
    YogClass_define_method(env, klass, "include?", include);
    YogClass_define_property(env, klass, "size", get_size, NULL);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
