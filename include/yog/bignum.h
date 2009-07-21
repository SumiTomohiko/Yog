#if !defined(__YOG_BIGNUM_H__)
#define __YOG_BIGNUM_H__

#include "gmp.h"
#include "yog/object.h"

struct YogBignum {
    YOGBASICOBJ_HEAD;
    mpz_t num;
};

typedef struct YogBignum YogBignum;

#define BIGNUM_NUM(bignum)  PTR_AS(YogBignum, (bignum))->num

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/bignum.c */
YogVal YogBignum_from_int(YogEnv*, int);
YogVal YogBignum_from_str(YogEnv*, YogVal, int);
YogVal YogBignum_klass_new(YogEnv*);
YogVal YogBignum_multiply(YogEnv*, YogVal, YogVal);
YogVal YogBignum_subtract(YogEnv*, YogVal, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */