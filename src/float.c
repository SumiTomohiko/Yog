#include "yog/config.h"
#include <ctype.h>
#if defined(HAVE_FLOAT_H)
#   include <float.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/float.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_FLOAT)) { \
        YogError_raise_TypeError((env), "self must be Float"); \
    } \
} while (0)
#define CHECK_SELF_TYPE2(env, self)  do { \
    YogVal obj = HDL2VAL((self)); \
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_FLOAT)) { \
        YogError_raise_TypeError((env), "self must be Float"); \
    } \
} while (0)

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal f = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, YogFloat);
    YogBasicObj_init(env, f, TYPE_FLOAT, 0, klass);
    PTR_AS(YogFloat, f)->val = 0;

    RETURN(env, f);
}

YogVal
YogFloat_new(YogEnv* env)
{
    YogVal f = alloc(env, env->vm->cFloat);
    PTR_AS(YogFloat, f)->val = 0;
    return f;
}

static void
remove_trailing_zero(YogEnv* env, char* buffer)
{
    char* pc = strchr(buffer, 'e');
    if (pc == NULL) {
        pc = buffer + strlen(buffer);
    }
    char* src = pc;
    while ((pc[- 1] == '0') && isdigit(pc[- 2])) {
        pc--;
    }
    memmove(pc, src, strlen(src) + 1);
}

YogVal
YogFloat_binop_ufo(YogEnv* env, YogVal self, YogVal f)
{
    if (!IS_PTR(f) || (BASIC_OBJ_TYPE(f) != TYPE_FLOAT)) {
        return YNIL;
    }

    if (FLOAT_NUM(self) < FLOAT_NUM(f)) {
        return INT2VAL(-1);
    }
    if (FLOAT_NUM(self) == FLOAT_NUM(f)) {
        return INT2VAL(0);
    }
    return INT2VAL(1);
}

static YogVal
ufo(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* f)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFloat_binop_ufo(env, HDL2VAL(self), HDL2VAL(f));
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    CHECK_SELF_TYPE(env, self);
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal n = YUNDEF;
    PUSH_LOCAL(env, n);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "hash", params, args, kw);

    /**
     * Here came from Gauche-0.9. The Gauche's author doesn't know it is good
     * hash.
     */
    int_t h = (int_t)(FLOAT_NUM(self) * 2654435761UL);
    n = YogVal_from_int(env, h);

    RETURN(env, n);
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

    if (YogSysdeps_isnan(FLOAT_NUM(self))) {
        s = YogString_from_str(env, "NaN");
        RETURN(env, s);
    }

    char buffer[32];
    double val = PTR_AS(YogFloat, self)->val;
    YogSysdeps_snprintf(buffer, array_sizeof(buffer), "%#.12g", val);
    remove_trailing_zero(env, buffer);

    s = YogString_from_str(env, buffer);

    RETURN(env, s);
}

static YogVal
negative(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal f = YUNDEF;
    PUSH_LOCAL(env, f);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "-self", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    f = YogFloat_new(env);
    FLOAT_NUM(f) = - FLOAT_NUM(self);

    RETURN(env, f);
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

YogVal
YogFloat_binop_add(YogEnv* env, YogHandle* self, YogHandle* f)
{
    YogVal right = HDL2VAL(f);
    if (IS_FIXNUM(right)) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) + VAL2INT(right);
        return result;
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        double x = mpz_get_d(BIGNUM_NUM(right));
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) + x;
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) + FLOAT_NUM(HDL2VAL(f));
        return result;
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "+");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
add(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* f)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFloat_binop_add(env, self, f);
}

YogVal
YogFloat_binop_subtract(YogEnv* env, YogHandle* self, YogHandle* f)
{
    YogVal right = HDL2VAL(f);
    if (IS_FIXNUM(right)) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) - VAL2INT(HDL2VAL(f));
        return result;
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal result = YogFloat_new(env);
        mpz_t* h = &BIGNUM_NUM(HDL2VAL(f));
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) - mpz_get_d(*h);
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) - FLOAT_NUM(HDL2VAL(f));
        return result;
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "-");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
subtract(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* f)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFloat_binop_subtract(env, self, f);
}

