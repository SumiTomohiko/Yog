#include "yog/config.h"
#if defined(YOG_HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <stdlib.h>
#include <math.h>
#include <gmp.h>
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/float.h"
#include "yog/frame.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_FIXNUM(self)) { \
        YogError_raise_TypeError((env), "self must be Fixnum"); \
    } \
} while (0)
#define CHECK_SELF_TYPE2(env, self)  do { \
    if (!IS_FIXNUM(HDL2VAL((self)))) { \
        YogError_raise_TypeError((env), "self must be Fixnum"); \
    } \
} while (0)

static YogVal
to_s_internal(YogEnv* env, int_t self, int_t radix)
{
    /**
     * buffer is small for 64bit.
     * Issue: e03cb3906e74d58cc481879d9042ab51fd9c8340
     */
    char buffer[31];
    char* pend = buffer + array_sizeof(buffer) - 1;
    *pend = '\0';
    char* p = pend - 1;
    int_t num = abs(self);
    do {
        int_t mod = num % radix;
        *p = mod < 10 ? '0' + mod : 'a' + mod - 10;
        p--;
        num /= radix;
    } while (num != 0);
    if (self < 0) {
        *p = '-';
        p--;
    }
    return YogString_from_string(env, p + 1);
}

static YogVal
to_s(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* radix)
{
    CHECK_SELF_TYPE2(env, self);
    YogMisc_check_Fixnum_optional(env, radix, "radix");
    if (radix == NULL) {
        return to_s_internal(env, HDL2INT(self), 10);
    }
    if ((HDL2INT(radix) < 2) || (36 < HDL2INT(radix))) {
        const char* fmt = "radix must be in range from 2 to 36, not %d";
        YogError_raise_ValueError(env, fmt, HDL2INT(radix));
    }
    return to_s_internal(env, HDL2INT(self), HDL2INT(radix));
}

YogVal
YogFixnum_add_bignum(YogEnv* env, YogVal self, YogVal bignum)
{
    SAVE_ARGS2(env, self, bignum);
    YogVal left_and_result = YUNDEF;
    PUSH_LOCAL(env, left_and_result);

    left_and_result = YogBignum_from_int(env, VAL2INT(self));
    mpz_add(BIGNUM_NUM(left_and_result), BIGNUM_NUM(left_and_result), BIGNUM_NUM(bignum));

    RETURN(env, left_and_result);
}

YogVal
YogFixnum_binop_add(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return YogVal_from_int(env, VAL2INT(self) + VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = (double)VAL2INT(self) + FLOAT_NUM(right);
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        return YogFixnum_add_bignum(env, self, right);
    }

    YogError_raise_binop_type_error(env, self, right, "+");

    /* NOTREACHED */
    return YUNDEF;
}

static YogVal
add(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_add(env, HDL2VAL(self), n);
}

YogVal
YogFixnum_binop_subtract(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return YogVal_from_int(env, VAL2INT(self) - VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = (double)VAL2INT(self) - FLOAT_NUM(HDL2VAL(n));
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal bignum = YogBignum_from_int(env, VAL2INT(self));
        YogHandle* h = YogHandle_REGISTER(env, bignum);
        return YogBignum_binop_subtract(env, h, n);
    }

    YogError_raise_binop_type_error(env, self, right, "-");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
subtract(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_subtract(env, HDL2VAL(self), n);
}

static YogVal
multiply_int(YogEnv* env, YogVal self, YogHandle* right)
{
    int_t n = VAL2INT(self);
    if (n == 0) {
        return INT2VAL(0);
    }
    int_t m = VAL2INT(HDL2VAL(right));
    int_t l = n * m;
    if ((l / n == m) && FIXABLE(l)) {
        return INT2VAL(l);
    }

    YogHandle* bignum = YogHandle_REGISTER(env, YogBignum_from_int(env, n));
    return YogBignum_binop_multiply(env, bignum, right);
}

YogVal
YogFixnum_binop_multiply(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return multiply_int(env, self, n);
    }
    else if (IS_BOOL(right) || IS_NIL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        double f = FLOAT_NUM(HDL2VAL(n));
        FLOAT_NUM(result) = (double)VAL2INT(self) * f;
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal bignum = YogBignum_from_int(env, VAL2INT(self));
        YogHandle* h = YogHandle_REGISTER(env, bignum);
        return YogBignum_binop_multiply(env, h, n);
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_STRING) {
        return YogString_binop_multiply(env, n, self);
    }

    YogError_raise_binop_type_error(env, self, right, "*");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
multiply(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_multiply(env, HDL2VAL(self), n);
}

static double
divide_int(YogEnv* env, YogVal left, YogVal right)
{
    if (VAL2INT(right) == 0) {
        YogError_raise_ZeroDivisionError(env, "Fixnum division by zero");
    }
    return (double)VAL2INT(left) / VAL2INT(right);
}

static double
divide_float(YogEnv* env, YogVal left, YogVal right)
{
    if (FLOAT_NUM(right) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "Float division");
    }
    return VAL2INT(left) / FLOAT_NUM(right);
}

