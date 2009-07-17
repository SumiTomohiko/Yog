#include "gmp.h"
#include "yog/bignum.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/yog.h"

static YogVal
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    size_t size = mpz_sizeinbase(PTR_AS(YogBignum, self)->num, 10) + 2;
    s = YogString_new_size(env, size);
    mpz_get_str(STRING_CSTR(s), 10, PTR_AS(YogBignum, self)->num);
    PTR_AS(YogCharArray, PTR_AS(YogString, s)->body)->size = size;

    RETURN(env, s);
}

YogVal
YogBignum_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Bignum", env->vm->cObject);
    YogKlass_define_method(env, klass, "to_s", to_s);

    RETURN(env, klass);
}

static void
YogBignum_initialize(YogEnv* env, YogVal self)
{
    YogBasicObj_init(env, self, 0, env->vm->cBignum);
    mpz_init(PTR_AS(YogBignum, self)->num);
}

static void
YogBignum_finalize(YogEnv* env, void* ptr)
{
    YogBignum* bignum = ptr;
    mpz_clear(bignum->num);
}

static YogVal
YogBignum_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = ALLOC_OBJ(env, YogBasicObj_keep_children, YogBignum_finalize, YogBignum);
    YogBignum_initialize(env, bignum);

    RETURN(env, bignum);
}

YogVal
YogBignum_from_int(YogEnv* env, int n)
{
    SAVE_LOCALS(env);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = YogBignum_new(env);
    mpz_set_si(PTR_AS(YogBignum, bignum)->num, n);

    RETURN(env, bignum);
}

YogVal
YogBignum_from_str(YogEnv* env, YogVal s, int base)
{
    SAVE_ARG(env, s);
    YogVal bignum = YUNDEF;
    PUSH_LOCAL(env, bignum);

    bignum = YogBignum_new(env);
    const char* str = STRING_CSTR(s);
    if (mpz_set_str(PTR_AS(YogBignum, bignum)->num, str, base) != 0) {
        YOG_BUG(env, "mpz_set_str failed");
    }

    RETURN(env, bignum);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
