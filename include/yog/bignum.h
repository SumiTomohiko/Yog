#if !defined(YOG_BIGNUM_H_INCLUDED)
#define YOG_BIGNUM_H_INCLUDED

#include <bigd.h>
#include "yog/object.h"
#include "yog/yog.h"

struct YogBignum {
    YOGBASICOBJ_HEAD;
    BIGD num;
    int_t sign; /* 1 or -1 */
};

typedef struct YogBignum YogBignum;

#define TYPE_BIGNUM TO_TYPE(YogBignum_define_classes)

#define BIGNUM_NUM(bignum)  PTR_AS(YogBignum, (bignum))->num
#define BIGNUM_SIGN(bignum) PTR_AS(YogBignum, (bignum))->sign

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/bignum.c */
YogVal YogBignum_and(YogEnv*, YogHandle*, int_t);
YogVal YogBignum_binop_add(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_and(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_divide(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_floor_divide(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_lshift(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_modulo(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_multiply(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_or(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_power(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_rshift(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_subtract(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_binop_ufo(YogEnv*, YogVal, YogVal);
YogVal YogBignum_binop_xor(YogEnv*, YogHandle*, YogHandle*);
int YogBignum_compare_with_int(YogEnv*, YogVal, int_t);
int YogBignum_compare_with_long(YogEnv*, YogVal, long);
int_t YogBignum_compare_with_long_long(YogEnv*, YogVal, long long);
int YogBignum_compare_with_unsigned_int(YogEnv*, YogVal, uint_t);
int YogBignum_compare_with_unsigned_long(YogEnv*, YogVal, unsigned long);
int_t YogBignum_compare_with_unsigned_long_long(YogEnv*, YogVal, unsigned long long);
void YogBignum_define_classes(YogEnv*, YogVal);
YogVal YogBignum_from_int(YogEnv*, int_t);
YogVal YogBignum_from_long_long(YogEnv*, long long);
YogVal YogBignum_from_str(YogEnv*, YogVal, int_t);
YogVal YogBignum_from_unsigned_int(YogEnv*, uint_t);
YogVal YogBignum_from_unsigned_long_long(YogEnv*, unsigned long long);
YogVal YogBignum_lshift(YogEnv*, YogHandle*, int_t);
YogVal YogBignum_modulo(YogEnv*, YogHandle*, YogHandle*);
YogVal YogBignum_or(YogEnv*, YogHandle*, int_t);
YogVal YogBignum_power(YogEnv*, YogHandle*, int_t);
double YogBignum_to_float(YogEnv*, YogHandle*);
long YogBignum_to_long(YogEnv*, YogVal, const char*);
long long YogBignum_to_long_long(YogEnv*, YogVal, const char*);
YogVal YogBignum_to_s(YogEnv*, YogVal);
SIGNED_TYPE YogBignum_to_signed_type(YogEnv*, YogVal, const char*);
unsigned long YogBignum_to_unsigned_long(YogEnv*, YogVal, const char*);
unsigned long long YogBignum_to_unsigned_long_long(YogEnv*, YogVal, const char*);
UNSIGNED_TYPE YogBignum_to_unsigned_type(YogEnv*, YogVal, const char*);
YogVal YogBignum_xor(YogEnv*, YogHandle*, int_t);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
