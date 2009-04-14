#if !defined(__YOG_THREAD_H__)
#define __YOG_THREAD_H__

#include <setjmp.h>
#include "yog/yog.h"

struct YogJmpBuf {
    jmp_buf buf;
    struct YogJmpBuf* prev;
};

typedef struct YogJmpBuf YogJmpBuf;

#define NUM_VALS    5

struct YogLocals {
    struct YogLocals* next;
    unsigned int num_vals;
    unsigned int size;
    YogVal* vals[NUM_VALS];
};

typedef struct YogLocals YogLocals;

#include "yog/env.h"

#define SAVE_LOCALS(env)        YogLocals* __cur_locals__ = (env)->th->locals
#define RESTORE_LOCALS(env)     (env)->th->locals = __cur_locals__
#if 0
#   define PUSH_LOCAL_TABLE(env, tbl) \
do { \
    unsigned int i; \
    for (i = 0; i < tbl.num_vals; i++) { \
        DPRINTF("tbl.vals[%d]=%p", i, tbl.vals[i]); \
    } \
    tbl.next = (env)->th->locals; \
    (env)->th->locals = &tbl; \
} while (0)
#else
#   define PUSH_LOCAL_TABLE(env, tbl) \
do { \
    tbl.next = (env)->th->locals; \
    (env)->th->locals = &tbl; \
} while (0)
#endif
#define PUSH_LOCAL(env, x) \
    YogLocals __locals_##x##__; \
    __locals_##x##__.num_vals = 1; \
    __locals_##x##__.size = 1; \
    __locals_##x##__.vals[0] = &(x); \
    __locals_##x##__.vals[1] = NULL; \
    __locals_##x##__.vals[2] = NULL; \
    __locals_##x##__.vals[3] = NULL; \
    __locals_##x##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##__);
#define PUSH_LOCALS2(env, x, y) \
    YogLocals __locals_##x##_##y##__; \
    __locals_##x##_##y##__.num_vals = 2; \
    __locals_##x##_##y##__.size = 1; \
    __locals_##x##_##y##__.vals[0] = &(x); \
    __locals_##x##_##y##__.vals[1] = &(y); \
    __locals_##x##_##y##__.vals[2] = NULL; \
    __locals_##x##_##y##__.vals[3] = NULL; \
    __locals_##x##_##y##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##__);
#define PUSH_LOCALS3(env, x, y, z) \
    YogLocals __locals_##x##_##y##_##z##__; \
    __locals_##x##_##y##_##z##__.num_vals = 3; \
    __locals_##x##_##y##_##z##__.size = 1; \
    __locals_##x##_##y##_##z##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##__.vals[3] = NULL; \
    __locals_##x##_##y##_##z##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##__);
#define PUSH_LOCALS4(env, x, y, z, t) \
    YogLocals __locals_##x##_##y##_##z##_##t##__; \
    __locals_##x##_##y##_##z##_##t##__.num_vals = 4; \
    __locals_##x##_##y##_##z##_##t##__.size = 1; \
    __locals_##x##_##y##_##z##_##t##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##_##t##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##_##t##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##_##t##__.vals[3] = &(t); \
    __locals_##x##_##y##_##z##_##t##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##_##t##__);
#define PUSH_LOCALS5(env, x, y, z, t, u) \
    YogLocals __locals_##x##_##y##_##z##_##t##_##u##__; \
    __locals_##x##_##y##_##z##_##t##_##u##__.num_vals = 5; \
    __locals_##x##_##y##_##z##_##t##_##u##__.size = 1; \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[3] = &(t); \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[4] = &(u); \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##_##t##_##u##__);
#define PUSH_LOCALSX(env, num, x) \
    YogLocals __locals_##x##__; \
    __locals_##x##__.num_vals = 1; \
    __locals_##x##__.size = (num); \
    __locals_##x##__.vals[0] = (x); \
    __locals_##x##__.vals[1] = NULL; \
    __locals_##x##__.vals[2] = NULL; \
    __locals_##x##__.vals[3] = NULL; \
    __locals_##x##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##__);
#define SAVE_ARG(env, x)        SAVE_LOCALS((env)); \
                                PUSH_LOCAL((env), x)
