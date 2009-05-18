#include <stdlib.h>
#include <string.h>
#if HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
#if defined(GC_BDW)
#   include "gc.h"
#endif
#include "yog/array.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
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
#include "yog/klass.h"
#include "yog/thread.h"
#include "yog/yog.h"

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogThread* thread = ptr;

#define KEEP(member)    YogGC_keep(env, &thread->member, keeper, heap)
    KEEP(prev);
    KEEP(next);
#undef KEEP

    void* thread_heap = thread->THREAD_GC;
#define KEEP(member)    YogGC_keep(env, &thread->member, keeper, thread_heap)
    KEEP(cur_frame);
    KEEP(jmp_val);
    KEEP(block);
#undef KEEP
}

void 
YogThread_initialize(YogEnv* env, YogVal thread, YogVal klass)
{
    YogBasicObj_init(env, thread, 0, klass);

    PTR_AS(YogThread, thread)->prev = YUNDEF;
    PTR_AS(YogThread, thread)->next = YUNDEF;

#if defined(GC_COPYING)
#   define GC   copying
#elif defined(GC_MARK_SWEEP)
#   define GC   mark_sweep
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define GC   mark_sweep_compact
#elif defined(GC_GENERATIONAL)
#   define GC   generational
#elif defined(GC_BDW)
#   define GC   bdw
#endif
    PTR_AS(YogThread, thread)->GC = NULL;
#undef GC

    PTR_AS(YogThread, thread)->cur_frame = YNIL;
    PTR_AS(YogThread, thread)->jmp_buf_list = NULL;
    PTR_AS(YogThread, thread)->jmp_val = YUNDEF;
    PTR_AS(YogThread, thread)->locals = NULL;

    PTR_AS(YogThread, thread)->block = YUNDEF;
    PTR_AS(YogThread, thread)->pthread = NULL;
}

#if defined(GC_COPYING)
void 
YogThread_config_copying(YogEnv* env, YogVal thread, BOOL gc_stress, size_t init_heap_size, void* root, ChildrenKeeper root_keeper) 
{
    YogCopying* copying = malloc(sizeof(YogCopying));
    YOG_ASSERT(env, copying != NULL, "Can't allocate YogCopying");
    YogCopying_initialize(env, copying, gc_stress, init_heap_size, root, root_keeper);
    copying->refered = TRUE;
    YogVm_add_heap(env, env->vm, copying);

    PTR_AS(YogThread, thread)->copying = copying;
}
#endif

#if defined(GC_MARK_SWEEP)
void 
YogThread_config_mark_sweep(YogEnv* env, YogVal thread, size_t threshold, void* root, ChildrenKeeper root_keeper) 
{
    YogMarkSweep* mark_sweep = malloc(sizeof(YogMarkSweep));
    YOG_ASSERT(env, mark_sweep != NULL, "Can't allocate YogMarkSweep");
    YogMarkSweep_initialize(env, mark_sweep, threshold, root, root_keeper);
    mark_sweep->refered = TRUE;
    YogVm_add_heap(env, env->vm, mark_sweep);

    PTR_AS(YogThread, thread)->mark_sweep = mark_sweep;
}
#endif

#if defined(GC_MARK_SWEEP_COMPACT)
void 
YogThread_config_mark_sweep_compact(YogEnv* env, YogVal thread, size_t chunk_size, size_t threshold, void* root, ChildrenKeeper root_keeper) 
{
    size_t size = sizeof(YogMarkSweepCompact);
    YogMarkSweepCompact* mark_sweep_compact = malloc(size);
    YOG_ASSERT(env, mark_sweep_compact != NULL, "Can't allocate YogMarkSweepCompact");
    YogMarkSweepCompact_initialize(env, mark_sweep_compact, chunk_size, threshold, root, root_keeper);
    mark_sweep_compact->refered = TRUE;
    YogVm_add_heap(env, env->vm, mark_sweep_compact);

    PTR_AS(YogThread, thread)->mark_sweep_compact = mark_sweep_compact;
}
#endif

#if defined(GC_GENERATIONAL)
void 
YogThread_config_generational(YogEnv* env, YogVal thread, BOOL gc_stress, size_t young_heap_size, size_t old_chunk_size, size_t old_threshold, unsigned int tenure, void* root, ChildrenKeeper root_keeper) 
{
    YogGenerational* generational = malloc(sizeof(YogGenerational));
    YOG_ASSERT(env, generational != NULL, "Can't allocate YogGenerational");
    YogGenerational_initialize(env, generational, gc_stress, young_heap_size, old_chunk_size, old_threshold, tenure, root, root_keeper);
    generational->refered = TRUE;
    YogVm_add_heap(env, env->vm, generational);

    PTR_AS(YogThread, thread)->generational = generational;
}
#endif

#if defined(GC_BDW)
void 
YogThread_config_bdw(YogEnv* env, YogVal thread, BOOL gc_stress) 
{
    YogBDW* bdw = GC_MALLOC(sizeof(YogBDW));
    YogBDW_initialize(env, bdw, gc_stress);

    PTR_AS(YogThread, thread)->bdw = bdw;
}
#endif

static void
finalize(YogEnv* env, void* ptr)
{
    YogThread* thread = ptr;
#if !defined(GC_BDW)
    GC_TYPE* heap = thread->THREAD_GC;
    if (heap != NULL) {
        heap->refered = FALSE;
    }
#endif
    thread->THREAD_GC = NULL;
}

static YogVal 
allocate(YogEnv* env, YogVal klass) 
{
    SAVE_ARG(env, klass);

    YogVal thread = ALLOC_OBJ(env, keep_children, finalize, YogThread);
    YogThread_initialize(env, thread, klass);

    RETURN(env, thread);
}

YogVal 
YogThread_new(YogEnv* env) 
{
    return allocate(env, env->vm->cThread);
}

struct ThreadArg {
    struct YogVm* vm;
    YogVal thread;
    YogVal vararg;
};

typedef struct ThreadArg ThreadArg;

static void
ThreadArg_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    ThreadArg* arg = ptr;
#define KEEP(member)    YogGC_keep(env, &arg->member, keeper, heap)
    KEEP(thread);
    KEEP(vararg);
#undef KEEP
}

static YogVal
ThreadArg_new(YogEnv* env)
{
    YogVal arg = ALLOC_OBJ(env, ThreadArg_keep_children, NULL, ThreadArg);
    PTR_AS(ThreadArg, arg)->vm = NULL;
    PTR_AS(ThreadArg, arg)->thread = YUNDEF;
    PTR_AS(ThreadArg, arg)->vararg = YUNDEF;
    return arg;
}

static void*
run_of_new_thread(void* arg)
{
    ThreadArg* thread_arg = arg;
    YogVal thread = thread_arg->thread;
    YogEnv env;
    env.vm = thread_arg->vm;
    env.thread = thread;
    SAVE_LOCALS(&env);
    YogLocals locals0;
    locals0.num_vals = 1;
    locals0.size = 1;
    locals0.vals[0] = &env.thread;
    locals0.vals[1] = NULL;
    locals0.vals[2] = NULL;
    locals0.vals[3] = NULL;
    locals0.vals[4] = NULL;
    PUSH_LOCAL_TABLE(&env, locals0);

    unsigned int size = YogArray_size(&env, thread_arg->vararg);
    YogVal args[size];
    YogVal body = PTR_AS(YogArray, thread_arg->vararg)->body;
    memcpy(args, PTR_AS(YogValArray, body)->items, size);
    PUSH_LOCALSX(&env, size, args);

    YogVal block = PTR_AS(YogThread, thread)->block;
    YogVal retval = YogEval_call_block(&env, block, size, args);

    YogVm_remove_thread(&env, env.vm, thread);

    RETURN(&env, (void*)retval);
}

static YogVal
run(YogEnv* env)
{
    SAVE_LOCALS(env);

    YogVal self = SELF(env);
    YogVal vararg = ARG(env, 0);
    YogVal arg = YUNDEF;
    PUSH_LOCALS3(env, self, vararg, arg);

    pthread_t* pthread = malloc(sizeof(pthread_t));
    YOG_ASSERT(env, pthread != NULL, "Can't allocate pthread_t");
    PTR_AS(YogThread, self)->pthread = pthread;

    arg = ThreadArg_new(env);
    PTR_AS(ThreadArg, arg)->vm = env->vm;
    PTR_AS(ThreadArg, arg)->thread = self;
    PTR_AS(ThreadArg, arg)->vararg = vararg;

    YogVm_add_thread(env, env->vm, self);
    if (pthread_create(pthread, NULL, run_of_new_thread, (void*)arg) != 0) {
        YOG_BUG(env, "Can't create new thread");
    }

    RETURN(env, self);
}

static YogVal
initialize(YogEnv* env)
{
    YogVal self = SELF(env);
    YogVal block = ARG(env, 0);
    PTR_AS(YogThread, self)->block = block;
    return self;
}

YogVal
YogThread_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);

    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Thread", env->vm->cObject);
    YogKlass_define_allocator(env, klass, allocate);
    YogKlass_define_method(env, klass, "initialize", initialize, 1, 0, 0, 0, "block", NULL);
    YogKlass_define_method(env, klass, "run", run, 0, 1, 0, 0, NULL);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
