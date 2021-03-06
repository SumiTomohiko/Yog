#include "yog/config.h"
#include <stdlib.h>
#include <strings.h>
#include "yog/array.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/frame.h"
#include "yog/get_args.h"
#include "yog/object.h"
#include "yog/sysdeps.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct SwitchContext {
#if defined(__i386__)
    void* eip;
    void* esp;
    void* ebp;
#else
    /**
     * Coroutines must preserve callee-saved registers. Because the code of
     * restoring them is at the end of coroutine_main(), but it is never
     * executed.
     */
    void* rip;
    void* rsp;
    void* rbp;
    void* rbx;
    void* r12;
    void* r13;
    void* r14;
    void* r15;
#endif
};

#if defined(__i386__)
#   define CTX_IP(ctx)  (ctx).eip
#else
#   define CTX_IP(ctx)  (ctx).rip
#endif

typedef struct SwitchContext SwitchContext;

struct Coroutine {
    struct YogBasicObj base;

    /**
     * Coroutines' machine stack layout
     *
     * On i386
     * =======
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
     * |          | | YogHandles
     * |          | |
     * +----------+ |
     * |          | |
     * |          | | YogLocalsAnchor
     * |          | |
     * +----------+ v
     *              stack bottom (higher address)
     *
     * On amd64
     * ========
     *
     * The stack is without env and self. Because a caller passes arguments
     * through registers on amd64.
     *
     *              stack top (lower address)
     * +----------+ ^
     * |          | | <- Coroutine::machine_stack
     * +----------+ ^
     * |          | |
     * :          : |
     * |          | |
     * +----------+ |
     * |          | | <- initial stack pointer
     * |          | | YogHandles
     * |          | |
     * +----------+ |
     * |          | |
     * |          | | YogLocalsAnchor
     * |          | |
     * +----------+ v
     *              stack bottom (higher address)
     */
    void* machine_stack;

    struct SwitchContext ctx_to_resume;
    struct SwitchContext ctx_to_yield;

    int_t machine_stack_size;

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
    uint_t status;

    YogVal args;
    YogJmpBuf* prev_jmp_buf;
};

typedef struct Coroutine Coroutine;

#define TYPE_COROUTINE TO_TYPE(Coroutine_init)

#define STATUS_SUSPENDED    0
#define STATUS_RUNNING      1
#define STATUS_DEAD         2

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_COROUTINE)) { \
        YogError_raise_TypeError((env), "self must be Coroutine"); \
    } \
} while (0)

static YogLocalsAnchor*
machine_stack2locals(YogEnv* env, void* stack, uint_t size)
{
    return (YogLocalsAnchor*)((char*)stack + size - sizeof(YogLocalsAnchor));
}

static YogHandles*
machine_stack2handles(YogEnv* env, void* stack, uint_t size)
{
    uintptr_t ptr = (uintptr_t)machine_stack2locals(env, stack, size);
    return (YogHandles*)(ptr - sizeof(YogHandles));
}

static void
register_handles(YogEnv* env, YogHandles* handles)
{
    YogHandles_init(handles);
    handles->heap = PTR_AS(YogThread, env->thread)->heap;
    YogVM_add_handles(env, env->vm, handles);
}

static void
register_locals(YogEnv* env, YogLocalsAnchor* locals)
{
    locals->prev = locals->next = NULL;
    locals->body = NULL;
    locals->heap = PTR_AS(YogThread, env->thread)->heap;
    YogVM_add_locals(env, env->vm, locals);
}

static void
SwitchContext_init(YogEnv* env, SwitchContext* ctx)
{
    bzero(ctx, sizeof(*ctx));
}

#if defined(__amd64__)
static void
coroutine_main_wrapper()
{
    __asm__ __volatile__(
        "movq %r13, %rdi\n\t"
        "movq %r14, %rsi\n\t"
        "jmpq *%r12\n");
}
#endif