YogVal
YogFloat_binop_multiply(YogEnv* env, YogHandle* self, YogHandle* f)
{
    YogVal right = HDL2VAL(f);
    if (IS_FIXNUM(right)) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) * VAL2INT(right);
        return result;
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal result = YogFloat_new(env);
        mpz_t* h = &BIGNUM_NUM(HDL2VAL(f));
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) * mpz_get_d(*h);
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) * FLOAT_NUM(HDL2VAL(f));
        return result;
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "*");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
multiply(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* f)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFloat_binop_multiply(env, self, f);
}

static YogVal
divide_internal(YogEnv* env, YogHandle* self, YogHandle* f, const char* opname)
{
    YogVal right = HDL2VAL(f);
    if (IS_FIXNUM(right)) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) / VAL2INT(right);
        return result;
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        YogVal result = YogFloat_new(env);
        double d = mpz_get_d(BIGNUM_NUM(HDL2VAL(f)));
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) / d;
        return result;
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        YogVal result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(HDL2VAL(self)) / FLOAT_NUM(HDL2VAL(f));
        return result;
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, opname);
    /* NOTREACHED */

    return YUNDEF;
}

YogVal
YogFloat_binop_divide(YogEnv* env, YogHandle* self, YogHandle* f)
{
    return divide_internal(env, self, f, "/");
}

YogVal
YogFloat_binop_floor_divide(YogEnv* env, YogHandle* self, YogHandle* f)
{
    return divide_internal(env, self, f, "//");
}

static YogVal
floor_divide(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* f)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFloat_binop_floor_divide(env, self, f);
}

static YogVal
divide(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* f)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFloat_binop_divide(env, self, f);
}

static YogVal
power_float(YogEnv* env, YogHandle* self, double exp)
{
    if (FLOAT_NUM(HDL2VAL(self)) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "0.0 cannot be raised to a negative power");
    }

    YogVal retval = YogFloat_new(env);
    FLOAT_NUM(retval) = pow(FLOAT_NUM(HDL2VAL(self)), exp);
    return retval;
}

YogVal
YogFloat_power(YogEnv* env, YogVal self, int_t exp)
{
    if (FLOAT_NUM(self) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "0.0 cannot be raised to a negative power");
    }

    double x = FLOAT_NUM(self);
    YogVal retval = YogFloat_new(env);
    FLOAT_NUM(retval) = pow(x, (double)exp);
    return retval;
}

YogVal
YogFloat_binop_power(YogEnv* env, YogHandle* self, YogHandle* f)
{
    YogVal right = HDL2VAL(f);
    if (IS_FIXNUM(right)) {
        return YogFloat_power(env, HDL2VAL(self), VAL2INT(right));
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        return power_float(env, self, FLOAT_NUM(right));
    }

    YogError_raise_binop_type_error(env, HDL2VAL(self), right, "**");
    /* NOTREACHED */

    return YUNDEF;
}

static YogVal
power(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* f)
{
    CHECK_SELF_TYPE2(env, self);
    return YogFloat_binop_power(env, self, f);
}

YogVal
YogFloat_from_float(YogEnv* env, double f)
{
    SAVE_LOCALS(env);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    val = YogFloat_new(env);
    PTR_AS(YogFloat, val)->val = f;

    RETURN(env, val);
}

YogVal
YogFloat_from_str(YogEnv* env, YogVal s)
{
    SAVE_ARG(env, s);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    double f = strtod(STRING_CSTR(s), NULL);
    val = YogFloat_new(env);
    PTR_AS(YogFloat, val)->val = f;

    RETURN(env, val);
}

void
YogFloat_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cFloat = YUNDEF;
    PUSH_LOCAL(env, cFloat);
    YogVM* vm = env->vm;

    cFloat = YogClass_new(env, "Float", vm->cObject);
    YogClass_define_allocator(env, cFloat, alloc);
    YogClass_include_module(env, cFloat, vm->mComparable);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cFloat, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("+self", positive);
    DEFINE_METHOD("-self", negative);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...) do { \
    YogClass_define_method2(env, cFloat, pkg, name, __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("*", multiply, "f", NULL);
    DEFINE_METHOD2("**", power, "f", NULL);
    DEFINE_METHOD2("+", add, "f", NULL);
    DEFINE_METHOD2("-", subtract, "f", NULL);
    DEFINE_METHOD2("/", divide, "f", NULL);
    DEFINE_METHOD2("//", floor_divide, "f", NULL);
    DEFINE_METHOD2("<=>", ufo, "f", NULL);
#undef DEFINE_METHOD2
    vm->cFloat = cFloat;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
