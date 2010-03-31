#if !defined(__YOG_MARK_SWEEP_H__)
#define __YOG_MARK_SWEEP_H__

#include <stddef.h>
#include "yog/gc.h"
#include "yog/yog.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/gc/mark-sweep.c */
YOG_EXPORT void* YogMarkSweep_alloc(YogEnv*, YogHeap*, ChildrenKeeper, Finalizer, size_t);
YOG_EXPORT void YogMarkSweep_delete(YogEnv*, YogHeap*);
YOG_EXPORT void YogMarkSweep_delete_garbage(YogEnv*, YogHeap*);
YOG_EXPORT BOOL YogMarkSweep_is_empty(YogEnv*, YogHeap*);
YOG_EXPORT void YogMarkSweep_keep_vm(YogEnv*, YogHeap*);
YOG_EXPORT YogHeap* YogMarkSweep_new(YogEnv*, size_t);
YOG_EXPORT void YogMarkSweep_prepare(YogEnv*, YogHeap*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
