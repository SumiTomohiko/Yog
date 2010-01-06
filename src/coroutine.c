#include "yog/config.h"
#include <alloca.h>
#include <stdlib.h>
#if defined(HAVE_WINDOWS_H)
#   include <windows.h>
#endif
#include "yog/array.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/get_args.h"
#include "yog/object.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#if defined(_WIN32)
struct MainParam {
    struct YogVM* vm;
    YogVal thread;
    YogVal coroutine;
    YogLocalsAnchor locals;
};

typedef struct MainParam MainParam;
#else
struct SwitchContext {
    void* eip;
    void* esp;
    void* ebp;
};

typedef struct SwitchContext SwitchContext;
#endif

struct Coroutine {
    struct YogBasicObj base;

#if defined(_WIN32)
    void* fiber_to_resume;
    void* fiber_to_yield;
    struct MainParam* param;
#else
    /**
     * Coroutines' machine stack layout (x86)
     *
     *              stack top (lower address)
     * +----------+ ^
     * |          | | <- Coroutine::machine_stack
     * +----------+ ^
     * |          | |
     * :          : |
     * |          | |
     * +----------+ |
     * |0xdeaddead| | dummy return address <- initial stack pointer
     * +----------+ |
     * |          | | env
     * +----------+ |
     * |          | | self
     * +----------+ |
     * |          | |
     * |          | | YogLocalsAnchor
     * |          | |
     * +----------+ v
     *              stack bottom (higher address)
     */
    void* machine_stack;
    uint_t machine_stack_size;

    struct SwitchContext ctx_to_resume;
    struct SwitchContext ctx_to_yield;
#endif

    /**
     * boundary_frame is the next frame of the bottom of the coroutines.
     *
     *   frames   top
     * +--------+ ^
     * |        | | ^
     * :        : | | coroutine
     * |        | | v
     * +--------+ |
     * |        | | ^ <- boundary_frame
     * +--------+ | |
     * |        | | | coroutine or not coroutine
     * :        : | |
     * |        | | v
     * +--------+ v
     *            bottom
     */
    YogVal boundary_frame;
    YogVal block;

    YogVal args;
};

typedef struct Coroutine Coroutine;

#define TYPE_COROUTINE  ((type_t)Coroutine_init)

#if !defined(_WIN32)
static void
switch_context(YogEnv* env, SwitchContext* to, SwitchContext* cont)
{
    __asm__ __volatile__(
        "movl $1f, (%0)\n\t"
        "movl %%esp, 4(%0)\n\t"
        "movl %%ebp, 8(%0)\n\t"
        "movl 8(%1), %%ebp\n\t"
        "movl 4(%1), %%esp\n\t"
        "jmp *(%1)\n"
        "1:\n"
        : : "r" (cont), "r" (to) : "eax", "ebx", "ecx", "edx"
    );
}
#endif

static void
yield_coroutine(YogEnv* env, YogVal self)
{
#if defined(_WIN32)
    SwitchToFiber(PTR_AS(Coroutine, self)->fiber_to_yield);
#else
    SwitchContext* to = &PTR_AS(Coroutine, self)->ctx_to_yield;
    SwitchContext* cont = &PTR_AS(Coroutine, self)->ctx_to_resume;
    switch_context(env, to, cont);
#endif
}

static void
return_args(YogEnv* env, YogVal args)
{
    SAVE_ARG(env, args);
    YogVal a = YUNDEF;
    PUSH_LOCAL(env, a);
    YOG_ASSERT(env, IS_PTR(args), "args must be pointer (0x%08x)", args);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(args) == TYPE_ARRAY, "invalid args type (0x%08x)", BASIC_OBJ_TYPE(args));

    if (YogArray_size(env, args) == 0) {
        a = YogArray_of_size(env, 1);
        YogArray_push(env, a, YNIL);
        YogCFrame_return_multi_value(env, env->frame, a);
        RETURN_VOID(env);
    }

    YogCFrame_return_multi_value(env, env->frame, args);

    RETURN_VOID(env);
}

static YogVal
yield(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal coroutine = env->coroutine;
    YogVal frame = YUNDEF;
    YogVal boundary = YUNDEF;
    YogVal a = YUNDEF;
    PUSH_LOCALS4(env, coroutine, frame, boundary, a);
    YogCArg params[] = { { "*", &a }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "yield", params, args, kw);

    PTR_AS(Coroutine, coroutine)->args = a;

    boundary = PTR_AS(Coroutine, coroutine)->boundary_frame;
    frame = env->frame;
    while (IS_PTR(frame) && (PTR_AS(YogFrame, frame)->prev != boundary)) {
        frame = PTR_AS(YogFrame, frame)->prev;
    }
    YOG_ASSERT(env, IS_PTR(frame), "boundary frame not found");
    PTR_AS(YogFrame, frame)->prev = YUNDEF;
    boundary = YUNDEF;  /* Kill previous frames from the machine stack */

    yield_coroutine(env, coroutine);

    boundary = PTR_AS(Coroutine, coroutine)->boundary_frame;
    PTR_AS(YogFrame, frame)->prev = boundary;

    return_args(env, PTR_AS(Coroutine, coroutine)->args);
    RETURN(env, YUNDEF);
}

