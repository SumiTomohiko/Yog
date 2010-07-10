#if !defined(__YOG_CALLABLE_H__)
#define __YOG_CALLABLE_H__

#if defined(HAVE_STDINT_H)
#   include <stdint.h>
#endif
#include "yog/object.h"
#include "yog/yog.h"

struct YogNativeFunction {
    struct YogBasicObj base;

    ID class_name;
    YogVal pkg;
    ID func_name;
    YogAPI f;
};

typedef struct YogNativeFunction YogNativeFunction;

DECL_AS_TYPE(YogNativeFunction_new);
#define TYPE_NATIVE_FUNCTION    TO_TYPE(YogNativeFunction_new)

struct YogFunction {
    struct YogBasicObj base;

    YogVal code;
    YogVal globals;
    YogVal outer_frame;
    YogVal frame_to_long_return;
    YogVal frame_to_long_break;
    BOOL needs_self;

    ID name;
};

typedef struct YogFunction YogFunction;

DECL_AS_TYPE(YogFunction_new);
#define TYPE_FUNCTION TO_TYPE(YogFunction_new)

struct YogInstanceMethod {
    struct YogBasicObj base;

    YogVal self;
    YogVal f;
};

typedef struct YogInstanceMethod YogInstanceMethod;

DECL_AS_TYPE(YogInstanceMethod_new);
#define TYPE_INSTANCE_METHOD TO_TYPE(YogInstanceMethod_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/callable.c */
YOG_EXPORT YogVal YogCallable_call(YogEnv*, YogVal, uint_t, YogVal*);
YOG_EXPORT YogVal YogCallable_call1(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogCallable_call2(YogEnv*, YogVal, uint_t, YogVal*, YogVal);
YOG_EXPORT void YogFunction_define_classes(YogEnv*, YogVal);
YOG_EXPORT YogVal YogFunction_new(YogEnv*);
YOG_EXPORT YogVal YogInstanceMethod_define_class(YogEnv*, YogVal);
YOG_EXPORT YogVal YogInstanceMethod_new(YogEnv*);
YOG_EXPORT YogVal YogNativeFunction_new(YogEnv*, ID, YogVal, const char*, YogAPI);
YOG_EXPORT YogVal YogNativeInstanceMethod_define_class(YogEnv*, YogVal);
YOG_EXPORT YogVal YogNativeInstanceMethod_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
