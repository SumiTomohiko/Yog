#include <pthread.h>
#include <string.h>
#include "yog/array.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/object.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Barrier {
    YOGBASICOBJ_HEAD;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned int counter;
};

typedef struct Barrier Barrier;

#define TYPE_BARRIER    ((type_t)Barrier_alloc)

static YogVal
Barrier_init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    if (pthread_mutex_init(&PTR_AS(Barrier, self)->mutex, NULL) != 0) {
        YOG_BUG(env, "pthread_mutex_init failed");
    }
    if (pthread_cond_init(&PTR_AS(Barrier, self)->cond, NULL) != 0) {
        YOG_BUG(env, "pthread_cond_init failed");
    }

    YogVal count = YogArray_at(env, args, 0);
    /* TODO: check type */
    PTR_AS(Barrier, self)->counter = count;

    RETURN(env, self);
}

static YogVal
Barrier_wait(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    pthread_mutex_t* mutex = &PTR_AS(Barrier, self)->mutex;
    int err;
    if ((err = pthread_mutex_lock(mutex)) != 0) {
        YOG_BUG(env, "pthread_mutex_lock failed: %s", strerror(err));
    }
    PTR_AS(Barrier, self)->counter--;
    while (PTR_AS(Barrier, self)->counter == 0) {
        pthread_cond_t* cond = &PTR_AS(Barrier, self)->cond;
        FREE_FROM_GC(env);
        pthread_cond_wait(cond, mutex);
        BIND_TO_GC(env);
    }
    if (pthread_mutex_unlock(mutex) != 0) {
        YOG_BUG(env, "pthread_mutex_lock failed");
    }

    RETURN(env, self);
}

static void
Barrier_finalize(YogEnv* env, void* ptr)
{
    Barrier* barrier = ptr;

    if (pthread_mutex_destroy(&barrier->mutex) != 0) {
        YOG_WARN(env, "pthread_mutex_destroy failed");
    }
    if (pthread_cond_destroy(&barrier->cond) != 0) {
        YOG_WARN(env, "pthread_cond_destroy failed");
    }
}

static void
Barrier_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);
}

static YogVal
Barrier_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal barrier = YUNDEF;
    PUSH_LOCAL(env, barrier);

    barrier = ALLOC_OBJ(env, Barrier_keep_children, Barrier_finalize, Barrier);
    YogBasicObj_init(env, barrier, TYPE_BARRIER, 0, klass);

    RETURN(env, barrier);
}

struct AtomicInt {
    YOGBASICOBJ_HEAD;
    int value;
};

typedef struct AtomicInt AtomicInt;

#define TYPE_ATOMIC_INT     ((type_t)AtomicInt_alloc)

static void
AtomicInt_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);
}

static YogVal
AtomicInt_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal atomic_int = YUNDEF;
    PUSH_LOCAL(env, atomic_int);

    atomic_int = ALLOC_OBJ(env, AtomicInt_keep_children, NULL, AtomicInt);
    YogBasicObj_init(env, atomic_int, TYPE_ATOMIC_INT, 0, klass);
    PTR_AS(AtomicInt, atomic_int)->value = 0;

    RETURN(env, atomic_int);
}

static YogVal
AtomicInt_init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogVal value = YogArray_at(env, args, 0);
    /* TODO: check type */
    PTR_AS(AtomicInt, self)->value = VAL2INT(value);

    RETURN(env, self);
}

static YogVal
AtomicInt_inc(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    __asm__ __volatile__("lock incl %0": "+m" (PTR_AS(AtomicInt, self)->value));
    RETURN(env, self);
}

static YogVal
AtomicInt_get(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    return INT2VAL(PTR_AS(AtomicInt, self)->value);
}

void
YogInit_concurrent(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cBarrier = YUNDEF;
    YogVal cAtomicInt = YUNDEF;
    PUSH_LOCALS2(env, cBarrier, cAtomicInt);

    YogVM* vm = env->vm;
    cBarrier = YogClass_new(env, "Barrier", vm->cObject);
    YogClass_define_allocator(env, cBarrier, Barrier_alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cBarrier, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("init", Barrier_init);
    DEFINE_METHOD("wait!", Barrier_wait);
#undef DEFINE_METHOD
    YogObj_set_attr(env, pkg, "Barrier", cBarrier);

    cAtomicInt = YogClass_new(env, "AtomicInt", vm->cObject);
    YogClass_define_allocator(env, cAtomicInt, AtomicInt_alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cAtomicInt, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("init", AtomicInt_init);
    DEFINE_METHOD("inc!", AtomicInt_inc);
    DEFINE_METHOD("get", AtomicInt_get);
#undef DEFINE_METHOD
    YogObj_set_attr(env, pkg, "AtomicInt", cAtomicInt);

    YogObj_set_attr(env, pkg, "Thread", vm->cThread);

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
