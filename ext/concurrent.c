#include <pthread.h>
#include "yog/env.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Barrier {
    YOGBASICOBJ_HEAD;
    pthread_barrier_t barrier;
};

typedef struct Barrier Barrier;

static YogVal
Barrier_initialize(YogEnv* env)
{
    YogVal self = SELF(env);
    SAVE_ARG(env, self);

    YogVal count = ARG(env, 0);
    /* TODO: check type */
    pthread_barrier_t* barrier = &PTR_AS(Barrier, self)->barrier;
    if (pthread_barrier_init(barrier, NULL, VAL2INT(count)) != 0) {
        YOG_BUG(env, "pthread_barrier_init failed");
    }

    RETURN(env, self);
}

static YogVal
Barrier_wait(YogEnv* env)
{
    YogVal self = SELF(env);
    SAVE_ARG(env, self);

    pthread_barrier_wait(&PTR_AS(Barrier, self)->barrier);

    RETURN(env, self);
}

static void
Barrier_finalize(YogEnv* env, void* ptr)
{
    Barrier* barrier = ptr;

    if (pthread_barrier_destroy(&barrier->barrier) != 0) {
        YOG_WARN(env, "pthread_barrier_destroy failed");
    }
}

static YogVal
Barrier_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal barrier = YUNDEF;
    PUSH_LOCAL(env, barrier);

    barrier = ALLOC_OBJ(env, NULL, Barrier_finalize, Barrier);
    YogBasicObj_init(env, barrier, 0, klass);

    RETURN(env, barrier);
}

void
YogInit_concurrent(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);

    YogVal cBarrier = YUNDEF;
    PUSH_LOCAL(env, cBarrier);

    YogVM* vm = env->vm;
    cBarrier = YogKlass_new(env, "Barrier", vm->cObject);
    YogKlass_define_allocator(env, cBarrier, Barrier_alloc);
    YogKlass_define_method(env, cBarrier, "initialize", Barrier_initialize, 0, 0, 0, 1, "count", NULL);
    YogKlass_define_method(env, cBarrier, "wait!", Barrier_wait, 0, 0, 0, 0, NULL);
    YogObj_set_attr(env, pkg, "Barrier", cBarrier);

    YogObj_set_attr(env, pkg, "Thread", vm->cThread);

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
