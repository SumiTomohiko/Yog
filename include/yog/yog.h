#if !defined(__YOG_YOG_H__)
#define __YOG_YOG_H__

#if defined(HAVE_CONFIG_H)
#   include "config.h"
#endif
#if defined(HAVE_LIMITS_H)
#   include <limits.h>
#endif
#include <stdio.h>

#define BOOL    int_t
#define FALSE   (0)
#define TRUE    (!FALSE)

/**
 * tagged pointer
 *
 * FEDC BA98 7654 3210 FEDC BA98 7654 3210
 * ---------------------------------------
 * xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxx1 Fixnum
 * 0000 0000 0000 0000 0000 0000 0000 0010 undef (0x02)
 * 0000 0000 0000 0000 0000 0000 0000 0110 nil (0x06)
 * xxxx xxxx xxxx xxxx xxxx xxxx xxxx 1010 Bool
 * 0000 0000 0000 0000 0000 0000 0000 1010 false (0x0a)
 * 0000 0000 0000 0000 0000 0000 0001 1010 true (0x1a)
 * xxxx xxxx xxxx xxxx xxxx xxxx xxxx 1110 Symbol
 * xxxx xxxx xxxx xxxx xxxx xxxx xxxx xx00 pointer
 */

#if SIZEOF_VOIDP == SIZEOF_INT
typedef unsigned int YogVal;
typedef unsigned int ID;
#   define INVALID_ID       VAL2ID(UINT_MAX & (~1))
typedef unsigned int pc_t;
#   define SIGNED_TYPE      int
#   define UNSIGNED_TYPE    unsigned int
#   define UNSIGNED_MAX     UINT_MAX
#elif SIZEOF_VOIDP == SIZEOF_LONG
typedef unsigned long YogVal;
typedef unsigned long ID;
#   define INVALID_ID       VAL2ID(ULONG_MAX & (~1))
typedef unsigned long pc_t;
#   define SIGNED_TYPE      long
#   define UNSIGNED_TYPE    unsigned long
#   define UNSIGNED_MAX     ULONG_MAX
#elif SIZEOF_VOIDP == SIZEOF_LONG_LONG
typedef unsigned long long YogVal;
typedef unsigned long long ID;
#   define INVALID_ID       VAL2ID(ULLONG_MAX & (~1))
typedef unsigned long long pc_t;
#   define SIGNED_TYPE      long long
#   define UNSIGNED_TYPE    unsigned long long
#   define UNSIGNED_MAX     ULLONG_MAX
#else
#   error "No integer type available to represent pointers"
#endif

typedef UNSIGNED_TYPE uint_t;
typedef SIGNED_TYPE int_t;

#define YUNDEF          0x02
#define YNIL            0x06
#define YFALSE          0x0a
#define YTRUE           0x1a

#define INT2VAL(n)      (((n) << 1) + 1)
#define PTR2VAL(ptr)    ((YogVal)(ptr))
#define ID2VAL(id)      (((id) << 4) | 0x0e)
#define VAL2INT(v)      ((SIGNED_TYPE)(v) >> 1)
#define VAL2ID(v)       ((v) >> 4)
#define VAL2PTR(v)      ((void*)(v))
#define VAL2BOOL(v)     ((v) >> 4)

#define PTR_AS(type, v)     ((type*)VAL2PTR(v))

#define IS_UNDEF(v)     ((v) == 0x02)
#define IS_PTR(v)       (((v) & 0x03) == 0)
#define IS_FIXNUM(v)    (((v) & 0x01) == 0x01)
#define IS_BOOL(v)      (((v) & 0x0f) == 0x0a)
#define IS_TRUE(v)      ((v) == YTRUE)
#define IS_FALSE(v)     ((v) == YFALSE)
#define IS_NIL(v)       ((v) == 0x06)
#define IS_SYMBOL(v)    (((v) & 0x0f) == 0x0e)

#define YOG_TEST(v)     (!IS_NIL((v)) && !IS_FALSE((v)))

#define YINT_MAX    VAL2INT(INT_MAX)
#define YINT_MIN    VAL2INT(INT_MIN)
#define FIXABLE(n)  ((YINT_MIN <= (n)) && ((n) <= YINT_MAX))

typedef struct YogEnv YogEnv;

typedef void* (*ObjectKeeper)(YogEnv*, void*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper, void*);
typedef void (*Finalizer)(YogEnv*, void*);

typedef uint_t flags_t;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/value.c */
YogVal YogVal_from_int(YogEnv*, int_t);
YogVal YogVal_get_attr(YogEnv*, YogVal, ID);
YogVal YogVal_get_descr(YogEnv*, YogVal, YogVal, YogVal);
YogVal YogVal_get_klass(YogEnv*, YogVal);
BOOL YogVal_is_subklass_of(YogEnv*, YogVal, YogVal);
void YogVal_print(YogEnv*, YogVal);
void YogVal_set_attr(YogEnv*, YogVal, ID, YogVal);

/* PROTOTYPE_END */

#define DPRINTF(...)    do { \
    printf("%s:%d ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
    fflush(stdout); \
} while (0)

#define array_sizeof(a)     (sizeof(a) / sizeof(a[0]))
#define MAIN_MODULE_NAME    "__main__"

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
