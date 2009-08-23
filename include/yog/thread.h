#if !defined(__YOG_THREAD_H__)
#define __YOG_THREAD_H__

#include <setjmp.h>
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
#include "yog/object.h"
#include "yog/yog.h"

enum YogJmpStatus {
    JMP_RAISE = 1,
    JMP_RETURN = 2,
    JMP_BREAK = 3,
};

typedef enum YogJmpStatus YogJmpStatus;

struct YogJmpBuf {
    jmp_buf buf;
    struct YogJmpBuf* prev;
};

typedef struct YogJmpBuf YogJmpBuf;

struct YogLocals {
    struct YogLocals* next;
    uint_t num_vals;
    uint_t size;
    YogVal* vals[4];
};

typedef struct YogLocals YogLocals;

#include "yog/env.h"

#define SAVE_LOCALS(env) \
    YogLocals* __cur_locals__ = PTR_AS(YogThread, (env)->thread)->locals
#define RESTORE_LOCALS(env) \
    PTR_AS(YogThread, (env)->thread)->locals = __cur_locals__
#if 0
#   define PUSH_LOCAL_TABLE(env, tbl) \
do { \
    uint_t i; \
    TRACE("tbl=%p", &tbl); \
    for (i = 0; i < tbl.num_vals; i++) { \
        TRACE("tbl.vals[%d]=%p", i, tbl.vals[i]); \
    } \
    tbl.next = PTR_AS(YogThread, (env)->thread)->locals; \
    PTR_AS(YogThread, (env)->thread)->locals = &tbl; \
} while (0)
#else
#   define PUSH_LOCAL_TABLE(env, tbl) \
do { \
    tbl.next = PTR_AS(YogThread, (env)->thread)->locals; \
    PTR_AS(YogThread, (env)->thread)->locals = &tbl; \
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
    PUSH_LOCAL_TABLE(env, __locals_##x##__);
#define PUSH_LOCALS2(env, x, y) \
    YogLocals __locals_##x##_##y##__; \
    __locals_##x##_##y##__.num_vals = 2; \
    __locals_##x##_##y##__.size = 1; \
    __locals_##x##_##y##__.vals[0] = &(x); \
    __locals_##x##_##y##__.vals[1] = &(y); \
    __locals_##x##_##y##__.vals[2] = NULL; \
    __locals_##x##_##y##__.vals[3] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##__);
#define PUSH_LOCALS3(env, x, y, z) \
    YogLocals __locals_##x##_##y##_##z##__; \
    __locals_##x##_##y##_##z##__.num_vals = 3; \
    __locals_##x##_##y##_##z##__.size = 1; \
    __locals_##x##_##y##_##z##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##__.vals[3] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##__);
#define PUSH_LOCALS4(env, x, y, z, t) \
    YogLocals __locals_##x##_##y##_##z##_##t##__; \
    __locals_##x##_##y##_##z##_##t##__.num_vals = 4; \
    __locals_##x##_##y##_##z##_##t##__.size = 1; \
    __locals_##x##_##y##_##z##_##t##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##_##t##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##_##t##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##_##t##__.vals[3] = &(t); \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##_##t##__);
#define PUSH_LOCALS5(env, x, y, z, t, u) \
    PUSH_LOCALS4(env, x, y, z, t); \
    PUSH_LOCAL(env, u)
#define PUSH_LOCALS6(env, x, y, z, t, u, v) \
    PUSH_LOCALS4(env, x, y, z, t); \
    PUSH_LOCALS2(env, u, v)
#define PUSH_LOCALS7(env, x, y, z, t, u, v, w) \
    PUSH_LOCALS4(env, x, y, z, t); \
    PUSH_LOCALS3(env, u, v, w)
#define PUSH_LOCALS8(env, x, y, z, t, u, v, w, p) \
    PUSH_LOCALS4(env, x, y, z, t); \
    PUSH_LOCALS4(env, u, v, w, p)
#define PUSH_LOCALSX(env, num, x) \
    YogLocals __locals_##x##__; \
    __locals_##x##__.num_vals = 1; \
    __locals_##x##__.size = (num); \
    __locals_##x##__.vals[0] = (x); \
    __locals_##x##__.vals[1] = NULL; \
    __locals_##x##__.vals[2] = NULL; \
    __locals_##x##__.vals[3] = NULL; \
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
#define POP_LOCALS(env) do { \
    YogLocals* next = PTR_AS(YogThread, (env)->thread)->locals->next; \
    PTR_AS(YogThread, (env)->thread)->locals = next; \
} while (0)
#define RETURN(env, val)        do { \
    RESTORE_LOCALS(env); \
    return val; \
} while (0)
#define RETURN_VOID(env)        do { \
    RESTORE_LOCALS(env); \
    return; \
} while (0)

