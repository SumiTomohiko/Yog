#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "yog/error.h"
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
#include "yog/sysdeps.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define MAIN_THREAD(vm)     (vm)->main_thread

typedef void (*GC)(YogEnv*);

static void
wakeup_gc_thread(YogEnv* env)
{
    DEBUG(TRACE("%p: enter wakeup_gc_thread", env));
    YogVM* vm = env->vm;
    if (pthread_cond_signal(&vm->threads_suspend_cond) != 0) {
        YOG_BUG(env, "pthread_cond_signal failed");
    }
    DEBUG(TRACE("%p: exit wakeup_gc_thread", env));
}

static void
wait_condition_variable(YogEnv* env, pthread_cond_t* cond, pthread_mutex_t* mutex)
{
    if (pthread_cond_wait(cond, mutex) != 0) {
        YOG_BUG(env, "pthread_cond_wait failed");
    }
}

static void
wait_gc_finish(YogEnv* env)
{
    DEBUG(TRACE("%p: enter wait_gc_finish", env));
    YogVM* vm = env->vm;
    uint_t id = vm->gc_id;
    while (vm->running_gc && (vm->gc_id == id)) {
        pthread_cond_t* cond = &vm->gc_finish_cond;
        pthread_mutex_t* mutex = &vm->global_interp_lock;
        wait_condition_variable(env, cond, mutex);
    }
    DEBUG(TRACE("%p: exit wait_gc_finish", env));
}

static void
decrement_suspend_counter(YogEnv* env)
{
    DEBUG(TRACE("%p: enter decrement_suspend_counter", env));
    YogVM* vm = env->vm;
    vm->suspend_counter--;
    if (vm->suspend_counter == 0) {
        wakeup_gc_thread(env);
    }
    DEBUG(TRACE("%p: exit decrement_suspend_counter", env));
}

/**
 * This function assumes that caller holds global interpreter lock.
 */
void
YogGC_suspend(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_suspend", env));
    decrement_suspend_counter(env);
    wait_gc_finish(env);
    DEBUG(TRACE("%p: exit YogGC_suspend", env));
}

YogVal
YogGC_alloc(YogEnv* env, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    DEBUG(TRACE("%p: enter YogGC_alloc: keeper=%p, finalizer=%p, size=%u", env, keeper, finalizer, size));
    YogVM* vm = env->vm;
    if (vm->waiting_suspend) {
        YogVM_acquire_global_interp_lock(env, vm);
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

    DEBUG(TRACE("%p: exit YogGC_alloc", env));
    if (ptr == NULL) {
        YogError_out_of_memory(env);
    }

    return PTR2VAL(ptr);
}

#if !defined(GC_BDW)
static void
wakeup_suspend_threads(YogEnv* env)
{
    DEBUG(TRACE("%p: enter wakeup_suspend_threads", env));
    YogVM* vm = env->vm;
    if (pthread_cond_broadcast(&vm->gc_finish_cond) != 0) {
        YOG_BUG(env, "pthread_cond_broadcast failed");
    }
    DEBUG(TRACE("%p: exit wakeup_suspend_threads", env));
}

static void
wait_suspend(YogEnv* env)
{
    DEBUG(TRACE("%p: enter wait_suspend", env));
    YogVM* vm = env->vm;
    while (vm->suspend_counter != 0) {
        pthread_cond_t* cond = &vm->threads_suspend_cond;
        pthread_mutex_t* mutex = &vm->global_interp_lock;
        wait_condition_variable(env, cond, mutex);
    }
    DEBUG(TRACE("%p: exit wait_suspend", env));
}

static uint_t
count_running_threads(YogEnv* env, YogVM* vm)
{
    DEBUG(TRACE("%p: enter count_running_threads: vm=%p", env, vm));
    uint_t n = 0;
    YogVal thread = vm->running_threads;
    while (IS_PTR(thread)) {
        n += PTR_AS(YogThread, thread)->gc_bound ? 1 : 0;
        thread = PTR_AS(YogThread, thread)->next;
    }

    DEBUG(TRACE("%p: exit count_running_threads: n=%u", env, n));
    return n;
}

static void
run_gc(YogEnv* env, GC gc)
{
    DEBUG(TRACE("%p: enter run_gc: gc=%p", env, gc));
    YogVM* vm = env->vm;
    uint_t threads_num = count_running_threads(env, vm);
    if (0 < threads_num) {
        vm->suspend_counter = threads_num - 1;
        vm->waiting_suspend = TRUE;
        wait_suspend(env);
        (*gc)(env);
        vm->waiting_suspend = FALSE;
    }
    DEBUG(TRACE("%p: exit run_gc", env));
}

static void
perform(YogEnv* env, GC gc)
{
    DEBUG(TRACE("%p: enter perform: gc=%p", env, gc));
    YogVM* vm = env->vm;
    YogVM_acquire_global_interp_lock(env, vm);
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
    DEBUG(TRACE("%p: exit perform", env));
}
#endif

static void
destroy_memory(void* p, size_t size)
{
    memset(p, 0xfd, size);
}

void
YogGC_free_memory(YogEnv* env, void* p, size_t size)
{
    destroy_memory(p, size);
    free(p);
}

#if !defined(GC_BDW)
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

    YogGC_free_memory(env, heap, sizeof(GC_TYPE));
#undef IS_EMPTY
#undef FINALIZE
}

