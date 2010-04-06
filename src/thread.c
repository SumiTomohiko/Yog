#include "yog/config.h"
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
#include "yog/callable.h"
#include "yog/class.h"
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
#include "yog/get_args.h"
#include "yog/sysdeps.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_THREAD)) { \
        YogError_raise_TypeError((env), "self must be Thread"); \
    } \
} while (0)

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogThread* thread = PTR_AS(YogThread, ptr);

#define KEEP(member)    YogGC_KEEP(env, thread, member, keeper, heap)
    KEEP(prev);
    KEEP(next);
#undef KEEP

    void* thread_heap = PTR_AS(YogThread, thread)->heap;
#define KEEP(member)    YogGC_KEEP(env, thread, member, keeper, thread_heap)
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

    YogBasicObj_init(env, thread, TYPE_THREAD, 0, klass);

    PTR_AS(YogThread, thread)->heap = NULL;

    PTR_AS(YogThread, thread)->jmp_buf_list = NULL;
    PTR_AS(YogThread, thread)->jmp_val = YUNDEF;
    PTR_AS(YogThread, thread)->frame_to_long_jump = YUNDEF;

    PTR_AS(YogThread, thread)->block = YUNDEF;
    PTR_AS(YogThread, thread)->gc_bound = TRUE;

    PTR_AS(YogThread, thread)->recursive_stack = YUNDEF;
    PTR_AS(YogThread, thread)->env = env;
}

#if defined(GC_COPYING)
void
YogThread_config_copying(YogEnv* env, YogVal thread, size_t heap_size)
{
    YogHeap* heap = YogCopying_new(env, heap_size);
    YogVM_add_heap(env, env->vm, heap);
    PTR_AS(YogThread, thread)->heap = heap;
}
#endif

#if defined(GC_MARK_SWEEP)
void
YogThread_config_mark_sweep(YogEnv* env, YogVal thread, size_t heap_size)
{
    YogHeap* heap = YogMarkSweep_new(env, heap_size);
    YogVM_add_heap(env, env->vm, heap);
    PTR_AS(YogThread, thread)->heap = heap;
}
#endif

#if defined(GC_MARK_SWEEP_COMPACT)
void
YogThread_config_mark_sweep_compact(YogEnv* env, YogVal thread, size_t heap_size)
{
    YogHeap* heap = YogMarkSweepCompact_new(env, heap_size);
    YogVM_add_heap(env, env->vm, heap);
    PTR_AS(YogThread, thread)->heap = heap;
}
#endif

#if defined(GC_GENERATIONAL)
void
YogThread_config_generational(YogEnv* env, YogVal thread, size_t young_heap_size, size_t chunk_size, size_t old_heap_size, uint_t age)
{
    YogHeap* heap = YogGenerational_new(env, young_heap_size, chunk_size, old_heap_size, age);
    YogVM_add_heap(env, env->vm, heap);
    PTR_AS(YogThread, thread)->heap = heap;
}
#endif

#if defined(GC_BDW)
void
YogThread_config_bdw(YogEnv* env, YogVal thread)
{
    YogHeap* heap = YogBDW_new(env);
    PTR_AS(YogThread, thread)->heap = heap;
}
#endif

static void
finalize(YogEnv* env, void* ptr)
{
    YogThread* thread = PTR_AS(YogThread, ptr);
#if !defined(GC_BDW)
    YogHeap* heap = (YogHeap*)thread->heap;
    if (heap != NULL) {
        heap->refered = FALSE;
    }
#endif
    thread->heap = NULL;
}

static YogVal
alloc_obj(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal thread = ALLOC_OBJ(env, keep_children, finalize, YogThread);
    YogThread_init(env, thread, klass);
    RETURN(env, thread);
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal thread = alloc_obj(env, klass);
#if defined(GC_BDW)
    YogThread_config_bdw(env, thread);
#elif defined(GC_COPYING)
#   define HEAP_SIZE    (1 * 1024 * 1024)
    YogThread_config_copying(env, thread, HEAP_SIZE);
#   undef HEAP_SIZE
#elif defined(GC_MARK_SWEEP)
    size_t threshold = 1 * 1024 * 1024;
    if (env->vm->gc_stress) {
        threshold = 0;
    }
    YogThread_config_mark_sweep(env, thread, threshold);
#elif defined(GC_MARK_SWEEP_COMPACT)
    size_t threshold = 1 * 1024 * 1024;
    YogThread_config_mark_sweep_compact(env, thread, threshold);
#elif defined(GC_GENERATIONAL)
#   define HEAP_SIZE    (1 * 1024 * 1024)
#   define CHUNK_SIZE   (1 * 1024 * 1024)
#   define TENURE       32
#   define THRESHOLD    (1 * 1024 * 1024)
    YogThread_config_generational(env, thread, HEAP_SIZE, CHUNK_SIZE, THRESHOLD, TENURE);
    YogGenerational_alloc_heap(env, PTR_AS(YogThread, thread)->heap);
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
    return alloc_obj(env, env->vm->cThread);
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
#define KEEP(member)    YogGC_KEEP(env, arg, member, keeper, heap)
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
    PTR_AS(YogThread, thread)->env = &env;
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
        YogVal* args = (YogVal*)YogSysdeps_alloca(sizeof(YogVal) * size);
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
join(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "join", params, args, kw);
    CHECK_SELF_TYPE(env, self);

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
run(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal vararg = YNIL;
    YogVal arg = YUNDEF;
    PUSH_LOCALS2(env, vararg, arg);

    YogCArg params[] = { { "|", NULL }, { "arg", &vararg }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "run", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    arg = ThreadArg_new(env);
    PTR_AS(ThreadArg, arg)->vm = env->vm;
    YogGC_UPDATE_PTR(env, PTR_AS(ThreadArg, arg), thread, self);
    YogGC_UPDATE_PTR(env, PTR_AS(ThreadArg, arg), vararg, vararg);

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
init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    YogGC_UPDATE_PTR(env, PTR_AS(YogThread, self), block, block);

    RETURN(env, self);
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
    YogGC_UPDATE_PTR(env, PTR_AS(YogThread, self), recursive_stack, stack);

    RETURN_VOID(env);
}

static YogVal
get_recursive_stack(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal stack = YUNDEF;
    PUSH_LOCAL(env, stack);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_recursive_stack", params, args, kw);

    ensure_recursive_stack(env, self);
    stack = PTR_AS(YogThread, self)->recursive_stack;

    RETURN(env, stack);
}

void
YogThread_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cThread = YUNDEF;
    PUSH_LOCAL(env, cThread);
    YogVM* vm = env->vm;

    cThread = YogClass_new(env, "Thread", vm->cObject);
    YogClass_define_allocator(env, cThread, alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cThread, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("init", init);
    DEFINE_METHOD("run", run);
    DEFINE_METHOD("join", join);
#undef DEFINE_METHOD
#define DEFINE_PROP(name, getter, setter)   do { \
    YogClass_define_property(env, cThread, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("__recursive_stack__", get_recursive_stack, NULL);
#undef DEFINE_PROP
    vm->cThread = cThread;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