static void __attribute__((noinline))
switch_context(YogEnv* env, SwitchContext* to, SwitchContext* cont)
{
#if defined(__i386__)
    __asm__ __volatile__(
        "movl $1f, (%0)\n\t"
        "movl %%esp, 4(%0)\n\t"
        "movl %%ebp, 8(%0)\n\t"
        "movl 8(%1), %%ebp\n\t"
        "movl 4(%1), %%esp\n\t"
        "jmp *(%1)\n"
        "1:\n" :
        :
        "r" (cont), "r" (to) :
        "eax", "ebx", "ecx", "edx");
#else
    /**
     * The following code came from LuaCoco 1.1.7 (http://coco.luajit.org/).
     * Coco is Copyright (C) 2005-2012 Mike Pall.
     *
     * I changed some parts. Registers were writable in LuaCoco, but I thought
     * that it is not needed. So I changed;
     * 1. outputs from ("+S" (cont), "+D" (to)) to nothing.
     * 2. inputs from nothing to ("S" (cont), "D" (to)).
     * 3. registers to nothing.
     */
    __asm__ __volatile__(
        "leaq 1f(%%rip), %%rax\n\t"
        "movq %%rax, (%0)\n\t"
        "movq %%rsp, 8(%0)\n\t"
        "movq %%rbp, 16(%0)\n\t"
        "movq %%rbx, 24(%0)\n\t"
        "movq %%r12, 32(%0)\n\t"
        "movq %%r13, 40(%0)\n\t"
        "movq %%r14, 48(%0)\n\t"
        "movq %%r15, 56(%0)\n\t"
        "movq 56(%1), %%r15\n\t"
        "movq 48(%1), %%r14\n\t"
        "movq 40(%1), %%r13\n\t"
        "movq 32(%1), %%r12\n\t"
        "movq 24(%1), %%rbx\n\t"
        "movq 16(%1), %%rbp\n\t"
        "movq 8(%1), %%rsp\n\t"
        "jmpq *(%1)\n"
        "1:\n" :
        :                           /* outputs */
        "S" (cont), "D" (to) :      /* inputs */
        /**
         * registers (empty): LuaCoco's original was
         * "rax", "rcx", "rdx", "r8", "r9", "r10", "r11", "memory", "cc"
         */
        );
#endif
}

static void
yield_coroutine(YogEnv* env, YogVal self, uint_t status)
{
    PTR_AS(Coroutine, self)->status = status;

    SwitchContext* to = &PTR_AS(Coroutine, self)->ctx_to_yield;
    SwitchContext* cont = &PTR_AS(Coroutine, self)->ctx_to_resume;
    switch_context(env, to, cont);
}

/**
 * main function of coroutines. This function is invoked with coroutines'
 * machine stack (Coroutine::machine_stack). This function never return.
 */
static void
coroutine_main(YogEnv* env, YogVal self)
{
    YogEnv coroutine_env = ENV_INIT;
    YogVal thread = env->thread;
    coroutine_env.vm = env->vm;
    coroutine_env.thread = thread;
    void* stack = PTR_AS(Coroutine, self)->machine_stack;
    uint_t stack_size = PTR_AS(Coroutine, self)->machine_stack_size;
    coroutine_env.locals = machine_stack2locals(env, stack, stack_size);
    coroutine_env.handles = machine_stack2handles(env, stack, stack_size);
    coroutine_env.coroutine = self;
    coroutine_env.frame = PTR_AS(Coroutine, self)->boundary_frame;
    SAVE_LOCALS(&coroutine_env);
    PTR_AS(YogThread, thread)->env = &coroutine_env;
    DECL_LOCALS(locals);
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
    YogVal* a = (YogVal*)YogSysdeps_alloca(sizeof(YogVal) * size);
    uint_t i;
    for (i = 0; i < size; i++) {
        a[i] = YogArray_at(&coroutine_env, args, i);
    }

    block = PTR_AS(Coroutine, self)->block;
    retval = YogCallable_call(&coroutine_env, block, size, a);

    args = YogArray_of_size(&coroutine_env, 1);
    YogArray_push(&coroutine_env, args, retval);
    YogGC_UPDATE_PTR(env, PTR_AS(Coroutine, self), args, args);

    RESTORE_LOCALS(&coroutine_env);
    yield_coroutine(&coroutine_env, self, STATUS_DEAD);
}

