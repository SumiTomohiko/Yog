#ifndef __YOG_ERROR_H__
#define __YOG_ERROR_H__

#include "yog/exception.h"
#include "yog/string.h"
#include "yog/yog.h"

#define YOG_ASSERT(env, test, ...)  do { \
    if (!(test)) { \
        YogString* msg = YogString_new_format(env, __VA_ARGS__); \
        YogException* exc = YogBugException_new(env); \
        exc->message = OBJ2VAL(msg); \
        YogVal val = OBJ2VAL(exc); \
        YogError_raise(env, val); \
    } \
} while (0)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/error.c */
void YogError_raise(YogEnv*, YogVal);
void YogError_raise_index_error(YogEnv*, const char*);
void YogError_raise_type_error(YogEnv*, const char*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
