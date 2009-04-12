#if !defined(__YOG_ERROR_H__)
#define __YOG_ERROR_H__

#if 0
#include "yog/exception.h"
#include "yog/string.h"
#endif
#include "yog/yog.h"

#define JMP_RAISE   (1)

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
void YogError_bug(YogEnv*, const char*, unsigned int, const char*, ...);
void YogError_raise(YogEnv*, YogVal);
void YogError_raise_index_error(YogEnv*, const char*);
void YogError_raise_type_error(YogEnv*, const char*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
