#include "config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#include <errno.h>
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
#if defined(GC_BDW)
#   include "gc.h"
#   include "gc_pthread_redirects.h"
#endif
#include "yog/array.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/function.h"
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
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogThread* thread = PTR_AS(YogThread, ptr);

#define KEEP(member)    YogGC_keep(env, &thread->member, keeper, heap)
    KEEP(prev);
    KEEP(next);
#undef KEEP

    void* thread_heap = THREAD_HEAP(thread);
#define KEEP(member)    YogGC_keep(env, &thread->member, keeper, thread_heap)
    KEEP(jmp_val);
    KEEP(frame_to_long_jump);
    KEEP(block);
    KEEP(recursive_stack);
#undef KEEP
}

void
YogThread_init(YogEnv* env, YogVal thread, YogVal klass)
{
    PTR_AS(YogThread, thread)->prev = YUNDEF;
    PTR_AS(YogThread, thread)->next = YUNDEF;

    PTR_AS(YogThread, thread)->thread_id = YogVM_issue_thread_id(env, env->vm);
    PTR_AS(YogThread, thread)->next_obj_id = 0;

    YogBasicObj_init(env, thread, 0, klass);

    PTR_AS(YogThread, thread)->heap = NULL;

    PTR_AS(YogThread, thread)->jmp_buf_list = NULL;
    PTR_AS(YogThread, thread)->jmp_val = YUNDEF;
    PTR_AS(YogThread, thread)->frame_to_long_jump = YUNDEF;

    PTR_AS(YogThread, thread)->block = YUNDEF;
    PTR_AS(YogThread, thread)->gc_bound = TRUE;

    PTR_AS(YogThread, thread)->recursive_stack = YUNDEF;
}

#if defined(GC_COPYING)
void
YogThread_config_copying(YogEnv* env, YogVal thread, size_t init_heap_size)
{
    YogCopying* copying = (YogCopying*)malloc(sizeof(YogCopying));
    YOG_ASSERT(env, copying != NULL, "Can't allocate YogCopying");
    YogCopying_init(env, copying, init_heap_size);
    copying->refered = TRUE;
    YogVM_add_heap(env, env->vm, copying);

    PTR_AS(YogThread, thread)->heap = copying;
}
#endif

#if defined(GC_MARK_SWEEP)
void
YogThread_config_mark_sweep(YogEnv* env, YogVal thread, size_t threshold)
{
    YogMarkSweep* mark_sweep = malloc(sizeof(YogMarkSweep));
    YOG_ASSERT(env, mark_sweep != NULL, "Can't allocate YogMarkSweep");
    YogMarkSweep_init(env, mark_sweep, threshold);
    mark_sweep->refered = TRUE;
    YogVM_add_heap(env, env->vm, mark_sweep);

    PTR_AS(YogThread, thread)->heap = mark_sweep;
}
#endif

#if defined(GC_MARK_SWEEP_COMPACT)
void
YogThread_config_mark_sweep_compact(YogEnv* env, YogVal thread, size_t chunk_size, size_t threshold)
{
    size_t size = sizeof(YogMarkSweepCompact);
    YogMarkSweepCompact* mark_sweep_compact = malloc(size);
    YOG_ASSERT(env, mark_sweep_compact != NULL, "Can't allocate YogMarkSweepCompact");
    YogMarkSweepCompact_init(env, mark_sweep_compact, chunk_size, threshold);
    mark_sweep_compact->refered = TRUE;
    YogVM_add_heap(env, env->vm, mark_sweep_compact);

    PTR_AS(YogThread, thread)->heap = mark_sweep_compact;
}
#endif

#if defined(GC_GENERATIONAL)
void
YogThread_config_generational(YogEnv* env, YogVal thread, size_t young_heap_size, size_t old_chunk_size, size_t old_threshold, uint_t tenure)
{
    YogGenerational* generational = malloc(sizeof(YogGenerational));
    YOG_ASSERT(env, generational != NULL, "Can't allocate YogGenerational");
    YogGenerational_init(env, generational, young_heap_size, old_chunk_size, old_threshold, tenure);
    generational->refered = TRUE;
    YogVM_add_heap(env, env->vm, generational);

    PTR_AS(YogThread, thread)->heap = generational;
}
#endif

