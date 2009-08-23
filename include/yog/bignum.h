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
YogVal YogBignum_and(YogEnv*, YogVal, YogVal);
YogVal YogBignum_from_int(YogEnv*, int_t);
YogVal YogBignum_from_str(YogEnv*, YogVal, int_t);
YogVal YogBignum_define_class(YogEnv*);
YogVal YogBignum_lshift(YogEnv*, YogVal, int_t);
YogVal YogBignum_modulo(YogEnv*, YogVal, YogVal);
YogVal YogBignum_multiply(YogEnv*, YogVal, YogVal);
YogVal YogBignum_or(YogEnv*, YogVal, YogVal);
YogVal YogBignum_power(YogEnv*, YogVal, YogVal);
YogVal YogBignum_subtract(YogEnv*, YogVal, YogVal);
YogVal YogBignum_xor(YogEnv*, YogVal, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
