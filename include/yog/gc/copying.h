#ifndef __COPYING_H__
#define __COPYING_H__

/* TODO: commonize with yog/yog.h */
#if !defined(__YOG_YOG_H__) && !defined(__YOG_GC_MARK_SWEEP_COMPACT_H__)
typedef int BOOL;
#define FALSE   0
#define TRUE    (!(FALSE))
typedef struct YogEnv YogEnv;
typedef void* (*ObjectKeeper)(YogEnv*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper);
typedef void (*Finalizer)(YogEnv*, void*);
#endif

struct YogCopyingHeader {
#if 0
    struct GcObjectStat stat;
#endif
    ChildrenKeeper keeper;
    Finalizer finalizer;
    void* forwarding_addr;
    size_t size;
    unsigned int id;
#if defined(GC_GENERATIONAL)
    unsigned int servive_num;
    BOOL updated;
#endif
};

typedef struct YogCopyingHeader YogCopyingHeader;

#define ERR_COPYING_NONE            0
#define ERR_COPYING_MALLOC          1
#define ERR_COPYING_OUT_OF_MEMORY   2

struct YogCopyingHeap {
    size_t size;
    unsigned char* free;
    unsigned char items[0];
};

typedef struct YogCopyingHeap YogCopyingHeap;

struct YogCopying {
    unsigned int err;
    BOOL stress;
    struct YogCopyingHeap* active_heap;
    struct YogCopyingHeap* inactive_heap;
    unsigned char* scanned;
    unsigned char* unscanned;
    void* root;
    ChildrenKeeper root_keeper;
};

typedef struct YogCopying YogCopying;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/gc/copying.c */
void* YogCopying_alloc(YogEnv*, YogCopying*, ChildrenKeeper, Finalizer, size_t);
void* YogCopying_copy(YogEnv*, YogCopying*, void*);
void YogCopying_do_gc(YogEnv*, YogCopying*, ObjectKeeper);
void YogCopying_finalize(YogEnv*, YogCopying*);
void YogCopying_gc(YogEnv*, YogCopying*);
void YogCopying_initialize(YogEnv*, YogCopying*, BOOL, size_t, void*, ChildrenKeeper);
BOOL YogCopying_is_in_active_heap(YogEnv*, YogCopying*, void*);
BOOL YogCopying_is_in_inactive_heap(YogEnv*, YogCopying*, void*);
void YogCopying_iterate_objects(YogEnv*, YogCopying*, void (*)(YogEnv*, YogCopyingHeader*));

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
