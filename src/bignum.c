#include "gmp.h"
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/error.h"
#include "yog/fixnum.h"
#include "yog/float.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/yog.h"

#define BIGNUM2FLOAT(bignum)    mpz_get_d(BIGNUM_NUM(bignum))

static YogVal
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    size_t size = mpz_sizeinbase(PTR_AS(YogBignum, self)->num, 10) + 2;
    s = YogString_new_size(env, size);
    mpz_get_str(STRING_CSTR(s), 10, PTR_AS(YogBignum, self)->num);
    PTR_AS(YogCharArray, PTR_AS(YogString, s)->body)->size = size;

    RETURN(env, s);
}

static void
YogBignum_finalize(YogEnv* env, void* ptr)
{
    YogBignum* bignum = ptr;
    mpz_clear(bignum->num);
}

static void
YogBignum_initialize(YogEnv* env, YogVal self)
{
    YogBasicObj_init(env, self, 0, env->vm->cBignum);
    mpz_init(PTR_AS(YogBignum, self)->num);
}

static YogVal
YogBignum_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = ALLOC_OBJ(env, YogBasicObj_keep_children, YogBignum_finalize, YogBignum);
    YogBignum_initialize(env, bignum);

    RETURN(env, bignum);
}

static YogVal
negative(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = YogBignum_new(env);
    mpz_neg(BIGNUM_NUM(bignum), BIGNUM_NUM(self));

    RETURN(env, bignum);
}

static YogVal
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        result = YogBignum_from_int(env, VAL2INT(right));
        mpz_add(BIGNUM_NUM(result), BIGNUM_NUM(self), BIGNUM_NUM(result));
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = mpz_get_d(BIGNUM_NUM(self)) + FLOAT_NUM(right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogBignum_new(env);
        mpz_add(BIGNUM_NUM(result), BIGNUM_NUM(self), BIGNUM_NUM(right));
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "+");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
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
YogBignum_subtract(YogEnv* env, YogVal self, YogVal bignum)
{
    SAVE_ARGS2(env, self, bignum);
    YogVal result = YUNDEF;
    PUSH_LOCAL(env, result);

    result = YogBignum_new(env);
    mpz_sub(BIGNUM_NUM(result), BIGNUM_NUM(self), BIGNUM_NUM(bignum));
    result = normalize(env, result);

    RETURN(env, result);
}

static YogVal
subtract_int(YogEnv* env, YogVal self, int_t right)
{
    SAVE_ARG(env, self);
    YogVal result = YUNDEF;
    YogVal bignum = YUNDEF;
    PUSH_LOCALS2(env, result, bignum);

    bignum = YogBignum_from_int(env, right);
    result = YogBignum_subtract(env, self, bignum);

    RETURN(env, result);
}

static YogVal
subtract(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        result = subtract_int(env, self, VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = mpz_get_d(BIGNUM_NUM(self)) - FLOAT_NUM(right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogBignum_subtract(env, self, right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "-");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
multiply(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        result = YogFixnum_multiply(env, right, self);
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = mpz_get_d(BIGNUM_NUM(self)) * FLOAT_NUM(right);
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = YogBignum_new(env);
        mpz_mul(BIGNUM_NUM(result), BIGNUM_NUM(self), BIGNUM_NUM(right));
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "*");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
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
    FLOAT_NUM(result) = BIGNUM2FLOAT(self) / right;

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
    FLOAT_NUM(result) = BIGNUM2FLOAT(self) / right;

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
    FLOAT_NUM(result) = BIGNUM2FLOAT(self) / BIGNUM2FLOAT(bignum);

    RETURN(env, result);
}

static YogVal
divide(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        result = divide_int(env, self, VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = divide_float(env, self, FLOAT_NUM(right));
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
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

static YogVal
floor_divide(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, !IS_UNDEF(right), "right is undef");
    if (IS_FIXNUM(right)) {
        result = floor_divide_int(env, self, VAL2INT(right));
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (IS_OBJ_OF(env, right, cFloat)) {
        result = divide_float(env, self, FLOAT_NUM(right));
        RETURN(env, result);
    }
    else if (IS_OBJ_OF(env, right, cBignum)) {
        result = floor_divide_bignum(env, self, right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "//");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
positive(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    return self;
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
rshift(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS2(env, retval, right);

    right = YogArray_at(env, args, 0);
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
lshift(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS2(env, retval, right);

    right = YogArray_at(env, args, 0);
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
YogBignum_multiply(YogEnv* env, YogVal self, YogVal bignum)
{
    SAVE_ARGS2(env, self, bignum);
    YogVal result = YUNDEF;
    PUSH_LOCAL(env, result);

    result = YogBignum_new(env);
    mpz_mul(BIGNUM_NUM(result), BIGNUM_NUM(self), BIGNUM_NUM(bignum));

    RETURN(env, result);
}

YogVal
YogBignum_or(YogEnv* env, YogVal self, YogVal n)
{
    SAVE_ARGS2(env, self, n);
    YogVal retval = YUNDEF;
    YogVal bignum = YUNDEF;
    PUSH_LOCALS2(env, retval, bignum);

    YOG_ASSERT(env, !IS_UNDEF(n), "undefined value");
    if (!IS_FIXNUM(n) && !(IS_PTR(n) && IS_OBJ_OF(env, n, cBignum))) {
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
or(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogBignum_or(env, self, YogArray_at(env, args, 0));

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
    else if (IS_PTR(n) && IS_OBJ_OF(env, n, cBignum)) {
        bignum = n;
    }
    if (IS_UNDEF(bignum)) {
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
    else if (IS_PTR(n) && IS_OBJ_OF(env, n, cBignum)) {
        bignum = n;
    }
    if (IS_UNDEF(bignum)) {
        YogError_raise_binop_type_error(env, self, n, "&");
    }

    result = YogBignum_new(env);
    mpz_and(BIGNUM_NUM(result), BIGNUM_NUM(self), BIGNUM_NUM(bignum));
    result = normalize(env, result);

    RETURN(env, result);
}

static YogVal
and(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogBignum_and(env, self, YogArray_at(env, args, 0));

    RETURN(env, retval);
}

static YogVal
xor(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogBignum_xor(env, self, YogArray_at(env, args, 0));

    RETURN(env, retval);
}

YogVal
YogBignum_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Bignum", env->vm->cObject);
#define DEFINE_METHOD(name, f)  YogKlass_define_method(env, klass, (name), (f))
    DEFINE_METHOD("-self", negative);
    DEFINE_METHOD("+self", positive);
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("-", subtract);
    DEFINE_METHOD("*", multiply);
    DEFINE_METHOD("/", divide);
    DEFINE_METHOD("//", floor_divide);
    DEFINE_METHOD("<<", lshift);
    DEFINE_METHOD(">>", rshift);
    DEFINE_METHOD("|", or);
    DEFINE_METHOD("&", and);
    DEFINE_METHOD("^", xor);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
