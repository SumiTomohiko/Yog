#if !defined(__YOG_VM_H__)
#define __YOG_VM_H__

#include <pthread.h>
#if HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
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
#include "yog/yog.h"

#define SURVIVE_INDEX_MAX    8

struct YogVM {
    BOOL gc_stress;

    ID next_id;
    YogVal id2name;
    YogVal name2id;
    pthread_rwlock_t sym_lock;

    YogVal cObject;
    YogVal cKlass;
    YogVal cInt;
    YogVal cString;
    YogVal cRegexp;
    YogVal cMatch;
    YogVal cPackage;
    YogVal cBool;
    YogVal cBuiltinBoundMethod;
    YogVal cBoundMethod;
    YogVal cBuiltinUnboundMethod;
    YogVal cUnboundMethod;
    YogVal cPackageBlock;
    YogVal cNil;
    YogVal cFloat;
    YogVal cThread;

    YogVal eException;
    YogVal eBugException;
    YogVal eTypeError;
    YogVal eIndexError;

    YogVal pkgs;
    pthread_rwlock_t pkgs_lock;
    YogVal search_path;

    YogVal encodings;

    YogVal main_thread;
    YogVal running_threads;

    pthread_mutex_t global_interp_lock;
    BOOL running_gc;
    BOOL waiting_suspend;
    unsigned int suspend_counter;
    pthread_cond_t threads_suspend_cond;
    pthread_cond_t gc_finish_cond;
    void* heaps;
    void* last_heap;
    pthread_cond_t vm_finish_cond;
    unsigned int gc_id;
};

typedef struct YogVM YogVM;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/vm.c */
void YogVM_acquire_global_interp_lock(YogEnv*, YogVM*);
void YogVM_add_heap(YogEnv*, YogVM*, void*);
void YogVM_add_thread(YogEnv*, YogVM*, YogVal);
void YogVM_boot(YogEnv*, YogVM*);
void YogVM_configure_search_path(YogEnv*, YogVM*, const char*);
void YogVM_delete(YogEnv*, YogVM*);
const char* YogVM_id2name(YogEnv*, YogVM*, ID);
YogVal YogVM_import_package(YogEnv*, YogVM*, ID);
void YogVM_init(YogVM*);
ID YogVM_intern(YogEnv*, YogVM*, const char*);
void YogVM_keep_children(YogEnv*, void*, ObjectKeeper, void*);
void YogVM_register_package(YogEnv*, YogVM*, const char*, YogVal);
void YogVM_release_global_interp_lock(YogEnv*, YogVM*);
void YogVM_remove_thread(YogEnv*, YogVM*, YogVal);
void YogVM_set_main_thread(YogEnv*, YogVM*, YogVal);
void YogVM_wait_finish(YogEnv*, YogVM*);

/* PROTOTYPE_END */

#include "yog/env.h"

#define INTERN(s)   YogVM_intern(env, env->vm, s)

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
