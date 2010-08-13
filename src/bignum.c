#include <math.h>
#include "gmp.h"
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/class.h"
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
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

#define BASE    10
    size_t size = mpz_sizeinbase(PTR_AS(YogBignum, self)->num, BASE) + 2;
    s = YogString_of_size(env, size);
    mpz_get_str(STRING_CSTR(s), BASE, PTR_AS(YogBignum, self)->num);
    STRING_SIZE(s) = size;
#undef BASE

    RETURN(env, s);
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
YogBignum_add(YogEnv* env, YogHandle* self, YogHandle* n)
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
    return YogBignum_add(env, self, n);
}

static YogVal
normalize(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);

    if (!mpz_fits_sint_p(BIGNUM_NUM(self))) {
        RETURN(env, self);
    }
    int_t n = mpz_get_si(BIGNUM_NUM(self));
    if (!FIXABLE(n)) {
        RETURN(env, self);
    }

    RETURN(env, INT2VAL(n));
}

YogVal
YogBignum_subtract(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        YogVal bignum = YogBignum_from_int(env, right);
        return YogBignum_subtract(env, self, YogHandle_REGISTER(env, bignum));
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
    return YogBignum_subtract(env, self, n);
}

static YogVal
multiply(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogBignum_multiply(env, self, n);
}

static YogVal
divide_int(YogEnv* env, YogVal self, int_t right)
{
    SAVE_ARG(env, self);
    YogVal result = YUNDEF;
    PUSH_LOCAL(env, result);

    if (right == 0) {
        YogError_raise_ZeroDivisionError(env, "Bignum division by zero");
    }

    result = YogFloat_new(env);
    FLOAT_NUM(result) = mpz_get_d(BIGNUM_NUM(self)) / right;

    RETURN(env, result);
}

static YogVal
divide_float(YogEnv* env, YogVal self, double right)
{
    SAVE_ARG(env, self);
    YogVal result = YUNDEF;
    PUSH_LOCAL(env, result);

    if (right == 0.0) {
        YogError_raise_ZeroDivisionError(env, "float division");
    }

    result = YogFloat_new(env);
    FLOAT_NUM(result) = mpz_get_d(BIGNUM_NUM(self)) / right;

    RETURN(env, result);
}

static YogVal
divide_bignum(YogEnv* env, YogVal self, YogVal bignum)
{
    SAVE_ARGS2(env, self, bignum);
    YogVal result = YUNDEF;
    PUSH_LOCAL(env, result);

    if (mpz_fits_sint_p(BIGNUM_NUM(bignum))) {
        result = divide_int(env, self, mpz_get_si(BIGNUM_NUM(bignum)));
    }

    result = YogFloat_new(env);
#define BIGNUM2FLOAT(bignum)    mpz_get_d(BIGNUM_NUM(bignum))
    FLOAT_NUM(result) = BIGNUM2FLOAT(self) / BIGNUM2FLOAT(bignum);
#undef BIGNUM2FLOAT

    RETURN(env, result);
}

