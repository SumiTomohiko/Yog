#include <math.h>
#include "gmp.h"
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/fixnum.h"
#include "yog/float.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_BIGNUM)) { \
        YogError_raise_TypeError((env), "self must be Bignum"); \
    } \
} while (0)
#define CHECK_SELF_TYPE2(env, self)  do { \
    YogVal bignum = HDL2VAL((self)); \
    if (!IS_PTR(bignum) || (BASIC_OBJ_TYPE(bignum) != TYPE_BIGNUM)) { \
        YogError_raise_TypeError((env), "self must be Bignum"); \
    } \
} while (0)

YogVal
YogBignum_to_s(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);

#define BASE 10
    /* 1 is for sign, another 1 is for a '\0' terminator */
    size_t size = mpz_sizeinbase(PTR_AS(YogBignum, self)->num, BASE) + 1 + 1;
    char buf[size];
    mpz_get_str(buf, BASE, PTR_AS(YogBignum, self)->num);
#undef BASE
    RETURN(env, YogString_from_string(env, buf));
}

static YogVal
to_s(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_s", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    s = YogBignum_to_s(env, self);

    RETURN(env, s);
}

static void
YogBignum_finalize(YogEnv* env, void* ptr)
{
    YogBignum* bignum = PTR_AS(YogBignum, ptr);
    mpz_clear(bignum->num);
}

static void
YogBignum_init(YogEnv* env, YogVal self)
{
    YogBasicObj_init(env, self, TYPE_BIGNUM, 0, env->vm->cBignum);
    mpz_init(PTR_AS(YogBignum, self)->num);
}

static YogVal
YogBignum_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = ALLOC_OBJ(env, YogBasicObj_keep_children, YogBignum_finalize, YogBignum);
    YogBignum_init(env, bignum);

    RETURN(env, bignum);
}

static YogVal
negative(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "-self", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    bignum = YogBignum_new(env);
    mpz_neg(BIGNUM_NUM(bignum), BIGNUM_NUM(self));

    RETURN(env, bignum);
}

YogVal
YogBignum_binop_add(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        YogVal result = YogBignum_from_int(env, VAL2INT(right));
        mpz_t* num = &BIGNUM_NUM(HDL2VAL(self));
        mpz_add(BIGNUM_NUM(result), *num, BIGNUM_NUM(result));
        return result;
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        mpz_t* num = &BIGNUM_NUM(HDL2VAL(self));
        FLOAT_NUM(result) = mpz_get_d(*num) + FLOAT_NUM(HDL2VAL(n));
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal result = YogBignum_new(env);
        mpz_t* num = &BIGNUM_NUM(HDL2VAL(self));
        mpz_add(BIGNUM_NUM(result), *num, BIGNUM_NUM(HDL2VAL(n)));
        return result;
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "+");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
add(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_add(env, self, n);
}

static YogVal
normalize(YogEnv* env, YogVal self)
{
    if (!mpz_fits_sint_p(BIGNUM_NUM(self))) {
        return self;
    }
    int_t n = mpz_get_si(BIGNUM_NUM(self));
    if (!FIXABLE(n)) {
        return self;
    }
    return INT2VAL(n);
}

YogVal
YogBignum_binop_subtract(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        YogVal bignum = YogBignum_from_int(env, VAL2INT(right));
        YogHandle* h = YogHandle_REGISTER(env, bignum);
        return YogBignum_binop_subtract(env, self, h);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        mpz_t* h = &BIGNUM_NUM(HDL2VAL(self));
        FLOAT_NUM(result) = mpz_get_d(*h) - FLOAT_NUM(HDL2VAL(n));
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal result = YogBignum_new(env);
        mpz_t* h = &BIGNUM_NUM(HDL2VAL(self));
        mpz_sub(BIGNUM_NUM(result), *h, BIGNUM_NUM(HDL2VAL(n)));
        return normalize(env, result);
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "-");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
subtract(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_subtract(env, self, n);
}

static YogVal
multiply(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_multiply(env, self, n);
}

static YogVal
divide_int(YogEnv* env, YogHandle* self, int_t right)
{
    if (right == 0) {
        YogError_raise_ZeroDivisionError(env, "Bignum division by zero");
    }
    YogVal result = YogFloat_new(env);
    FLOAT_NUM(result) = mpz_get_d(BIGNUM_NUM(HDL2VAL(self))) / right;
    return result;
}