struct YogThread {
    struct YogBasicObj base;

    YogVal prev;
    YogVal next;

    uint_t thread_id;
    uint_t next_obj_id;

    void* heap;
    struct YogLocals* locals;
    BOOL gc_bound;

    YogVal cur_frame;

    struct YogJmpBuf* jmp_buf_list;
    YogVal jmp_val;
    YogVal frame_to_long_jump;

    YogVal block;

    pthread_t pthread;

    YogVal recursive_stack;
};

typedef struct YogThread YogThread;

#define __THREAD_HEAP__(type, thread)   ((type*)PTR_AS(YogThread, (thread))->heap)
#if defined(GC_COPYING)
#   define THREAD_HEAP(thread)  __THREAD_HEAP__(YogCopying, thread)
#elif defined(GC_MARK_SWEEP)
#   define THREAD_HEAP(thread)  __THREAD_HEAP__(YogMarkSweep, thread)
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define THREAD_HEAP(thread)  __THREAD_HEAP__(YogMarkSweepCompact, thread)
#elif defined(GC_GENERATIONAL)
#   define THREAD_HEAP(thread)  __THREAD_HEAP__(YogGenerational, thread)
#elif defined(GC_BDW)
#   define THREAD_HEAP(thread)  __THREAD_HEAP__(YogBDW, thread)
#endif

#define PUSH_JMPBUF(thread, jmpbuf)     do { \
    jmpbuf.prev = PTR_AS(YogThread, (thread))->jmp_buf_list; \
    PTR_AS(YogThread, (thread))->jmp_buf_list = &jmpbuf; \
} while (0)
#define POP_JMPBUF(env)     do { \
    YogJmpBuf* prev = PTR_AS(YogThread, env->thread)->jmp_buf_list->prev; \
    PTR_AS(YogThread, env->thread)->jmp_buf_list = prev; \
} while (0)
#define LONGJMP(env, status)    do { \
    YogJmpBuf* list = PTR_AS(YogThread, env->thread)->jmp_buf_list; \
    YOG_ASSERT(env, list != NULL, "no longjmp destination"); \
    longjmp(list->buf, status); \
} while (0)

#define SAVE_CURRENT_STAT(env, name)    \
    YogVal name##_cur_frame = PTR_AS(YogThread, env->thread)->cur_frame; \
    PUSH_LOCAL(env, name##_cur_frame); \
    YogLocals* name##_locals = PTR_AS(YogThread, env->thread)->locals; \
    YogJmpBuf* name##_jmpbuf = PTR_AS(YogThread, env->thread)->jmp_buf_list
#define RESTORE_STAT(env, name) \
    PTR_AS(YogThread, env->thread)->jmp_buf_list = name##_jmpbuf; \
    PTR_AS(YogThread, env->thread)->locals = name##_locals; \
    PTR_AS(YogThread, env->thread)->cur_frame = name##_cur_frame

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/thread.c */
void YogThread_config_bdw(YogEnv*, YogVal);
void YogThread_config_copying(YogEnv*, YogVal, size_t);
void YogThread_config_generational(YogEnv*, YogVal, size_t, size_t, size_t, uint_t);
void YogThread_config_mark_sweep(YogEnv*, YogVal, size_t);
void YogThread_config_mark_sweep_compact(YogEnv*, YogVal, size_t, size_t);
void YogThread_initialize(YogEnv*, YogVal, YogVal);
void YogThread_issue_object_id(YogEnv*, YogVal, YogVal);
YogVal YogThread_define_class(YogEnv*);
YogVal YogThread_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
