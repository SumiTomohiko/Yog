#if !defined(__COPYING_H__)
#define __COPYING_H__

#if HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
#include "yog/yog.h"

struct YogCopyingHeader {
    ChildrenKeeper keeper;
    Finalizer finalizer;
    void* forwarding_addr;
    size_t size;
#if defined(GC_GENERATIONAL)
    unsigned int survive_num;
    BOOL updated;
    unsigned int gen;
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
    struct YogCopying* prev;
    struct YogCopying* next;
    BOOL refered;

    unsigned int err;
    size_t heap_size;
    struct YogCopyingHeap* active_heap;
    struct YogCopyingHeap* inactive_heap;
    unsigned char* scanned;
    unsigned char* unscanned;
};

typedef struct YogCopying YogCopying;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/gc/copying.c */
YOG_EXPORT void* YogCopying_alloc(YogEnv*, YogCopying*, ChildrenKeeper, Finalizer, size_t);
YOG_EXPORT void YogCopying_alloc_heap(YogEnv*, YogCopying*);
YOG_EXPORT void YogCopying_cheney_scan(YogEnv*, YogCopying*);
YOG_EXPORT void* YogCopying_copy(YogEnv*, YogCopying*, void*);
YOG_EXPORT void YogCopying_delete_garbage(YogEnv*, YogCopying*);
YOG_EXPORT void YogCopying_finalize(YogEnv*, YogCopying*);
YOG_EXPORT void YogCopying_init(YogEnv*, YogCopying*, size_t);
YOG_EXPORT BOOL YogCopying_is_empty(YogEnv*, YogCopying*);
YOG_EXPORT BOOL YogCopying_is_in_active_heap(YogEnv*, YogCopying*, void*);
YOG_EXPORT BOOL YogCopying_is_in_inactive_heap(YogEnv*, YogCopying*, void*);
YOG_EXPORT void YogCopying_iterate_objects(YogEnv*, YogCopying*, void (*)(YogEnv*, YogCopyingHeader*));
YOG_EXPORT void YogCopying_keep_vm(YogEnv*, YogCopying*);
YOG_EXPORT void YogCopying_post_gc(YogEnv*, YogCopying*);
YOG_EXPORT void YogCopying_prepare(YogEnv*, YogCopying*);
YOG_EXPORT void YogCopying_scan(YogEnv*, YogCopying*, ObjectKeeper, void*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
