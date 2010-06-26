#include "yog/config.h"
#include <ctype.h>
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/string.h"
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
YogEncoding_get_ascii(YogEnv* env)
{
    return env->vm->encAscii;
}

YogVal
YogEncoding_get_utf8(YogEnv* env)
{
    return env->vm->encUtf8;
}

YogVal
YogEncoding_get_default(YogEnv* env)
{
    return YogEncoding_get_utf8(env);
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

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal enc = YUNDEF;
    PUSH_LOCAL(env, enc);

    enc = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, YogEncoding);
    YogBasicObj_init(env, enc, TYPE_ENCODING, 0, klass);
    PTR_AS(YogEncoding, enc)->onig_enc = NULL;

    RETURN(env, enc);
}

void
YogEncoding_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cEncoding = YUNDEF;
    PUSH_LOCAL(env, cEncoding);
    YogVM* vm = env->vm;

    cEncoding = YogClass_new(env, "Encoding", vm->cObject);
    YogClass_define_allocator(env, cEncoding, alloc);
    vm->cEncoding = cEncoding;

    RETURN_VOID(env);
}

YogVal
YogEncoding_new(YogEnv* env, OnigEncoding onig_enc)
{
    SAVE_LOCALS(env);
    YogVal enc = YUNDEF;
    PUSH_LOCAL(env, enc);

    enc = alloc(env, env->vm->cEncoding);
    PTR_AS(YogEncoding, enc)->onig_enc = onig_enc;

    RETURN(env, enc);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
