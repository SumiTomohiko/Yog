#include <pthread.h>
#include <sys/types.h>
#include "yog/env.h"
#if defined(GC_COPYING)
#   include "yog/gc/copying.h"
#elif defined(GC_MARK_SWEEP)
#   include "yog/gc/mark-sweep.h"
#elif defined(GC_MARK_SWEEP_COMPACT)
#   include "yog/gc/mark-sweep-compact.h"
#elif defined(GC_GENERATIONAL)
#   include "yog/gc/generational.h"
#elif defined(GC_BDW)
#   include "yog/gc/bdw.h"
#endif
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static void 
wakeup_gc_thread(YogEnv* env) 
{
    YogVm* vm = env->vm;
    pthread_cond_signal(&vm->threads_suspend_cond);
}

static void 
wait_gc_finish(YogEnv* env) 
{
    YogVm* vm = env->vm;
    while (vm->running_gc) {
        pthread_cond_wait(&vm->gc_finish_cond, &vm->global_interp_lock);
    }
}

YogVal 
YogGC_allocate(YogEnv* env, ChildrenKeeper keeper, Finalizer finalizer, size_t size) 
{
    YogVm* vm = env->vm;
    if (vm->waiting_suspend) {
        YogVm_aquire_global_interp_lock(env, vm);
        vm->suspend_counter--;
        if (vm->suspend_counter == 0) {
            wakeup_gc_thread(env);
        }
        wait_gc_finish(env);
        YogVm_release_global_interp_lock(env, vm);
    }

    YogVal thread = env->thread;
#if defined(GC_COPYING)
#   define GC       &PTR_AS(YogThread, thread)->copying
#   define ALLOC    YogCopying_alloc
#elif defined(GC_MARK_SWEEP)
#   define GC       &PTR_AS(YogThread, thread)->mark_sweep
#   define ALLOC    YogMarkSweep_alloc
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define GC       &PTR_AS(YogThread, thread)->mark_sweep_compact
#   define ALLOC    YogMarkSweepCompact_alloc
#elif defined(GC_GENERATIONAL)
#   define GC       &PTR_AS(YogThread, thread)->generational
#   define ALLOC    YogGenerational_alloc
#elif defined(GC_BDW)
#   define GC       &PTR_AS(YogThread, thread)->bdw
#   define ALLOC    YogBDW_alloc
#endif
    void* ptr = ALLOC(env, GC, keeper, finalizer, size);
#undef ALLOC
#undef GC

    if (ptr != NULL) {
        return PTR2VAL(ptr);
    }
    else {
        return YNIL;
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
