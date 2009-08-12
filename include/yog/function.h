#if !defined(__YOG_FUNCTION_H__)
#define __YOG_FUNCTION_H__

#include <stdint.h>
#include "yog/object.h"
#include "yog/yog.h"

struct YogNativeFunction {
    struct YogBasicObj base;

    ID func_name;
    ID klass_name;
    void* f;
};

typedef struct YogNativeFunction YogNativeFunction;

struct YogFunction {
    struct YogBasicObj base;

    YogVal code;
    YogVal globals;
    YogVal outer_vars;
};

typedef struct YogFunction YogFunction;

struct YogInstanceMethod {
    struct YogBasicObj base;

    YogVal self;
    YogVal f;
};

typedef struct YogInstanceMethod YogInstanceMethod;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/function.c */
YogVal YogCallable_call(YogEnv*, YogVal, uint_t, YogVal*);
YogVal YogCallable_call1(YogEnv*, YogVal, YogVal);
YogVal YogCallable_call2(YogEnv*, YogVal, uint_t, YogVal*, YogVal);
YogVal YogFunction_klass_new(YogEnv*);
YogVal YogFunction_new(YogEnv*);
YogVal YogInstanceMethod_klass_new(YogEnv*);
YogVal YogInstanceMethod_new(YogEnv*);
YogVal YogNativeFunction_klass_new(YogEnv*);
YogVal YogNativeFunction_new(YogEnv*, ID, const char*, void*);
YogVal YogNativeInstanceMethod_klass_new(YogEnv*);
YogVal YogNativeInstanceMethod_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
