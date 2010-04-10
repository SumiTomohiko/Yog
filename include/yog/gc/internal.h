#if !defined(__YOG_GC_INTERNAL_H__)
#define __YOG_GC_INTERNAL_H__

/**
 * This header includes definitions for the generational GC. Don't disclose this
 * header to external.
 */

#include "yog/yog.h"

struct RememberedSet {
    uint_t pos;
    uint_t size;
    void* items[0];
};

typedef struct RememberedSet RememberedSet;

#define SIZEOF_REMEMBERED_SET(size) \
                                (sizeof(RememberedSet) + sizeof(void*) * (size))

struct YoungHeader {
    uint_t age;

    /**
     * generation must be at the end.
     */
    uint_t generation;
};

typedef struct YoungHeader YoungHeader;

struct OldHeader {
    BOOL remembered;

    /**
     * generation must be at the end.
     */
    uint_t generation;
};

typedef struct OldHeader OldHeader;

RememberedSet* YogGenerational_get_remembered_set(YogEnv*, YogHeap*);
void YogGenerational_trace_remembered_set(YogEnv*, YogHeap*, RememberedSet*);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
