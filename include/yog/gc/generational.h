#if !defined(__YOG_GC_GENERATIONAL_H__)
#define __YOG_GC_GENERATIONAL_H__

#include <stddef.h>
#include "yog/gc.h"
#include "yog/yog.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/gc/generational.c */
YOG_EXPORT void* YogGenerational_alloc(YogEnv*, YogHeap*, ChildrenKeeper, Finalizer, size_t);
YOG_EXPORT void YogGenerational_alloc_heap(YogEnv*, YogHeap*);
YOG_EXPORT void* YogGenerational_copy_young_object(YogEnv*, void*, ObjectKeeper, void*);
YOG_EXPORT void YogGenerational_delete(YogEnv*, YogHeap*);
YOG_EXPORT BOOL YogGenerational_is_empty(YogEnv*, YogHeap*);
YOG_EXPORT void YogGenerational_major_cheney_scan(YogEnv*, YogHeap*);
YOG_EXPORT void YogGenerational_major_delete_garbage(YogEnv*, YogHeap*);
YOG_EXPORT void YogGenerational_major_keep_vm(YogEnv*, YogHeap*);
YOG_EXPORT void YogGenerational_major_post_gc(YogEnv*, YogHeap*);
YOG_EXPORT void YogGenerational_minor_cheney_scan(YogEnv*, YogHeap*);
YOG_EXPORT void YogGenerational_minor_delete_garbage(YogEnv*, YogHeap*);
YOG_EXPORT void YogGenerational_minor_keep_vm(YogEnv*, YogHeap*);
YOG_EXPORT void YogGenerational_minor_post_gc(YogEnv*, YogHeap*);
YOG_EXPORT YogHeap* YogGenerational_new(YogEnv*, size_t, size_t, size_t, uint_t);
YOG_EXPORT void YogGenerational_prepare(YogEnv*, YogHeap*);
YOG_EXPORT void YogGenerational_trace_grey(YogEnv*, YogHeap*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