#if defined(GC_BDW)
void
YogThread_config_bdw(YogEnv* env, YogVal thread)
{
    YogBDW* bdw = GC_MALLOC(sizeof(YogBDW));
    YogBDW_init(env, bdw);

    PTR_AS(YogThread, thread)->heap = bdw;
}
#endif

static void
finalize(YogEnv* env, void* ptr)
{
    YogThread* thread = PTR_AS(YogThread, ptr);
#if !defined(GC_BDW)
    GC_TYPE* heap = (GC_TYPE*)thread->heap;
    if (heap != NULL) {
        heap->refered = FALSE;
    }
#endif
    thread->heap = NULL;
}

static YogVal
allocate_object(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal thread = ALLOC_OBJ(env, keep_children, finalize, YogThread);
    YogThread_init(env, thread, klass);
    RETURN(env, thread);
}

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal thread = allocate_object(env, klass);
#if defined(GC_BDW)
    YogThread_config_bdw(env, thread);
#elif defined(GC_COPYING)
#   define HEAP_SIZE    (1 * 1024 * 1024)
    YogThread_config_copying(env, thread, HEAP_SIZE);
    YogCopying_allocate_heap(env, (YogCopying*)PTR_AS(YogThread, thread)->heap);
#   undef HEAP_SIZE
#elif defined(GC_MARK_SWEEP)
    size_t threshold = 1 * 1024 * 1024;
    if (env->vm->gc_stress) {
        threshold = 0;
    }
    YogThread_config_mark_sweep(env, thread, threshold);
#elif defined(GC_MARK_SWEEP_COMPACT)
    size_t threshold = 1 * 1024 * 1024;
    if (env->vm->gc_stress) {
        threshold = 0;
    }
#   define CHUNK_SIZE   (1 * 1024 * 1024)
    YogThread_config_mark_sweep_compact(env, thread, CHUNK_SIZE, threshold);
#   undef CHUNK_SIZE
#elif defined(GC_GENERATIONAL)
#   define HEAP_SIZE    (1 * 1024 * 1024)
#   define CHUNK_SIZE   (1 * 1024 * 1024)
#   define TENURE       32
#   define THRESHOLD    (1 * 1024 * 1024)
    YogThread_config_generational(env, thread, HEAP_SIZE, CHUNK_SIZE, THRESHOLD, TENURE);
    YogGenerational_allocate_heap(env, PTR_AS(YogThread, thread)->heap);
#   undef THRESHOLD
#   undef TENURE
#   undef CHUNK_SIZE
#   undef HEAP_SIZE
#endif

    RETURN(env, thread);
}

YogVal
YogThread_new(YogEnv* env)
{
    return allocate_object(env, env->vm->cThread);
}

struct ThreadArg {
    struct YogVM* vm;
    YogVal thread;
    YogVal vararg;
};

typedef struct ThreadArg ThreadArg;

static void
ThreadArg_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    ThreadArg* arg = PTR_AS(ThreadArg, ptr);
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
thread_main(void* arg)
{
#if defined(__MINGW32__) || defined(_MSC_VER)
    if (!pthread_win32_thread_attach_np()) {
        YOG_BUG(NULL, "pthread_win32_thread_attach_np failed");
    }
#endif

    YogVal thread_arg = (YogVal)arg;
    YogVM* vm = PTR_AS(ThreadArg, thread_arg)->vm;
    YogVal thread = PTR_AS(ThreadArg, thread_arg)->thread;
    YogLocalsAnchor locals = LOCALS_ANCHOR_INIT;
    locals.heap = PTR_AS(YogThread, thread)->heap;
    YogEnv env = ENV_INIT;
    env.vm = vm;
    env.thread = thread;
    env.locals = &locals;
    SAVE_LOCALS(&env);
    YogLocals locals0;
    locals0.num_vals = 3;
    locals0.size = 1;
    locals0.vals[0] = &env.thread;
    locals0.vals[1] = &thread_arg;
    locals0.vals[2] = &env.frame;
    locals0.vals[3] = NULL;
    PUSH_LOCAL_TABLE(&env, locals0);
    YogVM_add_locals(&env, vm, &locals);

    YogVal vararg = PTR_AS(ThreadArg, thread_arg)->vararg;
    YogVal block = PTR_AS(YogThread, thread)->block;
    if (IS_PTR(vararg)) {
        uint_t size = YogArray_size(&env, vararg);
#if !defined(alloca) && defined(_alloca)
#   define alloca   _alloca
#endif
        YogVal* args = (YogVal*)alloca(sizeof(YogVal) * size);
        YogVal body = PTR_AS(YogArray, vararg)->body;
        memcpy(args, PTR_AS(YogValArray, body)->items, size);
        PUSH_LOCALSX(&env, size, args);

        YogCallable_call(&env, block, size, args);
    }
    else {
        YogCallable_call(&env, block, 0, NULL);
    }

    RESTORE_LOCALS(&env);

    YogVM_remove_thread(&env, vm, env.thread);
    YogVM_remove_locals(&env, vm, &locals);

#if defined(__MINGW32__) || defined(_MSC_VER)
    pthread_win32_thread_detach_np();
#endif

    return NULL;
}

