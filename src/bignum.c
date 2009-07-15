#if defined(HAVE_CONFIG_H)
#   include "config.h"
#endif
#if defined(HAVE_LIMITS_H)
#   include <limits.h>
#endif
#include "yog/bignum.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/yog.h"

YogVal
YogBignum_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Bignum", env->vm->cObject);

    RETURN(env, klass);
}

static YogVal
YogBignum_new(YogEnv* env, unsigned int size)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = ALLOC_OBJ_ITEM(env, YogBasicObj_keep_children, NULL, YogBignum, size, unsigned int);
    YogBasicObj_init(env, bignum, 0, env->vm->cBignum);
    PTR_AS(YogBignum, bignum)->sign = 1;
    PTR_AS(YogBignum, bignum)->size = size;
    unsigned int i;
    for (i = 0; i < size; i++) {
        PTR_AS(YogBignum, bignum)->items[i] = 0;
    }

    RETURN(env, bignum);
}

YogVal
YogBignum_from_int(YogEnv* env, int n)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = YogBignum_new(env, 1);
    if (n == INT_MIN) {
        PTR_AS(YogBignum, bignum)->sign = -1;
        PTR_AS(YogBignum, bignum)->items[0] = (BIGNUM_DIGIT)INT_MAX + 1;
    }
    else if (n < 0) {
        PTR_AS(YogBignum, bignum)->sign = -1;
        PTR_AS(YogBignum, bignum)->items[0] = - n;
    }
    else {
        PTR_AS(YogBignum, bignum)->items[0] = n;
    }

    RETURN(env, bignum);
}

YogVal
YogBignum_from_str(YogEnv* env, YogVal s)
{
    SAVE_ARG(env, s);
    YogVal bignum = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS2(env, bignum, body);

    body = PTR_AS(YogString, s)->body;
    unsigned int size = PTR_AS(YogCharArray, body)->size;
    YOG_ASSERT(env, 0 < size, "string is empty");
    unsigned int bits_per_digit = sizeof(BIGNUM_DIGIT) * CHAR_BIT;
    /* 4 is bits number needed for 10 */
    unsigned int max_size = 4 * size / bits_per_digit + 1;
    bignum = YogBignum_new(env, max_size);
    unsigned int digits_size = 1;
    unsigned int i;
    for (i = 0; i < size; i++) {
        char c = PTR_AS(YogCharArray, body)->items[i];
        BIGNUM_DIGIT_DOUBLE num = c - '0';
        unsigned int j = 0;
        while (1) {
            while (j < digits_size) {
                BIGNUM_DIGIT_DOUBLE digit = PTR_AS(YogBignum, bignum)->items[j];
                num += 10 * digit;
                BIGNUM_DIGIT lower = num & (bits_per_digit - 1);
                PTR_AS(YogBignum, bignum)->items[j] = lower;
                num >>= bits_per_digit;
                j++;
            }
            if (num != 0) {
                digits_size++;
                continue;
            }
            break;
        }
    }

    RETURN(env, bignum);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