static YogVal
divide_float(YogEnv* env, YogHandle* self, double right)
{
    if (right == 0.0) {
        YogError_raise_ZeroDivisionError(env, "Float division");
        /* NOTREACHED */
    }
    YogVal result = YogFloat_new(env);
    FLOAT_NUM(result) = mpz_get_d(BIGNUM_NUM(HDL2VAL(self))) / right;
    return result;
}

static YogVal
divide_bignum(YogEnv* env, YogHandle* self, YogHandle* bignum)
{
    mpz_t* n = &BIGNUM_NUM(HDL2VAL(bignum));
    if (mpz_fits_sint_p(*n)) {
        return divide_int(env, self, mpz_get_si(*n));
    }

    YogVal result = YogFloat_new(env);
#define BIGNUM2FLOAT(bignum) mpz_get_d(BIGNUM_NUM(HDL2VAL((bignum))))
    FLOAT_NUM(result) = BIGNUM2FLOAT(self) / BIGNUM2FLOAT(bignum);
#undef BIGNUM2FLOAT
    return result;
}

YogVal
YogBignum_binop_divide(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return divide_int(env, self, VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        return divide_float(env, self, FLOAT_NUM(right));
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        return divide_bignum(env, self, n);
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "/");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
divide(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_divide(env, self, n);
}

static YogVal
floor_divide_int(YogEnv* env, YogHandle* self, int_t right)
{
    if (right == 0) {
        YogError_raise_ZeroDivisionError(env, "Bignum division by zero");
        /* NOTREACHED */
    }

    YogVal bignum = YogBignum_from_int(env, right);
    mpz_t* h = &BIGNUM_NUM(bignum);
    mpz_fdiv_q(*h, BIGNUM_NUM(HDL2VAL(self)), *h);
    return normalize(env, bignum);
}

static BOOL
is_zero(YogVal bignum)
{
    if (!mpz_fits_sint_p(BIGNUM_NUM(bignum))) {
        return FALSE;
    }
    if (mpz_get_si(BIGNUM_NUM(bignum)) != 0) {
        return FALSE;
    }

    return TRUE;
}

static YogVal
floor_divide_bignum(YogEnv* env, YogHandle* self, YogHandle* right)
{
    if (is_zero(HDL2VAL(right))) {
        YogError_raise_ZeroDivisionError(env, "Bignum division by zero");
        /* NOTREACHED */
    }

    YogVal bignum = YogBignum_new(env);
    mpz_t* h = &BIGNUM_NUM(bignum);
    mpz_fdiv_q(*h, BIGNUM_NUM(HDL2VAL(self)), BIGNUM_NUM(HDL2VAL(right)));
    return normalize(env, bignum);
}

YogVal
YogBignum_modulo(YogEnv* env, YogHandle* self, YogHandle* n)
{
    /* gcc can't know that bignum is always initialized */
    YogHandle* bignum = NULL;
    YogVal m = HDL2VAL(n);
    if (IS_FIXNUM(m)) {
        YogVal val = YogBignum_from_int(env, VAL2INT(m));
        bignum = YogHandle_REGISTER(env, val);
    }
    else if (IS_PTR(m) && (BASIC_OBJ_TYPE(m) == TYPE_BIGNUM)) {
        bignum = n;
    }
    else {
        /* NOTREACHED? */
        YOG_BUG(env, "YogBignum_modulo got an unexpected type argument");
    }

    YogVal result = YogBignum_new(env);
    mpz_t* l = &BIGNUM_NUM(HDL2VAL(self));
    mpz_mod(BIGNUM_NUM(result), *l, BIGNUM_NUM(HDL2VAL(bignum)));
    return normalize(env, result);
}

YogVal
YogBignum_binop_modulo(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right) || (IS_PTR(right) && (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM))) {
        return YogBignum_modulo(env, self, n);
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "%");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
modulo(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_modulo(env, self, n);
}

