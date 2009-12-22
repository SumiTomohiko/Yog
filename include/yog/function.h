#if !defined(__YOG_FUNCTION_H__)
#define __YOG_FUNCTION_H__

#include "config.h"
#if defined(HAVE_STDINT_H)
#   include <stdint.h>
#endif
#include "yog/object.h"
#include "yog/yog.h"

struct YogNativeFunction {
    struct YogBasicObj base;

    ID func_name;
    ID class_name;
    void* f;
};

typedef struct YogNativeFunction YogNativeFunction;

#define TYPE_NATIVE_FUNCTION    ((type_t)YogNativeFunction_new)

struct YogFunction {
    struct YogBasicObj base;

    YogVal code;
    YogVal globals;
    YogVal outer_vars;
    YogVal frame_to_long_return;
    YogVal frame_to_long_break;

    YogVal klass;
    ID name;
};

typedef struct YogFunction YogFunction;

#define TYPE_FUNCTION   ((type_t)YogFunction_new)

struct YogInstanceMethod {
    struct YogBasicObj base;

    YogVal self;
    YogVal f;
};

typedef struct YogInstanceMethod YogInstanceMethod;

#define TYPE_INSTANCE_METHOD    ((type_t)YogInstanceMethod_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
#if defined(__cplusplus)
extern "C" {
#endif
/* src/function.c */
YogVal YogCallable_call(YogEnv*, YogVal, uint_t, YogVal*);
YogVal YogCallable_call1(YogEnv*, YogVal, YogVal);
YogVal YogCallable_call2(YogEnv*, YogVal, uint_t, YogVal*, YogVal);
YogVal YogFunction_define_class(YogEnv*);
YogVal YogFunction_new(YogEnv*);
YogVal YogInstanceMethod_define_class(YogEnv*);
YogVal YogInstanceMethod_new(YogEnv*);
YogVal YogNativeFunction_define_class(YogEnv*);
YogVal YogNativeFunction_new(YogEnv*, ID, const char*, void*);
YogVal YogNativeInstanceMethod_define_class(YogEnv*);
YogVal YogNativeInstanceMethod_new(YogEnv*);


#if defined(__cplusplus)
}
#endif
/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
