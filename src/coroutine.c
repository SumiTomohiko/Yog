#include <malloc.h>
#include <stdlib.h>
#include "yog/class.h"
#include "yog/error.h"
#include "yog/object.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct SwitchContext {
    void* ebp;
    void* esp;
    void* eip;
};

typedef struct SwitchContext SwitchContext;

struct Coroutine {
    struct YogBasicObj base;
    void* machine_stack;
    uint_t machine_stack_size;
    YogVal bottom_frame;
    YogVal top_frame;
    struct SwitchContext ctx_to_resume;
    struct SwitchContext ctx_to_yield;
    YogVal block;
};

typedef struct Coroutine Coroutine;

static YogVal
yield(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    /* TODO */
    RETURN(env, self);
}

static YogVal
resume(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    /* TODO */
    RETURN(env, self);
}

static YogVal
init(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    /* TODO */
    RETURN(env, self);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    Coroutine* coro = (Coroutine*)ptr;
#define KEEP(member)    YogGC_keep(env, &coro->member, keeper, heap)
    KEEP(bottom_frame);
    KEEP(top_frame);
    KEEP(block);
#undef KEEP
}

static YogLocalsAnchor*
machine_stack2locals(YogEnv* env, void* stack, uint_t size)
{
    return (YogLocalsAnchor*)((char*)stack + size - sizeof(YogLocalsAnchor));
}

static void
finalize(YogEnv* env, void* ptr)
{
    Coroutine* coro = (Coroutine*)ptr;

    YogLocalsAnchor* locals = machine_stack2locals(env, coro->machine_stack, coro->machine_stack_size);
    YogVM_remove_locals(env, env->vm, locals);
    free(coro->machine_stack);
}

static void
coroutine_main(YogEnv* env, YogVal self)
{
}

static void
Coroutine_init(YogEnv* env, YogVal self, YogVal klass)
{
    SAVE_ARGS2(env, self, klass);
    YogBasicObj_init(env, self, 0, klass);

#define PAGE_SIZE   4096
    uint_t machine_stack_size = PAGE_SIZE;
#undef PAGE_SIZE
    void* machine_stack = malloc(machine_stack_size);
    YOG_ASSERT(env, machine_stack != NULL, "malloc failed");
    YogLocalsAnchor* locals = machine_stack2locals(env, machine_stack, machine_stack_size);
    locals->prev = locals->next = NULL;
    locals->body = NULL;
    locals->heap = PTR_AS(YogThread, env->thread)->heap;
    YogVM_add_locals(env, env->vm, locals);

    PTR_AS(Coroutine, self)->machine_stack = machine_stack;
    PTR_AS(Coroutine, self)->machine_stack_size = machine_stack_size;
    PTR_AS(Coroutine, self)->bottom_frame = YUNDEF;
    PTR_AS(Coroutine, self)->top_frame = YUNDEF;
    PTR_AS(Coroutine, self)->ctx_to_resume.ebp = NULL;
    PTR_AS(Coroutine, self)->ctx_to_resume.esp = NULL;
    PTR_AS(Coroutine, self)->ctx_to_resume.eip = NULL;
    PTR_AS(Coroutine, self)->ctx_to_yield.ebp = NULL;
    PTR_AS(Coroutine, self)->ctx_to_yield.esp = NULL;
    PTR_AS(Coroutine, self)->ctx_to_yield.eip = NULL;

    RETURN_VOID(env);
}

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal coro = YUNDEF;
    PUSH_LOCAL(env, coro);

    coro = ALLOC_OBJ(env, keep_children, finalize, Coroutine);
    Coroutine_init(env, coro, klass);

    RETURN(env, coro);
}

YogVal
YogCoroutine_define_class(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Coroutine", env->vm->cObject);
    YogClass_define_allocator(env, klass, allocate);
    YogClass_define_class_method(env, klass, "yield", yield);
#define DEFINE_METHOD(name, f)  YogClass_define_method(env, klass, name, f)
    DEFINE_METHOD("init", init);
    DEFINE_METHOD("resume", resume);
#undef DEFINE_METHOD

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
