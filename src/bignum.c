#include <limits.h>
#include <math.h>
#include <stdlib.h>

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

/* Zero Or Die */
#define ZOD(x) \
    if ((x) != 0) { \
        YOG_BUG(env, "%s failed.", #x); \
    } \

static size_t
compute_needed_size(YogVal n)
{
    size_t size_for_sign = 0 < BIGNUM_SIGN(n) ? 0 : 1;
    return size_for_sign + bdConvToDecimal(BIGNUM_NUM(n), NULL, 0) + 1;
}

static void
conv_to_decimal(YogVal n, char* dest, size_t size)
{
    char* p = dest;
    size_t len = size;
    if (BIGNUM_SIGN(n) < 0) {
        *p = '-';
        p++;
        len--;
    }
    bdConvToDecimal(BIGNUM_NUM(n), p, len);
}

#define CONV_TO_DECIMAL(dest, src) \
    size_t __size__ = compute_needed_size((src)); \
    char dest[__size__]; \
    conv_to_decimal((src), dest, __size__)

YogVal
YogBignum_to_s(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    CONV_TO_DECIMAL(buf, self);
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
    bdFree(&bignum->num);
}

static void
YogBignum_init(YogEnv* env, YogVal self)
{
    YogBasicObj_init(env, self, TYPE_BIGNUM, 0, env->vm->cBignum);
    PTR_AS(YogBignum, self)->num = bdNew();
    PTR_AS(YogBignum, self)->sign = 1;
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
    ZOD(bdSetEqual(BIGNUM_NUM(bignum), BIGNUM_NUM(self)));
    BIGNUM_SIGN(bignum) = - BIGNUM_SIGN(self);

    RETURN(env, bignum);
}

static YogVal
normalize(YogEnv* env, YogVal self)
{
    CONV_TO_DECIMAL(buf, self);
    char* endptr;
    long n = strtol(buf, &endptr, 10);
    if ((*endptr == '\0') && FIXABLE(n)) {
        return INT2VAL(n);
    }
    return self;
}

static YogVal
add_two_bignum(YogEnv* env, YogHandle* self, YogHandle* n)
{
    if ((BIGNUM_SIGN(HDL2VAL(self)) < 0) && (0 < BIGNUM_SIGN(HDL2VAL(n)))) {
        return add_two_bignum(env, n, self);
    }

    YogVal bignum = YogBignum_new(env);
    YogVal left = HDL2VAL(self);
    YogVal right = HDL2VAL(n);
    if (BIGNUM_SIGN(left) == BIGNUM_SIGN(right)) {
        bdAdd(BIGNUM_NUM(bignum), BIGNUM_NUM(left), BIGNUM_NUM(right));
        BIGNUM_SIGN(bignum) = BIGNUM_SIGN(left);
        return bignum;
    }
    if (bdCompare(BIGNUM_NUM(left), BIGNUM_NUM(right)) < 0) {
        BIGD dest = BIGNUM_NUM(bignum);
        ZOD(bdSubtract(dest, BIGNUM_NUM(right), BIGNUM_NUM(left)));
        BIGNUM_SIGN(bignum) = -1;
        return normalize(env, bignum);
    }
    ZOD(bdSubtract(BIGNUM_NUM(bignum), BIGNUM_NUM(left), BIGNUM_NUM(right)));
    return normalize(env, bignum);
}

double
YogBignum_to_float(YogEnv* env, YogHandle* self)
{
    CONV_TO_DECIMAL(buf, HDL2VAL(self));
    char* endptr;
    double x = strtod(buf, &endptr);
    YOG_ASSERT2(env, *endptr == '\0');
    return x;
}

YogVal
YogBignum_binop_add(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        YogHandle* h = VAL2HDL(env, YogBignum_from_int(env, VAL2INT(right)));
        return add_two_bignum(env, self, h);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        double f = YogBignum_to_float(env, self);
        FLOAT_NUM(result) = f + FLOAT_NUM(HDL2VAL(n));
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        return add_two_bignum(env, self, n);
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
copy(YogEnv* env, YogHandle* self)
{
    YogVal bignum = YogBignum_new(env);
    YogVal n = HDL2VAL(self);
    ZOD(bdSetEqual(BIGNUM_NUM(bignum), BIGNUM_NUM(n)));
    BIGNUM_SIGN(bignum) = BIGNUM_SIGN(n);
    return bignum;
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
        double f = YogBignum_to_float(env, self);
        FLOAT_NUM(result) = f - FLOAT_NUM(HDL2VAL(n));
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal m = copy(env, n);
        BIGNUM_SIGN(m) = - BIGNUM_SIGN(HDL2VAL(n));
        return add_two_bignum(env, self, VAL2HDL(env, m));
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
    FLOAT_NUM(result) = YogBignum_to_float(env, self) / right;
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
    FLOAT_NUM(result) = YogBignum_to_float(env, self) / right;
    return result;
}

static YogVal
divide_bignum(YogEnv* env, YogHandle* self, YogHandle* bignum)
{
    YogVal result = YogFloat_new(env);
    double f = YogBignum_to_float(env, self);
    FLOAT_NUM(result) = f / YogBignum_to_float(env, bignum);
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

static int
divide_BIGD(BIGD q, BIGD u, BIGD v)
{
    BIGD _ = bdNew();
    int retval = bdDivide(q, _, u, v);
    bdFree(&_);
    return retval;
}

static YogVal
floor_divide_bignum_without_normailzing(YogEnv* env, YogHandle* self, YogHandle* right)
{
    if (bdIsZero(BIGNUM_NUM(HDL2VAL(right)))) {
        YogError_raise_ZeroDivisionError(env, "Bignum division by zero");
        /* NOTREACHED */
    }
    if (BIGNUM_SIGN(HDL2VAL(self)) == BIGNUM_SIGN(HDL2VAL(right))) {
        YogVal retval = YogBignum_new(env);
        BIGD u = BIGNUM_NUM(HDL2VAL(self));
        BIGD v = BIGNUM_NUM(HDL2VAL(right));
        ZOD(divide_BIGD(BIGNUM_NUM(retval), u, v));
        return retval;
    }
    YogVal retval = YogBignum_new(env);
    BIGD n = BIGNUM_NUM(retval);
    ZOD(divide_BIGD(n, BIGNUM_NUM(HDL2VAL(self)), BIGNUM_NUM(HDL2VAL(right))));
    bdIncrement(n);

    BIGNUM_SIGN(retval) = -1;

    return retval;
}

static YogVal
floor_divide_bignum(YogEnv* env, YogHandle* self, YogHandle* right)
{
    YogVal val = floor_divide_bignum_without_normailzing(env, self, right);
    return normalize(env, val);
}

static YogHandle*
denormalize_to_bignum(YogEnv* env, YogHandle* n)
{
    YogVal val = HDL2VAL(n);
    if (IS_FIXNUM(val)) {
        return VAL2HDL(env, YogBignum_from_int(env, VAL2INT(val)));
    }
    if (IS_PTR(val) && (BASIC_OBJ_TYPE(val) == TYPE_BIGNUM)) {
        return n;
    }
    YOG_BUG(env, "YogBignum_modulo got an unexpected type argument");
    /* NOTREACHED */

    return NULL;
}

YogVal
YogBignum_modulo(YogEnv* env, YogHandle* self, YogHandle* n)
{
    YogHandle* n2 = denormalize_to_bignum(env, n);
    YogVal div = floor_divide_bignum_without_normailzing(env, self, n2);
    YogVal mul = YogBignum_binop_multiply(env, VAL2HDL(env, div), n);
    return YogBignum_binop_subtract(env, self, VAL2HDL(env, mul));
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
        YogVal m = YogBignum_from_int(env, VAL2INT(right));
        return YogBignum_binop_floor_divide(env, self, VAL2HDL(env, m));
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
    ZOD(bdSetEqual(BIGNUM_NUM(retval), BIGNUM_NUM(self)));
    bdIncrement(BIGNUM_NUM(retval));
    BIGNUM_SIGN(retval) = - BIGNUM_SIGN(self);

    RETURN(env, retval);
}

YogVal
YogBignum_lshift(YogEnv* env, YogHandle* self, int_t width)
{
    YogVal retval = YogBignum_new(env);
    bdShiftLeft(BIGNUM_NUM(retval), BIGNUM_NUM(HDL2VAL(self)), width);
    BIGNUM_SIGN(retval) = BIGNUM_SIGN(HDL2VAL(self));
    return retval;
}

static YogVal
YogBignum_rshift(YogEnv* env, YogHandle* self, int_t width)
{
    YogVal retval = YogBignum_new(env);
    bdShiftRight(BIGNUM_NUM(retval), BIGNUM_NUM(HDL2VAL(self)), width);
    BIGNUM_SIGN(retval) = BIGNUM_SIGN(HDL2VAL(self));
    if (BIGNUM_SIGN(retval) < 0) {
        bdIncrement(BIGNUM_NUM(retval));
    }
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
    if (buf[0] == '-') {
        YOG_ASSERT2(env, n < 0);
        bdConvFromDecimal(BIGNUM_NUM(bignum), &buf[1]);
        BIGNUM_SIGN(bignum) = -1;
        RETURN(env, bignum);
    }
    bdConvFromDecimal(BIGNUM_NUM(bignum), buf);

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
    bdConvFromDecimal(BIGNUM_NUM(bignum), buf);

    RETURN(env, bignum);
}

YogVal
YogBignum_from_unsigned_int(YogEnv* env, uint_t n)
{
    YogVal bignum = YogBignum_new(env);
    ZOD(bdSetShort(BIGNUM_NUM(bignum), n));
    return bignum;
}

YogVal
YogBignum_from_int(YogEnv* env, int_t n)
{
    YogVal bignum = YogBignum_new(env);
    if (n < 0) {
        ZOD(bdSetShort(BIGNUM_NUM(bignum), - n));
        BIGNUM_SIGN(bignum) = -1;
        return bignum;
    }

    ZOD(bdSetShort(BIGNUM_NUM(bignum), n));
    return bignum;
}

YogVal
YogBignum_from_str(YogEnv* env, YogVal s, int_t base)
{
    /* FIXME: Cannot accept any base but 10 */
    if (base != 10) {
        YogError_raise_ValueError(env, "Cannot accept any base but 10");
    }

    YogHandle* str = YogHandle_REGISTER(env, s);
    YogHandle* bignum = YogHandle_REGISTER(env, YogBignum_new(env));
    YogHandle* enc = YogHandle_REGISTER(env, env->vm->encAscii);
    YogVal ascii = YogEncoding_conv_from_yog(env, enc, str);
    bdConvFromDecimal(BIGNUM_NUM(HDL2VAL(bignum)), BINARY_CSTR(ascii));

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
        double f = YogBignum_to_float(env, self);
        FLOAT_NUM(result) = f * FLOAT_NUM(HDL2VAL(n));
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal result = YogBignum_new(env);
        BIGD m = BIGNUM_NUM(HDL2VAL(self));
        ZOD(bdMultiply(BIGNUM_NUM(result), m, BIGNUM_NUM(HDL2VAL(n))));
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
    BIGD m = BIGNUM_NUM(retval);
    bdOrBits(m, BIGNUM_NUM(HDL2VAL(self)), BIGNUM_NUM(HDL2VAL(n)));
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
    YogVal retval = YogBignum_new(env);
    BIGD m = BIGNUM_NUM(retval);
    bdXorBits(m, BIGNUM_NUM(HDL2VAL(self)), BIGNUM_NUM(HDL2VAL(n)));
    return normalize(env, retval);
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
    BIGD m = BIGNUM_NUM(result);
    bdAndBits(m, BIGNUM_NUM(HDL2VAL(self)), BIGNUM_NUM(HDL2VAL(n)));
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
        return YNIL;
    }

    int_t m = bdCompare(BIGNUM_NUM(self), BIGNUM_NUM(n));
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
    ZOD(bdSetEqual(BIGNUM_NUM(retval), BIGNUM_NUM(self)));

    RETURN(env, retval);
}

YogVal
YogBignum_power(YogEnv* env, YogHandle* self, int_t exp)
{
    if (exp < 0) {
        YogVal f = YogFloat_new(env);
        FLOAT_NUM(f) = 1 / YogBignum_to_float(env, self);
        return YogFloat_power(env, f, - exp);
    }
    else if (exp == 0) {
        return INT2VAL(1);
    }

    YogVal bignum = YogBignum_clone(env, HDL2VAL(self));
    BIGD m = BIGNUM_NUM(HDL2VAL(self));
    int_t i;
    for (i = exp; 1 < i; i--) {
        ZOD(bdMultiply_s(BIGNUM_NUM(bignum), BIGNUM_NUM(bignum), m));
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
        YogVal retval = YogFloat_new(env);
        double f = YogBignum_to_float(env, self);
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

int
YogBignum_compare_with_unsigned_long(YogEnv* env, YogVal self, unsigned long n)
{
    if (BIGNUM_SIGN(self) < 0) {
        return -1;
    }

    char buf[21];
    snprintf(buf, array_sizeof(buf), "%lu", n);
    BIGD n2 = bdNew();
    bdConvFromDecimal(n2, buf);
    int retval = bdCompare(BIGNUM_NUM(self), n2);
    bdFree(&n2);
    return retval;
}

int
YogBignum_compare_with_unsigned_int(YogEnv* env, YogVal self, uint_t n)
{
    if (BIGNUM_SIGN(self) < 0) {
        return -1;
    }

    char buf[21];
    snprintf(buf, array_sizeof(buf), "%u", n);
    BIGD n2 = bdNew();
    bdConvFromDecimal(n2, buf);
    int retval = bdCompare(BIGNUM_NUM(self), n2);
    bdFree(&n2);
    return retval;
}

#define RETURN_IF_DIFFERENT_SIGN(self, n) do { \
    int sign_of_self = BIGNUM_SIGN((self)); \
    int sign_of_n = 0 < (n) ? 1 : -1; \
    if (sign_of_self * sign_of_n < 0) { \
        return sign_of_self; \
    } \
} while (0)

int
YogBignum_compare_with_long(YogEnv* env, YogVal self, long n)
{
    RETURN_IF_DIFFERENT_SIGN(self, n);

    /* FIXME: In case of n == LONG_MIN */
    int sign_of_n = 0 < n ? 1 : -1;
    long abs_of_n = sign_of_n * n;
    char buf[21];
    snprintf(buf, array_sizeof(buf), "%lu", abs_of_n);
    BIGD n2 = bdNew();
    bdConvFromDecimal(n2, buf);
    int retval = bdCompare(BIGNUM_NUM(self), n2);
    bdFree(&n2);

    return 0 < BIGNUM_SIGN(self) ? retval : - retval;
}

int
YogBignum_compare_with_int(YogEnv* env, YogVal self, int_t n)
{
    int sign_of_self = BIGNUM_SIGN(self);
    int sign_of_n = 0 < n ? 1 : -1;
    if (sign_of_self * sign_of_n < 0) {
        /* When sign of self and that of n are different */
        return sign_of_self;
    }

    /* FIXME: In case of n == LONG_MIN */
    int_t abs_of_n = sign_of_n * n;
    char buf[21];
    snprintf(buf, array_sizeof(buf), "%d", abs_of_n);
    BIGD n2 = bdNew();
    bdConvFromDecimal(n2, buf);
    int retval = bdCompare(BIGNUM_NUM(self), n2);
    bdFree(&n2);

    return 0 < sign_of_self ? retval : - retval;
}

static void
check_range_unsigned(YogEnv* env, YogVal self, const char* name, unsigned long max)
{
    if (YogBignum_compare_with_unsigned_long(env, self, 0) < 0) {
        const char* fmt = "%s must be greater or equal than 0, not %S";
        YogError_raise_ValueError(env, fmt, name, self);
    }
    if (0 < YogBignum_compare_with_unsigned_long(env, self, max)) {
        const char* fmt = "%s must be less or equal %lu, not %S";
        YogError_raise_ValueError(env, fmt, name, max, self);
    }
}

unsigned long
YogBignum_to_unsigned_long(YogEnv* env, YogVal self, const char* name)
{
    CHECK_SELF_TYPE(env, self);
    check_range_unsigned(env, self, name, ULONG_MAX);

    CONV_TO_DECIMAL(buf, self);
    char* endptr;
    unsigned long l = strtoul(buf, &endptr, 10);
    YOG_ASSERT2(env, *endptr == '\0');

    return l;
}

UNSIGNED_TYPE
YogBignum_to_unsigned_type(YogEnv* env, YogVal self, const char* name)
{
    CHECK_SELF_TYPE(env, self);
    check_range_unsigned(env, self, name, UNSIGNED_MAX);

    CONV_TO_DECIMAL(buf, self);
    char* endptr;
    UNSIGNED_TYPE n = strtoul(buf, &endptr, 10);
    YOG_ASSERT2(env, *endptr == '\0');

    return n;
}

static void
check_range(YogEnv* env, YogVal self, const char* name, long min, long max)
{
    if (YogBignum_compare_with_int(env, self, min) < 0) {
        const char* fmt = "%s must be greater or equal %d, not %S";
        YogError_raise_ValueError(env, fmt, name, min, self);
    }
    if (0 < YogBignum_compare_with_int(env, self, max)) {
        const char* fmt = "%s must be less or equal %d, not %S";
        YogError_raise_ValueError(env, fmt, name, max, self);
    }
}

long
YogBignum_to_long(YogEnv* env, YogVal self, const char* name)
{
    CHECK_SELF_TYPE(env, self);
    check_range(env, self, name, LONG_MIN, LONG_MAX);

    CONV_TO_DECIMAL(buf, self);
    char* endptr;
    long l = strtol(buf, &endptr, 10);
    YOG_ASSERT2(env, *endptr == '\0');

    return l;
}

SIGNED_TYPE
YogBignum_to_signed_type(YogEnv* env, YogVal self, const char* name)
{
    CHECK_SELF_TYPE(env, self);
    check_range(env, self, name, SIGNED_MIN, SIGNED_MAX);

    CONV_TO_DECIMAL(buf, self);
    char* endptr;
    SIGNED_TYPE n = strtol(buf, &endptr, 10);
    YOG_ASSERT2(env, *endptr == '\0');

    return n;
}

int_t
YogBignum_compare_with_long_long(YogEnv* env, YogVal self, long long n)
{
    SAVE_ARG(env, self);
    RETURN_IF_DIFFERENT_SIGN(self, n);

    YogVal n2 = YogBignum_from_long_long(env, n);
    int retval = bdCompare(BIGNUM_NUM(self), BIGNUM_NUM(n2));

    RETURN(env, 0 < BIGNUM_SIGN(self) ? retval : - retval);
}

int_t
YogBignum_compare_with_unsigned_long_long(YogEnv* env, YogVal self, unsigned long long n)
{
    SAVE_ARG(env, self);
    if (BIGNUM_SIGN(self) < 0) {
        RETURN(env, -1);
    }

    YogVal n2 = YogBignum_from_unsigned_long_long(env, n);
    RETURN(env, bdCompare(BIGNUM_NUM(self), BIGNUM_NUM(n2)));
}

long long
YogBignum_to_long_long(YogEnv* env, YogVal self, const char* name)
{
    SAVE_ARG(env, self);

    if ((YogBignum_compare_with_long_long(env, self, INT64_MIN) < 0) || (0 < YogBignum_compare_with_long_long(env, self, INT64_MAX))) {
        YogError_raise_ValueError(env, "%s must be between %lld and %lld", name, INT64_MIN, INT64_MAX);
    }

    CONV_TO_DECIMAL(buf, self);
    char* endptr;
    long long retval = strtoll(buf, &endptr, 10);
    YOG_ASSERT2(env, *endptr == '\0');

    RETURN(env, retval);
}

unsigned long long
YogBignum_to_unsigned_long_long(YogEnv* env, YogVal self, const char* name)
{
    SAVE_ARG(env, self);

    if ((YogBignum_compare_with_unsigned_int(env, self, 0) < 0) || (0 < YogBignum_compare_with_unsigned_long_long(env, self, UINT64_MAX))) {
        YogError_raise_ValueError(env, "%s must be between 0 and %llu", name, UINT64_MAX);
    }

    CONV_TO_DECIMAL(buf, self);
    char* endptr;
    unsigned long long retval = strtoull(buf, &endptr, 10);
    YOG_ASSERT2(env, *endptr == '\0');

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
