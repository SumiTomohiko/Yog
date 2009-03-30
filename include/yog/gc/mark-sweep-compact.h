#ifndef __YOG_GC_MARK_SWEEP_COMPACT_H__
#define __YOG_GC_MARK_SWEEP_COMPACT_H__

#include <stddef.h>

/* TODO: commonize with yog/yog.h */
#if !defined(__YOG_YOG_H__)
typedef int BOOL;
#define FALSE   0
#define TRUE    (!(FALSE))
typedef struct YogEnv YogEnv;
typedef void* (*ObjectKeeper)(YogEnv*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper);
typedef void (*Finalizer)(YogEnv*, void*);
#endif

struct YogMarkSweepCompactHeader {
    unsigned int flags;
#if 0
    struct GcObjectStat stat;
#endif
    struct YogMarkSweepCompactHeader* forwarding_addr;
    struct YogMarkSweepCompactHeader* prev;
    struct YogMarkSweepCompactHeader* next;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    BOOL marked;
    BOOL updated;
    size_t size;
};

#define OBJ_USED        0x01
#define IS_OBJ_USED(o)  ((o)->flags & OBJ_USED)

typedef struct YogMarkSweepCompactHeader YogMarkSweepCompactHeader;

#define MARK_SWEEP_COMPACT_NUM_SIZE         7
#define MARK_SWEEP_COMPACT_SIZE2INDEX_SIZE  2049

#define ERR_MSC_NONE    0
#define ERR_MSC_MMAP    1
#define ERR_MSC_MUNMAP  2
#define ERR_MSC_MALLOC  3

struct YogMarkSweepCompact {
    unsigned int err;
    size_t chunk_size;
    struct YogMarkSweepCompactChunk* chunks;
    struct YogMarkSweepCompactChunk* all_chunks;
    struct YogMarkSweepCompactChunk* all_chunks_last;
    struct YogMarkSweepCompactPage* pages[MARK_SWEEP_COMPACT_NUM_SIZE];
    size_t freelist_size[MARK_SWEEP_COMPACT_NUM_SIZE];
    unsigned int size2index[MARK_SWEEP_COMPACT_SIZE2INDEX_SIZE];
    struct YogMarkSweepCompactHeader* header;
    size_t threshold;
    size_t allocated_size;
    void* root;
    ChildrenKeeper root_keeper;
#if defined(GC_GENERATIONAL)
    BOOL in_gc;
#endif
};

typedef struct YogMarkSweepCompact YogMarkSweepCompact;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/gc/mark-sweep-compact.c */
void* YogMarkSweepCompact_alloc(YogEnv*, YogMarkSweepCompact*, ChildrenKeeper, Finalizer, size_t);
void YogMarkSweepCompact_delete_garbage(YogEnv*, YogMarkSweepCompact*);
void YogMarkSweepCompact_do_compaction(YogEnv*, YogMarkSweepCompact*, ObjectKeeper);
void YogMarkSweepCompact_finalize(YogEnv*, YogMarkSweepCompact*);
void YogMarkSweepCompact_gc(YogEnv*, YogMarkSweepCompact*);
void YogMarkSweepCompact_initialize(YogEnv*, YogMarkSweepCompact*, size_t, size_t, void*, ChildrenKeeper);
void* YogMarkSweepCompact_mark_recursively(YogEnv*, void*, ObjectKeeper);
void YogMarkSweepCompact_unmark_all(YogEnv*, YogMarkSweepCompact*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
