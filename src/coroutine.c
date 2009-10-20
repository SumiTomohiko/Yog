#include "yog/class.h"
#include "yog/object.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct SwitchContext {
    void* ebp;
    void* esp;
    void* eip;
};

typedef struct SwitchContext SwitchContext;

struct Coroutine {
    struct YogBasicObj base;
    void* machine_stack;
    YogVal bottom_frame;
    YogVal top_frame;
    struct SwitchContext ctx_to_resume;
    struct SwitchContext ctx_to_yield;
};

typedef struct Coroutine Coroutine;

static YogVal
yield(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    /* TODO */
    RETURN(env, self);
}

static YogVal
resume(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    /* TODO */
    RETURN(env, self);
}

static YogVal
init(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    /* TODO */
    RETURN(env, self);
}

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    RETURN(env, obj);
}

YogVal
YogCoroutine_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Coroutine", env->vm->cObject);
    YogClass_define_allocator(env, klass, allocate);
    YogClass_define_class_method(env, klass, "yield", yield);
#define DEFINE_METHOD(name, f)  YogClass_define_method(env, klass, name, f)
    DEFINE_METHOD("init", init);
    DEFINE_METHOD("resume", resume);
#undef DEFINE_METHOD

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
