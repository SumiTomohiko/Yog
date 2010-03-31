#if !defined(__YOG_GC_BDW_H__)
#define __YOG_GC_BDW_H__

#include <stddef.h>
#include "yog/gc.h"
#include "yog/yog.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/gc/bdw.c */
YOG_EXPORT void* YogBDW_alloc(YogEnv*, YogHeap*, ChildrenKeeper, Finalizer, size_t);
YOG_EXPORT YogHeap* YogBDW_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