static void
delete_heaps(YogEnv* env)
{
    GC_TYPE* heap = (GC_TYPE*)env->vm->heaps;
    while (heap != NULL) {
        GC_TYPE* next = heap->next;
        delete_heap(env, heap);
        heap = next;
    }
}
#endif

#define ITERATE_HEAPS(vm, proc)     do { \
    GC_TYPE* heap = (GC_TYPE*)(vm)->heaps; \
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

#if defined(GC_MARK_SWEEP_COMPACT)
static uint_t
count_heaps(YogEnv* env)
{
    uint_t n = 0;
    ITERATE_HEAPS(env->vm, n++);
    return n;
}

static void
init_compactors(YogEnv* env, uint_t size, YogCompactor* compactors)
{
    uint_t i;
    for (i = 0; i < size; i++) {
        YogCompactor_init(env, &compactors[i]);
    }
}

static void
do_compaction(YogEnv* env)
{
    uint_t heaps = count_heaps(env);
    YogCompactor* compactors = (YogCompactor*)YogSysdeps_alloca(sizeof(YogCompactor) * heaps);
    init_compactors(env, heaps, compactors);
#define EACH_HEAP(proc)     do { \
    GC_TYPE* heap = (GC_TYPE*)env->vm->heaps; \
    uint_t i = 0; \
    while (heap != NULL) { \
        proc; \
        heap = heap->next; \
        i++; \
    } \
} while (0)
    EACH_HEAP(YogMarkSweepCompact_alloc_virtually(env, heap, &compactors[i]));
    YogMarkSweepCompactPage** first_free_pages = (YogMarkSweepCompactPage**)YogSysdeps_alloca(sizeof(YogMarkSweepCompactPage*) * heaps);
    EACH_HEAP(first_free_pages[i] = compactors[i].next_page);

    YogVM* vm = env->vm;
    ITERATE_HEAPS(vm, YogMarkSweepCompact_prepare(env, heap));
    YogVM_keep_children(env, vm, YogMarkSweepCompact_update_ptr, THREAD_HEAP(MAIN_THREAD(vm)));
    ITERATE_HEAPS(vm, YogMarkSweepCompact_update_front_header(env, heap));

    init_compactors(env, heaps, compactors);
    EACH_HEAP(YogMarkSweepCompact_move_objs(env, heap, &compactors[i]));

    EACH_HEAP(YogMarkSweepCompact_shrink(env, heap, &compactors[i], first_free_pages[i]));
#undef EACH_HEAP
}
#endif

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
#if defined(GC_MARK_SWEEP_COMPACT)
    do_compaction(env);
#endif
}

void
YogGC_perform(YogEnv* env)
{
    perform(env, gc);
}

void
YogGC_delete(YogEnv* env)
{
    prepare(env);
    delete_garbage(env);
    post_gc(env);
    delete_heaps(env);
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
    DEBUG(TRACE("%p: enter minor_gc", env));
    prepare(env);
    minor_keep_vm(env);
    trace_grey(env);
    minor_cheney_scan(env);
    minor_delete_garbage(env);
    minor_post_gc(env);
    delete_heaps(env);
    DEBUG(TRACE("%p: exit minor_gc", env));
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
    DEBUG(TRACE("%p: enter major_gc", env));
    prepare(env);
    major_keep_vm(env);
    major_cheney_scan(env);
    major_delete_garbage(env);
    major_post_gc(env);
    delete_heaps(env);
    DEBUG(TRACE("%p: exit major_gc", env));
}

void
YogGC_delete(YogEnv* env)
{
    prepare(env);
    major_delete_garbage(env);
    major_post_gc(env);
    delete_heaps(env);
}

void
YogGC_perform_minor(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_perform_minor", env));
    perform(env, minor_gc);
    DEBUG(TRACE("%p: exit YogGC_perform_minor", env));
}

void
YogGC_perform_major(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_perform_major", env));
    perform(env, major_gc);
    DEBUG(TRACE("%p: exit YogGC_perform_major", env));
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

void
YogGC_free_from_gc(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_free_from_gc", env));
#if !defined(GC_BDW)
    YogVM* vm = env->vm;
    YogVM_acquire_global_interp_lock(env, vm);
    while (vm->waiting_suspend) {
        YogGC_suspend(env);
    }
    PTR_AS(YogThread, env->thread)->gc_bound = FALSE;
    YogVM_release_global_interp_lock(env, vm);
#endif
    DEBUG(TRACE("%p: exit YogGC_free_from_gc", env));
}

void
YogGC_bind_to_gc(YogEnv* env)
{
    DEBUG(TRACE("%p: enter YogGC_bind_to_gc", env));
#if !defined(GC_BDW)
    YogVM* vm = env->vm;
    YogVM_acquire_global_interp_lock(env, vm);
    while (vm->waiting_suspend) {
        wait_gc_finish(env);
    }
    PTR_AS(YogThread, env->thread)->gc_bound = TRUE;
    YogVM_release_global_interp_lock(env, vm);
#endif
    DEBUG(TRACE("%p: exit YogGC_bind_to_gc", env));
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
