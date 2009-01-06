#include <ctype.h>
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/st.h"
#include "yog/yog.h"

char* 
YogEncoding_left_adjust_char_head(YogEnv* env, YogEncoding* enc, const char* start, const char* p) 
{
    return (char*)(*enc->onig_enc->left_adjust_char_head)((OnigUChar*)start, (OnigUChar*)p);
}

YogEncoding* 
YogEncoding_get_default(YogEnv* env) 
{
    ID name = INTERN("utf-8");
    YogVal key = ID2VAL(name);
    YogVal val = YUNDEF;
    if (!YogTable_lookup(env, ENV_VM(env)->encodings, key, &val)) {
        YOG_ASSERT(env, FALSE, "Can't find default encoding.");
    }
    return VAL2PTR(val);
}

int 
YogEncoding_mbc_size(YogEnv* env, YogEncoding* enc, const char* p) 
{
    return (*enc->onig_enc->mbc_enc_len)((const unsigned char*)p);
}

YogString* 
YogEncoding_normalize_name(YogEnv* env, YogString* name) 
{
    YogString* s = YogString_clone(env, name);
    unsigned int size = YogString_size(env, s) - 1;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        char c = YogString_at(env, s, i);
        char c2 = 0;
        if (c == '_') {
            c2 = '-';
        }
        else {
            c2 = tolower(c);
        }
        s->body->items[i] = c2;
    }

    return s;
}

YogEncoding* 
YogEncoding_new(YogEnv* env, OnigEncoding onig_enc) 
{
    YogEncoding* enc = ALLOC_OBJ(env, NULL, NULL, YogEncoding);
    enc->onig_enc = onig_enc;

    return enc;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
