#include "yog/config.h"
#include "yog/array.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/dict.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
get_size(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_size", params, args, kw);

    int_t size = YogDict_size(env, self);
    retval = YogVal_from_int(env, size);

    RETURN(env, retval);
}

static YogVal
include(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal elem = YUNDEF;
    PUSH_LOCAL(env, elem);

    YogCArg params[] = { { "obj", &elem }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "include?", params, args, kw);

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
add(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal elem = YUNDEF;
    YogVal objs = YUNDEF;
    PUSH_LOCALS2(env, elem, objs);

    YogCArg params[] = { { "*", &objs }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "add", params, args, kw);

    uint_t size = YogArray_size(env, objs);
    uint_t i;
    for (i = 0; i < size; i++) {
        elem = YogArray_at(env, objs, i);
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

    set = YogDict_alloc(env, env->vm->cSet);

    RETURN(env, set);
}

static YogVal
each(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* block)
{
    SAVE_LOCALS(env);
    YogVal iter = YUNDEF;
    PUSH_LOCAL(env, iter);
    YogVal params[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(params), params);

    iter = YogDict_get_iterator(env, HDL2VAL(self));
    while (YogDictIterator_next(env, iter)) {
        params[0] = YogDictIterator_current_key(env, iter);
        YogCallable_call(env, HDL2VAL(block), array_sizeof(params), params);
    }

    RETURN(env, YNIL);
}

void
YogSet_eval_builtin_script(YogEnv* env, YogVal klass)
{
    const char* src =
#include "set.inc"
    ;
    YogMisc_eval_source(env, VAL2HDL(env, klass), src);
}

void
YogSet_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cSet = YUNDEF;
    PUSH_LOCAL(env, cSet);
    YogVM* vm = env->vm;

    cSet = YogClass_new(env, "Set", vm->cObject);
    YogClass_define_allocator(env, cSet, YogDict_alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cSet, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("add", add);
    DEFINE_METHOD("include?", include);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...)  do { \
    YogClass_define_method2(env, cSet, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("each", each, "&", NULL);
#undef DEFINE_METHOD2
#define DEFINE_PROP(name, getter, setter)   do { \
    YogClass_define_property(env, cSet, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("size", get_size, NULL);
#undef DEFINE_PROP
    vm->cSet = cSet;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