YogVal
YogBignum_binop_floor_divide(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return floor_divide_int(env, self, VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        return divide_float(env, self, FLOAT_NUM(right));
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        return floor_divide_bignum(env, self, n);
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "//");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
floor_divide(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_floor_divide(env, self, n);
}

static YogVal
positive(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "+self", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    RETURN(env, self);
}

static YogVal
not(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "~self", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    retval = YogBignum_new(env);
    mpz_com(BIGNUM_NUM(retval), BIGNUM_NUM(self));

    RETURN(env, retval);
}

YogVal
YogBignum_lshift(YogEnv* env, YogHandle* self, int_t width)
{
    YogVal retval = YogBignum_new(env);
    mpz_mul_2exp(BIGNUM_NUM(retval), BIGNUM_NUM(HDL2VAL(self)), width);
    return retval;
}

static YogVal
YogBignum_rshift(YogEnv* env, YogHandle* self, int_t width)
{
    YogVal retval = YogBignum_new(env);
    mpz_fdiv_q_2exp(BIGNUM_NUM(retval), BIGNUM_NUM(HDL2VAL(self)), width);
    return retval;
}

YogVal
YogBignum_binop_rshift(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (!IS_FIXNUM(right)) {
        YogError_raise_binop_type_error(env, HDL2VAL(self), right, ">>");
        /* NOTREACHED */
    }

    int_t m = VAL2INT(right);
    if (0 < m) {
        return YogBignum_rshift(env, self, m);
    }
    return YogBignum_lshift(env, self, - m);
}

static YogVal
rshift(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_rshift(env, self, n);
}

YogVal
YogBignum_binop_lshift(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (!IS_FIXNUM(right)) {
        YogError_raise_binop_type_error(env, HDL2VAL(self), right, "<<");
        /* NOTREACHED */
    }
    int_t width = VAL2INT(right);
    if (0 < width) {
        return YogBignum_lshift(env, self, width);
    }
    return YogBignum_rshift(env, self, - width);
}

static YogVal
lshift(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_lshift(env, self, n);
}

YogVal
YogBignum_from_long_long(YogEnv* env, long long n)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    char buf[21]; /* 64bit integer with '\0' needs at most 21bytes */
    snprintf(buf, array_sizeof(buf), "%lld", n);
    bignum = YogBignum_new(env);
    mpz_set_str(BIGNUM_NUM(bignum), buf, 10);

    RETURN(env, bignum);
}

YogVal
YogBignum_from_unsigned_long_long(YogEnv* env, unsigned long long n)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    char buf[21]; /* 64bit integer with '\0' needs at most 21bytes */
    snprintf(buf, array_sizeof(buf), "%llu", n);
    bignum = YogBignum_new(env);
    mpz_set_str(BIGNUM_NUM(bignum), buf, 10);

    RETURN(env, bignum);
}

YogVal
YogBignum_from_unsigned_int(YogEnv* env, uint_t n)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = YogBignum_new(env);
    mpz_set_ui(PTR_AS(YogBignum, bignum)->num, n);

    RETURN(env, bignum);
}

YogVal
YogBignum_from_int(YogEnv* env, int_t n)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = YogBignum_new(env);
    mpz_set_si(PTR_AS(YogBignum, bignum)->num, n);

    RETURN(env, bignum);
}

YogVal
YogBignum_from_str(YogEnv* env, YogVal s, int_t base)
{
    YogHandle* str = YogHandle_REGISTER(env, s);
    YogHandle* bignum = YogHandle_REGISTER(env, YogBignum_new(env));
    YogHandle* enc = YogHandle_REGISTER(env, env->vm->encAscii);
    YogVal ascii = YogEncoding_conv_from_yog(env, enc, str);
    const char* pc = BINARY_CSTR(ascii);
    if (mpz_set_str(HDL_AS(YogBignum, bignum)->num, pc, base) != 0) {
        YOG_BUG(env, "mpz_set_str failed");
    }

    return HDL2VAL(bignum);
}

YogVal
YogBignum_binop_multiply(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return YogFixnum_binop_multiply(env, right, self);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        double f = FLOAT_NUM(HDL2VAL(n));
        FLOAT_NUM(result) = mpz_get_d(BIGNUM_NUM(HDL2VAL(self))) * f;
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal result = YogBignum_new(env);
        mpz_t* h = &BIGNUM_NUM(HDL2VAL(self));
        mpz_mul(BIGNUM_NUM(result), *h, BIGNUM_NUM(HDL2VAL(n)));
        return result;
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "*");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
or_bignum(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal retval = YogBignum_new(env);
    mpz_t* m = &BIGNUM_NUM(HDL2VAL(self));
    mpz_ior(BIGNUM_NUM(retval), *m, BIGNUM_NUM(HDL2VAL(n)));
    return retval;
}

YogVal
YogBignum_or(YogEnv* env, YogHandle* self, int_t n)
{
    YogVal bignum = YogBignum_from_int(env, n);
    YogHandle* h = YogHandle_REGISTER(env, bignum);
    return or_bignum(env, self, h);
}

YogVal
YogBignum_binop_or(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return YogBignum_or(env, self, VAL2INT(right));
    }
    else if (IS_PTR(right) && (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM)) {
        return or_bignum(env, self, n);
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "|");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
or(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_or(env, self, n);
}

