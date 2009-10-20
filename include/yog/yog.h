#if !defined(__YOG_YOG_H__)
#define __YOG_YOG_H__

#include "config.h"
#if defined(HAVE_LIMITS_H)
#   include <limits.h>
#endif
#include <stdio.h>

#define BOOL    int_t
/* windef.h defines FALSE and TRUE. */
#if !defined(FALSE)
#   define FALSE   0
#endif
#if !defined(TRUE)
#   define TRUE    (!FALSE)
#endif

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
#if defined(SIZEOF_VOID_)
#   define SIZEOF_VOIDP     SIZEOF_VOID_
#endif

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

struct YogLocals {
    struct YogLocals* next;
    uint_t num_vals;
    uint_t size;
    YogVal* vals[4];
};

typedef struct YogLocals YogLocals;

struct YogLocalsAnchor {
    struct YogLocals* locals;
};

typedef struct YogLocalsAnchor YogLocalsAnchor;

struct YogEnv {
    struct YogVM* vm;
    YogVal thread;
    struct YogLocalsAnchor* locals;
};

typedef struct YogEnv YogEnv;

#define SAVE_LOCALS(env) \
    YogLocals* __cur_locals__ = PTR_AS(YogThread, (env)->thread)->locals
#define RESTORE_LOCALS(env) \
    PTR_AS(YogThread, (env)->thread)->locals = __cur_locals__
#if 0
#   define PUSH_LOCAL_TABLE(env, tbl)   do { \
    uint_t i; \
    TRACE("tbl=%p", &tbl); \
    for (i = 0; i < tbl.num_vals; i++) { \
        TRACE("tbl.vals[%d]=%p", i, tbl.vals[i]); \
    } \
    tbl.next = PTR_AS(YogThread, (env)->thread)->locals; \
    PTR_AS(YogThread, (env)->thread)->locals = &tbl; \
} while (0)
#else
#   define PUSH_LOCAL_TABLE(env, tbl)   do { \
    tbl.next = PTR_AS(YogThread, (env)->thread)->locals; \
    PTR_AS(YogThread, (env)->thread)->locals = &tbl; \
} while (0)
#endif
#define PUSH_LOCAL(env, x) \
    YogLocals __locals_##x##__; \
    __locals_##x##__.num_vals = 1; \
    __locals_##x##__.size = 1; \
    __locals_##x##__.vals[0] = &(x); \
    __locals_##x##__.vals[1] = NULL; \
    __locals_##x##__.vals[2] = NULL; \
    __locals_##x##__.vals[3] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##__);
#define PUSH_LOCALS2(env, x, y) \
    YogLocals __locals_##x##_##y##__; \
    __locals_##x##_##y##__.num_vals = 2; \
    __locals_##x##_##y##__.size = 1; \
    __locals_##x##_##y##__.vals[0] = &(x); \
    __locals_##x##_##y##__.vals[1] = &(y); \
    __locals_##x##_##y##__.vals[2] = NULL; \
    __locals_##x##_##y##__.vals[3] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##__);
#define PUSH_LOCALS3(env, x, y, z) \
    YogLocals __locals_##x##_##y##_##z##__; \
    __locals_##x##_##y##_##z##__.num_vals = 3; \
    __locals_##x##_##y##_##z##__.size = 1; \
    __locals_##x##_##y##_##z##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##__.vals[3] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##__);
#define PUSH_LOCALS4(env, x, y, z, t) \
    YogLocals __locals_##x##_##y##_##z##_##t##__; \
    __locals_##x##_##y##_##z##_##t##__.num_vals = 4; \
    __locals_##x##_##y##_##z##_##t##__.size = 1; \
    __locals_##x##_##y##_##z##_##t##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##_##t##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##_##t##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##_##t##__.vals[3] = &(t); \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##_##t##__);
#define PUSH_LOCALS5(env, x, y, z, t, u) \
    PUSH_LOCALS4(env, x, y, z, t); \
    PUSH_LOCAL(env, u)
#define PUSH_LOCALS6(env, x, y, z, t, u, v) \
    PUSH_LOCALS4(env, x, y, z, t); \
    PUSH_LOCALS2(env, u, v)
#define PUSH_LOCALS7(env, x, y, z, t, u, v, w) \
    PUSH_LOCALS4(env, x, y, z, t); \
    PUSH_LOCALS3(env, u, v, w)
#define PUSH_LOCALS8(env, x, y, z, t, u, v, w, p) \
    PUSH_LOCALS4(env, x, y, z, t); \
    PUSH_LOCALS4(env, u, v, w, p)
#define PUSH_LOCALSX(env, num, x) \
    YogLocals __locals_##x##__; \
    __locals_##x##__.num_vals = 1; \
    __locals_##x##__.size = (num); \
    __locals_##x##__.vals[0] = (x); \
    __locals_##x##__.vals[1] = NULL; \
    __locals_##x##__.vals[2] = NULL; \
    __locals_##x##__.vals[3] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##__);
#define SAVE_ARG(env, x)        SAVE_LOCALS((env)); \
                                PUSH_LOCAL((env), x)
#define SAVE_ARGS2(env, x, y)   SAVE_LOCALS((env)); \
                                PUSH_LOCALS2((env), x, y)
#define SAVE_ARGS3(env, x, y, z)  \
                                SAVE_LOCALS((env)); \
                                PUSH_LOCALS3((env), x, y, z)
#define SAVE_ARGS4(env, x, y, z, t)  \
                                SAVE_LOCALS((env)); \
                                PUSH_LOCALS4((env), x, y, z, t)
#define SAVE_ARGS5(env, x, y, z, t, u)  \
                                SAVE_LOCALS((env)); \
                                PUSH_LOCALS5((env), x, y, z, t, u)
#define POP_LOCALS(env) do { \
    YogLocals* next = PTR_AS(YogThread, (env)->thread)->locals->next; \
    PTR_AS(YogThread, (env)->thread)->locals = next; \
} while (0)
#define RETURN(env, val)        do { \
    RESTORE_LOCALS(env); \
    return val; \
} while (0)
#define RETURN_VOID(env)        do { \
    RESTORE_LOCALS(env); \
    return; \
} while (0)

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
YogVal YogVal_get_class(YogEnv*, YogVal);
YogVal YogVal_get_descr(YogEnv*, YogVal, YogVal, YogVal);
BOOL YogVal_is_subclass_of(YogEnv*, YogVal, YogVal);
void YogVal_print(YogEnv*, YogVal);
void YogVal_set_attr(YogEnv*, YogVal, ID, YogVal);

/* PROTOTYPE_END */

#if defined(__GNUC__)
#   define TRACE(...)    do { \
    printf("%s:%d ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
    fflush(stdout); \
} while (0)
#else
#   define TRACE    printf("%s:%d ", __FILE__, __LINE__); printf
#endif

#define array_sizeof(a)     (sizeof(a) / sizeof(a[0]))
#define MAIN_MODULE_NAME    "__main__"

#if !defined(HAVE_STDINT_H)
typedef unsigned char uint8_t;
#endif

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
