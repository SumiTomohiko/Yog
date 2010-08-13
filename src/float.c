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

static YogVal
cmp(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    CHECK_SELF_TYPE(env, self);
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal f = YUNDEF;
    PUSH_LOCAL(env, f);
    YogCArg params[] = { { "f", &f }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "<=>", params, args, kw);
    if (!IS_PTR(f) || (BASIC_OBJ_TYPE(f) != TYPE_FLOAT)) {
        RETURN(env, YNIL);
    }

    if (FLOAT_NUM(self) < FLOAT_NUM(f)) {
        RETURN(env, INT2VAL(-1));
    }
    else if (FLOAT_NUM(self) == FLOAT_NUM(f)) {
        RETURN(env, INT2VAL(0));
    }

    RETURN(env, INT2VAL(1));
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
YogFloat_add(YogEnv* env, YogHandle* self, YogHandle* f)
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
    return YogFloat_add(env, self, f);
}

static YogVal
subtract(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    YogCArg params[] = { { "f", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "-", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (IS_FIXNUM(right)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) - VAL2INT(right);
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) - mpz_get_d(BIGNUM_NUM(right));
        RETURN(env, result);
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) - FLOAT_NUM(right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "-");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
multiply(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    YogCArg params[] = { { "f", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "*", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (IS_FIXNUM(right)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) * VAL2INT(right);
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) * mpz_get_d(BIGNUM_NUM(right));
        RETURN(env, result);
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) * FLOAT_NUM(right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, "*");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
div_(YogEnv* env, YogVal self, YogVal args, YogVal kw, const char* opname)
{
    SAVE_ARGS3(env, self, args, kw);
    YogVal right = YUNDEF;
    YogVal result = YUNDEF;
    PUSH_LOCALS2(env, right, result);

    YogCArg params[] = { { "f", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, opname, params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (IS_FIXNUM(right)) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) / VAL2INT(right);
        RETURN(env, result);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_BIGNUM) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) / mpz_get_d(BIGNUM_NUM(right));
        RETURN(env, result);
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        result = YogFloat_new(env);
        FLOAT_NUM(result) = FLOAT_NUM(self) / FLOAT_NUM(right);
        RETURN(env, result);
    }

    YogError_raise_binop_type_error(env, self, right, opname);

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
divide(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    return div_(env, self, args, kw, "/");
}

static YogVal
floor_divide(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    return div_(env, self, args, kw, "//");
}

static YogVal
power_float(YogEnv* env, YogVal self, double exp)
{
    SAVE_ARG(env, self);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    if (FLOAT_NUM(self) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "0.0 cannot be raised to a negative power");
    }

    retval = YogFloat_new(env);
    FLOAT_NUM(retval) = pow(FLOAT_NUM(self), exp);

    RETURN(env, retval);
}

static YogVal
power_int(YogEnv* env, YogVal self, int_t exp)
{
    SAVE_ARG(env, self);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    if (FLOAT_NUM(self) == 0.0) {
        YogError_raise_ZeroDivisionError(env, "0.0 cannot be raised to a negative power");
    }

    retval = YogFloat_new(env);
    FLOAT_NUM(retval) = pow(FLOAT_NUM(self), (double)exp);

    RETURN(env, retval);
}

YogVal
YogFloat_power(YogEnv* env, YogVal self, int_t exp)
{
    return power_int(env, self, exp);
}

static YogVal
power(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS2(env, retval, right);

    YogCArg params[] = { { "f", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "**", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (IS_FIXNUM(right)) {
        retval = power_int(env, self, VAL2INT(right));
        RETURN(env, retval);
    }
    else if (IS_NIL(right) || IS_BOOL(right) || IS_SYMBOL(right)) {
    }
    else if (BASIC_OBJ_TYPE(right) == TYPE_FLOAT) {
        retval = power_float(env, self, FLOAT_NUM(right));
        RETURN(env, retval);
    }

    YogError_raise_binop_type_error(env, self, right, "**");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
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
    DEFINE_METHOD("*", multiply);
    DEFINE_METHOD("**", power);
    DEFINE_METHOD("+self", positive);
    DEFINE_METHOD("-", subtract);
    DEFINE_METHOD("-self", negative);
    DEFINE_METHOD("/", divide);
    DEFINE_METHOD("//", floor_divide);
    DEFINE_METHOD("<=>", cmp);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...) do { \
    YogClass_define_method2(env, cFloat, pkg, name, __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("+", add, "f", NULL);
#undef DEFINE_METHOD2
    vm->cFloat = cFloat;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