static YogVal
join(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);

    void* retval = NULL;
    FREE_FROM_GC(env);
    if (pthread_join(PTR_AS(YogThread, self)->pthread, &retval) != 0) {
        BIND_TO_GC(env);
        YOG_BUG(env, "pthread_join failed");
        /* NOTREACHED */
    }
    BIND_TO_GC(env);

    RETURN(env, PTR2VAL(retval));
}

static YogVal
run(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal vararg = YUNDEF;
    YogVal arg = YUNDEF;
    PUSH_LOCALS2(env, vararg, arg);

    if (0 < YogArray_size(env, args)) {
        vararg = YogArray_at(env, args, 0);
    }
    else {
        vararg = YNIL;
    }

    arg = ThreadArg_new(env);
    PTR_AS(ThreadArg, arg)->vm = env->vm;
    PTR_AS(ThreadArg, arg)->thread = self;
    PTR_AS(ThreadArg, arg)->vararg = vararg;

    YogVM_add_thread(env, env->vm, self);

    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        YOG_BUG(env, "pthread_attr_init failed");
    }
    if (pthread_attr_setstacksize(&attr, 10 * 1024 * 1024) != 0) {
        YOG_BUG(env, "pthread_attr_setstacksize failed");
    }

#if !defined(GC_BDW)
#   define CREATE_THREAD    pthread_create
#else
#   define CREATE_THREAD    GC_pthread_create
#endif
    pthread_t* pt = &PTR_AS(YogThread, self)->pthread;
    if (CREATE_THREAD(pt, &attr, thread_main, (void*)arg) != 0) {
        YOG_BUG(env, "can't create new thread: %s", strerror(errno));
    }
#undef CREATE_THREAD

    if (pthread_attr_destroy(&attr) != 0) {
        YOG_BUG(env, "pthread_attr_destroy failed");
    }

    RETURN(env, self);
}

static YogVal
init(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    PTR_AS(YogThread, self)->block = block;
    return self;
}

void
YogThread_issue_object_id(YogEnv* env, YogVal self, YogVal obj)
{
    PTR_AS(YogBasicObj, obj)->id_upper = PTR_AS(YogThread, self)->thread_id;
    PTR_AS(YogBasicObj, obj)->id_lower = PTR_AS(YogThread, self)->next_obj_id;
    PTR_AS(YogThread, self)->next_obj_id++;
    YOG_ASSERT(env, PTR_AS(YogThread, self)->next_obj_id != 0, "object id overflow");
}

static void
ensure_recursive_stack(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal stack = YUNDEF;
    PUSH_LOCAL(env, stack);

    if (IS_PTR(PTR_AS(YogThread, self)->recursive_stack)) {
        RETURN_VOID(env);
    }

    stack = YogArray_new(env);
    PTR_AS(YogThread, self)->recursive_stack = stack;

    RETURN_VOID(env);
}

static YogVal
get_recursive_stack(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal stack = YUNDEF;
    PUSH_LOCAL(env, stack);

    ensure_recursive_stack(env, self);
    stack = PTR_AS(YogThread, self)->recursive_stack;

    RETURN(env, stack);
}

YogVal
YogThread_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);

    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Thread", env->vm->cObject);
    YogClass_define_allocator(env, klass, allocate);
#define DEFINE_METHOD(name, f)  YogClass_define_method(env, klass, name, f)
    DEFINE_METHOD("init", init);
    DEFINE_METHOD("run", run);
    DEFINE_METHOD("join", join);
#undef DEFINE_METHOD
    YogClass_define_property(env, klass, "__recursive_stack__", get_recursive_stack, NULL);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