static YogLocalsAnchor*
machine_stack2locals(YogEnv* env, void* stack, uint_t size)
{
    return (YogLocalsAnchor*)((char*)stack + size - sizeof(YogLocalsAnchor));
}

#if defined(_WIN32)
#   define MAIN_PARAM   void* p
#else
#   define CALLBACK
#   define MAIN_PARAM   YogEnv* env, YogVal self
#endif
/**
 * For Linux and *BSD:
 * main function of coroutines. This function is invoked with coroutines'
 * machine stack (Coroutine::machine_stack). This function never return.
 */
static void CALLBACK
coroutine_main(MAIN_PARAM)
{
    YogEnv coroutine_env = ENV_INIT;
    YogVal thread;
#if defined(_WIN32)
    MainParam* param = (MainParam*)p;
    thread = param->thread;
    coroutine_env.vm = param->vm;
    coroutine_env.thread = thread;
    coroutine_env.locals = &param->locals;
    coroutine_env.coroutine = param->coroutine;
    coroutine_env.frame = PTR_AS(Coroutine, param->coroutine)->boundary_frame;
    YogVal self = param->coroutine;
#else
    thread = env->thread;
    coroutine_env.vm = env->vm;
    coroutine_env.thread = thread;
    void* stack = PTR_AS(Coroutine, self)->machine_stack;
    uint_t stack_size = PTR_AS(Coroutine, self)->machine_stack_size;
    coroutine_env.locals = machine_stack2locals(env, stack, stack_size);
    coroutine_env.coroutine = self;
    coroutine_env.frame = PTR_AS(Coroutine, self)->boundary_frame;
#endif
    SAVE_LOCALS(&coroutine_env);
    PTR_AS(YogThread, thread)->env = &coroutine_env;
    YogLocals locals;
    locals.num_vals = 4;
    locals.size = 1;
    locals.vals[0] = &self;
    locals.vals[1] = &coroutine_env.thread;
    locals.vals[2] = &coroutine_env.coroutine;
    locals.vals[3] = &coroutine_env.frame;
    PUSH_LOCAL_TABLE(&coroutine_env, locals);
    YogVal args = YUNDEF;
    YogVal retval = YUNDEF;
    YogVal block = YUNDEF;
    PUSH_LOCALS3(&coroutine_env, args, retval, block);

    args = PTR_AS(Coroutine, self)->args;
    uint_t size = IS_PTR(args) ? YogArray_size(&coroutine_env, args) : 0;
    YogVal* a = (YogVal*)alloca(sizeof(YogVal) * size);
    uint_t i;
    for (i = 0; i < size; i++) {
        a[i] = YogArray_at(&coroutine_env, args, i);
    }

    block = PTR_AS(Coroutine, self)->block;
    retval = YogCallable_call(&coroutine_env, block, size, a);

    args = YogArray_of_size(env, 1);
    YogArray_push(env, args, retval);
    PTR_AS(Coroutine, self)->args = args;

    RESTORE_LOCALS(&coroutine_env);
    yield_coroutine(&coroutine_env, self);
}
#undef CALLBACK
#undef MAIN_PARAM

static YogVal
resume(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal a = YUNDEF;
    PUSH_LOCAL(env, a);
    YogCArg params[] = { { "*", &a }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "resume", params, args, kw);

    PTR_AS(Coroutine, self)->boundary_frame = env->frame;
    PTR_AS(Coroutine, self)->args = a;

#if defined(_WIN32)
    void* fiber_to_yield = GetCurrentFiber();
    if ((fiber_to_yield == NULL) || (fiber_to_yield == (void*)0x1e00)) {
        fiber_to_yield = ConvertThreadToFiber(NULL);
        YOG_ASSERT(env, fiber_to_yield != NULL, "ConvertThreadToFiber failed");
    }
    PTR_AS(Coroutine, self)->fiber_to_yield = fiber_to_yield;
    PTR_AS(Coroutine, self)->param->thread = env->thread;
    SwitchToFiber(PTR_AS(Coroutine, self)->fiber_to_resume);
#else
    if (PTR_AS(Coroutine, self)->ctx_to_resume.eip == NULL) {
        void* stack = PTR_AS(Coroutine, self)->machine_stack;
        uint_t size = PTR_AS(Coroutine, self)->machine_stack_size;
        void** locals = (void**)machine_stack2locals(env, stack, size);
        locals[-1] = (void*)self;
        locals[-2] = (void*)env;
        locals[-3] = (void*)0xdeaddead;
        PTR_AS(Coroutine, self)->ctx_to_resume.eip = coroutine_main;
        PTR_AS(Coroutine, self)->ctx_to_resume.esp = &locals[-3];
    }

    SwitchContext* to = &PTR_AS(Coroutine, self)->ctx_to_resume;
    SwitchContext* cont = &PTR_AS(Coroutine, self)->ctx_to_yield;
    switch_context(env, to, cont);
#endif

    return_args(env, PTR_AS(Coroutine, self)->args);
    RETURN(env, YUNDEF);
}

static YogVal
init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);

    PTR_AS(Coroutine, self)->block = block;
    RETURN(env, self);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    Coroutine* coro = (Coroutine*)ptr;
#define KEEP(member)    YogGC_keep(env, &coro->member, keeper, heap)
#if defined(_WIN32)
    KEEP(param->thread);
    KEEP(param->coroutine);
#endif
    KEEP(boundary_frame);
    KEEP(block);
    KEEP(args);
#undef KEEP
}

static void
finalize(YogEnv* env, void* ptr)
{
    Coroutine* coro = (Coroutine*)ptr;

#if defined(_WIN32)
    DeleteFiber(coro->fiber_to_resume);
    YogLocalsAnchor* locals = &coro->param->locals;
    YogVM_remove_locals(env, env->vm, locals);
    free(coro->param);
#else
    YogLocalsAnchor* locals = machine_stack2locals(env, coro->machine_stack, coro->machine_stack_size);
    YogVM_remove_locals(env, env->vm, locals);
    free(coro->machine_stack);
#endif
}

#if !defined(_WIN32)
static void
SwitchContext_init(YogEnv* env, SwitchContext* ctx)
{
    ctx->eip = NULL;
    ctx->esp = NULL;
    ctx->ebp = NULL;
}
#endif

static void
register_locals(YogEnv* env, YogLocalsAnchor* locals)
{
    locals->prev = locals->next = NULL;
    locals->body = NULL;
    locals->heap = PTR_AS(YogThread, env->thread)->heap;
    YogVM_add_locals(env, env->vm, locals);
}

static void
Coroutine_init(YogEnv* env, YogVal self, YogVal klass)
{
    SAVE_ARGS2(env, self, klass);
    YogBasicObj_init(env, self, TYPE_COROUTINE, 0, klass);

#define STACK_SIZE  (2048 * 4096)
#if defined(_WIN32)
    MainParam* param = (MainParam*)malloc(sizeof(MainParam));
    YOG_ASSERT(env, param != NULL, "can't allocate MainParam");
    param->vm = env->vm;
    param->thread = YUNDEF;
    param->coroutine = self;
    register_locals(env, &param->locals);
    void* fiber = CreateFiber(STACK_SIZE, coroutine_main, param);
    YOG_ASSERT(env, fiber != NULL, "CreateFiber failed");
    PTR_AS(Coroutine, self)->fiber_to_resume = fiber;
    PTR_AS(Coroutine, self)->fiber_to_yield = NULL;
    PTR_AS(Coroutine, self)->param = param;
#else
    uint_t machine_stack_size = STACK_SIZE;
    void* machine_stack = malloc(machine_stack_size);
    YOG_ASSERT(env, machine_stack != NULL, "malloc failed");
    YogLocalsAnchor* locals = machine_stack2locals(env, machine_stack, machine_stack_size);
    register_locals(env, locals);

    PTR_AS(Coroutine, self)->machine_stack = machine_stack;
    PTR_AS(Coroutine, self)->machine_stack_size = machine_stack_size;
    SwitchContext_init(env, &PTR_AS(Coroutine, self)->ctx_to_resume);
    SwitchContext_init(env, &PTR_AS(Coroutine, self)->ctx_to_yield);
#endif
#undef STACK_SIZE

    PTR_AS(Coroutine, self)->boundary_frame = YUNDEF;
    PTR_AS(Coroutine, self)->block = YUNDEF;
    PTR_AS(Coroutine, self)->args = YUNDEF;

    RETURN_VOID(env);
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal coro = YUNDEF;
    PUSH_LOCAL(env, coro);

    coro = ALLOC_OBJ(env, keep_children, finalize, Coroutine);
    Coroutine_init(env, coro, klass);

    RETURN(env, coro);
}

void
YogCoroutine_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cCoroutine = YUNDEF;
    PUSH_LOCAL(env, cCoroutine);
    YogVM* vm = env->vm;

    cCoroutine = YogClass_new(env, "Coroutine", vm->cObject);
    YogClass_define_allocator(env, cCoroutine, alloc);
#define DEFINE_CLASS_METHOD(name, f)    do { \
    YogClass_define_class_method(env, cCoroutine, pkg, (name), (f)); \
} while (0)
    DEFINE_CLASS_METHOD("yield", yield);
#undef DEFINE_CLASS_METHOD
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cCoroutine, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("init", init);
    DEFINE_METHOD("resume", resume);
#undef DEFINE_METHOD
    vm->cCoroutine = cCoroutine;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
