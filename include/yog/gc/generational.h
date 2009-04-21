#if !defined(__YOG_GC_GENERATIONAL_H__)
#define __YOG_GC_GENERATIONAL_H__

#include <stddef.h>
#include "yog/gc/copying.h"
#include "yog/gc/mark-sweep-compact.h"

/* TODO: commonize with yog/yog.h */
#if !defined(__YOG_YOG_H__) && !defined(__YOG_GC_GENERATIONAL_H__)
typedef int BOOL;
#define FALSE   0
#define TRUE    (!(FALSE))
typedef struct YogEnv YogEnv;
typedef void* (*ObjectKeeper)(YogEnv*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper);
typedef void (*Finalizer)(YogEnv*, void*);
#endif

#define ERR_GEN_NONE        0
#define ERR_GEN_MMAP        1
#define ERR_GEN_MUNMAP      2
#define ERR_GEN_MALLOC      3
#define ERR_GEN_UNKNOWN     4

struct YogGenerational {
    unsigned int err;
    struct YogCopying copying;
    struct YogMarkSweepCompact msc;
    unsigned int tenure;
};

typedef struct YogGenerational YogGenerational;

#define PTR2GEN(p)      (*((unsigned int*)(p) - 1))
#define IS_YOUNG(p)     (PTR2GEN(p) == GEN_YOUNG)

#define GEN_YOUNG   1
#define GEN_OLD     2

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/gc/generational.c */
void* YogGenerational_alloc(YogEnv*, YogGenerational*, ChildrenKeeper, Finalizer, size_t);
void* YogGenerational_copy_young_object(YogEnv*, void*, ObjectKeeper);
void YogGenerational_finalize(YogEnv*, YogGenerational*);
void YogGenerational_initialize(YogEnv*, YogGenerational*, BOOL, size_t, size_t, size_t, unsigned int, void*, ChildrenKeeper);
void YogGenerational_major_gc(YogEnv*, YogGenerational*);
void YogGenerational_minor_gc(YogEnv*, YogGenerational*);
void YogGenerational_oldify_all(YogEnv*, YogGenerational*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
