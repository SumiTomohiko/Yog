#if !defined(__YOG_FUNCTION_H__)
#define __YOG_FUNCTION_H__

#include <stdint.h>
#include "yog/object.h"
#include "yog/yog.h"

struct YogCallable {
    struct YogBasicObj base;

    void (*exec)(YogEnv*, YogVal, uint8_t, YogVal*, YogVal, uint8_t, YogVal*, YogVal, YogVal);
    YogVal (*call)(YogEnv*, YogVal, uint8_t, YogVal*, YogVal, uint8_t, YogVal*, YogVal, YogVal);
};

typedef struct YogCallable YogCallable;

struct YogNativeFunction {
    struct YogCallable base;

    ID name;
    void* f;
};

typedef struct YogNativeFunction YogNativeFunction;

struct YogFunction {
    struct YogCallable base;

    YogVal code;
    YogVal globals;
    YogVal outer_vars;
};

typedef struct YogFunction YogFunction;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/function.c */
YogVal YogFunction_klass_new(YogEnv*);
YogVal YogFunction_new(YogEnv*);
YogVal YogNativeFunction_new(YogEnv*, const char*, void*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