static void
alloc_machine_stack(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    int_t machine_stack_size = PTR_AS(Coroutine, self)->machine_stack_size;

    void* machine_stack = malloc(machine_stack_size);
    YOG_ASSERT(env, machine_stack != NULL, "malloc failed");
    YogLocalsAnchor* locals = machine_stack2locals(env, machine_stack, machine_stack_size);
    register_locals(env, locals);
    YogHandles* handles = machine_stack2handles(env, machine_stack, machine_stack_size);
    register_handles(env, handles);
    PTR_AS(Coroutine, self)->machine_stack = machine_stack;

    RETURN_VOID(env);
}

static void
Coroutine_init(YogEnv* env, YogVal self, YogVal klass)
{
    SAVE_ARGS2(env, self, klass);
    YogBasicObj_init(env, self, TYPE_COROUTINE, 0, klass);

    PTR_AS(Coroutine, self)->machine_stack = NULL;
    SwitchContext_init(env, &PTR_AS(Coroutine, self)->ctx_to_resume);
    SwitchContext_init(env, &PTR_AS(Coroutine, self)->ctx_to_yield);

    PTR_AS(Coroutine, self)->machine_stack_size = 2048 * 4096;
    PTR_AS(Coroutine, self)->boundary_frame = YUNDEF;
    PTR_AS(Coroutine, self)->block = YUNDEF;
    PTR_AS(Coroutine, self)->status = STATUS_SUSPENDED;
    PTR_AS(Coroutine, self)->args = YUNDEF;
    PTR_AS(Coroutine, self)->prev_jmp_buf = NULL;

    RETURN_VOID(env);
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

static YogJmpBuf*
find_bottom_jmp_buf(YogEnv* env, YogVal coroutine)
{
    YogJmpBuf* buf = PTR_AS(YogThread, env->thread)->jmp_buf_list;
    while (buf->prev != PTR_AS(Coroutine, coroutine)->prev_jmp_buf) {
        buf = buf->prev;
    }
    return buf;
}

static YogVal
yield(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal coroutine = env->coroutine;
    YogVal frame = YUNDEF;
    YogVal boundary = YUNDEF;
    YogVal a = YUNDEF;
    YogVal thread = YUNDEF;
    PUSH_LOCALS5(env, coroutine, frame, boundary, a, thread);
    YogCArg params[] = { { "*", &a }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "yield", params, args, kw);

    YogGC_UPDATE_PTR(env, PTR_AS(Coroutine, coroutine), args, a);

    boundary = PTR_AS(Coroutine, coroutine)->boundary_frame;
    frame = env->frame;
    while (IS_PTR(frame) && (PTR_AS(YogFrame, frame)->prev != boundary)) {
        frame = PTR_AS(YogFrame, frame)->prev;
    }
    YOG_ASSERT(env, IS_PTR(frame), "boundary frame not found");
    PTR_AS(YogFrame, frame)->prev = YUNDEF;
    boundary = YUNDEF;  /* Kill previous frames from the machine stack */

    YogHandle_sync_scope_with_env(env);
    thread = env->thread;
    YogJmpBuf* top_jmp_buf = PTR_AS(YogThread, thread)->jmp_buf_list;
    YogJmpBuf* bottom_jmp_buf = find_bottom_jmp_buf(env, coroutine);
    yield_coroutine(env, coroutine, STATUS_SUSPENDED);
    bottom_jmp_buf->prev = PTR_AS(YogThread, thread)->jmp_buf_list;
    PTR_AS(YogThread, thread)->jmp_buf_list = top_jmp_buf;

    boundary = PTR_AS(Coroutine, coroutine)->boundary_frame;
    YogGC_UPDATE_PTR(env, PTR_AS(YogFrame, frame), prev, boundary);

    return_args(env, PTR_AS(Coroutine, coroutine)->args);
    RETURN(env, YUNDEF);
}

static YogVal
resume(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    /*TODO: If two threads resume same Coroutine, it will break */
    CHECK_SELF_TYPE(env, self);
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal a = YUNDEF;
    PUSH_LOCAL(env, a);
    YogCArg params[] = { { "*", &a }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "resume", params, args, kw);

    if (PTR_AS(Coroutine, self)->status == STATUS_DEAD) {
        YogError_raise_CoroutineError(env, "dead coroutine called");
    }

    YogGC_UPDATE_PTR(env, PTR_AS(Coroutine, self), boundary_frame, env->frame);
    PTR_AS(Coroutine, self)->status = STATUS_RUNNING;
    YogGC_UPDATE_PTR(env, PTR_AS(Coroutine, self), args, a);

    if (CTX_IP(PTR_AS(Coroutine, self)->ctx_to_resume) == NULL) {
        alloc_machine_stack(env, self);
        void* stack = PTR_AS(Coroutine, self)->machine_stack;
        uint_t size = PTR_AS(Coroutine, self)->machine_stack_size;
        void** locals = (void**)machine_stack2handles(env, stack, size);
        void* ip;
#if defined(__i386__)
        locals[-1] = (void*)self;
        locals[-2] = (void*)env;
        locals[-3] = (void*)0xdeaddead;
        PTR_AS(Coroutine, self)->ctx_to_resume.esp = &locals[-3];
        ip = (void*)coroutine_main;
#else
        SwitchContext* ctx = &PTR_AS(Coroutine, self)->ctx_to_resume;
        /**
         * I explain what the next expression for rsp is.
         *
         * clang 3.0 with -O2 emits the following code for coroutine_main.
         *
         * 000000000043a5b0 <coroutine_main>:
         *   43a5b0:	55                   	push   %rbp
         *   43a5b1:	48 89 e5             	mov    %rsp,%rbp
         *   :
         *   43a5cb:	66 0f ef c0          	pxor   %xmm0,%xmm0
         *   43a5cf:	0f 29 45 c0          	movaps %xmm0,-0x40(%rbp)
         *
         * clang uses movaps instruction of SSE to make memory zero (this memory
         * is for coroutine_env). movaps requires that destination address
         * (-0x40(%rbp)) is 16-byte aligned. Because of this, rbp must be
         * 16-byte aligned at movaps. rbp is from rsp, and rsp is decreased by
         * 8 bytes for push at the beginning of coroutine_main. In other words,
         * rsp must satisfy ((rsp % 16) == 8) at this time. The following
         * expression arranges rsp for it.
         */
        ctx->rsp = (void*)((((uintptr_t)locals - 8) & (~0x0fLU)) + 8);
        ctx->r12 = (void*)coroutine_main;
        /**
         * Be careful. ctx->r13 and ctx->r14 holds a pointer, but GC does not
         * update it nor its inside. GC is not allowed from here to next
         * switch_context().
         */
        ctx->r13 = (void*)env;
        ctx->r14 = (void*)self;
        ip = (void*)coroutine_main_wrapper;
#endif
        CTX_IP(PTR_AS(Coroutine, self)->ctx_to_resume) = ip;
    }

    SwitchContext* to = &PTR_AS(Coroutine, self)->ctx_to_resume;
    SwitchContext* cont = &PTR_AS(Coroutine, self)->ctx_to_yield;
    YogHandle_sync_scope_with_env(env);
    YogJmpBuf* prev_jmp_buf = PTR_AS(YogThread, env->thread)->jmp_buf_list;
    PTR_AS(Coroutine, self)->prev_jmp_buf = prev_jmp_buf;
    switch_context(env, to, cont);
    PTR_AS(YogThread, env->thread)->jmp_buf_list = prev_jmp_buf;

    return_args(env, PTR_AS(Coroutine, self)->args);
    RETURN(env, YUNDEF);
}

static void
set_machine_stack_size(YogEnv* env, YogVal self, YogVal machine_stack_size)
{
    SAVE_ARGS2(env, self, machine_stack_size);
    int_t size = YogVal_to_signed_type(env, machine_stack_size, "machine stack size");
    PTR_AS(Coroutine, self)->machine_stack_size = size;
    RETURN_VOID(env);
}

static YogVal
init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    CHECK_SELF_TYPE(env, self);
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal machine_stack_size = YUNDEF;
    PUSH_LOCAL(env, machine_stack_size);
    YogCArg params[] = {
        { "|", NULL },
        { "machine_stack_size", &machine_stack_size },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);

    YogGC_UPDATE_PTR(env, PTR_AS(Coroutine, self), block, block);
    if (!IS_UNDEF(machine_stack_size)) {
        set_machine_stack_size(env, self, machine_stack_size);
    }

    RETURN(env, self);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    Coroutine* coro = (Coroutine*)ptr;
#define KEEP(member)    YogGC_KEEP(env, coro, member, keeper, heap)
    KEEP(boundary_frame);
    KEEP(block);
    KEEP(args);
#undef KEEP
}

