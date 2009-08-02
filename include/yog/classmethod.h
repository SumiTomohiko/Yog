#if !defined(__YOG_CLASSMETHOD_H__)
#define __YOG_CLASSMETHOD_H__

#include "yog/object.h"
#include "yog/yog.h"

struct YogClassMethod {
    struct YogBasicObj base;
    YogVal f;
};

typedef struct YogClassMethod YogClassMethod;

/* PROTOTYPE_START */
/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
