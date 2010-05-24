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
YOG_EXPORT int_t YogBignum_compare_with_int(YogEnv*, YogVal, int_t);
YOG_EXPORT int_t YogBignum_compare_with_long_long(YogEnv*, YogVal, long long);
YOG_EXPORT int_t YogBignum_compare_with_unsigned_int(YogEnv*, YogVal, uint_t);
YOG_EXPORT int_t YogBignum_compare_with_unsigned_long_long(YogEnv*, YogVal, unsigned long long);
YOG_EXPORT void YogBignum_define_classes(YogEnv*, YogVal);
YOG_EXPORT YogVal YogBignum_from_int(YogEnv*, int_t);
YOG_EXPORT YogVal YogBignum_from_long_long(YogEnv*, long long);
YOG_EXPORT YogVal YogBignum_from_str(YogEnv*, YogVal, int_t);
YOG_EXPORT YogVal YogBignum_from_unsigned_int(YogEnv*, uint_t);
YOG_EXPORT YogVal YogBignum_from_unsigned_long_long(YogEnv*, unsigned long long);
YOG_EXPORT YogVal YogBignum_lshift(YogEnv*, YogVal, int_t);
YOG_EXPORT YogVal YogBignum_modulo(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogBignum_multiply(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogBignum_or(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogBignum_power(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogBignum_subtract(YogEnv*, YogVal, YogVal);
YOG_EXPORT long long YogBignum_to_long_long(YogEnv*, YogVal, const char*);
YOG_EXPORT YogVal YogBignum_to_s(YogEnv*, YogVal);
YOG_EXPORT SIGNED_TYPE YogBignum_to_signed_type(YogEnv*, YogVal, const char*);
YOG_EXPORT unsigned long long YogBignum_to_unsigned_long_long(YogEnv*, YogVal, const char*);
YOG_EXPORT UNSIGNED_TYPE YogBignum_to_unsigned_type(YogEnv*, YogVal, const char*);
YOG_EXPORT YogVal YogBignum_xor(YogEnv*, YogVal, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
