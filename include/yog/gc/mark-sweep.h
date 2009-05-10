#if !defined(__YOG_MARK_SWEEP_H__)
#define __YOG_MARK_SWEEP_H__

#include <stddef.h>

/* TODO: commonize with yog/yog.h */
#if !defined(__YOG_YOG_H__)
typedef struct YogEnv YogEnv;
typedef void* (*ObjectKeeper)(YogEnv*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper);
typedef void (*Finalizer)(YogEnv*, void*);
#endif

struct YogMarkSweep {
    struct YogMarkSweepHeader* header;
    size_t threshold;
    size_t allocated_size;
    void* root;
    ChildrenKeeper root_keeper;
};

typedef struct YogMarkSweep YogMarkSweep;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/gc/mark-sweep.c */
void* YogMarkSweep_alloc(YogEnv*, YogMarkSweep*, ChildrenKeeper, Finalizer, size_t);
void YogMarkSweep_finalize(YogEnv*, YogMarkSweep*);
void YogMarkSweep_gc(YogEnv*, YogMarkSweep*);
void YogMarkSweep_initialize(YogEnv*, YogMarkSweep*, size_t, void*, ChildrenKeeper);
void YogMarkSweep_prepare(YogEnv*, YogMarkSweep*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
