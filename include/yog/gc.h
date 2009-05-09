#if !defined(__YOG_GC_H__)
#define __YOG_GC_H__

#define ALLOC_OBJ_SIZE(env, keep_children, finalizer, size) \
    YogGC_allocate(env, keep_children, finalizer, size)
#define ALLOC_OBJ(env, keep_children, finalizer, type) \
    ALLOC_OBJ_SIZE(env, keep_children, finalizer, sizeof(type))
#define ALLOC_OBJ_ITEM(env, keep_children, finalizer, type, size, item_type) \
    ALLOC_OBJ_SIZE(env, keep_children, finalizer, sizeof(type) + size * sizeof(item_type))

#include <sys/types.h>
#include "yog/yog.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/gc.c */
YogVal YogGC_allocate(YogEnv*, ChildrenKeeper, Finalizer, size_t);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */