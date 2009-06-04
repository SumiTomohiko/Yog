#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "yog/env.h"
#include "yog/gc.h"
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
#include "yog/misc.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define MAIN_THREAD(vm)     (vm)->main_thread

typedef void (*GC)(YogEnv*);

static void 
wakeup_gc_thread(YogEnv* env) 
{
    YogVM* vm = env->vm;
    pthread_cond_signal(&vm->threads_suspend_cond);
}

static void 
wait_gc_finish(YogEnv* env) 
{
    YogVM* vm = env->vm;
    unsigned int id = vm->gc_id;
    while (vm->running_gc && (vm->gc_id == id)) {
        pthread_cond_wait(&vm->gc_finish_cond, &vm->global_interp_lock);
    }
}

static void
decrement_suspend_counter(YogEnv* env)
{
    YogVM* vm = env->vm;
    vm->suspend_counter--;
    if (vm->suspend_counter == 0) {
        wakeup_gc_thread(env);
    }
}

/**
 * This function assumes that caller holds global interpreter lock.
 */
void 
YogGC_suspend(YogEnv* env) 
{
    decrement_suspend_counter(env);
    wait_gc_finish(env);
}

YogVal 
YogGC_allocate(YogEnv* env, ChildrenKeeper keeper, Finalizer finalizer, size_t size) 
{
    YogVM* vm = env->vm;
    if (vm->waiting_suspend) {
        YogVM_aquire_global_interp_lock(env, vm);
        YogGC_suspend(env);
        YogVM_release_global_interp_lock(env, vm);
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
    void* ptr = ALLOC(env, THREAD_HEAP(thread), keeper, finalizer, size);
#undef ALLOC

    if (ptr != NULL) {
        return PTR2VAL(ptr);
    }
    else {
        return YNIL;
    }
}

#if !defined(GC_BDW)
static void 
wakeup_suspend_threads(YogEnv* env) 
{
    YogVM* vm = env->vm;
    pthread_cond_signal(&vm->gc_finish_cond);
}

static void 
wait_suspend(YogEnv* env) 
{
    YogVM* vm = env->vm;
    while (vm->suspend_counter != 0) {
        pthread_cond_wait(&vm->threads_suspend_cond, &vm->global_interp_lock);
    }
}

static unsigned int
count_running_threads(YogEnv* env, YogVM* vm)
{
    unsigned int n = 0;
    YogVal thread = vm->running_threads;
    while (IS_PTR(thread)) {
        n += PTR_AS(YogThread, thread)->gc_bound ? 1 : 0;
        thread = PTR_AS(YogThread, thread)->next;
    }

    return n;
}

static void 
run_gc(YogEnv* env, GC gc)
{
    YogVM* vm = env->vm;
    unsigned int threads_num = count_running_threads(env, vm);
    if (0 < threads_num) {
        vm->suspend_counter = threads_num - 1;
        vm->waiting_suspend = TRUE;
        wait_suspend(env);
        (*gc)(env);
        vm->waiting_suspend = FALSE;
    }
}

void
YogGC_free_from_gc(YogEnv* env)
{
    YogVM* vm = env->vm;
    YogVM_aquire_global_interp_lock(env, vm);
    while (vm->waiting_suspend) {
        YogGC_suspend(env);
    }
    PTR_AS(YogThread, env->thread)->gc_bound = FALSE;
    YogVM_release_global_interp_lock(env, vm);
}

void
YogGC_bind_to_gc(YogEnv* env)
{
    YogVM* vm = env->vm;
    YogVM_aquire_global_interp_lock(env, vm);
    while (vm->waiting_suspend) {
        wait_gc_finish(env);
    }
    PTR_AS(YogThread, env->thread)->gc_bound = TRUE;
    YogVM_release_global_interp_lock(env, vm);
}

static void 
perform(YogEnv* env, GC gc) 
{
    YogVM* vm = env->vm;
    YogVM_aquire_global_interp_lock(env, vm);
    if (vm->waiting_suspend) {
        YogGC_suspend(env);
    }
    else {
        vm->running_gc = TRUE;
        run_gc(env, gc);
        vm->running_gc = FALSE;
        wakeup_suspend_threads(env);
        vm->gc_id++;
    }
    YogVM_release_global_interp_lock(env, vm);
}
#endif

#if !defined(GC_BDW)
static void
destroy_memory(void* p, size_t size)
{
    memset(p, 0xfd, size);
}

static void
free_memory(void* p, size_t size)
{
    destroy_memory(p, size);
    free(p);
}

static void
delete_heap(YogEnv* env, GC_TYPE* heap)
{
    if (heap->refered) {
        return;
    }

#if defined(GC_COPYING)
#   define FINALIZE     YogCopying_finalize
#   define IS_EMPTY     YogCopying_is_empty
#elif defined(GC_MARK_SWEEP)
#   define FINALIZE     YogMarkSweep_finalize
#   define IS_EMPTY     YogMarkSweep_is_empty
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define FINALIZE     YogMarkSweepCompact_finalize
#   define IS_EMPTY     YogMarkSweepCompact_is_empty
#elif defined(GC_GENERATIONAL)
#   define FINALIZE     YogGenerational_finalize
#   define IS_EMPTY     YogGenerational_is_empty
#endif
    if (!IS_EMPTY(env, heap)) {
        return;
    }

    FINALIZE(env, heap);

    YogVM* vm = env->vm;
    if (vm->last_heap == heap) {
        vm->last_heap = heap->prev;
    }
    DELETE_FROM_LIST(env->vm->heaps, heap);

    free_memory(heap, sizeof(GC_TYPE));
#undef IS_EMPTY
#undef FINALIZE
}

static void
delete_heaps(YogEnv* env)
{
    GC_TYPE* heap = env->vm->heaps;
    while (heap != NULL) {
        GC_TYPE* next = heap->next;
        delete_heap(env, heap);
        heap = next;
    }
}
#endif

#define ITERATE_HEAPS(vm, proc)     do { \
    GC_TYPE* heap = (vm)->heaps; \
    while (heap != NULL) { \
        proc; \
        heap = heap->next; \
    }; \
} while (0)

