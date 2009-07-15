#if !defined(__YOG_BIGNUM_H__)
#define __YOG_BIGNUM_H__

#include "yog/object.h"

struct YogBignum {
    YOGBASICOBJ_HEAD;

    unsigned int size;
    unsigned char items[0];
};

typedef struct YogBignum YogBignum;

/* PROTOTYPE_START */
/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
