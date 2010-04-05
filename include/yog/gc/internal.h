#if !defined(__YOG_GC_INTERNAL_H__)
#define __YOG_GC_INTERNAL_H__

/**
 * This header includes definitions for the generational GC. Don't disclose this
 * header to external.
 */

#include "yog/yog.h"

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

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */

