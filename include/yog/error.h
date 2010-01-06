#if !defined(__YOG_ERROR_H__)
#define __YOG_ERROR_H__

#include "yog/yog.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
#if defined(__cplusplus)
extern "C" {
#endif
/* src/error.c */
void YogError_bug(YogEnv*, const char*, uint_t, const char*, ...);
void YogError_out_of_memory(YogEnv*);
void YogError_print_stacktrace(YogEnv*);
void YogError_raise(YogEnv*, YogVal);
void YogError_raise_ArgumentError(YogEnv*, const char*, ...);
void YogError_raise_AttributeError(YogEnv*, const char*, ...);
void YogError_raise_EOFError(YogEnv*, const char*, ...);
void YogError_raise_ImportError(YogEnv*, const char*, ...);
void YogError_raise_IndexError(YogEnv*, const char*, ...);
void YogError_raise_KeyError(YogEnv*, const char*, ...);
void YogError_raise_LocalJumpError(YogEnv*, const char*, ...);
void YogError_raise_NameError(YogEnv*, const char*, ...);
void YogError_raise_SyntaxError(YogEnv*, const char*, ...);
void YogError_raise_TypeError(YogEnv*, const char*, ...);
void YogError_raise_ValueError(YogEnv*, const char*, ...);
void YogError_raise_ZeroDivisionError(YogEnv*, const char*, ...);
void YogError_raise_binop_type_error(YogEnv*, YogVal, YogVal, const char*);
void YogError_raise_comparison_type_error(YogEnv*, YogVal, YogVal);
void YogError_raise_sys_call_err(YogEnv*, int);
void YogError_warn(YogEnv*, const char*, uint_t, const char*, ...);


#if defined(__cplusplus)
}
#endif
/* PROTOTYPE_END */

#if defined(__GNUC__)
#   define YOG_WARN(env, ...)  do { \
    YogError_warn(env, __FILE__, __LINE__, __VA_ARGS__); \
} while (0)
#   define YOG_BUG(env, ...)    do { \
    YogError_bug(env, __FILE__, __LINE__, __VA_ARGS__); \
} while (0)
#   define YOG_ASSERT(env, test, ...)  do { \
    if (!(test)) { \
        YOG_BUG(env, __VA_ARGS__); \
    } \
} while (0)
#else
#   include <stdarg.h>
#   include <stdio.h>
static void
YOG_WARN(YogEnv* env, const char* fmt, ...)
{
    /* TODO */
}

static void
YOG_BUG(YogEnv* env, const char* fmt, ...)
{
    /* TODO */
}

static void
YOG_ASSERT(YogEnv* env, BOOL test, const char* fmt, ...)
{
    /* TODO */
}
#endif

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