static YogVal
xor_bignum(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal result = YogBignum_new(env);
    mpz_t* m = &BIGNUM_NUM(HDL2VAL(self));
    mpz_xor(BIGNUM_NUM(result), *m, BIGNUM_NUM(HDL2VAL(n)));
    return normalize(env, result);
}

YogVal
YogBignum_xor(YogEnv* env, YogHandle* self, int_t n)
{
    YogHandle* bignum = YogHandle_REGISTER(env, YogBignum_from_int(env, n));
    return xor_bignum(env, self, bignum);
}

static YogVal
and_bignum(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal result = YogBignum_new(env);
    mpz_t* m = &BIGNUM_NUM(HDL2VAL(self));
    mpz_and(BIGNUM_NUM(result), *m, BIGNUM_NUM(HDL2VAL(n)));
    return normalize(env, result);
}

YogVal
YogBignum_and(YogEnv* env, YogHandle* self, int_t n)
{
    YogVal bignum = YogBignum_from_int(env, n);
    return and_bignum(env, self, YogHandle_REGISTER(env, bignum));
}

YogVal
YogBignum_binop_and(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return YogBignum_and(env, self, VAL2INT(right));
    }
    else if (IS_PTR(right) && (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM)) {
        return and_bignum(env, self, n);
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "&");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
and(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_and(env, self, n);
}

YogVal
YogBignum_binop_xor(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return YogBignum_xor(env, self, VAL2INT(right));
    }
    else if (IS_PTR(right) && (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM)) {
        return xor_bignum(env, self, n);
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "^");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
xor(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_xor(env, self, n);
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "hash", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    s = YogBignum_to_s(env, self);
    int_t h = YogString_hash(env, s);

    RETURN(env, INT2VAL(h));
}

YogVal
YogBignum_binop_ufo(YogEnv* env, YogVal self, YogVal n)
{
    if (!IS_PTR(n) || (BASIC_OBJ_TYPE(n) != TYPE_BIGNUM)) {
        YogError_raise_TypeError(env, "operand must be Bignum");
    }

    int_t m = mpz_cmp(BIGNUM_NUM(self), BIGNUM_NUM(n));
    if (m < 0) {
        return INT2VAL(-1);
    }
    if (m == 0) {
        return INT2VAL(0);
    }
    return INT2VAL(1);
}

static YogVal
ufo(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_ufo(env, HDL2VAL(self), HDL2VAL(n));
}

static YogVal
YogBignum_clone(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogBignum_new(env);
    mpz_set(BIGNUM_NUM(retval), BIGNUM_NUM(self));

    RETURN(env, retval);
}

YogVal
YogBignum_power(YogEnv* env, YogHandle* self, int_t exp)
{
    if (exp < 0) {
        YogVal f = YogFloat_new(env);
        FLOAT_NUM(f) = 1 / mpz_get_d(BIGNUM_NUM(HDL2VAL(self)));
        return YogFloat_power(env, f, - exp);
    }
    else if (exp == 0) {
        return INT2VAL(1);
    }

    YogVal bignum = YogBignum_clone(env, HDL2VAL(self));
    mpz_t* m = &BIGNUM_NUM(HDL2VAL(self));
    int_t i;
    for (i = exp; 1 < i; i--) {
        mpz_mul(BIGNUM_NUM(bignum), BIGNUM_NUM(bignum), *m);
    }
    return bignum;
}