YogVal
YogFixnum_binop_divide(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_BOOL(right) || IS_NIL(right) || IS_SYMBOL(right)) {
        YogError_raise_binop_type_error(env, self, right, "/");
        /* NOTREACHED */
    }

    YogVal result = YogFloat_new(env);
    right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        FLOAT_NUM(result) = divide_int(env, self, right);
        return result;
    }
    if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        FLOAT_NUM(result) = divide_float(env, self, right);
        return result;
    }
    if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        FLOAT_NUM(result) = VAL2INT(self) / mpz_get_d(BIGNUM_NUM(right));
        return result;
    }
    /* NOTREACHED */
    YOG_BUG(env, "Invalid operand (%p)", n);

    return YUNDEF;
}

static YogVal
divide(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_divide(env, HDL2VAL(self), n);
}

static int_t
floor_divide_int(YogEnv* env, YogVal left, YogVal right)
{
    int_t n = VAL2INT(left);
    int_t m = VAL2INT(right);
    if (m == 0) {
        YogError_raise_ZeroDivisionError(env, "Fixnum division by zero");
    }
    div_t q = div(n, m);
    if (((q.rem < 0) && (0 < m)) || ((0 < q.rem) && (m < 0))) {
        return q.quot - 1;
    }
    return q.quot;
}

static double
floor_divide_float(YogEnv* env, YogVal left, YogVal right)
{
    if (FLOAT_NUM(right) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "float division");
    }
    return VAL2INT(left) / FLOAT_NUM(right);
}

YogVal
YogFixnum_binop_modulo(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return INT2VAL(VAL2INT(self) % VAL2INT(right));
    }
    else if (IS_PTR(right) && (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM)) {
        YogVal bignum = YogBignum_from_int(env, VAL2INT(self));
        YogHandle* h = YogHandle_REGISTER(env, bignum);
        return YogBignum_modulo(env, h, n);
    }

    YogError_raise_binop_type_error(env, self, right, "%");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
modulo(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_modulo(env, HDL2VAL(self), n);
}

YogVal
YogFixnum_binop_floor_divide(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return INT2VAL(floor_divide_int(env, self, right));
    }
    else if (IS_BOOL(right) || IS_NIL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = floor_divide_float(env, self, HDL2VAL(n));
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal result = YogBignum_from_int(env, VAL2INT(self));
        mpz_t* h = &BIGNUM_NUM(HDL2VAL(n));
        mpz_fdiv_q(BIGNUM_NUM(result), BIGNUM_NUM(result), *h);
        return result;
    }

    YogError_raise_binop_type_error(env, self, right, "//");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
floor_divide(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_floor_divide(env, HDL2VAL(self), n);
}

static YogVal
do_rshift(YogEnv* env, int_t val, int_t width)
{
    YOG_ASSERT(env, 0 <= width, "negative width (%d)", width);

    if (width < sizeof(int_t) * CHAR_BIT) {
        return INT2VAL(val >> width);
    }

    return INT2VAL(0);
}

static YogVal
do_lshift(YogEnv* env, int_t val, int_t width)
{
    YOG_ASSERT(env, 0 <= width, "negative width (%d)", width);
    if (width < sizeof(int_t) * CHAR_BIT) {
        int_t result = val << width;
        if ((result >> width == val) && FIXABLE(result)) {
            return INT2VAL(result);
        }
    }

    YogHandle* bignum = YogHandle_REGISTER(env, YogBignum_from_int(env, val));
    return YogBignum_lshift(env, bignum, width);
}