static YogVal
divide(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "/", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (IS_FIXNUM(right)) {
        result = divide_int(env, self, VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        result = divide_float(env, self, FLOAT_NUM(right));
        RETURN(env, result);
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        result = divide_bignum(env, self, right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "/");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
floor_divide_int(YogEnv* env, YogVal self, int_t right)
{
    SAVE_ARG(env, self);
    YogVal bignum = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, bignum, result);

    if (right == 0) {
        YogError_raise_ZeroDivisionError(env, "Bignum division by zero");
    }

    bignum = YogBignum_from_int(env, right);
    mpz_fdiv_q(BIGNUM_NUM(bignum), BIGNUM_NUM(self), BIGNUM_NUM(bignum));
    result = normalize(env, bignum);

    RETURN(env, result);
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
floor_divide_bignum(YogEnv* env, YogVal self, YogVal right)
{
    SAVE_ARGS2(env, self, right);
    YogVal bignum = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, bignum, result);

    if (is_zero(right)) {
        YogError_raise_ZeroDivisionError(env, "Bignum division by zero");
    }

    bignum = YogBignum_new(env);
    mpz_fdiv_q(BIGNUM_NUM(bignum), BIGNUM_NUM(self), BIGNUM_NUM(right));
    result = normalize(env, bignum);

    RETURN(env, result);
}

YogVal
YogBignum_modulo(YogEnv* env, YogVal self, YogVal n)
{
    SAVE_ARGS2(env, self, n);
    YogVal bignum = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, bignum, result);

    if (IS_FIXNUM(n)) {
        bignum = YogBignum_from_int(env, VAL2INT(n));
    }
    else if (IS_PTR(n) && (BASIC_OBJ_TYPE(n) == TYPE_BIGNUM)) {
        bignum = n;
    }
    else {
        YogError_raise_binop_type_error(env, self, n, "%");
    }

    result = YogBignum_new(env);
    mpz_mod(BIGNUM_NUM(result), BIGNUM_NUM(self), BIGNUM_NUM(bignum));
    result = normalize(env, result);

    RETURN(env, result);
}

static YogVal
modulo(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "%", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (IS_FIXNUM(right) || (IS_PTR(right) && (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM))) {
        result = YogBignum_modulo(env, self, right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "%");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
floor_divide(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "//", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (IS_FIXNUM(right)) {
        result = floor_divide_int(env, self, VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        result = divide_float(env, self, FLOAT_NUM(right));
        RETURN(env, result);
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        result = floor_divide_bignum(env, self, right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "//");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
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
YogBignum_lshift(YogEnv* env, YogVal self, int_t width)
{
    YOG_ASSERT(env, 0 <= width, "negative width (%d)", width);

    SAVE_ARG(env, self);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogBignum_new(env);
    mpz_mul_2exp(BIGNUM_NUM(retval), BIGNUM_NUM(self), width);

    RETURN(env, retval);
}

static YogVal
YogBignum_rshift(YogEnv* env, YogVal self, int_t width)
{
    YOG_ASSERT(env, 0 <= width, "negative width (%d)", width);

    SAVE_ARG(env, self);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogBignum_new(env);
    mpz_fdiv_q_2exp(BIGNUM_NUM(retval), BIGNUM_NUM(self), width);

    RETURN(env, retval);
}

static YogVal
rshift(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS2(env, retval, right);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, ">>", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (!IS_FIXNUM(right)) {
        YogError_raise_binop_type_error(env, self, right, ">>");
    }
    int_t n = VAL2INT(right);
    if (0 < n) {
        retval = YogBignum_rshift(env, self, n);
    }
    else {
        retval = YogBignum_lshift(env, self, - n);
    }

    RETURN(env, retval);
}

static YogVal
lshift(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS2(env, retval, right);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "<<", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (!IS_FIXNUM(right)) {
        YogError_raise_binop_type_error(env, self, right, "<<");
    }
    int_t n = VAL2INT(right);
    if (0 < n) {
        retval = YogBignum_lshift(env, self, n);
    }
    else {
        retval = YogBignum_rshift(env, self, - n);
    }

    RETURN(env, retval);
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
    SAVE_ARG(env, s);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = YogBignum_new(env);
    const char* str = STRING_CSTR(s);
    if (mpz_set_str(PTR_AS(YogBignum, bignum)->num, str, base) != 0) {
        YOG_BUG(env, "mpz_set_str failed");
    }

    RETURN(env, bignum);
}

YogVal
YogBignum_multiply(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return YogFixnum_multiply(env, right, self);
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

YogVal
YogBignum_or(YogEnv* env, YogVal self, YogVal n)
{
    SAVE_ARGS2(env, self, n);
    YogVal retval = YUNDEF;
    YogVal bignum = YUNDEF;
    PUSH_LOCALS2(env, retval, bignum);

    YOG_ASSERT(env, !IS_UNDEF(n), "undefined value");
    if (!IS_FIXNUM(n) && !(IS_PTR(n) && (BASIC_OBJ_TYPE(n) == TYPE_BIGNUM))) {
        YogError_raise_binop_type_error(env, self, n, "|");
    }

    if (IS_FIXNUM(n)) {
        bignum = YogBignum_from_int(env, VAL2INT(n));
    }
    else {
        bignum = n;
    }
    retval = YogBignum_new(env);
    mpz_ior(BIGNUM_NUM(retval), BIGNUM_NUM(self), BIGNUM_NUM(bignum));

    RETURN(env, retval);
}

static YogVal
or(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS2(env, retval, right);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "|", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    retval = YogBignum_or(env, self, right);

    RETURN(env, retval);
}

YogVal
YogBignum_xor(YogEnv* env, YogVal self, YogVal n)
{
    YOG_ASSERT(env, !IS_UNDEF(n), "undefined value");

    SAVE_ARGS2(env, self, n);
    YogVal bignum = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, bignum, result);

    if (IS_FIXNUM(n)) {
        bignum = YogBignum_from_int(env, VAL2INT(n));
    }
    else if (IS_PTR(n) && (BASIC_OBJ_TYPE(n) == TYPE_BIGNUM)) {
        bignum = n;
    }
    else {
        YogError_raise_binop_type_error(env, self, n, "^");
    }

    result = YogBignum_new(env);
    mpz_xor(BIGNUM_NUM(result), BIGNUM_NUM(self), BIGNUM_NUM(bignum));
    result = normalize(env, result);

    RETURN(env, result);
}

YogVal
YogBignum_and(YogEnv* env, YogVal self, YogVal n)
{
    YOG_ASSERT(env, !IS_UNDEF(n), "undefined value");

    SAVE_ARGS2(env, self, n);
    YogVal bignum = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, bignum, result);

    if (IS_FIXNUM(n)) {
        bignum = YogBignum_from_int(env, VAL2INT(n));
    }
    else if (IS_PTR(n) && (BASIC_OBJ_TYPE(n) == TYPE_BIGNUM)) {
        bignum = n;
    }
    else {
        YogError_raise_binop_type_error(env, self, n, "&");
    }

    result = YogBignum_new(env);
    mpz_and(BIGNUM_NUM(result), BIGNUM_NUM(self), BIGNUM_NUM(bignum));
    result = normalize(env, result);

    RETURN(env, result);
}

static YogVal
and(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal right = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, right, retval);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "&", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    retval = YogBignum_and(env, self, right);

    RETURN(env, retval);
}

static YogVal
xor(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS2(env, retval, right);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "^", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    retval = YogBignum_xor(env, self, right);

    RETURN(env, retval);
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

static YogVal
compare(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal right = YUNDEF;
    PUSH_LOCAL(env, right);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "<=>", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    if (!IS_PTR(right) || (BASIC_OBJ_TYPE(right) != TYPE_BIGNUM)) {
        YogError_raise_TypeError(env, "operand must be Bignum");
    }

    int_t n = mpz_cmp(BIGNUM_NUM(self), BIGNUM_NUM(right));
    if (n < 0) {
        RETURN(env, INT2VAL(-1));
    }
    else if (n == 0) {
        RETURN(env, INT2VAL(n));
    }

    RETURN(env, INT2VAL(1));
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

static YogVal
power_int(YogEnv* env, YogVal self, int_t exp)
{
    SAVE_ARG(env, self);
    YogVal f = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS3(env, f, bignum, retval);

    if (exp < 0) {
        f = YogFloat_new(env);
        FLOAT_NUM(f) = 1 / mpz_get_d(BIGNUM_NUM(self));
        retval = YogFloat_power(env, f, - exp);
        RETURN(env, retval);
    }
    else if (exp == 0) {
        RETURN(env, INT2VAL(1));
    }

    bignum = YogBignum_clone(env, self);
    int_t n = exp;
    while (1 < n) {
        mpz_mul(BIGNUM_NUM(bignum), BIGNUM_NUM(bignum), BIGNUM_NUM(self));
        n--;
    }

    RETURN(env, bignum);
}

YogVal
YogBignum_power(YogEnv* env, YogVal self, YogVal right)
{
    SAVE_ARGS2(env, self, right);
    YogVal retval = YUNDEF;
    YogVal f = YUNDEF;
    YogVal bignum = YUNDEF;
    YogVal mod = YUNDEF;
    PUSH_LOCALS4(env, retval, f, bignum, mod);

    if (IS_FIXNUM(right)) {
        retval = power_int(env, self, VAL2INT(right));
        RETURN(env, retval);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        double f = mpz_get_d(BIGNUM_NUM(self));
        retval = YogFloat_new(env);
        FLOAT_NUM(retval) = pow(f, FLOAT_NUM(right));
        RETURN(env, retval);
    }

    YogError_raise_binop_type_error(env, self, right, "**");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
power(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal right = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, right, retval);

    YogCArg params[] = { { "n", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "**", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    retval = YogBignum_power(env, self, right);

    RETURN(env, retval);
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
    DEFINE_METHOD("%", modulo);
    DEFINE_METHOD("&", and);
    DEFINE_METHOD("**", power);
    DEFINE_METHOD("+self", positive);
    DEFINE_METHOD("-self", negative);
    DEFINE_METHOD("/", divide);
    DEFINE_METHOD("//", floor_divide);
    DEFINE_METHOD("<<", lshift);
    DEFINE_METHOD("<=>", compare);
    DEFINE_METHOD(">>", rshift);
    DEFINE_METHOD("^", xor);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("to_s", to_s);
    DEFINE_METHOD("|", or);
    DEFINE_METHOD("~self", not);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...) do { \
    YogClass_define_method2(env, cBignum, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("*", multiply, "n", NULL);
    DEFINE_METHOD2("+", add, "n", NULL);
    DEFINE_METHOD2("-", subtract, "n", NULL);
#undef DEFINE_METHOD2
    vm->cBignum = cBignum;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
