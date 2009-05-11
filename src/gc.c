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

#define MAIN_THREAD(vm)     (vm)->threads

#define GC_OF(thread, type)     PTR_AS(YogThread, (thread))->type
#if defined(GC_COPYING)
#   define GET_GC(thread)       GC_OF(thread, copying)
#elif defined(GC_MARK_SWEEP)
#   define GET_GC(thread)       GC_OF(thread, mark_sweep)
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define GET_GC(thread)       GC_OF(thread, mark_sweep_compact)
#elif defined(GC_GENERATIONAL)
#   define GET_GC(thread)       GC_OF(thread, generational)
#elif defined(GC_BDW)
#   define GET_GC(thread)       GC_OF(thread, bdw)
#endif

typedef void (*GC)(YogEnv*);

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

/**
 * This function assumes that caller holds global interpreter lock.
 */
void 
YogGC_suspend(YogEnv* env) 
{
    YogVm* vm = env->vm;
    vm->suspend_counter--;
    if (vm->suspend_counter == 0) {
        wakeup_gc_thread(env);
    }
    wait_gc_finish(env);
}

YogVal 
YogGC_allocate(YogEnv* env, ChildrenKeeper keeper, Finalizer finalizer, size_t size) 
{
    YogVm* vm = env->vm;
    if (vm->waiting_suspend) {
        YogVm_aquire_global_interp_lock(env, vm);
        YogGC_suspend(env);
        YogVm_release_global_interp_lock(env, vm);
    }

    YogVal thread = env->thread;
#if defined(GC_COPYING)
#   define ALLOC    YogCopying_alloc
#elif defined(GC_MARK_SWEEP)
#   define ALLOC    YogMarkSweep_alloc
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define ALLOC    YogMarkSweepCompact_alloc
#elif defined(GC_GENERATIONAL)
#   define ALLOC    YogGenerational_alloc
#elif defined(GC_BDW)
#   define ALLOC    YogBDW_alloc
#endif
    void* ptr = ALLOC(env, GET_GC(thread), keeper, finalizer, size);
#undef ALLOC

    if (ptr != NULL) {
        return PTR2VAL(ptr);
    }
    else {
        return YNIL;
    }
}

#if !defined(GC_BDW)
static unsigned int 
count_threads(YogEnv* env, YogVm* vm) 
{
    unsigned int n = 0;
    YogVal thread = vm->threads;
    while (IS_PTR(thread)) {
        n++;
        thread = PTR_AS(YogThread, thread)->next;
    }

    return n;
}

static void 
wakeup_suspend_threads(YogEnv* env) 
{
    YogVm* vm = env->vm;
    pthread_cond_signal(&vm->gc_finish_cond);
}

static void 
wait_suspend(YogEnv* env) 
{
    YogVm* vm = env->vm;
    while (vm->suspend_counter != 0) {
        pthread_cond_wait(&vm->threads_suspend_cond, &vm->global_interp_lock);
    }
}

static void 
perform(YogEnv* env, GC gc) 
{
    YogVm* vm = env->vm;
    YogVm_aquire_global_interp_lock(env, vm);
    if (vm->waiting_suspend) {
        YogGC_suspend(env);
    }
    else {
        vm->running_gc = TRUE;
        unsigned int threads_num = count_threads(env, vm);
        if (0 < threads_num) {
            vm->suspend_counter = threads_num - 1;
            vm->waiting_suspend = TRUE;
            wait_suspend(env);
            (*gc)(env);
            vm->waiting_suspend = FALSE;
        }
        vm->running_gc = FALSE;
        wakeup_suspend_threads(env);
    }
    YogVm_release_global_interp_lock(env, vm);
}
#endif

#if defined(GC_COPYING) || defined(GC_MARK_SWEEP) || defined(GC_MARK_SWEEP_COMPACT)
static void 
prepare(YogEnv* env) 
{
    YogVal thread = env->vm->threads;
    while (IS_PTR(thread)) {
#if defined(GC_COPYING)
#   define PREPARE  YogCopying_prepare
#elif defined(GC_MARK_SWEEP)
#   define PREPARE  YogMarkSweep_prepare
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define PREPARE  YogMarkSweepCompact_prepare
#endif
        PREPARE(env, GET_GC(thread));
#undef PREPARE

        thread = PTR_AS(YogThread, thread)->next;
    }
}

static void 
keep_vm(YogEnv* env) 
{
    YogVal main_thread = MAIN_THREAD(env->vm);
#if defined(GC_COPYING)
#   define KEEP     YogCopying_keep_vm
#elif defined(GC_MARK_SWEEP)
#   define KEEP     YogMarkSweep_keep_vm
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define KEEP     YogMarkSweepCompact_keep_vm
#endif
    KEEP(env, GET_GC(main_thread));
#undef KEEP
}

#if defined(GC_COPYING)
static void 
cheney_scan(YogEnv* env) 
{
    YogVal thread = env->vm->threads;
    while (IS_PTR(thread)) {
        YogCopying_cheney_scan(env, GET_GC(thread));
        thread = PTR_AS(YogThread, thread)->next;
    }
}
#endif

static void
delete_garbage(YogEnv* env)
{
    YogVal thread = env->vm->threads;
    while (IS_PTR(thread)) {
#if defined(GC_COPYING)
#   define DELETE   YogCopying_delete_garbage
#elif defined(GC_MARK_SWEEP)
#   define DELETE   YogMarkSweep_delete_garbage
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define DELETE   YogMarkSweepCompact_delete_garbage
#endif
        DELETE(env, GET_GC(thread));
#undef DELETE
        thread = PTR_AS(YogThread, thread)->next;
    }
}

static void
post_gc(YogEnv* env)
{
    YogVal thread = env->vm->threads;
    while (IS_PTR(thread)) {
#if defined(GC_COPYING)
#   define POST     YogCopying_post_gc
#elif defined(GC_MARK_SWEEP)
#   define POST     YogMarkSweep_post_gc
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define POST     YogMarkSweepCompact_post_gc
#endif
        POST(env, GET_GC(thread));
#undef POST
        thread = PTR_AS(YogThread, thread)->next;
    }
}

static void 
gc(YogEnv* env) 
{
    prepare(env);
    keep_vm(env);
#if defined(GC_COPYING)
    cheney_scan(env);
#endif
    delete_garbage(env);
    post_gc(env);
}

void 
YogGC_perform(YogEnv* env) 
{
    perform(env, gc);
}
#endif

#if defined(GC_GENERATIONAL)
#   define GET_GEN(thread)  PTR_AS(YogThread, (thread))->generational
static void 
minor_gc(YogEnv* env) 
{
    YogVal main_thread = MAIN_THREAD(env->vm);
    YogGenerational* gen = GET_GEN(main_thread);
    YogGenerational_minor_gc(env, gen);
}

static void 
major_gc(YogEnv* env) 
{
    YogVal main_thread = MAIN_THREAD(env->vm);
    YogGenerational* gen = GET_GEN(main_thread);
    YogGenerational_major_gc(env, gen);
}
#   undef GET_GEN

void 
YogGC_perform_minor(YogEnv* env) 
{
    perform(env, minor_gc);
}

void 
YogGC_perform_major(YogEnv* env) 
{
    perform(env, major_gc);
}
#endif

void 
YogGC_keep(YogEnv* env, YogVal* val, ObjectKeeper keeper, void* heap) 
{
    if (!IS_PTR(*val)) {
        return;
    }

    *val = PTR2VAL((*keeper)(env, VAL2PTR(*val), heap));
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
