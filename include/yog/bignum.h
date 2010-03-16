#if !defined(__YOG_BIGNUM_H__)
#define __YOG_BIGNUM_H__

#include "gmp.h"
#include "yog/object.h"
#include "yog/yog.h"

struct YogBignum {
    YOGBASICOBJ_HEAD;
    mpz_t num;
};

typedef struct YogBignum YogBignum;

DECL_AS_TYPE(YogBignum_define_classes);
#define TYPE_BIGNUM TO_TYPE(YogBignum_define_classes)

#define BIGNUM_NUM(bignum)  PTR_AS(YogBignum, (bignum))->num

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/bignum.c */
YOG_EXPORT YogVal YogBignum_and(YogEnv*, YogVal, YogVal);
YOG_EXPORT void YogBignum_define_classes(YogEnv*, YogVal);
YOG_EXPORT YogVal YogBignum_from_int(YogEnv*, int_t);
YOG_EXPORT YogVal YogBignum_from_str(YogEnv*, YogVal, int_t);
YOG_EXPORT YogVal YogBignum_lshift(YogEnv*, YogVal, int_t);
YOG_EXPORT YogVal YogBignum_modulo(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogBignum_multiply(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogBignum_or(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogBignum_power(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogBignum_subtract(YogEnv*, YogVal, YogVal);
YOG_EXPORT SIGNED_TYPE YogBignum_to_signed_type(YogEnv*, YogVal, const char*);
YOG_EXPORT YogVal YogBignum_xor(YogEnv*, YogVal, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