static void
finalize(YogEnv* env, void* ptr)
{
    Coroutine* coro = (Coroutine*)ptr;

    void* machine_stack = coro->machine_stack;
    if (machine_stack != NULL) {
        YogLocalsAnchor* locals = machine_stack2locals(env, machine_stack, coro->machine_stack_size);
        YogVM_remove_locals(env, env->vm, locals);
        YogHandles* handles = machine_stack2handles(env, machine_stack, coro->machine_stack_size);
        YogVM_remove_handles(env, env->vm, handles);
        YogHandles_finalize(handles);
        free(machine_stack);
    }
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

static YogVal
query_status(YogEnv* env, YogVal self, const char* name, uint_t status, YogVal args, YogVal kw)
{
    CHECK_SELF_TYPE(env, self);
    SAVE_ARGS3(env, self, args, kw);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, name, params, args, kw);

    if (PTR_AS(Coroutine, self)->status == status) {
        RETURN(env, YTRUE);
    }
    RETURN(env, YFALSE);
}

static YogVal
get_suspended(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    return query_status(env, self, "get_suspended", STATUS_SUSPENDED, args, kw);
}

static YogVal
get_running(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    return query_status(env, self, "get_running", STATUS_RUNNING, args, kw);
}

static YogVal
get_dead(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    return query_status(env, self, "get_dead", STATUS_DEAD, args, kw);
}

static YogVal
get_status(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    CHECK_SELF_TYPE(env, self);
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_status", params, args, kw);

    uint_t status = PTR_AS(Coroutine, self)->status;

    RETURN(env, INT2VAL(status));
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
#define DEFINE_STATUS(name, val)    do { \
    YogObj_set_attr(env, cCoroutine, (name), INT2VAL(val)); \
} while (0)
    DEFINE_STATUS("SUSPENDED", STATUS_SUSPENDED);
    DEFINE_STATUS("RUNNING", STATUS_RUNNING);
    DEFINE_STATUS("DEAD", STATUS_DEAD);
#undef DEFINE_STATUS
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
#define DEFINE_PROP(name, getter, setter)   do { \
    YogClass_define_property(env, cCoroutine, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("suspended?", get_suspended, NULL);
    DEFINE_PROP("running?", get_running, NULL);
    DEFINE_PROP("dead?", get_dead, NULL);
    DEFINE_PROP("status", get_status, NULL);
#undef DEFINE_PROP
    vm->cCoroutine = cCoroutine;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
