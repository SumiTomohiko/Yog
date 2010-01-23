#if !defined(__YOG_THREAD_H__)
#define __YOG_THREAD_H__

#include <pthread.h>
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

struct YogThread {
    struct YogBasicObj base;

    YogVal prev;
    YogVal next;

    uint_t thread_id;
    uint_t next_obj_id;

    void* heap;
    BOOL gc_bound;

    struct YogJmpBuf* jmp_buf_list;
    YogVal jmp_val;
    YogVal frame_to_long_jump;

    YogVal block;

    pthread_t pthread;

    YogVal recursive_stack;

    YogEnv* env;
};

typedef struct YogThread YogThread;

#define TYPE_THREAD     ((type_t)YogThread_new)

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
    YogVal name##_cur_frame = env->frame; \
    PUSH_LOCAL((env), name##_cur_frame); \
    YogLocals* name##_locals = (env)->locals->body; \
    YogJmpBuf* name##_jmpbuf = PTR_AS(YogThread, (env)->thread)->jmp_buf_list
#define RESTORE_STAT(env, name) \
    PTR_AS(YogThread, (env)->thread)->jmp_buf_list = name##_jmpbuf; \
    PTR_AS(YogThread, (env)->thread)->env = (env); \
    (env)->locals->body = name##_locals; \
    env->frame = name##_cur_frame

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/thread.c */
YOG_EXPORT void YogThread_config_bdw(YogEnv*, YogVal);
YOG_EXPORT void YogThread_config_copying(YogEnv*, YogVal, size_t);
YOG_EXPORT void YogThread_config_generational(YogEnv*, YogVal, size_t, size_t, size_t, uint_t);
YOG_EXPORT void YogThread_config_mark_sweep(YogEnv*, YogVal, size_t);
YOG_EXPORT void YogThread_config_mark_sweep_compact(YogEnv*, YogVal, size_t, size_t);
YOG_EXPORT void YogThread_define_classes(YogEnv*, YogVal);
YOG_EXPORT void YogThread_init(YogEnv*, YogVal, YogVal);
YOG_EXPORT void YogThread_issue_object_id(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogThread_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