YogVal
YogBignum_binop_power(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return YogBignum_power(env, self, VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        double f = mpz_get_d(BIGNUM_NUM(HDL2VAL(self)));
        YogVal retval = YogFloat_new(env);
        FLOAT_NUM(retval) = pow(f, FLOAT_NUM(HDL2VAL(n)));
        return retval;
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "**");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
power(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_binop_power(env, self, n);
}

int_t
YogBignum_compare_with_unsigned_int(YogEnv* env, YogVal self, uint_t n)
{
    return mpz_cmp_ui(BIGNUM_NUM(self), n);
}

int_t
YogBignum_compare_with_int(YogEnv* env, YogVal self, int_t n)
{
    return mpz_cmp_si(BIGNUM_NUM(self), n);
}

UNSIGNED_TYPE
YogBignum_to_unsigned_type(YogEnv* env, YogVal self, const char* name)
{
    SAVE_ARG(env, self);
    CHECK_SELF_TYPE(env, self);

    if ((mpz_cmp_ui(BIGNUM_NUM(self), 0) < 0) || (0 < mpz_cmp_ui(BIGNUM_NUM(self), UNSIGNED_MAX))) {
        YogError_raise_ValueError(env, "%s must between %d and %d", name, 0, UNSIGNED_MAX);
    }
    UNSIGNED_TYPE ui = mpz_get_ui(BIGNUM_NUM(self));

    RETURN(env, ui);
}

SIGNED_TYPE
YogBignum_to_signed_type(YogEnv* env, YogVal self, const char* name)
{
    SAVE_ARG(env, self);
    CHECK_SELF_TYPE(env, self);

    if ((YogBignum_compare_with_int(env, self, SIGNED_MIN) < 0) || (0 < YogBignum_compare_with_int(env, self, SIGNED_MAX))) {
        YogError_raise_ValueError(env, "%s must between %d and %d", name, SIGNED_MIN, SIGNED_MAX);
    }
    SIGNED_TYPE si = mpz_get_si(BIGNUM_NUM(self));

    RETURN(env, si);
}

int_t
YogBignum_compare_with_long_long(YogEnv* env, YogVal self, long long n)
{
    SAVE_ARG(env, self);

    char buf[21];
    snprintf(buf, array_sizeof(buf), "%lld", n);
    mpz_t rop;
    mpz_init_set_str(rop, buf, 10);
    int_t retval = mpz_cmp(BIGNUM_NUM(self), rop);

    RETURN(env, retval);
}

int_t
YogBignum_compare_with_unsigned_long_long(YogEnv* env, YogVal self, unsigned long long n)
{
    SAVE_ARG(env, self);

    char buf[21];
    snprintf(buf, array_sizeof(buf), "%llu", n);
    mpz_t rop;
    mpz_init_set_str(rop, buf, 10);
    int_t retval = mpz_cmp(BIGNUM_NUM(self), rop);

    RETURN(env, retval);
}

long long
YogBignum_to_long_long(YogEnv* env, YogVal self, const char* name)
{
    SAVE_ARG(env, self);

    if ((YogBignum_compare_with_long_long(env, self, INT64_MIN) < 0) || (0 < YogBignum_compare_with_long_long(env, self, INT64_MAX))) {
        YogError_raise_ValueError(env, "%s must be between %lld and %lld", name, INT64_MIN, INT64_MAX);
    }
    char buf[21];
    mpz_get_str(buf, 10, BIGNUM_NUM(self));
    long long retval = 0;
    sscanf(buf, "%lld", &retval);

    RETURN(env, retval);
}

unsigned long long
YogBignum_to_unsigned_long_long(YogEnv* env, YogVal self, const char* name)
{
    SAVE_ARG(env, self);

    if ((YogBignum_compare_with_unsigned_int(env, self, 0) < 0) || (0 < YogBignum_compare_with_unsigned_long_long(env, self, UINT64_MAX))) {
        YogError_raise_ValueError(env, "%s must be between 0 and %llu", name, UINT64_MAX);
    }
    char buf[21];
    mpz_get_str(buf, 10, BIGNUM_NUM(self));
    unsigned long long retval;
    sscanf(buf, "%llu", &retval);

    RETURN(env, retval);
}

void
YogBignum_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cBignum = YUNDEF;
    PUSH_LOCAL(env, cBignum);
    YogVM* vm = env->vm;

    cBignum = YogClass_new(env, "Bignum", vm->cObject);
    YogClass_include_module(env, cBignum, vm->mComparable);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cBignum, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("+self", positive);
    DEFINE_METHOD("-self", negative);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("to_s", to_s);
    DEFINE_METHOD("~self", not);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...) do { \
    YogClass_define_method2(env, cBignum, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("%", modulo, "n", NULL);
    DEFINE_METHOD2("&", and, "n", NULL);
    DEFINE_METHOD2("*", multiply, "n", NULL);
    DEFINE_METHOD2("**", power, "n", NULL);
    DEFINE_METHOD2("+", add, "n", NULL);
    DEFINE_METHOD2("-", subtract, "n", NULL);
    DEFINE_METHOD2("/", divide, "n", NULL);
    DEFINE_METHOD2("//", floor_divide, "n", NULL);
    DEFINE_METHOD2("<<", lshift, "n", NULL);
    DEFINE_METHOD2("<=>", ufo, "n", NULL);
    DEFINE_METHOD2(">>", rshift, "n", NULL);
    DEFINE_METHOD2("^", xor, "n", NULL);
    DEFINE_METHOD2("|", or, "n", NULL);
#undef DEFINE_METHOD2
    vm->cBignum = cBignum;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