YogVal
YogFixnum_binop_xor(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return INT2VAL(VAL2INT(self) ^ VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        return YogBignum_xor(env, n, VAL2INT(self));
    }

    YogError_raise_binop_type_error(env, self, right, "^");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
xor(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_xor(env, HDL2VAL(self), n);
}

YogVal
YogFixnum_binop_and(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return INT2VAL(VAL2INT(self) & VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        return YogBignum_and(env, n, VAL2INT(self));
    }

    YogError_raise_binop_type_error(env, self, right, "&");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
and(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_and(env, HDL2VAL(self), n);
}

YogVal
YogFixnum_binop_or(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return INT2VAL(VAL2INT(self) | VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        return YogBignum_or(env, n, VAL2INT(self));
    }

    YogError_raise_binop_type_error(env, self, right, "|");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
or(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_or(env, HDL2VAL(self), n);
}

YogVal
YogFixnum_binop_rshift(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (!IS_FIXNUM(right)) {
        YogError_raise_binop_type_error(env, self, right, ">>");
        /* NOTREACHED */
    }

    int_t width = VAL2INT(right);
    if (0 < width) {
        return do_rshift(env, VAL2INT(self), width);
    }
    return do_lshift(env, VAL2INT(self), - width);
}

static YogVal
rshift(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_rshift(env, HDL2VAL(self), n);
}

YogVal
YogFixnum_binop_lshift(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (!IS_FIXNUM(right)) {
        YogError_raise_binop_type_error(env, self, right, "<<");
        /* NOTREACHED */
    }
    int_t width = VAL2INT(right);
    if (0 < width) {
        return do_lshift(env, VAL2INT(self), width);
    }
    return do_rshift(env, VAL2INT(self), - width);
}

static YogVal
lshift(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_lshift(env, HDL2VAL(self), n);
}

static YogVal
times(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "times", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    int_t n = VAL2INT(self);
    uint_t i;
    for (i = 0; i < n; i++) {
        YogVal args[1] = { INT2VAL(i) };
        YogCallable_call(env, block, array_sizeof(args), args);
    }

    RETURN(env, YNIL);
}

static YogVal
negative(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal n = YUNDEF;
    PUSH_LOCAL(env, n);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "-self", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    n = INT2VAL(- VAL2INT(self));

    RETURN(env, n);
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
    YogVal n = YUNDEF;
    PUSH_LOCAL(env, n);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "~self", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    n = INT2VAL(~ VAL2INT(self));

    RETURN(env, n);
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "hash", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    RETURN(env, self);
}

static YogVal
power_int(YogEnv* env, int_t base, int_t exp)
{
    if (exp < 0) {
        if (base == 0) {
            YogError_raise_ZeroDivisionError(env, "0.0 cannot be raised to a negative power");
        }

        YogVal f = YogFloat_new(env);
        FLOAT_NUM(f) = 1 / (double)base;
        return YogFloat_power(env, f, - exp);
    }
    else if (exp == 0) {
        return INT2VAL(1);
    }

    int_t x = base;
    int_t y = exp;
    while (1 < y) {
        int_t x2 = x * base;
        if (!FIXABLE(x2) || ((x != 0) && (x2 / x != base))) {
            YogVal bignum = YogBignum_from_int(env, base);
            YogHandle* h = YogHandle_REGISTER(env, bignum);
            return YogBignum_power(env, h, exp);
        }

        x = x2;
        y--;
    }

    return INT2VAL(x);
}

YogVal
YogFixnum_binop_power(YogEnv* env, YogVal self, YogHandle* n)
{
    YogVal right = HDL2VAL(n);
    if (IS_FIXNUM(right)) {
        return power_int(env, VAL2INT(self), VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        double base = (double)VAL2INT(self);
        double exp = FLOAT_NUM(right);
        YogVal f = YogFloat_new(env);
        FLOAT_NUM(f) = pow(base, exp);
        return f;
    }

    YogError_raise_binop_type_error(env, self, right, "**");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
power(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* n)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFixnum_binop_power(env, HDL2VAL(self), n);
}

YogVal
YogFixnum_binop_ufo(YogEnv* env, YogVal self, YogVal n)
{
    if (!IS_FIXNUM(n)) {
        return YNIL;
    }

    int_t m = VAL2INT(self) - VAL2INT(n);
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
    return YogFixnum_binop_ufo(env, HDL2VAL(self), HDL2VAL(n));
}

uint_t
YogFixnum_to_uint(YogEnv* env, YogVal self, const char* name)
{
    if (!IS_FIXNUM(self)) {
        YogMisc_raise_TypeError(env, self, name, "Fixnum");
    }
    int_t n = VAL2INT(self);
    if (n < 0) {
        const char* fmt = "%s must be greater or equal 0, not %d";
        YogError_raise_ValueError(env, fmt, name, n);
    }
    return (uint_t)n;
}

void
YogFixnum_eval_builtin_script(YogEnv* env, YogVal klass)
{
    const char* src =
#include "fixnum.inc"
    ;
    YogMisc_eval_source(env, VAL2HDL(env, klass), src);
}

void
YogFixnum_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cFixnum = YUNDEF;
    PUSH_LOCAL(env, cFixnum);
    YogVM* vm = env->vm;

    cFixnum = YogClass_new(env, "Fixnum", vm->cObject);
    YogClass_include_module(env, cFixnum, vm->mComparable);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cFixnum, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("+self", positive);
    DEFINE_METHOD("-self", negative);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("times", times);
    DEFINE_METHOD("~self", not);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...) do { \
    YogClass_define_method2(env, cFixnum, pkg, (name), __VA_ARGS__); \
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
    DEFINE_METHOD2("to_s", to_s, "|", "radix", NULL);
    DEFINE_METHOD2("|", or, "n", NULL);
#undef DEFINE_METHOD2
    vm->cFixnum = cFixnum;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
