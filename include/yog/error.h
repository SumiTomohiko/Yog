#if !defined(__YOG_ERROR_H__)
#define __YOG_ERROR_H__

#if 0
#include "yog/exception.h"
#include "yog/string.h"
#endif
#include "yog/yog.h"

#define JMP_RAISE   (1)

#define YOG_WARN(env, ...)  do { \
    YogError_warn(env, __FILE__, __LINE__, __VA_ARGS__); \
} while (0)

#define YOG_BUG(env, ...)    do { \
    YogError_bug(env, __FILE__, __LINE__, __VA_ARGS__); \
} while (0)

#define YOG_ASSERT(env, test, ...)  do { \
    if (!(test)) { \
        YOG_BUG(env, __VA_ARGS__); \
    } \
} while (0)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/error.c */
void YogError_bug(YogEnv*, const char*, uint_t, const char*, ...);
void YogError_out_of_memory(YogEnv*);
void YogError_print_stacktrace(YogEnv*);
void YogError_raise(YogEnv*, YogVal);
void YogError_raise_ArgumentError(YogEnv*, const char*, ...);
void YogError_raise_AttributeError(YogEnv*, const char*, ...);
void YogError_raise_ImportError(YogEnv*, const char*, ...);
void YogError_raise_IndexError(YogEnv*, const char*, ...);
void YogError_raise_KeyError(YogEnv*, const char*, ...);
void YogError_raise_NameError(YogEnv*, const char*, ...);
void YogError_raise_SyntaxError(YogEnv*, const char*, ...);
void YogError_raise_TypeError(YogEnv*, const char*, ...);
void YogError_raise_ValueError(YogEnv*, const char*, ...);
void YogError_raise_ZeroDivisionError(YogEnv*, const char*, ...);
void YogError_raise_binop_type_error(YogEnv*, YogVal, YogVal, const char*);
void YogError_warn(YogEnv*, const char*, uint_t, const char*, ...);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