#if defined(GC_COPYING) || defined(GC_MARK_SWEEP) || defined(GC_MARK_SWEEP_COMPACT)
static void 
prepare(YogEnv* env) 
{
#if defined(GC_COPYING)
#   define PREPARE  YogCopying_prepare
#elif defined(GC_MARK_SWEEP)
#   define PREPARE  YogMarkSweep_prepare
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define PREPARE  YogMarkSweepCompact_prepare
#endif
    ITERATE_HEAPS(env->vm, PREPARE(env, heap));
#undef PREPARE
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
    KEEP(env, THREAD_HEAP(main_thread));
#undef KEEP
}

#if defined(GC_COPYING)
static void 
cheney_scan(YogEnv* env) 
{
    ITERATE_HEAPS(env->vm, YogCopying_cheney_scan(env, heap));
}
#endif

static void
delete_garbage(YogEnv* env)
{
#if defined(GC_COPYING)
#   define DELETE   YogCopying_delete_garbage
#elif defined(GC_MARK_SWEEP)
#   define DELETE   YogMarkSweep_delete_garbage
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define DELETE   YogMarkSweepCompact_delete_garbage
#endif
    ITERATE_HEAPS(env->vm, DELETE(env, heap));
#undef DELETE
}

static void
post_gc(YogEnv* env)
{
#if defined(GC_COPYING)
#   define POST     YogCopying_post_gc
#elif defined(GC_MARK_SWEEP)
#   define POST     YogMarkSweep_post_gc
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define POST     YogMarkSweepCompact_post_gc
#endif
    ITERATE_HEAPS(env->vm, POST(env, heap));
#undef POST
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
    delete_heaps(env);
}

void 
YogGC_perform(YogEnv* env) 
{
    perform(env, gc);
}
#endif

#if defined(GC_GENERATIONAL)
static void
prepare(YogEnv* env)
{
    ITERATE_HEAPS(env->vm, YogGenerational_prepare(env, heap));
}

static void
minor_keep_vm(YogEnv* env)
{
    YogVal main_thread = MAIN_THREAD(env->vm);
    YogGenerational_minor_keep_vm(env, THREAD_HEAP(main_thread));
}

static void
minor_cheney_scan(YogEnv* env)
{
    ITERATE_HEAPS(env->vm, YogGenerational_minor_cheney_scan(env, heap));
}

static void
trace_grey(YogEnv* env)
{
    ITERATE_HEAPS(env->vm, YogGenerational_trace_grey(env, heap));
}

static void
minor_delete_garbage(YogEnv* env)
{
    ITERATE_HEAPS(env->vm, YogGenerational_minor_delete_garbage(env, heap));
}

static void
minor_post_gc(YogEnv* env)
{
    ITERATE_HEAPS(env->vm, YogGenerational_minor_post_gc(env, heap));
}

static void 
minor_gc(YogEnv* env) 
{
    prepare(env);
    minor_keep_vm(env);
    trace_grey(env);
    minor_cheney_scan(env);
    minor_delete_garbage(env);
    minor_post_gc(env);
    delete_heaps(env);
}

static void
major_keep_vm(YogEnv* env)
{
    YogVal main_thread = MAIN_THREAD(env->vm);
    YogGenerational_major_keep_vm(env, THREAD_HEAP(main_thread));
}

static void
major_cheney_scan(YogEnv* env)
{
    ITERATE_HEAPS(env->vm, YogGenerational_major_cheney_scan(env, heap));
}

static void
major_delete_garbage(YogEnv* env)
{
    ITERATE_HEAPS(env->vm, YogGenerational_major_delete_garbage(env, heap));
}

static void
major_post_gc(YogEnv* env)
{
    ITERATE_HEAPS(env->vm, YogGenerational_major_post_gc(env, heap));
}

static void 
major_gc(YogEnv* env) 
{
    prepare(env);
    major_keep_vm(env);
    major_cheney_scan(env);
    major_delete_garbage(env);
    major_post_gc(env);
    delete_heaps(env);
}

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
