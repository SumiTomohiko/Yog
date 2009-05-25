#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "yog/env.h"
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
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define MAIN_THREAD(vm)     (vm)->main_thread

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

#define SIGSUSPEND  SIGUSR1
#define SIGRESTART  SIGUSR2

typedef void (*GC)(YogEnv*);

static pthread_mutex_t global_gc_lock;
static YogVm* gc_vm;

YogVal 
YogGC_allocate(YogEnv* env, ChildrenKeeper keeper, Finalizer finalizer, size_t size) 
{
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
static void
aquire_global_gc_lock(YogEnv* env)
{
    if (pthread_mutex_lock(&global_gc_lock) != 0) {
        YOG_BUG(env, "pthread_mutex_lock failed");
    }
}

static void
release_global_gc_lock(YogEnv* env)
{
    if (pthread_mutex_unlock(&global_gc_lock) != 0) {
        YOG_BUG(env, "pthread_mutex_unlock failed");
    }
}

static void
set_gc_vm(YogEnv* env, YogVm* vm)
{
    gc_vm = vm;
}

static void
send_signal(YogEnv* env, YogVal thread, int signum)
{
    pthread_t target = PTR_AS(YogThread, thread)->pthread;
    pthread_t self = PTR_AS(YogThread, env->thread)->pthread;
    if (pthread_equal(self, target)) {
        return;
    }
    if (pthread_kill(target, signum) != 0) {
        YOG_BUG(env, "pthread_kill failed");
    }
}

static void
send_signal_all(YogEnv* env, int signum)
{
    YogVal thread = env->vm->running_threads;
    while (IS_PTR(thread)) {
        send_signal(env, thread, signum);
        thread = PTR_AS(YogThread, thread)->next;
    }
}

static void
suspend_all(YogEnv* env)
{
    send_signal_all(env, SIGSUSPEND);
}

static void
wait(YogEnv* env, YogVal thread)
{
    pthread_t target = PTR_AS(YogThread, thread)->pthread;
    pthread_t self = PTR_AS(YogThread, env->thread)->pthread;
    if (pthread_equal(self, target)) {
        return;
    }
    if (sem_wait(&env->vm->suspend_sem) != 0) {
        YOG_BUG(env, "sem_wait failed");
    }
}

static void
wait_all(YogEnv* env)
{
    YogVal thread = env->vm->running_threads;
    while (IS_PTR(thread)) {
        wait(env, thread);
        thread = PTR_AS(YogThread, thread)->next;
    }
}

static void
stop_world(YogEnv* env)
{
    aquire_global_gc_lock(env);
    set_gc_vm(env, env->vm);
    suspend_all(env);
    wait_all(env);
    release_global_gc_lock(env);
}

static void
restart_world(YogEnv* env)
{
    send_signal_all(env, SIGRESTART);
}

static void 
perform(YogEnv* env, GC gc) 
{
    YogVm* vm = env->vm;
    YogVm_aquire_global_interp_lock(env, vm);

    stop_world(env);
    (*gc)(env);
    restart_world(env);

    YogVm_release_global_interp_lock(env, vm);
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

    YogVm* vm = env->vm;
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
    if (!IS_PTR(main_thread)) {
        return;
    }
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
#   define GET_GEN(thread)  PTR_AS(YogThread, (thread))->THREAD_GC
static void
prepare(YogEnv* env)
{
    ITERATE_HEAPS(env->vm, YogGenerational_prepare(env, heap));
}

static void
minor_keep_vm(YogEnv* env)
{
    YogVal main_thread = MAIN_THREAD(env->vm);
    if (!IS_PTR(main_thread)) {
        return;
    }
    YogGenerational_minor_keep_vm(env, GET_GEN(main_thread));
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
    if (!IS_PTR(main_thread)) {
        return;
    }
    YogGenerational_major_keep_vm(env, GET_GEN(main_thread));
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

static void
make_signal_set(sigset_t* set)
{
    sigfillset(set);
#define DELETE_SIGNAL(signum)   sigdelset(set, signum)
    DELETE_SIGNAL(SIGINT);
    DELETE_SIGNAL(SIGQUIT);
    DELETE_SIGNAL(SIGABRT);
    DELETE_SIGNAL(SIGTERM);
#undef DELETE_SIGNAL
}

static void
sigsuspend_handler(int sig, siginfo_t* si, void* unused)
{
    sem_post(&gc_vm->suspend_sem);

    sigset_t set;
    make_signal_set(&set);
    sigdelset(&set, SIGRESTART);
    sigsuspend(&set);
}

static void
sigrestart_handler(int sig, siginfo_t* si, void* unused)
{
    /* do nothing */
}

void
YogGC_initialize(YogEnv* env)
{
    sigset_t set;
    make_signal_set(&set);

    struct sigaction act;
    act.sa_mask = set;
    act.sa_flags = SA_SIGINFO;
#define SIGACTION(signum, handler)  do { \
    act.sa_sigaction = handler; \
    if (sigaction(signum, &act, NULL) != 0) { \
        YOG_BUG(env, "sigaction failed"); \
    } \
} while (0)
    SIGACTION(SIGSUSPEND, sigsuspend_handler);
    SIGACTION(SIGRESTART, sigrestart_handler);
#undef SIGACTION

    pthread_mutex_init(&global_gc_lock, NULL);

    gc_vm = NULL;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