#define SAVE_ARGS2(env, x, y)   SAVE_LOCALS((env)); \
                                PUSH_LOCALS2((env), x, y)
#define SAVE_ARGS3(env, x, y, z)  \
                                SAVE_LOCALS((env)); \
                                PUSH_LOCALS3((env), x, y, z)
#define SAVE_ARGS4(env, x, y, z, t)  \
                                SAVE_LOCALS((env)); \
                                PUSH_LOCALS4((env), x, y, z, t)
#define SAVE_ARGS5(env, x, y, z, t, u)  \
                                SAVE_LOCALS((env)); \
                                PUSH_LOCALS5((env), x, y, z, t, u)
#define POP_LOCALS(env)         (env)->th->locals = (env)->th->locals->next
#define RETURN(env, val)        do { \
    RESTORE_LOCALS(env); \
    return val; \
} while (0)
#define RETURN_VOID(env)        do { \
    RESTORE_LOCALS(env); \
    return; \
} while (0)

struct YogThread {
    YogVal cur_frame;
    struct YogJmpBuf* jmp_buf_list;
    YogVal jmp_val;
    struct YogLocals* locals;
#if defined(GC_GENERATIONAL)
    YogVal** ref_tbl;
    YogVal** ref_tbl_ptr;
    YogVal** ref_tbl_limit;
#endif
};

typedef struct YogThread YogThread;

#include "yog/error.h"

#if defined(GC_GENERATIONAL)
#   include <stdlib.h>
#   include "yog/env.h"
#   include "yog/vm.h"
#   define IS_YOUNG_PTR(env, ptr)  (((env)->vm->gc.generational.copying.active_heap->items <= (unsigned char*)(ptr)) && ((unsigned char*)(ptr) <= (env)->vm->gc.generational.copying.active_heap->items + (env)->vm->gc.generational.copying.active_heap->size))
#   define IS_YOUNG(env, val)   (IS_PTR((val)) && IS_YOUNG_PTR((env), VAL2PTR(val)))
#   define REF_TBL_GROW_SIZE    256
#   define ADD_REF(env, val)    do { \
    YogThread* thread = (env)->th; \
    if (thread->ref_tbl_ptr == thread->ref_tbl_limit) { \
        unsigned int size = thread->ref_tbl_limit - thread->ref_tbl; \
        unsigned int new_size = size + REF_TBL_GROW_SIZE * sizeof(void**); \
        thread->ref_tbl = realloc(thread->ref_tbl, new_size); \
        YOG_ASSERT((env), thread->ref_tbl != NULL, "Can't grow ref_tbl"); \
        thread->ref_tbl_ptr = thread->ref_tbl + size / sizeof(YogVal*); \
        thread->ref_tbl_limit = thread->ref_tbl + new_size / sizeof(YogVal*); \
    } \
    *thread->ref_tbl_ptr = &(val); \
    thread->ref_tbl_ptr++; \
} while (0)
#   define MODIFY(env, fp, val)    do { \
    if (!IS_YOUNG_PTR((env), &(fp))) { \
        YogVal old = (fp); \
        if (!IS_YOUNG((env), old) && IS_YOUNG((env), (val))) { \
            ADD_REF((env), (fp)); \
        } \
    } \
    (fp) = (val); \
} while (0)
#else
#   define ADD_REF(env, ptr)
#   define MODIFY(env, fp, val)     (fp) = (val)
#endif

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/thread.c */
YogVal YogThread_call_block(YogEnv*, YogThread*, YogVal, unsigned int, YogVal*);
YogVal YogThread_call_method(YogEnv*, YogThread*, YogVal, const char*, unsigned int, YogVal*);
YogVal YogThread_call_method_id(YogEnv*, YogThread*, YogVal, ID, unsigned int, YogVal*);
void YogThread_eval_package(YogEnv*, YogThread*, YogVal);
void YogThread_finalize(YogEnv*, YogThread*);
void YogThread_initialize(YogEnv*, YogVal);
void YogThread_keep_children(YogEnv*, void*, ObjectKeeper);
YogVal YogThread_new(YogEnv*);
void YogThread_shrink_ref_tbl(YogEnv*, YogThread*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
