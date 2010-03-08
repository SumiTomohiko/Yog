#include <ctype.h>
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"

char*
YogEncoding_left_adjust_char_head(YogEnv* env, YogVal enc, const char* start, const char* p)
{
    OnigEncoding onig = PTR_AS(YogEncoding, enc)->onig_enc;
    return (char*)(*onig->left_adjust_char_head)((OnigUChar*)start, (OnigUChar*)p);
}

YogVal
YogEncoding_get_default(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

#define DEFAULT_ENCODING_NAME   "utf-8"
    ID name = YogVM_intern(env, env->vm, DEFAULT_ENCODING_NAME);
    if (!YogTable_lookup(env, env->vm->encodings, ID2VAL(name), &val)) {
        YOG_BUG(env, "can't find default encoding: %s (%u)", DEFAULT_ENCODING_NAME, name);
    }
#undef DEFAULT_ENCODING_NAME
    RETURN(env, val);
}

static YogVal
get_encoding_of_name(YogEnv* env, const char* name)
{
    SAVE_LOCALS(env);
    YogVal enc = YUNDEF;
    PUSH_LOCAL(env, enc);

    YogVM* vm = env->vm;
    ID id = YogVM_intern(env, vm, name);
    if (!YogTable_lookup(env, vm->encodings, ID2VAL(id), &enc)) {
        YogError_raise_ValueError(env, "encoding \"%s\" not found", name);
    }

    RETURN(env, enc);
}

YogVal
YogEncoding_get_ascii(YogEnv* env)
{
    return get_encoding_of_name(env, "ascii");
}

YogVal
YogEncoding_get_utf8(YogEnv* env)
{
    return get_encoding_of_name(env, "utf-8");
}

int_t
YogEncoding_mbc_size(YogEnv* env, YogVal enc, const char* p)
{
    OnigEncoding onig = PTR_AS(YogEncoding, enc)->onig_enc;
    return (*onig->mbc_enc_len)((const unsigned char*)p);
}

YogVal
YogEncoding_normalize_name(YogEnv* env, YogVal name)
{
    YogVal s = YogString_clone(env, name);
    uint_t size = YogString_size(env, s);
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        char c = YogString_at(env, s, i);
        char c2 = 0;
        if (c == '_') {
            c2 = '-';
        }
        else {
            c2 = tolower(c);
        }
        YogVal body = PTR_AS(YogString, s)->body;
        PTR_AS(YogCharArray, body)->items[i] = c2;
    }

    return s;
}

YogVal
YogEncoding_new(YogEnv* env, OnigEncoding onig_enc)
{
    YogVal enc = ALLOC_OBJ(env, NULL, NULL, YogEncoding);
    PTR_AS(YogEncoding, enc)->onig_enc = onig_enc;

    return enc;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
